#include <linux/kernel.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/stat.h>

#include "gas.h"

static struct kmem_cache *gas_inode_cache;

static struct inode* gas_alloc_inode(struct super_block *sb)
{
	struct gas_inode_info *info = (struct gas_inode_info *)
		kmem_cache_alloc(gas_inode_cache, GFP_KERNEL);
	return info ? &info->inode : NULL;
}

void gas_destroy_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);
	kmem_cache_free(gas_inode_cache,
		container_of(inode, struct gas_inode_info, inode));
}

void gas_destroy_inode(struct inode *inode)
{
	call_rcu(&inode->i_rcu, gas_destroy_callback)
}

struct gas_inode *gas_get_inode(struct super_block *sb, ino_t )

static struct buffer_head *gas_update_ionde(struct inode *inode)
{
	struct buffer_head *bh;
	struct gas_inode_info *info = container_of(inode, struct gas_inode_info, inode);
	struct gas_inode *gnode = gas_get_inode(inode->i_sb, inode->i_ino, &bh);
	if (!gnode)
		return NULL;
	
}

int gas_write_inode(struct inode *inode, struct writeback_control *ctrl)
{
	int err = 0;
	struct buffer_head *bh = gas_update_ionde(inode);
	if (!bh)
		return -EIO;
	if (ctrl->sync_mode == WB_SYNC_ALL && buffer_dirty(bh))
	{
		sync_dirty_buffer(hb);
		if (buffer_req(bh) && !buffer_uptodate(bh))
			err = -EIO;
	}
	brelas(bh);
	return err;
}

void sfs_evict_inode(struct inode *inode)
{

}

void gas_put_super(struct super_block *super_block)
{

}

static int gas_statfs(struct dentry *dentry, struct kstatfs *kstatfs)
{

}

