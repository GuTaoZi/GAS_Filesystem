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
	return (struct gas_inode *)((*p)->b_data+offset);
}

int gas_write_inode(struct inode *inode, struct writeback_control *ctrl)
{
	int err = 0;
	struct buffer_head *bh = gas_update_ionde(inode);
	if (!bh)
		return -EIO;
	if (ctrl->sync_mode == WB_SYNC_ALL && buffer_dirty(bh))
	{
		sync_dirty_buffer(hb);
		if (buffer_req(bh) && !buffer_uptodate(bh))
			err = -EIO;
	}
	brelas(bh);
	return err;
}


struct buffer_head *gas_update_inode(struct inode *inode)
{
	struct buffer_head *bh;
	struct gas_inode_info *info = container_of(inode, struct gas_inode_info, inode);
	struct gas_inode *gnode = gas_get_inode(inode->i_sb, inode->i_ino, &bh);
	if (!gnode)
		return NULL;
	gnode->i_size = cpu_to_le32(inode->i_size);
	gnode->i_mode = cpu_to_le16(inode->i_mode);
	gnode->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
	gnode->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
	gnode->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);
	gnode->i_uid = cpu_to_le32(i_uid_read(inode));
	gnode->i_gid = cpu_to_le32(i_gid_read(inode));
	gnode->i_nlink = cpu_to_le16(inode->i_nlink);
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode)) {
		gnode->i_blkaddr[0] = cpu_to_le32(new_encode_dev(inode->i_rdev));
		for (i = 1; i < 9; i++)
			gnode->i_blkaddr[i] = 0;
	} else for (i = 0; i < 9; i++)
			gnode->i_blkaddr[i] = gnode->blkaddr[i];
	mark_buffer_dirty(bh);
	return bh;
}

void sfs_evict_inode(struct inode *inode)
{

}