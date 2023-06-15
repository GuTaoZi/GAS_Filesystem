#include "gas.h"
#include <linux/aio.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/slab.h>
#include <linux/writeback.h>

/* User interfaces*/
struct gas_inode *gas_get_inode(struct super_block *sb, ino_t ino, struct buffer_head **p)
{
    struct gas_sb_info *sb_info = sb->s_fs_info;
    size_t block, offset;
    block = gas_inode_block(sb_info, ino);
    offset = gas_inode_offset(sb_info, ino);
    *p = sb_bread(sb, block);
    if (!*p)
        return NULL;
    return (struct gas_inode *)((*p)->b_data + offset);
}

void gas_truncate(struct inode * inode)
{
	if (!(S_ISREG(inode->i_mode) || 
		S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode)))
		return;
	gas_truncate_inode(inode);
}

extern const struct file_operations gas_dir_ops;

void gas_set_inode(struct inode *inode, dev_t rdev)
{
	inode->i_mapping->a_ops = &gas_a_ops;
	if (S_ISREG(inode->i_mode))
    {
		inode->i_op = &gas_file_inode_ops;
		inode->i_fop = &gas_file_ops;
	}
    else if (S_ISDIR(inode->i_mode))
    {
		inode->i_op = &gas_dir_inode_ops;
		inode->i_fop = &gas_dir_ops;
	}
    else if (S_ISLNK(inode->i_mode))
		inode->i_op = &gas_symlink_inode_ops;
	else { 
		inode->i_mapping->a_ops = NULL;
		init_special_inode(inode, inode->i_mode, rdev);
	}
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
        sync_dirty_buffer(bh);
        if (buffer_req(bh) && !buffer_uptodate(bh))
            err = -EIO;
    }
    brelse(bh);
    return err;
}

struct buffer_head *gas_update_inode(struct inode *inode)
{
    struct buffer_head *bh;
    struct gas_inode_info *info = container_of(inode, struct gas_inode_info, inode);
    struct gas_inode *gnode = gas_get_inode(inode->i_sb, inode->i_ino, &bh);
    int i;
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
        for (i = 1; i < 9; i++)
            gnode->i_blkaddr[i] = 0;
    }
    else{
        for (i = 0; i < 9; i++)
            gnode->i_blkaddr[i] = info->blkaddr[i];
    }
    mark_buffer_dirty(bh);
    return bh;
}

void gas_evict_inode(struct inode *inode)
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

void gas_inode_fill(struct gas_inode_info *info,
			struct gas_inode const *gnode)
{
	int i;

	info->inode.i_mode = le16_to_cpu(gnode->i_mode);
	info->inode.i_size = le32_to_cpu(gnode->i_size);
	info->inode.i_ctime.tv_sec = le32_to_cpu(gnode->i_ctime);
	info->inode.i_atime.tv_sec = le32_to_cpu(gnode->i_atime);
	info->inode.i_mtime.tv_sec = le32_to_cpu(gnode->i_mtime);
	info->inode.i_mtime.tv_nsec = info->inode.i_atime.tv_nsec =
				info->inode.i_ctime.tv_nsec = 0;
	i_uid_write(&info->inode, (uid_t)le32_to_cpu(gnode->i_uid));
	i_gid_write(&info->inode, (gid_t)le32_to_cpu(gnode->i_gid));
	set_nlink(&info->inode, le16_to_cpu(gnode->i_nlink));
	for (i = 0; i < 9; i++) 
		info->blkaddr[i] = gnode->i_blkaddr[i];
}

struct inode *gas_iget(struct super_block *sb, ino_t ino)
{
	struct gas_sb_info *sb_info = GAS_SB(sb);
	struct buffer_head *bh;
	struct gas_inode *gnode;
	struct gas_inode_info *info;
	struct inode *inode;
	size_t block, offset;
	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;
	info = GAS_INODE(inode);
	block = gas_inode_block(sb_info, ino);
	offset = gas_inode_offset(sb_info, ino);
	bh = sb_bread(sb, block);
	if (!bh)
		goto read_error;
	gnode = (struct gas_inode *)(bh->b_data + offset);
	gas_inode_fill(info, gnode);
	brelse(bh);
	gas_set_inode(inode, new_decode_dev(le32_to_cpu(info->blkaddr[0]))); 
	unlock_new_inode(inode);
	return inode;

read_error:
	iget_failed(inode);
	return ERR_PTR(-EIO);
}

