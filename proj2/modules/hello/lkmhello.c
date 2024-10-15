#include <linux/module.h>   // Required for all kernel modules
#include <linux/kernel.h>   // Required for printk()
#include <linux/init.h>     // Required for the macros

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group01");
MODULE_DESCRIPTION("Hello World Loadable Kernel Module");

// Function executed when the module is loaded
static int __init hello_init(void) {
    printk(KERN_INFO "Hello world! Group01 in kernel space\n");
    return 0; // Return 0 indicates successful load
}

// Function executed when the module is removed
static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye world! Group01 from kernel space\n");
}

// Macros to specify the initialization and cleanup functions
module_init(hello_init);
module_exit(hello_exit);


