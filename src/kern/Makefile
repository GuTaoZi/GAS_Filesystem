obj-m := gas.o
gas-objs := def.o gas_super.o gas_inode.o gas_namei.o gas_dir.o bitmap.o gas_itree.o gas_page.o
CONFIG_MODULE_SIG=n

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
