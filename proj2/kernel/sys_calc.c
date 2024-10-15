#include <linux/kernel.h>     // For printk()
#include <linux/syscalls.h>   // For SYSCALL_DEFINE
#include <linux/uaccess.h>    // For copy_to_user()

SYSCALL_DEFINE4(calc, int, param1, int, param2, char, operation, int __user *, result) {
    int res;

    switch (operation) {
        case '+':
            res = param1 + param2;
            break;
        case '-':
            res = param1 - param2;
            break;
        case '*':
            res = param1 * param2;
            break;
        case '/':
            if (param2 == 0) {
                printk(KERN_WARNING "Divide by zero error\n");
                return -1;  // Fail if divide by zero
            }
            res = param1 / param2;
            break;
        default:
            printk(KERN_WARNING "Invalid operation: %c\n", operation);
            return -1;  // Fail if operation is not valid
    }

    if (copy_to_user(result, &res, sizeof(int))) {
        return -1;  // Fail if we can't copy result to user space
    }

    return 0;  // Success
}
