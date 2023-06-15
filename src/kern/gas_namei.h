#ifndef __GAS_NAMEI_H__
#define __GAS_NAMEI_H__

#include <linux/fs.h>
#include <linux/types.h>
#include <stdbool.h>
// #include "gas.h"

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

#endif