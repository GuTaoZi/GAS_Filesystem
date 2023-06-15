<img src="https://s2.loli.net/2023/04/13/LnKtR6Qv2Yz4deh.png" alt="icon.png" width='500px' />

A custom Linux file system designed and implemented as a kernel module. Project for CS334 Operating System. [Task description here](https://github.com/oscomp/proj209-custom-filesystem).

## Contributors

| SID      | Name                                            | Contributions | Contribution Rate |
| -------- | ----------------------------------------------- | ------------- | ----------------- |
| 12111624 | [GuTaoZi](https://github.com/GuTaoZi)           |               | 33.33%            |
| 12110524 | [Artanisax](https://github.com/Artanisax)       |               | 33.33%            |
| 12112012 | [ShadowStorm](https://github.com/Jayfeather233) |               | 33.33%            |

## TODOs

- [ ] GAS File System
  - [x] Layout Component
    - [x] Super block
    - [x] INode
    - [x] Directory entry
    - [x] Tree structure
  - [x] VFS interfaces
    - [x] Page
    - [x] File operations
    - [x] Address space operations
    - [x] INode operations
      - [x] File
      - [x] Link
      - [x] Directory
  - [x] Annotations
- [ ] Project report
- [ ] Project video

## Project Structure

## Environment Setup

GAS file system implement the VFS interfaces of ext2 for `Linux-3.13.0-170`. Following are some steps to set up the environment.

1. Prepare a Linux distribution that can use Linux 3.13.0-170 as the kernel.

   Check whether the distribution supports this kernel:

   ```shell
   sudo apt-cache search linux-image
   ```

   If the available kernel lists contains the kernel version we want, then it's OK to run GAS file system in this environment.

   Here we choose `Ubuntu 14.04.6 LTS`, you can download the image [here](http://mirrors.163.com/ubuntu-releases/14.04.6/).

2. Install Linux kernel

   You can check the available kernel versions using command:

   ```shell
   sudo apt-cache search linux-image
   ```

   To install Linux-3.13.0-170 kernel, run the following command:

   ```shell
   sudo apt-get install linux-image-unsigned-3.13.0-170-generic
   ```

   You can change the kernel version according to the available version lists, as long as it's before Linux-3.15.

3. Switch Linux kernel

   Check the current kernel list:

   ```shell
   dpkg --get-selections | grep linux-image
   ```

   Open `/etc/default/grub` and edit:

   ```
   ...
   GRUB_DEFAULT=0
   GRUB_HIDDEN_TIMEOUT=-1
   GRUB_HIDDEN_TIMEOUT_QUIET=true
   GRUB_TIMEOUT=10
   GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
   GRUB_CMDLINE_LINUX_DEFAULT="text"
   GRUB_CMDLINE_LINUX="find_preseed=/preseed.cfg auto noprompt priority=critical locale=en_US"
   ...
   ```

   Update grub.cfg:

   ```shell
   sudo update-grub
   ```

4. Reboot and select kernel version

   After rebooting, select **ubuntu advanced options**, and switch to the proper Linux kernel.\

   <img src="https://s2.loli.net/2023/06/15/YfIX7aeuDhHG1AN.png" alt="image.png" style="zoom:50%;" />

   <img src="https://s2.loli.net/2023/06/15/7O1LxEv3q2fSlVN.png" alt="image.png" style="zoom:50%;" />

   You can check if the kernel version by running `uname -a`.

5. Clone project repository

   After installing and configuring some software packages like git, clone this repo using git:

   ```shell
   git clone https://github.com/GuTaoZi/GAS_Filesystem.git
   ```

   GAS file system only includes the kernel headers, no other dependency needed.

## Usage Guide

The shell commands to test GAS file system is integrated in shell scripts in `GAS_Filesystem/src/test`, you can execute the scripts or try commands yourself.

### Test scripts

`total.sh`: integrating all the following scripts (unmount immediately after successfully mounting)

`build_km_fs.sh`: build the kernel module and file system maker

`format_vdisk.sh`: create a virtual disk

`load_mount.sh`: load kernel module, mount virtual disk

`umount_rmmod.sh`: unmount virtual disk, remove module

### Step-by-step building

1. Build kernel module

   Run `make` in `GAS_Filesystem/src/kern`

2. Build file system maker

   Run `make` in `GAS_Filesystem/src/makefs`

3. Create and format a 4MB virtual disk

   ```shell
   dd if=/dev/zero of=vdisk bs=1M count=4
   ../tools/mkfs.gas vdisk
   ```

4. Load kernel module and mount the virtual disk

   ```shell
   insmod ../kern/gas.ko
   mount -o loop -t gas vdisk /mnt/gas
   ```

5. GAS file system is running in `/mnt/gas/`!

   ```shell
   ls -al /mnt/gas
   ```

   You can try some file operations in this directory.

6. Unmount the virtual disk and remove GAS file system from kernel modules

   ```shell
   umount /mnt/gas
   rmmod gas
   ```

## File System Layout

All the meta data are stored in little endian in the corresponding structures:

### Super block

```c
struct gas_super_block
{
    __le32 s_magic;			// magic number
    __le32 s_blocksize;		// block size
    __le32 s_bam_blocks;	// block alloc map block
    __le32 s_iam_blocks;	// inode alloc map block
    __le32 s_inode_blocks;	// inodes per block
    __le32 s_nblocks;		// number of blocks
    __le32 s_ninodes;		// number of inodes
};
```

### INode

```c
struct gas_inode
{
    __le16 i_mode;			// file type: file, directory, symlink etc.
    __le16 i_nlink;			// number of hard links references
    __le32 i_uid;			// Owner user id
    __le32 i_gid;			// Owner group id
    __le32 i_size;			// inode size
    __le32 i_atime;			// Access time
    __le32 i_mtime;			// Modify time
    __le32 i_ctime;			// Last change time
    __le32 i_blkaddr[9];	// 6 direct, single + double + triple indirect block address
};
```

### Directory entry

```c
struct gas_dir_entry
{
    char de_name[GAS_MAX_NAME_LEN];	// dentry name (max length 60)
    __le32 de_inode;				// dentry inode number
};
```

## VFS Interfaces

This project implemented the following interfaces of VFS, so you can directly use basic file operations like `cd`, `ls`, `touch`, `cat`, `echo` etc. under the directory `/mnt/gas`.

### Address space

```c
const struct address_space_operations gas_a_ops = {
    .readpage = gas_readpage,
    .readpages = gas_readpages,
    .writepage = gas_writepage,
    .writepages = gas_writepages,
    .write_begin = gas_write_begin,
    .write_end = gas_write_end,
    .bmap = gas_bmap,
    .direct_IO = gas_direct_io,
};
```

### File operations

```c
const struct file_operations gas_file_ops = {
    .llseek = generic_file_llseek,
    .read = do_sync_read,
    .aio_read = generic_file_aio_read,
    .write = do_sync_write,
    .aio_write = generic_file_aio_write,
    .mmap = generic_file_mmap,
    .splice_read = generic_file_splice_read,
    .splice_write = generic_file_splice_write
};
```

### INode: file, directory, symlink

```c
const struct inode_operations gas_file_inode_ops = {
    .getattr = gas_getattr,
};
const struct inode_operations gas_dir_inode_ops = {
    .create = gas_create,
    .lookup = gas_lookup,
    .link = gas_link,
    .unlink = gas_unlink,
    .symlink = gas_symlink,
    .mknod = gas_mknod,
    .mkdir = gas_mkdir,
    .rmdir = gas_rmdir,
    .getattr = gas_getattr,
};
const struct inode_operations gas_symlink_inode_ops = {
    .readlink = generic_readlink,
    .follow_link = page_follow_link_light,
    .put_link = page_put_link,
    .getattr = gas_getattr,
};
```

## File System Maker

The GAS file system maker is `/GAS_Filesystem/src/makefs/mkfs.c`.

This file system maker will open the virtual disk and initialize super block, block allocate map, inode allocate map and inode list, then it will create the root directory inode and do block cache synchronization.

Also this maker will print some basic information of the file system running on this virtual disk.

By execute `mkfs.gas`, you can format a virtual disk file into GAS file system style.

```shell
./mkfs.gas vdisk_name
```

<img src="https://s2.loli.net/2023/06/15/hq6IbuKWopA9mk7.png" alt="image.png" style="zoom:50%;" />

## Changelog

See [CHANGELOG.md](https://github.com/GuTaoZi/GAS_Filesystem/blob/main/CHANGELOG.md).