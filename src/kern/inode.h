#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/slab.h>

void sfs_inode_fill(struct sfs_inode_info *, struct sfs_inode const *);

sector_t sfs_inode_block(struct sfs_sb_info const *, ino_t);

size_t sfs_inode_offset(struct sfs_sb_info const *, ino_t);

void sfs_truncate(struct inode * );

void sfs_evict_inode(struct inode *);

void sfs_set_inode(struct inode *, dev_t);

struct inode *sfs_iget(struct super_block *, ino_t);

struct sfs_inode *sfs_get_inode(struct super_block *, ino_t, struct buffer_head **);

struct buffer_head *sfs_update_inode(struct inode *);

int sfs_write_inode(struct inode *, struct writeback_control *);

int sfs_writepage(struct page *, struct writeback_control *);

int sfs_writepages(struct address_space *, struct writeback_control *)

int sfs_readpage(struct file *, struct page *)

int sfs_readpages(struct file *, struct address_space *, struct list_head *, unsigned);

ssize_t sfs_direct_io(int, struct kiocb *, const struct iovec *, loff_t, unsigned long);

void sfs_write_failed(struct address_space *, loff_t);

int sfs_write_begin(struct file *, struct address_space *, loff_t,
                    unsigned, unsigned, struct page **, void **);

int sfs_write_end(struct file *, struct address_space *, loff_t,
                    unsigned, unsigned, struct page *, void *);

sector_t sfs_bmap(struct address_space *, sector_t);


const struct address_space_operations sfs_aops = {
	.readpage = sfs_readpage,
	.readpages = sfs_readpages,
	.writepage = sfs_writepage,
	.writepages = sfs_writepages,
	.write_begin = sfs_write_begin,
	.write_end = sfs_write_end,
	.bmap = sfs_bmap, 
	.direct_IO = sfs_direct_io
};
