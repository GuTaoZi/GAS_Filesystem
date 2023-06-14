#ifndef __SUPER_H__ 
#define __SUPER_H__

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vfs.h>

// Global Variable
static struct kmem_cache *sfs_inode_cache;

static struct super_operations const sfs_super_ops = {
	.alloc_inode		= sfs_alloc_inode,
	.destroy_inode		= sfs_destroy_inode,
	.write_inode		= sfs_write_inode,
	.evict_inode		= sfs_evict_inode,
	.put_super		= sfs_put_super,
	.statfs			= sfs_statfs,
};

// Function Declaration
void sfs_put_super(struct super_block *);

void sfs_super_block_fill(struct sfs_sb_info *, struct sfs_super_block const *);

static struct sfs_sb_info *sfs_super_block_read(struct super_block *);

static int sfs_statfs(struct dentry *, struct kstatfs *);

struct inode *sfs_alloc_inode(struct super_block *);

void sfs_destroy_callback(struct rcu_head *);

void sfs_destroy_inode(struct inode *);

void sfs_inode_init_once(void *);

int sfs_inode_cache_create(void);

void sfs_inode_cache_destroy(void);

int sfs_fill_super(struct super_block *, void *, int);

struct dentry *sfs_mount(struct file_system_type *, int, char const *, void *);

static int __init init_sfs_fs(void);

static void __exit exit_sfs_fs(void);

// Micro Bonding
module_init(init_sfs_fs);
module_exit(exit_sfs_fs); 
