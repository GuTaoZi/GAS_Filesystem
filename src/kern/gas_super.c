#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/vfs.h>
#include <linux/writeback.h>
#include <linux/init.h>
#include <linux/module.h>

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
static inline void gas_super_block_fill(struct gas_sb_info *sbi,
			struct gas_super_block const *dsb)
{
	sbi->s_magic = le32_to_cpu(dsb->s_magic);
	sbi->s_blocksize = le32_to_cpu(dsb->s_blocksize);
	sbi->s_bam_blocks = le32_to_cpu(dsb->s_bam_blocks);
	sbi->s_iam_blocks = le32_to_cpu(dsb->s_iam_blocks);
	sbi->s_inode_blocks = le32_to_cpu(dsb->s_inode_blocks);
	sbi->s_nblocks = le32_to_cpu(dsb->s_nblocks);
	sbi->s_ninodes = le32_to_cpu(dsb->s_ninodes);
	sbi->s_inodes_per_block = sbi->s_blocksize / sizeof(struct gas_inode); 
	sbi->s_bits_per_block = 8*sbi->s_blocksize;
	sbi->s_dir_entries_per_block =
			sbi->s_blocksize / sizeof(struct gas_dir_entry);
	sbi->s_inode_list_start = sbi->s_bam_blocks + sbi->s_iam_blocks + 1; 
	sbi->s_data_block_start = sbi->s_inode_list_start + sbi->s_inode_blocks;
}

static struct gas_sb_info *gas_super_block_read(struct super_block *sb)
{
	struct gas_sb_info *sbi = (struct gas_sb_info *)
			kzalloc(sizeof(struct gas_sb_info), GFP_NOFS);
	struct gas_super_block *dsb;
	struct buffer_head *bh;

	if (!sbi) {
		pr_err("gas cannot allocate super block\n");
		return NULL;
	}

	bh = sb_bread(sb, SUPER_BLOCK_NO);
	if (!bh) {
		pr_err("cannot read super block\n");
		goto free_memory;
	}

	dsb = (struct gas_super_block *)bh->b_data;
	gas_super_block_fill(sbi, dsb);
	brelse(bh);

	if (sbi->s_magic != MAGIC_NUMBER) {
		pr_err("wrong magic number %lu\n",
			(unsigned long)sbi->s_magic);
		goto free_memory;
	}

	return sbi;

free_memory:
	kfree(sbi);
	return NULL;
}

static int gas_fill_super(struct super_block *sb, void *data, int silent)
{
	struct gas_sb_info *sbi = gas_super_block_read(sb);
	struct buffer_head **map;
	struct inode *root;
	unsigned long i, block;

	if (!sbi)
		return -EINVAL;

	sb->s_magic = sbi->s_magic;
	sb->s_fs_info = sbi;
	sb->s_op = &gas_super_ops;
	sb->s_max_links = GAS_LINK_MAX;

	if (sb_set_blocksize(sb, sbi->s_blocksize) == 0) {
		pr_err("device does not support block size %lu\n",
			(unsigned long)sbi->s_blocksize);
		return -EINVAL;
	}

	if (!sbi->s_bam_blocks || !sbi->s_iam_blocks || !sbi->s_inode_blocks) {
		pr_err("Invalid meta: BAM(%ld), IAM(%ld), Inode list(%ld)\n",
			(long)sbi->s_bam_blocks, 
			(long)sbi->s_iam_blocks, 
			(long)sbi->s_inode_blocks);
		return -EINVAL;
	}	 

	map = kzalloc(sizeof(struct buffer_head *) * 
			(sbi->s_bam_blocks + sbi->s_iam_blocks), GFP_KERNEL);
	sbi->s_bam_bh = &map[0]; 
	sbi->s_iam_bh = &map[sbi->s_bam_blocks];

	block = 1;
	for (i = 0; i < sbi->s_bam_blocks; i++) {
		sbi->s_bam_bh[i] = sb_bread(sb, block);
		if (!sbi->s_bam_bh[i])
			goto error;
		block++;
	}  
	for (i = 0; i < sbi->s_iam_blocks; i++) {
		sbi->s_iam_bh[i] = sb_bread(sb, block);
		if (!sbi->s_iam_bh[i])
			goto error;
		block++;
	}  
	sbi->s_bam_last = 0;
	sbi->s_iam_last = 0;

	root = gas_iget(sb, GAS_ROOT_INO);
	if (IS_ERR(root))
		return PTR_ERR(root);

	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		pr_err("gas cannot create root\n");
		return -ENOMEM;
	}
	return 0;

error:
	for (i = 0; i < sbi->s_bam_blocks; i++)
		brelse(sbi->s_bam_bh[i]);
	for (i = 0; i < sbi->s_iam_blocks; i++)
		brelse(sbi->s_iam_bh[i]);
	kfree(map);
	return -EIO;	
}

static struct dentry *gas_mount(struct file_system_type *type, int flags,
			char const *dev, void *data)
{
	struct dentry *entry = 
			mount_bdev(type, flags, dev, data, gas_fill_super);

	if (IS_ERR(entry))
		pr_err("gas mounting failed\n");
	else
		pr_debug("gas mounted\n");
	return entry;
}

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

void gas_put_super(struct super_block *sb)
{
	// 释放超级块
	struct gas_sb_info *sb_info = sb->s_fs_info;
	if (sb_info) {
		int i;
		for (i = 0; i < sb_info->s_bam_blocks; i++)
			brelse(sb_info->s_bam_bh[i]); //
		for (i = 0; i < sb_info->s_iam_blocks; i++)
			brelse(sb_info->s_iam_bh[i]); //
		kfree(sb_info->s_bam_bh);
		kfree(sb_info);
	}
	sb->s_fs_info = NULL;
}

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

struct super_operations const gas_super_ops = {
    .alloc_inode = gas_alloc_inode,
    .destroy_inode = gas_destroy_inode,
    .write_inode = gas_write_inode,
    .evict_inode = gas_evict_inode,
    .put_super = gas_put_super,
    .statfs = gas_statfs,
};

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