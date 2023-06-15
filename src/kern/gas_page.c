#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/slab.h>

#include "gas.h"

int gas_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, gas_get_block, wbc);
}

int gas_writepages(struct address_space *mapping, struct writeback_control *wbc)
{
	return mpage_writepages(mapping, wbc, gas_get_block);
}

int gas_readpage(struct file *file, struct page *page)
{
	return mpage_readpage(page, gas_get_block);
}

int gas_readpages(struct file *file, struct address_space *mapping,
    struct list_head *pages, unsigned nr_pages)
{
	return mpage_readpages(mapping, pages, nr_pages, gas_get_block);
}

ssize_t gas_direct_io(int rw, struct kiocb *iocb,
		const struct iovec *iov, loff_t off, unsigned long nr_segs)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	return blockdev_direct_IO(rw, iocb, inode, iov, off, nr_segs, gas_get_block);
}

void gas_write_failed(struct address_space *mapping, loff_t to)
{
	struct inode *inode = mapping->host;
	if (to > inode->i_size) {
		truncate_pagecache(inode, inode->i_size);
		gas_truncate(inode);
	}	
}

int gas_write_begin(struct file *file, struct address_space *mapping, loff_t pos,
    unsigned len, unsigned flags, struct page **pagep, void **fsdata)
{
	int ret;
	ret = block_write_begin(mapping, pos, len, flags, pagep, gas_get_block);
	if (ret < 0)
		gas_write_failed(mapping, pos + len);
	return ret;
}

int gas_write_end(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned copied,
		struct page *page, void *fsdata)
{
	int ret;
	ret = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
	mark_inode_dirty(mapping->host);
	if (ret < len)
		gas_write_failed(mapping, pos + len);
	return ret;
}

sector_t gas_bmap(struct address_space *mapping, sector_t block)
{
	return generic_block_bmap(mapping, block, gas_get_block);
}