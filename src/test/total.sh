#!/bin/sh

cd ../kern
# make clean
make
cd ../tools
# make clean
make
cd ../test

sudo ./format_vdisk.sh
sudo ./load_mount.sh
sudo ./umount_rmmod.sh

cd ../kern
make clean

cd ../tools
make clean