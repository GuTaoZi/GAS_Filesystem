#!/bin/sh

# load kernel module
insmod ../kern/gasfs.ko

# mount gasfs using loopback 
mount -o loop -t gasfs vdisk /mnt/gas
ls -al /mnt/gas

