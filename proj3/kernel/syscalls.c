#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

/*
 * System call to set a reservation for a given task.
 */
asmlinkage long sys_set_rsv(pid_t pid, struct timespec __user *C, struct timespec __user *T) {
    struct task_struct *task;
    struct timespec c_val, t_val;

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

    // Set reservation values
    task->rsv_budget = c_val;
    task->rsv_period = t_val;
    task->rsv_set = true;

    // Log a message to the kernel
    printk(KERN_INFO "Reservation set for task %d: Budget (%ld sec, %ld nsec), Period (%ld sec, %ld nsec)\n",
           task->pid, c_val.tv_sec, c_val.tv_nsec, t_val.tv_sec, t_val.tv_nsec);

    put_task_struct(task);
    return 0; // Success
}

/*
 * System call to cancel a reservation for a given task.
 */
asmlinkage long sys_cancel_rsv(pid_t pid) {
    struct task_struct *task;

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

    // Cancel the reservation
    task->rsv_set = false;

    // Log a message to the kernel
    printk(KERN_INFO "Reservation canceled for task %d\n", task->pid);

    put_task_struct(task);
    return 0; // Success
}
