#ifndef __GAS_DIR_H__
#define __GAS_DIR_H__

#include <linux/fs.h>

int gas_add_link(struct dentry *dentry, struct inode *inode);
int gas_make_empty(struct inode *inode, struct inode *dir);
struct gas_dir_entry *
gas_find_entry(struct dentry *dentry, struct page **res_page);
int gas_delete_entry(struct gas_dir_entry *de, struct page *page);
int gas_empty_dir(struct inode *inode);
void gas_set_link(struct gas_dir_entry *de, struct page *page,
                  struct inode *inode);
struct gas_dir_entry *gas_dotdot(struct inode *dir, struct page **p);
ino_t gas_inode_by_name(struct inode *dir, struct qstr *child);