/*
struct gas_inode {
    __le16 i_mode;
    __le16 i_nlink;
    __le32 i_uid;
    __le32 i_gid;
    __le32 i_size;
    __le32 i_atime;
    __le32 i_mtime;
    __le32 i_ctime;
    __le32 i_blkaddr[9];    //6+1+1+1
};
static struct inode_operations gas_inode_ops = {
    .create     = gas_create,
    .lookup     = gas_lookup,
    .mkdir      = gas_mkdir,
    .permission = gas_permission
};
*/
#include "gas.h"

/* User interfaces*/

static int gas_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);

static struct dentry *gas_lookup(struct inode *dir, struct dentry *dentry, unsigned flags);

static int gas_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

static int gas_permission(struct inode *dir, );