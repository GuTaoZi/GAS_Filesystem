#ifndef __DEF_H__
#define __DEF_H__

#include "def.h"
#include "gas.h"
#include <linux/init.h>
#include <linux/module.h>

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