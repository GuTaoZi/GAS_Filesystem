#include <linux/fs.h>

#include "gas.h"

// add a file (non-dir)
static int add_nondir(struct dentry *dentry, struct inode *inode)
{
    int err = gas_add_link(dentry, inode);
    if (!err)
    {
        // link them together in vfs
        d_instantiate(dentry, inode);
        return 0;
    }
    else
    {
        iput(inode);
        return err;
    }
}

int gas_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t rdev)
{
    int err;
    struct inode *inode;

    // device is valid. Always returns 1 in linux3.13
    if (!new_valid_dev(rdev))
        return -EINVAL;

    // create a new inode under a directory
    inode = gas_new_inode(dir, mode, &err);
    if (!err && inode)
    {
        // set operations structure
        gas_set_inode(inode, rdev);
        mark_inode_dirty(inode);

        err = add_nondir(dentry, inode);
    }
    return err;
}

// mkdir
int gas_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    struct inode *inode;
    int err;

    // increate link count. Atomic
    inode_inc_link_count(dir);

    inode = gas_new_inode(dir, S_IFDIR | mode, &err);
    if (!inode)
        goto out_dir;

    inode_inc_link_count(inode);

    gas_set_inode(inode, 0);

    // make it useable. (Set from random to 0)
    err = gas_make_empty(inode, dir);
    if (err)
        goto out_fail;

    err = gas_add_link(dentry, inode);
    if (err)
        goto out_fail;

    d_instantiate(dentry, inode);
out:
    return err;

out_fail:
    inode_dec_link_count(inode);
    inode_dec_link_count(inode);
    iput(inode);
out_dir:
    inode_dec_link_count(dir);
    goto out;
}

// read out the inode by dentry
struct dentry *gas_lookup(struct inode *dir, struct dentry *dentry, unsigned flags)
{
    struct inode *inode = NULL;
    ino_t ino;

    if (dentry->d_name.len >= GAS_MAX_NAME_LEN)
        return ERR_PTR(-ENAMETOOLONG);

    // Find the inode
    ino = gas_inode_by_name(dir, &dentry->d_name);
    if (ino)
    {
        // read out inode
        inode = gas_iget(dir->i_sb, ino);
        if (IS_ERR(inode))
        {
            pr_err("Cannot read inode %lu", (unsigned long)ino);
            return ERR_PTR(PTR_ERR(inode));
        }
        // Add it into dentry's hash_list
        d_add(dentry, inode);
    }
    return NULL;
}

// create new inode (file)
int gas_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
    int err;
    struct inode *inode;

    inode = gas_new_inode(dir, mode, &err);
    if (!err && inode)
    {
        gas_set_inode(inode, 0);
        mark_inode_dirty(inode);
        err = add_nondir(dentry, inode);
    }
    return err;
}

// Soft link
int gas_symlink(struct inode *dir, struct dentry *dentry, const char *symname)
{
    int err = -ENAMETOOLONG;
    int i = strlen(symname) + 1;
    struct inode *inode;

    if (i > dir->i_sb->s_blocksize)
        goto out;

    inode = gas_new_inode(dir, S_IFLNK | 0777, &err);
    if (!inode)
        goto out;

    gas_set_inode(inode, 0);
    err = page_symlink(inode, symname, i);
    if (err)
        goto out_fail;

    err = add_nondir(dentry, inode);
out:
    return err;

out_fail:
    inode_dec_link_count(inode);
    iput(inode);
    goto out;
}

// hard link
int gas_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = old_dentry->d_inode;

    inode->i_ctime = CURRENT_TIME_SEC;
    inode_inc_link_count(inode);
    ihold(inode);
    return add_nondir(dentry, inode);
}

// unlink
int gas_unlink(struct inode *dir, struct dentry *dentry)
{
    int err = -ENOENT;
    struct inode *inode = dentry->d_inode;
    struct page *page;
    struct gas_dir_entry *de;

    de = gas_find_entry(dentry, &page);
    if (!de)
        goto end_unlink;

    err = gas_delete_entry(de, page);
    if (err)
        goto end_unlink;

    inode->i_ctime = dir->i_ctime;

    // decrease link count
    inode_dec_link_count(inode);
end_unlink:
    return err;
}

int gas_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = dentry->d_inode;
    int err = -ENOTEMPTY;

    // Only remove empty dir
    if (gas_empty_dir(inode))
    {
        // free dentry
        err = gas_unlink(dir, dentry);
        if (!err)
        {
            inode_dec_link_count(dir);
            inode_dec_link_count(inode);
        }
    }
    return err;
}

// used for cp, mv and so on
int gas_rename(struct inode *old_dir, struct dentry *old_dentry, struct inode *new_dir, struct dentry *new_dentry)
{
    struct inode *old_inode = old_dentry->d_inode;
    struct inode *new_inode = new_dentry->d_inode;
    struct page *dir_page = NULL;
    struct gas_dir_entry *dir_de = NULL;
    struct page *old_page;
    struct gas_dir_entry *old_de;
    int err = -ENOENT;

    old_de = gas_find_entry(old_dentry, &old_page);
    if (!old_de)
        goto out;

    if (S_ISDIR(old_inode->i_mode))
    {
        err = -EIO;
        dir_de = gas_dotdot(old_inode, &dir_page);
        if (!dir_de)
            goto out_old;
    }

    if (new_inode) // move to a exist place
    {
        struct page *new_page;
        struct gas_dir_entry *new_de;

        err = -ENOTEMPTY;
        if (dir_de && !gas_empty_dir(new_inode))
            goto out_dir;

        err = -ENOENT;
        new_de = gas_find_entry(new_dentry, &new_page);
        if (!new_de)
            goto out_dir;
        gas_set_link(new_de, new_page, old_inode);
        new_inode->i_ctime = CURRENT_TIME_SEC;
        if (dir_de)
            drop_nlink(new_inode);
        inode_dec_link_count(new_inode);
    }
    else
    {
        err = gas_add_link(new_dentry, old_inode);
        if (err)
            goto out_dir;
        if (dir_de)
            inode_inc_link_count(new_dir);
    }

    gas_delete_entry(old_de, old_page);
    mark_inode_dirty(old_inode);

    if (dir_de)
    {
        gas_set_link(dir_de, dir_page, new_dir);
        inode_dec_link_count(old_dir);
    }
    return 0;

out_dir:
    if (dir_de)
    {
        kunmap(dir_page);
        page_cache_release(dir_page);
    }
out_old:
    kunmap(old_page);
    page_cache_release(old_page);
out:
    return err;
}

// get some attribute
int gas_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
    struct super_block *sb = dentry->d_sb;

    generic_fillattr(dentry->d_inode, stat);
    stat->blocks = (sb->s_blocksize / 512) * gas_blocks(stat->size, sb);
    stat->blksize = sb->s_blocksize;
    return 0;
}
