#include "gas.h"

struct super_operations const gas_super_ops = {
    .alloc_inode = gas_alloc_inode,
    .destroy_inode = gas_destroy_inode,
    .write_inode = gas_write_inode,
    .evict_inode = gas_evict_inode,
    .put_super = gas_put_super,
    .statfs = gas_statfs,
};
const struct address_space_operations gas_a_ops = {
    .readpage = gas_readpage,
    .readpages = gas_readpages,
    .writepage = gas_writepage,
    .writepages = gas_writepages,
    .write_begin = gas_write_begin,
    .write_end = gas_write_end,
    .bmap = gas_bmap,
    .direct_IO = gas_direct_io
};

const struct file_operations gas_file_ops = {
    .llseek = generic_file_llseek,
    .read = do_sync_read,
    .aio_read = generic_file_aio_read,
    .write = do_sync_write,
    .aio_write = generic_file_aio_write,
    .mmap = generic_file_mmap,
    .splice_read = generic_file_splice_read,
    .splice_write = generic_file_splice_write
};


// operations for a file inode
const struct inode_operations gas_file_inode_ops = {
    .getattr = gas_getattr,
};

// operations for a link inode
const struct inode_operations gas_symlink_inode_ops = {
    .readlink = generic_readlink,
    .follow_link = page_follow_link_light,
    .put_link = page_put_link,
    .getattr = gas_getattr,
};

// operations for a directroy inode
const struct inode_operations gas_dir_inode_ops = {
    .create = gas_create,
    .lookup = gas_lookup,
    .link = gas_link,
    .unlink = gas_unlink,
    .symlink = gas_symlink,
    .mknod = gas_mknod,
    .mkdir = gas_mkdir,
    .rmdir = gas_rmdir,
    .getattr = gas_getattr,
};

DEFINE_RWLOCK(pointers_lock);