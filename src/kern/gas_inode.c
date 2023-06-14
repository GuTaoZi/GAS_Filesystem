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

struct gas_inode_info {
    __le32          blkaddr[9];
    struct inode	inode;
};

static struct inode_operations gas_inode_ops = {
    .create     = gas_create,
    .lookup     = gas_lookup,
    .mkdir      = gas_mkdir,
    .permission = gas_permission
};
*/

#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/slab.h>

#include "gas.h"

/* User interfaces*/

static int gas_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);

static struct dentry *gas_lookup(struct inode *dir, struct dentry *dentry, unsigned flags);

static int gas_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

struct gas_inode *gas_get_inode(struct super_block *sb, ino_t ino, struct buffer_head **p)
{
    struct gas_sb_info *sb_info = sb->s_sf_info;
    size_t block, offset;
    block = gas_inode_block(sb_info, ino);
    offset = gas_inode_offset(sb_info, ino);
    *p = sb_bread(sb, block);
    if (!*p)
        return NULL;
    return (struct gas_inode *)((*p)->b_data + offset);
}


struct buffer_head *gas_update_inode(struct inode *inode)
{
    struct buffer_head *bh;
    struct gas_inode_info *info = container_of(inode, struct gas_inode_info, inode);
    struct gas_inode *gnode = gas_get_inode(inode->i_sb, inode->i_ino, &bh);
    if (!gnode)
        return NULL;
    // CPU字节序转换到小端字节序
    gnode->i_size = cpu_to_le32(inode->i_size);
    gnode->i_mode = cpu_to_le16(inode->i_mode);
    gnode->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
    gnode->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
    gnode->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);
    gnode->i_uid = cpu_to_le32(i_uid_read(inode));
    gnode->i_gid = cpu_to_le32(i_gid_read(inode));
    gnode->i_nlink = cpu_to_le16(inode->i_nlink);
    // 确认文件类型（以字节为单位还是块为单位）
    if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
    {
        // i_rdev是一个用来存储inode对应的设备号的变量
        // 仅在inode代表一个字符设备或块设备时才有意义
        gnode->i_blkaddr[0] = cpu_to_le32(new_encode_dev(inode->i_rdev));
        for (int i = 1; i < 9; i++)
            gnode->i_blkaddr[i] = 0;
    }
    else
        for (int i = 0; i < 9; i++)
            gnode->i_blkaddr[i] = info->blkaddr[i];
    mark_buffer_dirty(bh);
    return bh;
}

int gas_write_inode(struct inode *inode, struct writeback_control *ctrl)
{
    int err = 0;
    struct buffer_head *bh = gas_update_inode(inode);
    if (!bh)
        return -EIO;
    // 缓冲头请求写回并且并由于写入失败变成不被更新的状态
    if (ctrl->sync_mode == WB_SYNC_ALL && buffer_dirty(bh))
    {
        sync_dirty_buffer(hb);
        if (buffer_req(bh) && !buffer_uptodate(bh))
            err = -EIO;
    }
    brelas(bh);
    return err;
}

void sfs_evict_inode(struct inode *inode)
{
    truncate_inode_pages(&inode->i_data, 0);
	if (!inode->i_nlink) {
		inode->i_size = 0;
		gas_truncate(inode);
	}
	invalidate_inode_buffers(inode);
	clear_inode(inode);
	if (!inode->i_nlink)
		gas_free_inode(inode); // in bitmap.c
}


