

sudo ./build_km_fs.sh   # build kernel module and file system maker
sudo ./format_vdisk.sh  # prepare vdisk using GAS format
sudo ./load_mount.sh    # load kernel module, mount the vdisk
sudo ./umount_rmmod.sh  # unmount the vdisk and remove module

cd ../kern
make clean

cd ../makefs
make clean