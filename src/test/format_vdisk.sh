#!/bin/sh

dd if=/dev/zero of=vdisk bs=1M count=4
../makefs/mkfs.gas vdisk
