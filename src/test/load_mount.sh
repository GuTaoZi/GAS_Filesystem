#!/bin/sh

# load kernel module
insmod ../kern/gas.ko

# mount gasfs using loopback 
mount -o loop -t gas vdisk /mnt/gas
ls -al /mnt/gas

