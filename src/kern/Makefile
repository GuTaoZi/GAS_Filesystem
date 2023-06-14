ifneq ($(KERNELRELEASE),)
obj-m := gas_super.o
gas_super-objs := gas_super.o gas_inode.o gas_namei.o gas_dir.o bitmap.o gas_itree.o 
else
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
endif
