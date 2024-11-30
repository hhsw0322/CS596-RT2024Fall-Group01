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
    info->consumed_time.tv_sec = 0;
    info->consumed_time.tv_nsec = 0; // Reset consumed time
    wake_up_process(info->task); // Wake up the task for its next period
    hrtimer_forward_now(timer, timespec_to_ktime(info->period)); // Restart timer
    return HRTIMER_RESTART;
}

/*
 * System call to set a reservation for a given task.
 */
asmlinkage long sys_set_rsv(pid_t pid, struct timespec __user *C, struct timespec __user *T) {
    struct task_struct *task;
    struct timespec c_val, t_val;
    struct rsv_task_info *new_rsv_info;

    if (copy_from_user(&c_val, C, sizeof(struct timespec)) ||
        copy_from_user(&t_val, T, sizeof(struct timespec))) {
        return -EFAULT;
    }

    if (c_val.tv_sec < 0 || c_val.tv_nsec < 0 || t_val.tv_sec < 0 || t_val.tv_nsec < 0) {
        return -EINVAL;
    }

    if (pid == 0) {
        task = current;
    } else {
        rcu_read_lock();
        task = find_task_by_vpid(pid);
        if (!task) {
            rcu_read_unlock();
            return -ESRCH;
        }
        get_task_struct(task);
        rcu_read_unlock();
    }

    if (task->rsv_set) {
        put_task_struct(task);
        return -EBUSY;
    }

    spin_lock(&rsv_lock);

    new_rsv_info = kmalloc(sizeof(*new_rsv_info), GFP_KERNEL);
    if (!new_rsv_info) {
        spin_unlock(&rsv_lock);
        put_task_struct(task);
        return -ENOMEM;
    }

    new_rsv_info->task = task;
    new_rsv_info->period = t_val;
    new_rsv_info->consumed_time.tv_sec = 0;
    new_rsv_info->consumed_time.tv_nsec = 0;

    hrtimer_init(&new_rsv_info->period_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    new_rsv_info->period_timer.function = &period_timer_callback;
    list_add(&new_rsv_info->list, &rsv_task_list);

    task->rsv_budget = c_val;
    task->rsv_period = t_val;
    task->rsv_set = true;

    hrtimer_start(&new_rsv_info->period_timer, timespec_to_ktime(t_val), HRTIMER_MODE_REL);

    spin_unlock(&rsv_lock);

    printk(KERN_ALERT "Reservation set for task %d\n", task->pid);

    put_task_struct(task);
    return 0;
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
    return -EINVAL;
}

/*
 * System call to cancel a reservation.
 */
asmlinkage long sys_cancel_rsv(pid_t pid) {
    struct task_struct *task;
    struct rsv_task_info *entry, *tmp;

    if (pid == 0) {
        task = current;
    } else {
        rcu_read_lock();
        task = find_task_by_vpid(pid);
        if (!task) {
            rcu_read_unlock();
            return -ESRCH;
        }
        get_task_struct(task);
        rcu_read_unlock();
    }

    if (!task->rsv_set) {
        put_task_struct(task);
        return -EINVAL;
    }

    spin_lock(&rsv_lock);
    list_for_each_entry_safe(entry, tmp, &rsv_task_list, list) {
        if (entry->task == task) {
            list_del(&entry->list);
            kfree(entry);
            hrtimer_cancel(&entry->period_timer);

            task->rsv_set = false;
            spin_unlock(&rsv_lock);
            printk(KERN_ALERT "Reservation canceled for task %d\n", task->pid);
            put_task_struct(task);
            return 0;
        }
    }
    spin_unlock(&rsv_lock);
    put_task_struct(task);
    return -EINVAL;
}
