#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#define SYS_CALC_NUM 397

asmlinkage long (*original_sys_calc)(int, int, char, int *);

// Custom system call that performs modulo operation
asmlinkage long mod_sys_calc(int param1, int param2, char operation, int *result) {
    int mod_result;
    if (param2 == 0) {
        printk(KERN_ERR "mod_calc: Division by zero error in modulo operation.\n");
        return -EINVAL;  // Return error if division by zero
    }
    mod_result = param1 % param2;
    if (copy_to_user(result, &mod_result, sizeof(int))) {
        printk(KERN_ERR "mod_calc: Failed to copy result to user space.\n");
        return -EFAULT;  // Return error if copy_to_user fails
    }
    return 0;
}

static struct kprobe kp = {
    .symbol_name = "sys_calc"
};

// kprobe pre-handler: called just before the probed instruction is executed
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    int param1 = regs->ARM_r0;    // First argument
    int param2 = regs->ARM_r1;    // Second argument
    char operation = regs->ARM_r2; // Third argument (operation)
    int *result = (int *)regs->ARM_r3; // Fourth argument (result pointer)

    // Call the custom sys_calc (mod_sys_calc)
    long ret = mod_sys_calc(param1, param2, operation, result);

    // Set the return value in the CPU register (r0)
    regs->ARM_r0 = ret;
    return 0;
}

static int __init mod_calc_init(void) {
    int ret;

    // Register the kprobe to hook the sys_calc system call
    kp.pre_handler = handler_pre;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        printk(KERN_ERR "mod_calc: Failed to register kprobe, error: %d\n", ret);
        return ret;
    }
    printk(KERN_INFO "mod_calc: kprobe registered to hook sys_calc\n");
    return 0;
}

static void __exit mod_calc_exit(void) {
    unregister_kprobe(&kp);
    printk(KERN_INFO "mod_calc: kprobe unregistered, sys_calc restored\n");
}

module_init(mod_calc_init);
module_exit(mod_calc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hook sys_calc using kprobes to perform modulo operations.");
MODULE_AUTHOR("CS 596 Group 01");
