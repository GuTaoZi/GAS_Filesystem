#include "gas.h"

static struct inode* gas_alloc_inode(struct super_block *sb)
{
	struct gas_inode_info *si =
        (struct gas_inode_info *) kmem_cache_alloc(gas_inode_cache, GFP_KERNEL);
	if (!si)
		return NULL;
	return &si->vfs_inode;
}

static void gas_destroy_inode(struct inode *inode)
{

}

static int gas_write_inode(struct inode *inode, struct writeback_control *)
{

}

static void gas_evict_inode(struct inode *inode)
{

}

static void gas_put_super(struct super_block *super_block)
{

}

static int gas_statfs(struct dentry *dentry, struct kstatfs *kstatfs)
{

}

