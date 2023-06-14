#ifndef __GAS_NAMEI_H__
#define __GAS_NAMEI_H__

#include <linux/fs.h>

int gas_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t rdev);
int gas_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);
struct dentry *gas_lookup(struct inode *dir, struct dentry *dentry, unsigned flags);
int gas_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);
int gas_symlink(struct inode *dir, struct dentry *dentry, const char *symname);
int gas_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry);
int gas_unlink(struct inode *dir, struct dentry *dentry);
int gas_rmdir(struct inode *dir, struct dentry *dentry);
int gas_rename(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry);
int gas_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat);

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

#endif