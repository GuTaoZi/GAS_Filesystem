#!/bin/sh

# load kernel module
insmod ../kernel/sfs.ko

# mount sfs using loopback 
mount -o loop -t sfs vdisk /mnt
ls -al /mnt

