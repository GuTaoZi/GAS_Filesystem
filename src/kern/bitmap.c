/* Inode and block bitmaps for GAS */
/*
 *  linux/fs/minix/bitmap.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * Modified for 680x0 by Hamish Macdonald
 * Fixed for 680x0 by Andreas Schwab
 */

/* bitmap.c contains the code that handles the inode and block bitmaps */

#include <linux/bitops.h>
#include <linux/buffer_head.h>
#include <linux/sched.h>

#include "bitmap.h"

static DEFINE_SPINLOCK(bitmap_lock);

/*
 * bitmap consists of blocks filled with 16bit words
 * bit set == busy, bit clear == free
 * endianness is a mess, but for counting zero bits it really doesn't matter...
 */
static __u32 count_free(struct buffer_head *map[], unsigned blocksize, __u32 numbits)
{
    __u32 sum = 0;
    unsigned blocks = DIV_ROUND_UP(numbits, blocksize * 8);

    while (blocks--)
    {
        unsigned words = blocksize / 2;
        __u16 *p = (__u16 *)(*map++)->b_data;
        while (words--)
            sum += 16 - hweight16(*p++);
    }

    return sum;
}

void gas_free_block(struct inode *inode, unsigned long block)
{
    struct super_block *sb = inode->i_sb;
    struct gas_sb_info *sbi = gas_SB(sb);
    struct buffer_head *bh;
    int k = sb->s_blocksize_bits + 3;
    unsigned long bit, idx;

    if (block < sbi->s_data_block_start || block >= sbi->s_nblocks)
    {
        pr_debug("Trying to free block not in datazone\n");
        return;
    }
    idx = block >> k;
    bit = block & ((1 << k) - 1);
    if (idx >= sbi->s_bam_blocks)
    {
        pr_debug("gas_free_block: nonexistent bitmap buffer\n");
        return;
    }
    bh = sbi->s_bam_bh[idx];
    spin_lock(&bitmap_lock);
    if (!test_and_clear_bit(bit, (unsigned long *)bh->b_data))
        pr_debug("gas_free_block (%s:%lu): bit already cleared\n", sb->s_id, block);
    spin_unlock(&bitmap_lock);
    mark_buffer_dirty(bh);
    return;
}

unsigned long gas_new_block(struct inode *inode, int *err)
{
    struct super_block *sb = inode->i_sb;
    struct gas_sb_info *sbi = gas_SB(sb);
    unsigned long block;
    int i;

    i = sbi->s_bam_last;
    do
    {
        spin_lock(&bitmap_lock);
        block = find_first_zero_bit((unsigned long *)sbi->s_bam_bh[i]->b_data, sbi->s_bits_per_block);
        if (block < sbi->s_bits_per_block)
        {
            set_bit(block, (unsigned long *)sbi->s_bam_bh[i]->b_data);
            spin_unlock(&bitmap_lock);
            block += i * sbi->s_bits_per_block;
            sbi->s_bam_last = i;
            mark_buffer_dirty(sbi->s_bam_bh[i]);
            *err = 0;
            return block;
        }
        spin_unlock(&bitmap_lock);
        i = (i + 1) % sbi->s_bam_blocks;
    } while (i != sbi->s_bam_last);

    *err = -ENOSPC;
    return 0;
}

unsigned long gas_count_free_blocks(struct super_block *sb)
{
    struct gas_sb_info *sbi = gas_SB(sb);
    u32 bits = sbi->s_nblocks - sbi->s_data_block_start + 1;

    return count_free(sbi->s_bam_bh, sb->s_blocksize, bits);
}

/* Clear the link count and mode of a deleted inode on disk. */

static void gas_clear_inode(struct inode *inode)
{
    struct buffer_head *bh = NULL;
    struct gas_inode *di;

    di = gas_get_inode(inode->i_sb, inode->i_ino, &bh);

    if (di)
    {
        di->i_nlink = cpu_to_le32(0);
        di->i_mode = cpu_to_le32(0);
    }
    if (bh)
    {
        mark_buffer_dirty(bh);
        brelse(bh);
    }
}

void gas_free_inode(struct inode *inode)
{
    struct super_block *sb = inode->i_sb;
    struct gas_sb_info *sbi = gas_SB(inode->i_sb);
    struct buffer_head *bh;
    int k = sb->s_blocksize_bits + 3;
    unsigned long ino, bit;

    ino = inode->i_ino;
    if (ino < 1 || ino > sbi->s_ninodes)
    {
        pr_debug("gas_free_inode: inode 0 or nonexistent inode\n");
        return;
    }
    bit = ino & ((1 << k) - 1);
    ino >>= k;
    if (ino >= sbi->s_iam_blocks)
    {
        pr_debug("gas_free_inode: nonexistent imap in superblock\n");
        return;
    }

    gas_clear_inode(inode); /* clear on-disk copy */

    bh = sbi->s_iam_bh[ino];
    spin_lock(&bitmap_lock);
    if (!test_and_clear_bit(bit, (unsigned long *)bh->b_data))
        pr_debug("gas_free_inode: bit %lu already cleared\n", bit);
    spin_unlock(&bitmap_lock);
    mark_buffer_dirty(bh);
}

struct inode *gas_new_inode(struct inode *dir, umode_t mode, int *err)
{
    struct super_block *sb = dir->i_sb;
    struct gas_sb_info *sbi = gas_SB(sb);
    struct inode *inode;
    unsigned long ino;
    struct gas_inode_info *si;
    int i;

    inode = new_inode(sb);
    if (!inode)
    {
        *err = -ENOMEM;
        return NULL;
    }

    i = sbi->s_iam_last;
    do
    {
        spin_lock(&bitmap_lock);
        ino = find_first_zero_bit((unsigned long *)sbi->s_iam_bh[i]->b_data, sbi->s_bits_per_block);
        if (ino < sbi->s_bits_per_block)
        {
            set_bit(ino, (unsigned long *)sbi->s_iam_bh[i]->b_data);
            spin_unlock(&bitmap_lock);
            ino += i * sbi->s_bits_per_block;
            sbi->s_iam_last = i;
            mark_buffer_dirty(sbi->s_iam_bh[i]);
            goto got_it;
        }
        spin_unlock(&bitmap_lock);
        i = (i + 1) % sbi->s_iam_blocks;
    } while (i != sbi->s_iam_last);

    *err = -ENOSPC;
    pr_debug("There is no free inode\n");
    iput(inode);
    return NULL;

got_it:
    si = gas_INODE(inode);
    memset((char *)&si->blkaddr, 0, 9 * sizeof(__le32));

    inode_init_owner(inode, dir, mode);
    inode->i_ino = ino;
    inode->i_atime = inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
    inode->i_size = 0;

    insert_inode_hash(inode);
    mark_inode_dirty(inode);
    *err = 0;
    return inode;
}

unsigned long gas_count_free_inodes(struct super_block *sb)
{
    struct gas_sb_info *sbi = gas_SB(sb);
    u32 bits = sbi->s_ninodes + 1;

    return count_free(sbi->s_iam_bh, sb->s_blocksize, bits);
}
