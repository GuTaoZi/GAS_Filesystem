#ifndef __SUPER_H__
#define __SUPER_H__

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vfs.h>
#include "gas.h"

static struct file_system_type gas_fs_type = {
    .owner = THIS_MODULE, .name = "gas", .mount = gas_mount, .kill_sb = kill_block_super, .fs_flags = FS_REQUIRES_DEV};

static int __init init_gas_fs(void)
{
    printk(KERN_INFO "GAS File Sysem Initialized.\n");
    printk(KERN_INFO "Made by GuTao, Artanisax, ShadowStorm.\n");
    return register_filesystem(&gas_fs_type);
}

static void __exit exit_gas_fs(void)
{
    unregister_filesystem(&gas_fs_type);
    printk(KERN_INFO "GAS File Sysem Exited.\n");
}

// Micro Bonding
module_init(init_gas_fs);
module_exit(exit_gas_fs);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gas file system");

#endif