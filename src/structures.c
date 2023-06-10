#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>

const struct file_operations sfs_file_ops = {
	.llseek = generic_file_llseek, //opt
    
	.read = new_sync_read,
	.read_iter = generic_file_read_iter,
    
	.write = do_sync_write,
	.write_iter = generic_file_aio_write, //opt
	.mmap = generic_file_mmap,   //opt
	.splice_read = generic_file_splice_read, //opt
	.splice_write = generic_file_splice_write //opt
};

const struct address_space_operations sfs_aops = { // Optional
	.readpage = sfs_readpage,
	.readpages = sfs_readpages,
	.writepage = sfs_writepage,
	.writepages = sfs_writepages,
	.write_begin = sfs_write_begin,
	.write_end = sfs_write_end,
	.bmap = sfs_bmap, 
	.direct_IO = sfs_direct_io
};

static struct file_system_type sfs_type = {
	.owner			= THIS_MODULE,
	.name			= "sfs",
	.mount			= sfs_mount,
	.kill_sb		= kill_block_super,
	.fs_flags		= FS_REQUIRES_DEV
};

static struct super_operations const sfs_super_ops = {
	.alloc_inode		= sfs_alloc_inode,
	.destroy_inode		= sfs_destroy_inode,
	.write_inode		= sfs_write_inode,
	.evict_inode		= sfs_evict_inode,
	.put_super		= sfs_put_super,
	.statfs			= sfs_statfs,
};

static struct inode_operations assoofs_inode_ops = {
	.create 	= assoofs_create,
	.lookup 	= assoofs_lookup,
	.mkdir 		= assoofs_mkdir,
};