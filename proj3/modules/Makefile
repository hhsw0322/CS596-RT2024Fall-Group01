# Specify the architecture and cross-compiler prefix for Raspberry Pi
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

# Define the kernel source path
KERNEL_SRC=~/Downloads/cs596-rpi-kernel-main

# Default target to build the module
all:
	$(MAKE) -C $(KERNEL_SRC) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

# Clean target to remove built files
clean:
	$(MAKE) -C $(KERNEL_SRC) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
