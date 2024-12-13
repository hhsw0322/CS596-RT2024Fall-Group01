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

static struct timespec budget = {1, 0}; // Default reservation budget of 1 second
static struct timespec period = {3, 0}; // Default reservation period of 3 seconds

static int __init rsv_module_init(void) {
    long result;

    // Set reservation for the current process
    result = sys_set_rsv(0, &budget, &period); // 0 means "current process"
    if (result == 0) {
        printk(KERN_INFO "Reservation Module: Successfully set reservation for current process\n");
    } else {
        printk(KERN_ALERT "Reservation Module: Failed to set reservation for current process, error %ld\n", result);
    }

    return 0;
}

static void __exit rsv_module_exit(void) {
    long result;

    // Cancel reservation for the current process
    result = sys_cancel_rsv(0); // 0 means "current process"
    if (result == 0) {
        printk(KERN_INFO "Reservation Module: Successfully canceled reservation for current process\n");
    } else {
        printk(KERN_ALERT "Reservation Module: Failed to cancel reservation for current process, error %ld\n", result);
    }
}

module_init(rsv_module_init);
module_exit(rsv_module_exit);

