# make kernel module
cd ../kern
# make clean
make

# make GAS file system
cd ../makefs
# make clean
make\

cd ../test
sudo ./format_vdisk.sh  # prepare vdisk using GAS format
sudo ./load_mount.sh    # load kernel module, mount the vdisk
sudo ./umount_rmmod.sh  # unmount the vdisk and remove module

cd ../kern
make clean

cd ../makefs
make clean