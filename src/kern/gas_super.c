#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/vfs.h>
#include <linux/writeback.h>

#include "gas.h"

// Global Variable
static struct kmem_cache *gas_inode_cache;

/*
static void gas_inode_init_once(void *p)
{
    // 提供给内核用的
	struct gas_inode_info *info = (struct gas_inode_info *)p;
	inode_init_once(&(info->inode));
}

static int gas_inode_cache_create(void)
{
	// 为inode_info申请缓存
	gas_inode_cache = kmem_cache_create("gas_inode",
		sizeof(struct gas_inode_info), 0,
		(SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD), gas_inode_init_once);
	if (gas_inode_cache == NULL)
		return -ENOMEM;
	return 0;
}

static void gas_inode_cache_destroy(void)
{
	rcu_barrier();
	kmem_cache_destroy(gas_inode_cache);
	gas_inode_cache = NULL;
}*/

struct inode *gas_alloc_inode(struct super_block *sb)
{
    // 在cache里申请一个node_info
    struct gas_inode_info *info = (struct gas_inode_info *)kmem_cache_alloc(gas_inode_cache, GFP_KERNEL);
    return info ? &info->inode : NULL;
}

void gas_destroy_callback(struct rcu_head *head)
{
    // RCU: Ready-Copy-Update
    // 写者复制更新，直到没有读者才写回
    struct inode *inode = container_of(head, struct inode, i_rcu);
    kmem_cache_free(gas_inode_cache, container_of(inode, struct gas_inode_info, inode));
}

void gas_destroy_inode(struct inode *inode)
{
    call_rcu(&inode->i_rcu, gas_destroy_callback);
}

// void gas_put_super(struct super_block *sb)
// {
// 	// 释放超级块
// 	struct gas_sb_info *sb_info = sb->s_sf_info;
// 	if (sb_info) {
// 		int i;
// 		for (i = 0; i < sb_info->s_bam_blocks; i++)
// 			brelse(sb_info->s_bam_bh[i]); //
// 		for (i = 0; i < sb_info->s_iam_blocks; i++)
// 			brelse(sb_info->s_iam_bh[i]); //
// 		kfree(sb_info->s_bam_bh);
// 		kfree(sb_info);
// 	}
// 	sb->s_fs_info = NULL;
// }

// 暂时看不懂
int gas_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    struct super_block *sb = dentry->d_sb;
    struct gas_sb_info *sb_info = sb->s_fs_info;
    u64 id = huge_encode_dev(sb->s_bdev->bd_dev);
    buf->f_type = sb->s_magic;
    buf->f_bsize = sb->s_blocksize;
    buf->f_blocks = sb_info->s_nblocks - sb_info->s_data_block_start;
    buf->f_bfree = gas_count_free_blocks(sb);
    buf->f_bavail = buf->f_bfree;
    buf->f_files = sb_info->s_ninodes;
    buf->f_ffree = gas_count_free_inodes(sb);
    buf->f_namelen = GAS_MAX_NAME_LEN;
    buf->f_fsid.val[0] = (u32)id;
    buf->f_fsid.val[1] = (u32)(id >> 32);
    return 0;
}
