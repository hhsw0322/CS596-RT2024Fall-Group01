obj-m := mod_calc.o
KDIR := /home/cs596group1/Downloads/cs596-rpi-kernel-main
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
