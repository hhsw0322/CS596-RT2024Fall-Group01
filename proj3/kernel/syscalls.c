#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/signal.h>

/*
 * Data structure for managing tasks with reservations.
 */
static LIST_HEAD(rsv_task_list); // List of tasks with reservations
static DEFINE_SPINLOCK(rsv_lock); // Lock for reservation management

struct rsv_task_info {
    struct task_struct *task;
    struct timespec period;
    struct timespec consumed_time;
    struct hrtimer period_timer;
    struct list_head list;
};

/*
 * High-resolution timer callback for task periods.
 */
static enum hrtimer_restart period_timer_callback(struct hrtimer *timer) {
    struct rsv_task_info *info = container_of(timer, struct rsv_task_info, period_timer);
    
    // Check if task has exceeded its budget
    if (timespec_compare(&info->consumed_time, &info->task->rsv_budget) > 0) {
        // Calculate utilization as a percentage
        long budget_ns = info->task->rsv_budget.tv_sec * 1000000000L + info->task->rsv_budget.tv_nsec;
        long consumed_ns = info->consumed_time.tv_sec * 1000000000L + info->consumed_time.tv_nsec;
        int utilization = (int)((consumed_ns * 100) / budget_ns);

        // Print budget overrun message to the kernel log
        printk(KERN_ALERT "Task %d: budget overrun (util: %d %%)\n", info->task->pid, utilization);

        // Send SIGUSR1 to notify of budget overrun
        send_sig(SIGUSR1, info->task, 0);
    }

    // Reset consumed time for the new period
    info->consumed_time.tv_sec = 0;
    info->consumed_time.tv_nsec = 0;

    // Restart the timer for the next period
    hrtimer_forward_now(timer, timespec_to_ktime(info->period));
    return HRTIMER_RESTART;
}

/*
 * Helper function to compute and set real-time priorities for tasks with reservations.
 */
void compute_and_set_priorities(void) {
    struct rsv_task_info *entry;
    struct rsv_task_info *task_array[50]; // Assumption: max 50 tasks
    int i = 0, j, k, priority = MAX_RT_PRIO - 1;

    // Collect tasks with reservations into an array for sorting
    list_for_each_entry(entry, &rsv_task_list, list) {
        task_array[i++] = entry;
    }

    // Sort tasks based on their period in ascending order (shorter period = higher priority)
    for (j = 0; j < i - 1; j++) {
        for (k = j + 1; k < i; k++) {
            if (timespec_compare(&task_array[j]->period, &task_array[k]->period) > 0) {
                struct rsv_task_info *tmp = task_array[j];
                task_array[j] = task_array[k];
                task_array[k] = tmp;
            }
        }
    }

    // Assign priorities using SCHED_FIFO
    for (j = 0; j < i; j++) {
        struct sched_param param;
        param.sched_priority = priority;
        sched_setscheduler(task_array[j]->task, SCHED_FIFO, &param);

        // If the next task has the same period, keep the same priority
        if (j < i - 1 && timespec_compare(&task_array[j]->period, &task_array[j + 1]->period) != 0) {
            priority--;
        }

        // Ensure priority doesn't go below minimum RT priority
        if (priority < 1) {
            priority = 1;
        }
    }
}

/*
 * System call to set a reservation for a given task.
 */
