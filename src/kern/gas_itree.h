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
}; /* Have triple indirect */

DEFINE_RWLOCK(pointers_lock);
/*
int block_to_path(struct inode *, long, int[DEPTH]);

block_t *i_data(struct inode *inode);

void add_chain(Indirect *, struct buffer_head *, block_t *);

int verify_chain(Indirect *, Indirect *);

block_t *block_end(struct buffer_head *);

Indirect *get_branch(struct inode *, int, int *, Indirect[DEPTH], int *);

int alloc_branch(struct inode *, int, int *, Indirect *);

int splice_branch(struct inode *, Indirect chain[DEPTH], Indirect *, int);

int get_block(struct inode *, sector_t, struct buffer_head *, int);

int all_zeroes(block_t *, block_t *);

Indirect *find_shared(struct inode *, int, int[DEPTH], Indirect[DEPTH], block_t *);

void free_data(struct inode *, block_t *, block_t *);

void free_branches(struct inode *, block_t *, block_t *, int);

void truncate(struct inode *);

unsigned nblocks(loff_t, struct super_block *);
*/
unsigned gas_blocks(loff_t, struct super_block *);

void gas_truncate_inode(struct inode *);

int gas_get_block(struct inode *, sector_t, struct buffer_head *, int);

#endif