#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/slab.h>

void gas_inode_fill(struct gas_inode_info *, struct gas_inode const *);

sector_t gas_inode_block(struct gas_sb_info const *, ino_t);

size_t gas_inode_offset(struct gas_sb_info const *, ino_t);

void gas_truncate(struct inode * );

void gas_evict_inode(struct inode *);

void gas_set_inode(struct inode *, dev_t);

struct inode *gas_iget(struct super_block *, ino_t);

struct gas_inode *gas_get_inode(struct super_block *, ino_t, struct buffer_head **);

struct buffer_head *gas_update_inode(struct inode *);

int gas_write_inode(struct inode *, struct writeback_control *);

int gas_writepage(struct page *, struct writeback_control *);

int gas_writepages(struct address_space *, struct writeback_control *)

int gas_readpage(struct file *, struct page *)

int gas_readpages(struct file *, struct address_space *, struct list_head *, unsigned);

ssize_t gas_direct_io(int, struct kiocb *, const struct iovec *, loff_t, unsigned long);

void gas_write_failed(struct address_space *, loff_t);

int gas_write_begin(struct file *, struct address_space *, loff_t,
                    unsigned, unsigned, struct page **, void **);

int gas_write_end(struct file *, struct address_space *, loff_t,
                    unsigned, unsigned, struct page *, void *);

sector_t gas_bmap(struct address_space *, sector_t);


const struct address_space_operations gas_aops = {
	.readpage = gas_readpage,
	.readpages = gas_readpages,
	.writepage = gas_writepage,
	.writepages = gas_writepages,
	.write_begin = gas_write_begin,
	.write_end = gas_write_end,
	.bmap = gas_bmap, 
	.direct_IO = gas_direct_io
};
