#ifndef __SUPER_H__ 
#define __SUPER_H__

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vfs.h>

// Global Variable
static struct kmem_cache *gas_inode_cache;

static struct super_operations const gas_super_ops = {
	.alloc_inode		= gas_alloc_inode,
	.destroy_inode		= gas_destroy_inode,
	.write_inode		= gas_write_inode,
	.evict_inode		= gas_evict_inode,
	.put_super		= gas_put_super,
	.statfs			= gas_statfs,
};

// Function Declaration
void gas_put_super(struct super_block *);

void gas_super_block_fill(struct gas_sb_info *, struct gas_super_block const *);

static struct gas_sb_info *gas_super_block_read(struct super_block *);

static int gas_statfs(struct dentry *, struct kstatfs *);

struct inode *gas_alloc_inode(struct super_block *);

void gas_destroy_callback(struct rcu_head *);

void gas_destroy_inode(struct inode *);

void gas_inode_init_once(void *);

int gas_inode_cache_create(void);

void gas_inode_cache_destroy(void);

int gas_fill_super(struct super_block *, void *, int);

struct dentry *gas_mount(struct file_system_type *, int, char const *, void *);

static int __init init_gas_fs(void);

static void __exit exit_gas_fs(void);

// Micro Bonding
module_init(init_gas_fs);
module_exit(exit_gas_fs); 
