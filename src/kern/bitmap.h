#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <linux/buffer_head.h>
#include <linux/bitops.h>
#include <linux/sched.h>

void gas_free_block(struct inode *inode, unsigned long block);
unsigned long gas_new_block(struct inode *inode, int *err);
unsigned long gas_count_free_blocks(struct super_block *sb);
static void gas_clear_inode(struct inode *inode);
void gas_free_inode(struct inode *inode);
struct inode *gas_new_inode(struct inode *dir, umode_t mode, int *err);
unsigned long gas_count_free_inodes(struct super_block *sb);

#endif