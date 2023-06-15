#ifndef __GAS_ITREE_H__
#define __GAS_ITREE_H__

#include <linux/buffer_head.h>

#define block_to_cpu le32_to_cpu
#define cpu_to_block cpu_to_le32
#define DIRCOUNT 6
#define INDIRCOUNT(sb) (1 << ((sb)->s_blocksize_bits - 2))

typedef u32 block_t; /* 32 bit, little-endian */

typedef struct
{
    block_t *p;
    block_t key;
    struct buffer_head *bh;
} Indirect;

enum
{
    DIRECT = 6,
    DEPTH = 4
}; // Have triple indirect

unsigned gas_blocks(loff_t, struct super_block *);
void gas_truncate_inode(struct inode *);
int gas_get_block(struct inode *, sector_t, struct buffer_head *, int);

#endif