asmlinkage long sys_set_rsv(pid_t pid, struct timespec __user *C, struct timespec __user *T) {
    struct task_struct *task;
    struct timespec c_val, t_val;
    struct rsv_task_info *new_rsv_info;

    // Copy C and T values from user space to kernel space
    if (copy_from_user(&c_val, C, sizeof(struct timespec)) ||
        copy_from_user(&t_val, T, sizeof(struct timespec))) {
        return -EFAULT; // Error copying from user
    }

    // Validate input arguments (C and T must be non-negative)
    if (c_val.tv_sec < 0 || c_val.tv_nsec < 0 || t_val.tv_sec < 0 || t_val.tv_nsec < 0) {
        return -EINVAL; // Invalid input
    }

    // Find the task by PID
    if (pid == 0) {
        task = current; // Apply to calling task
    } else {
        rcu_read_lock();
        task = find_task_by_vpid(pid);
        if (!task) {
            rcu_read_unlock();
            return -ESRCH; // No such process
        }
        get_task_struct(task);
        rcu_read_unlock();
    }

    // Check if the task already has a reservation
    if (task->rsv_set) {
        put_task_struct(task);
        return -EBUSY; // Reservation already exists
    }

    spin_lock(&rsv_lock);

    // Add task to reservation list
    new_rsv_info = kmalloc(sizeof(*new_rsv_info), GFP_KERNEL);
    if (!new_rsv_info) {
        spin_unlock(&rsv_lock);
        put_task_struct(task);
        return -ENOMEM; // Memory allocation failed
    }

    new_rsv_info->task = task;
    new_rsv_info->period = t_val;
    new_rsv_info->consumed_time.tv_sec = 0;
    new_rsv_info->consumed_time.tv_nsec = 0;

    hrtimer_init(&new_rsv_info->period_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    new_rsv_info->period_timer.function = &period_timer_callback;
    list_add(&new_rsv_info->list, &rsv_task_list);

    // Set reservation values
    task->rsv_budget = c_val;
    task->rsv_period = t_val;
    task->rsv_set = true;

    // Start the high-resolution timer for the reservation period
    hrtimer_start(&new_rsv_info->period_timer, timespec_to_ktime(t_val), HRTIMER_MODE_REL);

    // Recalculate and set priorities for all tasks with reservations
    compute_and_set_priorities();

    spin_unlock(&rsv_lock);

    // Log a message to the kernel
    printk(KERN_ALERT "Reservation set for task %d\n", task->pid);

    put_task_struct(task);
    return 0; // Success
}

/*
 * System call to wait until the next period.
 */
asmlinkage long sys_wait_until_next_period(void) {
    struct task_struct *task = current;
    struct rsv_task_info *entry;

    spin_lock(&rsv_lock);
    list_for_each_entry(entry, &rsv_task_list, list) {
        if (entry->task == task) {
            set_current_state(TASK_UNINTERRUPTIBLE);
            spin_unlock(&rsv_lock);
            schedule();
            return 0;
        }
    }
    spin_unlock(&rsv_lock);
    return -EINVAL; // Task does not have a reservation
}

/*
 * System call to cancel a reservation for a given task.
 */
asmlinkage long sys_cancel_rsv(pid_t pid) {
    struct task_struct *task;
    struct rsv_task_info *entry, *tmp;

    // Find the task by PID
    if (pid == 0) {
        task = current; // Apply to calling task
    } else {
        rcu_read_lock();
        task = find_task_by_vpid(pid);
        if (!task) {
            rcu_read_unlock();
            return -ESRCH; // No such process
        }
        get_task_struct(task);
        rcu_read_unlock();
    }

    // Check if the task has a reservation
    if (!task->rsv_set) {
        put_task_struct(task);
        return -EINVAL; // No reservation to cancel
    }

    spin_lock(&rsv_lock);

    // Find and remove task from reservation list
    list_for_each_entry_safe(entry, tmp, &rsv_task_list, list) {
        if (entry->task == task) {
            list_del(&entry->list);
            hrtimer_cancel(&entry->period_timer);
            kfree(entry);

            // Cancel the reservation
            task->rsv_set = false;

            // Recalculate priorities for remaining tasks
            compute_and_set_priorities();

            spin_unlock(&rsv_lock);

            // Log a message to the kernel
            printk(KERN_ALERT "Reservation canceled for task %d\n", task->pid);

            put_task_struct(task);
            return 0; // Success
        }
    }

    spin_unlock(&rsv_lock);

    put_task_struct(task);
    return -EINVAL; // Task did not have a reservation
}

EXPORT_SYMBOL(sys_set_rsv);
EXPORT_SYMBOL(sys_cancel_rsv);
EXPORT_SYMBOL(sys_wait_until_next_period);
