#ifndef __DEF_H__
#define __DEF_H__

#define GAS_MAX_NAME_LEN 256
#define GAS_BLOCK_SIZE 4096 // byte
#define MAGIC_NUMBER 0x6A5000F5

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>

#include "gas.h"

/**************************************
 *     Functions declaration          *
 **************************************/

// file_operations
static ssize_t gas_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t gas_write(struct file *, const char __user *, size_t, loff_t *);

// file_system_type
static struct dentry* gas_mount(struct file_system_type *, int, const char *, void *);

// super_operations
static struct inode* gas_alloc_inode(struct super_block *);
static void gas_destroy_inode(struct inode *);
static int gas_write_inode(struct inode *, struct writeback_control *);
static void gas_evict_inode(struct inode *);
static void gas_put_super(struct super_block *);
static int gas_statfs(struct dentry *, struct kstatfs *);

// inode_operations
static int gas_create(struct user_namespace *, struct inode *,struct dentry *, umode_t, bool);
static struct dentry* gas_lookup(struct inode *,struct dentry *, unsigned int);
static int gas_mkdir(struct user_namespace *, struct inode *,struct dentry *, umode_t);
static int gas_permission(struct user_namespace *, struct inode *, int);

/**************************************
 *     Structure declaration          *
 **************************************/
const struct file_operations gas_file_ops = {
    .llseek         = generic_file_llseek,
    
    .read           = gas_read,
    .read_iter      = generic_file_read_iter,
    
    .write          = gas_write,
    .write_iter     = generic_file_write_iter,
    .mmap           = generic_file_mmap,
    .splice_read    = generic_file_splice_read,
    .splice_write   = iter_file_splice_write
};

const static struct file_system_type gas_fs_type = {
    .owner      = THIS_MODULE,
    .name       = "gas",
    .mount      = gas_mount,
    .kill_sb    = kill_block_super,
    .fs_flags   = FS_REQUIRES_DEV
};

const static struct super_operations gas_super_ops = {
    .alloc_inode    = gas_alloc_inode,
    .destroy_inode  = gas_destroy_inode,
    .write_inode    = gas_write_inode,
    .evict_inode    = gas_evict_inode,
    .put_super      = gas_put_super,
    .statfs         = gas_statfs,
};

const static struct inode_operations gas_inode_ops = {
    .create     = gas_create,
    .lookup     = gas_lookup,
    .mkdir      = gas_mkdir,
    .link       = gas_link,
    .gas        = gas_symlink,
    .unlink     = gas_unlink,
	.rmdir		= gas_rmdir,
    .mknod		= sfs_mknod
};

#endif