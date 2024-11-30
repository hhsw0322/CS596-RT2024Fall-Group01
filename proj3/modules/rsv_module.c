#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 01");
MODULE_DESCRIPTION("Reservation Kernel Module for Project 3");

static int pid = -1;
static struct timespec budget = {1, 0}; // Default reservation budget of 1 second
static struct timespec period = {3, 0}; // Default reservation period of 3 seconds
module_param(pid, int, 0644);
MODULE_PARM_DESC(pid, "Process ID to set reservation for");

static int __init rsv_module_init(void) {
    long result;

    if (pid == -1) {
        printk(KERN_ALERT "Reservation Module: Invalid PID\n");
        return -EINVAL;
    }

    // Set reservation for the given PID
    result = sys_set_rsv(pid, &budget, &period);
    if (result == 0) {
        printk(KERN_INFO "Reservation Module: Successfully set reservation for PID %d\n", pid);
    } else {
        printk(KERN_ALERT "Reservation Module: Failed to set reservation for PID %d, error %ld\n", pid, result);
    }

    return 0;
}

static void __exit rsv_module_exit(void) {
    long result;

    if (pid == -1) {
        printk(KERN_ALERT "Reservation Module: Invalid PID for cancelation\n");
        return;
    }

    // Cancel reservation for the given PID
    result = sys_cancel_rsv(pid);
    if (result == 0) {
        printk(KERN_INFO "Reservation Module: Successfully canceled reservation for PID %d\n", pid);
    } else {
        printk(KERN_ALERT "Reservation Module: Failed to cancel reservation for PID %d, error %ld\n", pid, result);
    }
}

module_init(rsv_module_init);
module_exit(rsv_module_exit);
