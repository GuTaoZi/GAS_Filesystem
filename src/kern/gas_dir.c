#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/string.h>

#include "gas.h"

// Some useful functions
static inline size_t gas_dir_pages(struct inode *inode)
{
    return (inode->i_size + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
}

static inline size_t gas_dir_entry_page(size_t pos)
{
    return pos >> PAGE_CACHE_SHIFT;
}

static inline size_t gas_dir_entry_offset(size_t pos)
{
    return pos & (PAGE_CACHE_SIZE - 1);
}

static unsigned gas_last_byte(struct inode *inode, unsigned long page_nr)
{
    unsigned last_byte = PAGE_CACHE_SIZE;

    if (page_nr == (inode->i_size >> PAGE_CACHE_SHIFT))
        last_byte = inode->i_size & (PAGE_CACHE_SIZE - 1);
    return last_byte;
}

static int gas_dir_prepare_chunk(struct page *page, loff_t pos, unsigned len)
{
    return __block_write_begin(page, pos, len, gas_get_block);
}

static int gas_dir_commit_chunk(struct page *page, loff_t pos, unsigned len)
{
    struct address_space *mapping = page->mapping;
    struct inode *dir = mapping->host;
    int err = 0;
    block_write_end(NULL, mapping, pos, len, len, page, NULL);

    if (pos + len > dir->i_size)
    {
        i_size_write(dir, pos + len);
        mark_inode_dirty(dir);
    }
    if (IS_DIRSYNC(dir))
        err = write_one_page(page, 1);
    else
        unlock_page(page);
    return err;
}

static struct page *gas_dir_get_page(struct inode *inode, size_t n)
{
    struct address_space *mapping = inode->i_mapping;
    struct page *page = read_mapping_page(mapping, n, NULL);

    if (!IS_ERR(page))
        kmap(page);
    return page;
}

static void gas_dir_put_page(struct page *page)
{
    kunmap(page);
    put_page(page); // same as calling page_cache_release(page);
}

static int gas_dir_emit(struct dir_context *ctx, struct gas_dir_entry *de)
{
    unsigned type = DT_UNKNOWN;
    unsigned len = strlen(de->de_name);
    size_t ino = le32_to_cpu(de->de_inode);

    if (!ino)
        return 1; // skip
    else
        return dir_emit(ctx, de->de_name, len, ino, type);
}

static int gas_iterate(struct inode *inode, struct dir_context *ctx)
{
    size_t pages = gas_dir_pages(inode);
    size_t pidx = gas_dir_entry_page(ctx->pos);
    size_t off = gas_dir_entry_offset(ctx->pos);

    for (; pidx < pages; ++pidx, off = 0)
    {
        struct page *page = gas_dir_get_page(inode, pidx);
        struct gas_dir_entry *de;
        char *kaddr;

        if (IS_ERR(page))
        {
            pr_err("cannot access page %lu in %lu", (unsigned long)pidx, (unsigned long)inode->i_ino);
            return PTR_ERR(page);
        }

        kaddr = page_address(page);
        de = (struct gas_dir_entry *)(kaddr + off);
        while (off < PAGE_CACHE_SIZE && ctx->pos < inode->i_size)
        {
            if (!gas_dir_emit(ctx, de))
            {
                gas_dir_put_page(page);
                return 0;
            }
            ctx->pos += sizeof(struct gas_dir_entry);
            off += sizeof(struct gas_dir_entry);
            ++de;
        }
        gas_dir_put_page(page);
    }
    return 0;
}

static int gas_readdir(struct file *file, struct dir_context *ctx)
{
    return gas_iterate(file_inode(file), ctx);
}

const struct file_operations gas_dir_ops = {
    .llseek = generic_file_llseek, .read = generic_read_dir, .iterate = gas_readdir,
    //	.fsync = generic_file_fsync,
};

struct gas_filename_match
{
    struct dir_context ctx;
    ino_t ino;
    const char *name;
    int len;
};

static int gas_match(void *ctx, const char *name, int len, loff_t off, u64 ino, unsigned type)
{
    struct gas_filename_match *match = (struct gas_filename_match *)ctx;

    if (len != match->len)
        return 0;

    if (memcmp(match->name, name, len) == 0)
    {
        match->ino = ino;
        return 1;
    }
    return 0;
}

int gas_add_link(struct dentry *dentry, struct inode *inode)
{
    struct inode *dir = dentry->d_parent->d_inode;
    const char *name = dentry->d_name.name;
    struct page *page = NULL;
    unsigned long npages = gas_dir_pages(dir);
    unsigned long n;
    char *kaddr, *p;
    struct gas_dir_entry *de;
    loff_t pos;
    int err = 0;

    for (n = 0; n <= npages; n++)
    {
        char *limit, *dir_end;
        page = gas_dir_get_page(dir, n);
        err = PTR_ERR(page);
        if (IS_ERR(page))
            goto out;
        lock_page(page);
        kaddr = (char *)page_address(page);
        dir_end = kaddr + gas_last_byte(dir, n);
        limit = kaddr + PAGE_CACHE_SIZE - sizeof(struct gas_dir_entry);
        for (p = kaddr; p <= limit; p += sizeof(struct gas_dir_entry))
        {
            de = (struct gas_dir_entry *)p;
            if ((char *)de == dir_end)
            {
                /* We hit i_size */
                de->de_inode = cpu_to_le32(0);
                goto got_it;
            }
            if (!le32_to_cpu(de->de_inode))
                goto got_it;
            err = -EEXIST;
            if (strncmp(de->de_name, name, GAS_MAX_NAME_LEN - 1) == 0)
                goto out_unlock;
        }
        unlock_page(page);
        gas_dir_put_page(page);
    }
    BUG();
    return -EINVAL;

got_it:
    pos = page_offset(page) + (char *)de - (char *)page_address(page);
    err = gas_dir_prepare_chunk(page, pos, sizeof(struct gas_dir_entry));
    if (err)
        goto out_unlock;
    strncpy(de->de_name, name, GAS_MAX_NAME_LEN - 1);
    de->de_name[GAS_MAX_NAME_LEN - 1] = '\0';
    de->de_inode = cpu_to_le32(inode->i_ino);
    err = gas_dir_commit_chunk(page, pos, sizeof(struct gas_dir_entry));
    dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
    mark_inode_dirty(dir);
out_put:
    gas_dir_put_page(page);
out:
    return err;
out_unlock:
    unlock_page(page);
    goto out_put;
}

int gas_make_empty(struct inode *inode, struct inode *dir)
{
    struct page *page = grab_cache_page(inode->i_mapping, 0);
    char *kaddr;
    struct gas_dir_entry *de;
    int err;

    if (!page)
        return -ENOMEM;

    err = gas_dir_prepare_chunk(page, 0, 2 * sizeof(struct gas_dir_entry));
    if (err)
    {
        unlock_page(page);
        goto fail;
    }

    kaddr = kmap_atomic(page);
    memset(kaddr, 0, PAGE_CACHE_SIZE);

    de = (struct gas_dir_entry *)kaddr;
    de->de_inode = cpu_to_le32(inode->i_ino);
    strcpy(de->de_name, ".");
    de++;
    de->de_inode = cpu_to_le32(dir->i_ino);
    strcpy(de->de_name, "..");
    kunmap_atomic(kaddr);

    err = gas_dir_commit_chunk(page, 0, 2 * sizeof(struct gas_dir_entry));
fail:
    page_cache_release(page);
    return err;
}

/*
 *	gas_find_entry() modified from minix_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 */
struct gas_dir_entry *gas_find_entry(struct dentry *dentry, struct page **res_page)
{
    const char *name = dentry->d_name.name;
    // int namelen = dentry->d_name.len;
    struct inode *dir = dentry->d_parent->d_inode;
    unsigned long n;
    unsigned long npages = gas_dir_pages(dir);
    struct page *page = NULL;
    char *p;

    *res_page = NULL;

    for (n = 0; n < npages; n++)
    {
        char *kaddr, *limit;

        page = gas_dir_get_page(dir, n);
        if (IS_ERR(page))
            continue;

        kaddr = (char *)page_address(page);
        limit = kaddr + gas_last_byte(dir, n) - sizeof(struct gas_dir_entry);
        for (p = kaddr; p <= limit; p += sizeof(struct gas_dir_entry))
        {
            struct gas_dir_entry *de = (struct gas_dir_entry *)p;
            if (!le32_to_cpu(de->de_inode))
                continue;
            if (!strncmp(de->de_name, name, GAS_MAX_NAME_LEN))
                goto found;
        }
        gas_dir_put_page(page);
    }
    return NULL;

found:
    *res_page = page;
    return (struct gas_dir_entry *)p;
}

int gas_delete_entry(struct gas_dir_entry *de, struct page *page)
{
    struct inode *inode = page->mapping->host;
    char *kaddr = page_address(page);
    loff_t pos = page_offset(page) + (char *)de - kaddr;
    unsigned len = sizeof(struct gas_dir_entry);
    int err;

    lock_page(page);
    err = gas_dir_prepare_chunk(page, pos, len);
    if (err == 0)
    {
        de->de_inode = cpu_to_le32(0);
        err = gas_dir_commit_chunk(page, pos, len);
    }
    else
    {
        unlock_page(page);
    }
    gas_dir_put_page(page);
    inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
    mark_inode_dirty(inode);
    return err;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
int gas_empty_dir(struct inode *inode)
{
    struct page *page = NULL;
    unsigned long i, npages = gas_dir_pages(inode);

    for (i = 0; i < npages; i++)
    {
        char *p, *kaddr, *limit;

        page = gas_dir_get_page(inode, i);
        if (IS_ERR(page))
            continue;

        kaddr = (char *)page_address(page);
        limit = kaddr + gas_last_byte(inode, i) - sizeof(struct gas_dir_entry);
        for (p = kaddr; p <= limit; p += sizeof(struct gas_dir_entry))
        {
            struct gas_dir_entry *de = (struct gas_dir_entry *)p;

            if (le32_to_cpu(de->de_inode))
            {
                /* check for . and .. */
                if (de->de_name[0] != '.')
                    goto not_empty;
                if (!de->de_name[1])
                {
                    if (le32_to_cpu(de->de_inode) != inode->i_ino)
                        goto not_empty;
                }
                else if (de->de_name[1] != '.')
                    goto not_empty;
                else if (de->de_name[2])
                    goto not_empty;
            }
        }
        gas_dir_put_page(page);
    }
    return 1;

not_empty:
    gas_dir_put_page(page);
    return 0;
}

/* Releases the page */
void gas_set_link(struct gas_dir_entry *de, struct page *page, struct inode *inode)
{
    struct inode *dir = page->mapping->host;
    loff_t pos = page_offset(page) + (char *)de - (char *)page_address(page);
    int err;

    lock_page(page);

    err = gas_dir_prepare_chunk(page, pos, sizeof(struct gas_dir_entry));
    if (err == 0)
    {
        err = gas_dir_commit_chunk(page, pos, sizeof(struct gas_dir_entry));
    }
    else
    {
        unlock_page(page);
    }
    gas_dir_put_page(page);
    dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
    mark_inode_dirty(dir);
}

struct gas_dir_entry *gas_dotdot(struct inode *dir, struct page **p)
{
    struct page *page = gas_dir_get_page(dir, 0);
    struct gas_dir_entry *de = NULL;

    if (!IS_ERR(page))
    {
        de = (struct gas_dir_entry *)((char *)page_address(page) + sizeof(struct gas_dir_entry));
        *p = page;
    }
    return de;
}

ino_t gas_inode_by_name(struct inode *dir, struct qstr *child)
{
    struct gas_filename_match match = {.ctx = {&gas_match, 0}, .ino = 0, .name = child->name, .len = child->len};

    int err = gas_iterate(dir, &match.ctx);

    if (err)
        pr_err("Cannot find dir entry, error = %d", err);
    return match.ino;
}

/*
ino_t gas_inode_by_name(struct dentry *dentry)
{
    struct page *page;
    struct gas_dir_entry *de = gas_find_entry(dentry, &page);
    ino_t res = 0;

    if (de) {
        struct address_space *mapping = page->mapping;
        struct inode *inode = mapping->host;
        struct gas_sb_info *sbi = gas_SB(inode->i_sb);

        res = le32_to_cpu(de->de_inode);
        gas_dir_put_page(page);
    }
    return res;
}
*/
