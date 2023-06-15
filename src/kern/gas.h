#ifndef __GAS_H__
#define __GAS_H__

#define GAS_MAX_NAME_LEN 256
#define GAS_BLOCK_SIZE 4096 // byte
#define MAGIC_NUMBER 0x6A5000F5

#define BITS_PER_BLOCK          (8*GAS_BLOCK_SIZE)

#define SUPER_BLOCK_NO			0
#define GAS_BAD_INO			0
#define GAS_ROOT_INO			1
#define GAS_LINK_MAX			32000

#include <linux/fs.h>
#include <linux/types.h>

struct gas_super_block
{
    __le32 s_magic;
    __le32 s_blocksize;
    __le32 s_bam_blocks;
    __le32 s_iam_blocks;
    __le32 s_inode_blocks;
    __le32 s_nblocks;
    __le32 s_ninodes;
};
struct gas_sb_info
{
	__u32	s_magic;
	__u32	s_blocksize;
	__u32	s_bam_blocks;
	__u32	s_iam_blocks;
	__u32	s_inode_blocks;
	__u32	s_nblocks;
	__u32	s_ninodes;

	/* some additional info	*/
	__u32	s_inodes_per_block;
	__u32	s_bits_per_block;
	__u32	s_dir_entries_per_block;
	struct buffer_head **s_bam_bh;
	struct buffer_head **s_iam_bh;
	__u32	s_bam_last;
	__u32	s_iam_last;
	__u32	s_inode_list_start;
	__u32	s_data_block_start;
};

struct gas_inode
{
    __le16 i_mode;
    __le16 i_nlink;
    __le32 i_uid;
    __le32 i_gid;
    __le32 i_size;
    __le32 i_atime;
    __le32 i_mtime;
    __le32 i_ctime;
    __le32 i_blkaddr[9]; // 6+1+1+1
};

struct gas_inode_info
{
    __le32 blkaddr[9];
    struct inode inode;
};

struct gas_dir_entry
{
    char de_name[GAS_MAX_NAME_LEN];
    __le32 de_inode;
};

void gas_free_block(struct inode *inode, unsigned long block);
unsigned long gas_new_block(struct inode *inode, int *err);
unsigned long gas_count_free_blocks(struct super_block *sb);
void gas_free_inode(struct inode *inode);
struct inode *gas_new_inode(struct inode *dir, umode_t mode, int *err);
unsigned long gas_count_free_inodes(struct super_block *sb);
int gas_add_link(struct dentry *dentry, struct inode *inode);
int gas_make_empty(struct inode *inode, struct inode *dir);
struct gas_dir_entry *gas_find_entry(struct dentry *dentry, struct page **res_page);
int gas_delete_entry(struct gas_dir_entry *de, struct page *page);
int gas_empty_dir(struct inode *inode);
void gas_set_link(struct gas_dir_entry *de, struct page *page, struct inode *inode);
struct gas_dir_entry *gas_dotdot(struct inode *dir, struct page **p);
ino_t gas_inode_by_name(struct inode *dir, struct qstr *child);
void gas_inode_fill(struct gas_inode_info *, struct gas_inode const *);
static inline sector_t gas_inode_block(struct gas_sb_info const *sbi, ino_t ino)
{
	return (sector_t)(sbi->s_inode_list_start + 
		ino / sbi->s_inodes_per_block);
}
void gas_truncate(struct inode *);
void gas_evict_inode(struct inode *);
void gas_set_inode(struct inode *, dev_t);
struct inode *gas_iget(struct super_block *, ino_t);
struct gas_inode *gas_get_inode(struct super_block *, ino_t, struct buffer_head **);
struct buffer_head *gas_update_inode(struct inode *);
int gas_write_inode(struct inode *, struct writeback_control *);
int gas_writepage(struct page *, struct writeback_control *);
int gas_writepages(struct address_space *, struct writeback_control *);
int gas_readpage(struct file *, struct page *);
int gas_readpages(struct file *, struct address_space *, struct list_head *, unsigned);
ssize_t gas_direct_io(int, struct kiocb *, const struct iovec *, loff_t, unsigned long);
void gas_write_failed(struct address_space *, loff_t);
int gas_write_begin(struct file *, struct address_space *, loff_t, unsigned, unsigned, struct page **, void **);
int gas_write_end(struct file *, struct address_space *, loff_t, unsigned, unsigned, struct page *, void *);
sector_t gas_bmap(struct address_space *, sector_t);
void gas_put_super(struct super_block *);
struct inode *gas_alloc_inode(struct super_block *);
void gas_destroy_callback(struct rcu_head *);
void gas_destroy_inode(struct inode *);
int gas_statfs(struct dentry *dentry, struct kstatfs *kstatfs);

static inline struct gas_sb_info *GAS_SB(struct super_block *sb)
{
	return (struct gas_sb_info *)sb->s_fs_info;
}

static inline struct gas_inode_info *GAS_INODE(struct inode *inode)
{
	return container_of(inode, struct gas_inode_info, inode);
}

extern struct super_operations const gas_super_ops;
extern const struct address_space_operations gas_a_ops;
extern const struct file_operations gas_file_ops;
extern const struct inode_operations gas_file_inode_ops;
extern const struct inode_operations gas_symlink_inode_ops;
extern const struct inode_operations gas_dir_inode_ops;
extern rwlock_t pointers_lock;

#include "gas_namei.h"
#include "gas_itree.h"

#endif