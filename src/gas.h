#ifndef __GAS_H__
#define __GAS_H__

#include "def.h"

#include <linux/types.h>
#include <linux/fs.h>

struct gas_super_block {
    __le32 s_magic;
    __le32 s_blocksize;
    __le32 s_bam_blocks;
    __le32 s_iam_blocks;
    __le32 s_inode_blocks;
    __le32 s_nblocks;
    __le32 s_ninodes;
};

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

struct gas_dir_entry {
    char de_name[GAS_MAX_NAME_LEN];
    __le32 de_inode;
};

#endif