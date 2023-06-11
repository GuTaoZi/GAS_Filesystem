#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "bitmap.h"
#include "sfs.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef long long ll;

struct fs_config
{
    int fs_fd;
    u64 iam_blk;
    u64 ino_blk;
    u64 bam_blk;
    u64 blk_cnt;
    u64 ino_cnt;
    u64 data_st;
} conf;

#define BAM_BLOCK_START 1
#define IAM_BLOCK_START (BAM_BLOCK_START + conf.bam_blk)
#define INODE_LIST_START (IAM_BLOCK_START + conf.iam_blk)
#define DATA_BLOCK_START (INODE_LIST_START + conf.ino_blk)
#define INODES_PER_BLOCK (SFS_BLOCK_SIZE / sizeof(struct sfs_inode))

static void init_super_block()
{
    // char buffer[SFS_BLOCK_SIZE];
    // memset(buffer, 0, SFS_BLOCK_SIZE);
    printf("GAS: Initializing super block ...\n");
    char *buffer = (char *)malloc(SFS_BLOCK_SIZE);
    memset(buffer, 0, SFS_BLOCK_SIZE);
    struct sfs_super_block *sb = (struct sfs_super_block *)buffer;
    sb->s_magic = SFS_MAGIC;
    sb->s_blocksize = SFS_BLOCK_SIZE;
    sb->s_bam_blocks = conf.bam_blk;
    sb->s_iam_blocks = conf.iam_blk;
    sb->s_inode_blocks = conf.ino_blk;
    sb->s_nblocks = conf.blk_cnt;
    sb->s_ninodes = conf.ino_cnt;
    write_block(SUPER_BLOCK_NO, buffer);
}

static void init_block_alloc_map()
{
    printf("GAS: Initializing block alloc map ...\n");
    char *buffer = (char *)malloc(SFS_BLOCK_SIZE);
    u64 *map = (u64 *)buffer;
    int block = BAM_BLOCK_START;
    int preallocated = conf.data_st;
    int i;
    for (i = 1; i <= conf.bam_blk; i++)
    {
        if (preallocated > BITS_PER_BLOCK)
        {
            memset(buffer, 0xff, SFS_BLOCK_SIZE);
            preallocated -= BITS_PER_BLOCK;
        }
        else
        {
            memset(buffer, 0, SFS_BLOCK_SIZE);
            if (preallocated)
            {
                bitmap_set(map, 0, preallocated);
                preallocated = 0;
            }
        }
        if (i == conf.bam_blk)
        { // last BAM
            if (conf.blk_cnt != conf.bam_blk * BITS_PER_BLOCK)
            {
                int bits = (int)(conf.bam_blk * BITS_PER_BLOCK - conf.blk_cnt);

                bitmap_set(map, BITS_PER_BLOCK - bits, bits);
            }
        }
        write_block(block, buffer);
        block++;
    }
}

static void init_inode_alloc_map()
{
    printf("GAS: Initializing iNode alloc map ...\n");
    char *buffer = (char *)malloc(SFS_BLOCK_SIZE);
    u64 *map = (u64 *)buffer;
    int preallocated = 1;
    int block = IAM_BLOCK_START;
    int i;
    for (i = 1; i <= conf.iam_blk; i++)
    {
        if (preallocated > BITS_PER_BLOCK)
        {
            memset(buffer, 0xff, SFS_BLOCK_SIZE);
            preallocated -= BITS_PER_BLOCK;
        }
        else
        {
            memset(buffer, 0, SFS_BLOCK_SIZE);
            if (preallocated)
            {
                bitmap_set(map, 0, preallocated);
                preallocated = 0;
            }
        }
        if (i == conf.iam_blk)
        { // last IAM
            if (conf.ino_cnt != conf.iam_blk * BITS_PER_BLOCK)
            {
                int bits = (int)(conf.iam_blk * BITS_PER_BLOCK - conf.ino_cnt);

                bitmap_set(map, BITS_PER_BLOCK - bits, bits);
            }
        }
        write_block(block, buffer);
        block++;
    }
}

static void init_inode_list()
{
    printf("GAS: Initializing iNode list ...\n");
    char *buffer = (char *)malloc(SFS_BLOCK_SIZE);
    memset(buffer, 0, SFS_BLOCK_SIZE);
    int block = INODE_LIST_START;
    int i;
    for (i = 1; i <= conf.ino_blk; i++)
    {
        write_block(block, buffer);
        block++;
    }
}

int read_block(int blk_no, void *block)
{
    lseek(conf.fs_fd, blk_no * SFS_BLOCK_SIZE, SEEK_SET);
    return read(conf.fs_fd, block, SFS_BLOCK_SIZE);
}

int write_block(int blk_no, void *block)
{
    lseek(conf.fs_fd, blk_no * SFS_BLOCK_SIZE, SEEK_SET);
    return write(conf.fs_fd, block, SFS_BLOCK_SIZE);
}

struct blk_cache
{
    int dirty;
    int blk_no;
    char block[SFS_BLOCK_SIZE];
    struct blk_cache *next;
};

struct blk_cache *bc_head = NULL;

void bc_insert(struct blk_cache *new)
{
    new->next = bc_head;
    bc_head = new;
}

void bc_sync()
{
    struct blk_cache *p = bc_head;
    struct blk_cache *next;

    while (p)
    {
        next = p->next;
        if (p->dirty)
        {
            write_block(p->blk_no, p->block);
        }
        free(p);
        p = next;
    }
    bc_head = NULL;
}

struct blk_cache *bc_find(int blk_no)
{
    struct blk_cache *p = bc_head;

    while (p)
    {
        if (p->blk_no == blk_no)
            break;
        p = p->next;
    }
    return p;
}

void *bc_read(int blk_no)
{
    struct blk_cache *p;

    p = bc_find(blk_no);
    if (p)
    {
        return p->block;
    }

    p = malloc(sizeof(struct blk_cache));
    p->dirty = 0;
    p->blk_no = blk_no;
    p->next = NULL;
    read_block(blk_no, p->block);
    bc_insert(p);

    return p->block;
}

void bc_write(int blk_no, int sync)
{
    struct blk_cache *p;

    p = bc_find(blk_no);
    if (p)
    {
        if (sync)
        {
            write_block(p->blk_no, p->block);
            p->dirty = 0;
        }
        else
        {
            p->dirty = 1;
        }
    }
}

u32 allocate_blk(int blocks)
{
    u64 *map;
    u64 n;
    int i, block = BAM_BLOCK_START;

    for (i = 0; i < conf.bam_blk; i++)
    {
        map = (u64 *)bc_read(block);
        n = bitmap_alloc_region(map, BITS_PER_BLOCK, 0, blocks);
        if (n == INVALID_NO)
        {
            block++;
        }
        else
        {
            n += i * BITS_PER_BLOCK;
            bc_write(block, 0);
            break;
        }
    }

    return n;
}

u32 allocate_inode()
{
    u64 *map = (u64 *)bc_read(IAM_BLOCK_START);
    u64 n;

    n = bitmap_alloc_region(map, BITS_PER_BLOCK, SFS_ROOT_INO, 1);
    if (n == INVALID_NO)
    {
        printf("Error: cannot allocate inode\n");
    }
    else
    {
        bc_write(IAM_BLOCK_START, 0);
    }

    return n;
}

void free_inode(u32 ino)
{
    u64 *map = (u64 *)bc_read(IAM_BLOCK_START);

    if (ino >= INODES_PER_BLOCK)
        return;
    bitmap_free_region(map, ino, 1);
    bc_write(IAM_BLOCK_START, 0);
}

struct sfs_inode *get_inode(u32 ino)
{
    struct sfs_inode *ino_list = (struct sfs_inode *)bc_read(INODE_LIST_START);
    if (ino >= INODES_PER_BLOCK)
        return NULL;
    return &ino_list[ino];
}

u32 new_inode(mode_t mode, int byte_size)
{
    u32 ino;
    struct sfs_inode *ip;
    int nblocks;

    nblocks = (byte_size + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;

    ino = allocate_inode();
    if (ino == INVALID_NO)
    {
        return ino;
    }
    ip = get_inode(ino);
    if (ip == NULL)
    {
        printf("Cannot read inode\n");
        exit(1);
    }

    ip->i_blkaddr[0] = allocate_blk(nblocks);
    if (ip->i_blkaddr[0] == INVALID_NO)
    {
        free_inode(ino);
        return INVALID_NO;
    }

    ip->i_size = 0;
    if (S_ISDIR(mode))
        ip->i_nlink = 2;
    else
        ip->i_nlink = 1;
    ip->i_uid = getuid();
    ip->i_gid = getgid();
    ip->i_mode = mode;
    ip->i_ctime = time(NULL);
    ip->i_atime = ip->i_ctime;
    ip->i_mtime = ip->i_ctime;
    bc_write(INODE_LIST_START, 0);
    return ino;
}

u32 ll_mkdir(int entries)
{
    if (!entries)
        entries = 64;
    return new_inode(S_IFDIR | 0755, entries * sizeof(struct sfs_dir_entry));
}

int ll_write(u32 ino, char *data, int size);
int ll_read(u32 ino, char *data, int size);

void dump_inode(struct sfs_inode *ip);

void sfs_add_dir_entry(struct sfs_inode *ip, char *name, u32 new_ino)
{
    u32 left = SFS_BLOCK_SIZE - ip->i_size;
    u32 blk_no;
    u32 offset;
    struct sfs_dir_entry *dp;

    if (!left)
    {
        printf("Error: no enough space for creating a directory entry\n");
        printf("name = %s, new_ino = %d\n", name, new_ino);
        dump_inode(ip);
        bc_sync();
        exit(1);
    }

    blk_no = ip->i_blkaddr[0] + (ip->i_size / SFS_BLOCK_SIZE);
    offset = ip->i_size % SFS_BLOCK_SIZE;

    dp = (struct sfs_dir_entry *)((char *)bc_read(blk_no) + offset);
    strncpy(dp->de_name, name, SFS_MAX_NAME_LEN - 1);
    dp->de_name[SFS_MAX_NAME_LEN - 1] = '\0';
    dp->de_inode = new_ino;

    ip->i_size += sizeof(struct sfs_dir_entry);
    bc_write(blk_no, 0);
}

void dump_inode(struct sfs_inode *ip)
{
    printf("ip->i_blkaddr[0] = %d\n", ip->i_blkaddr[0]);
    printf("ip->i_size = %d\n", ip->i_size);
    printf("ip->i_mode = 0x%x\n", ip->i_mode);
}

void make_rootdir()
{
    u32 ino;
    struct sfs_inode *ip;

    ino = ll_mkdir(0);
    if (ino == INVALID_NO)
    {
        printf("Create root dir error\n");
        bc_sync();
        exit(1);
    }
    ip = get_inode(ino);
    sfs_add_dir_entry(ip, ".", SFS_ROOT_INO);
    sfs_add_dir_entry(ip, "..", SFS_ROOT_INO);
}

int main(int argc, char *argv[])
{
    char *block;
    off_t size;

    if (argc < 2)
    {
        printf("error\n");
        exit(1);
    }
    conf.fs_fd = open(argv[1], O_RDWR);
    if (conf.fs_fd < 0)
    {
        printf("file open error\n");
        exit(2);
    }

    // printf("                                                                                           \n"
    //        "                                                                                           \n"
    //        "                                   ...........                                             \n"
    //        "                                   -.:::::::::=                                            \n"
    //        "                                     ::::    :--                                           \n"
    //        "                                   . ....    ::.-                                          \n"
    //        "                                   ..--.     ::..-                                         \n"
    //        "                                   :         .-::-=                                        \n"
    //        "                                   :          ....:.                                       \n"
    //        "                                   :.-=--------=-::.                                       \n"
    //        "                                   :: ----------.-:.                                       \n"
    //        "                                   :-  ..........-:.                                       \n"
    //        "                                   :-    .::. ...-:.                                       \n"
    //        "                                   :-     ..    ..:..                                      \n"
    //        "                                   :-  .------  :-:.:-:                                    \n"
    //        "                                   ::..--:::::.-:     .-                                   \n"
    //        "                                   : :::::::::::       .:                                  \n"
    //        "                                   :   : .. : -       ..-                                  \n"
    //        "                                   :   - :..:.:      :- :.                                 \n"
    //        "                                   :   - :..:.:     -:  ::                                 \n"
    //        "                                   :.-.- :..:.- - .-.   :.                                 \n"
    //        "                                   ::..- .. : - .--     -                                  \n"
    //        "                                   :.--:      .: .     ::                                  \n"
    //        "                                   :           ::     :-                                   \n"
    //        "                                   -:::::::::::..--::-:                                    \n"
    //        "                                    ...........    .                                       \n"
    //        "                                                                                           \n"
    //        "                                                                                           \n"
    //        "                                .    ...                                                   \n"
    //        "     =**:   +-   .+**. .++++   :#:   &$$          .+**.                =                   \n"
    //        "    $&-==  -#$   @+--. :#===    -     :$          @+--.               .@                   \n"
    //        "   =$      &=#. :@     :@     =$$.    :$    +$$: :@     &:  -+  *@$- *$#$$=  +$$: -=$-&*   \n"
    //        "   $= ...  # &=  @&:   :@...   .@:    :$   +& :#  @&:   +&  $- **     :@..  +& :# +@:#:@   \n"
    //        "   $- *$$ -& -$   +@$. :#&&&    @:    :$   @=--@-  +@$. .# .@  =@-    .@    @=--@-++.$ @   \n"
    //        "   &=  -$ &$++#.    *$ :@       @:    :$   @*+++.    *$  &=++   -&#=  .@    @*+++.++.$ @   \n"
    //        "   =@  -$ #+++$=    -$ :@       @:    :$   &=        -$  -$@.     -$  .#    &=    ++.$ @   \n"
    //        "    *#&@&-$   =$-@&$@: :@     *$#$& .$$#$= :@&&$ -@&$@:   @$   *&&@=   $$&= :@&&$ ++.$ @   \n"
    //        "      :.       . .:.    .     .....  ....    ::   .:.     @-    ::      ..    ::     . .   \n"
    //        "                                                        =&&                                \n"
    //        "                                                        +=                                 \n"
    //        "                                                                                           \n");
    printf("=============================================================================\n"
           "                                                                             \n"
           " ██████╗  █████╗ ███████╗███████╗██╗██╗     ███████╗███████╗██╗   ██╗███████╗\n"
           "██╔════╝ ██╔══██╗██╔════╝██╔════╝██║██║     ██╔════╝██╔════╝╚██╗ ██╔╝██╔════╝\n"
           "██║  ███╗███████║███████╗█████╗  ██║██║     █████╗  ███████╗ ╚████╔╝ ███████╗\n"
           "██║   ██║██╔══██║╚════██║██╔══╝  ██║██║     ██╔══╝  ╚════██║  ╚██╔╝  ╚════██║\n"
           "╚██████╔╝██║  ██║███████║██║     ██║███████╗███████╗███████║   ██║   ███████║\n"
           " ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚══════╝╚══════╝╚══════╝   ╚═╝   ╚══════╝\n"
           "                                                                             \n");

    size = lseek(conf.fs_fd, 0, SEEK_END);

    // Initialize file system configuration
    conf.blk_cnt = size / SFS_BLOCK_SIZE;
    conf.bam_blk = (conf.blk_cnt + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    conf.ino_blk = (conf.blk_cnt / 4) / INODES_PER_BLOCK;
    conf.ino_cnt = conf.ino_blk * INODES_PER_BLOCK;
    conf.iam_blk = (conf.ino_cnt + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    conf.data_st = 1 + conf.bam_blk + conf.iam_blk + conf.ino_blk;

    printf("===============================GAS File Sys Info================================\n");
    printf("Disk size = %lld\n", (ll)size);
    printf("Number of Blocks = %lld\n", (ll)conf.blk_cnt);
    printf("BAM blocks = %lld\n", (ll)conf.bam_blk);
    printf("IAM blocks = %lld\n", (ll)conf.iam_blk);
    printf("inode blocks = %lld\n", (ll)conf.ino_blk);
    printf("Number of inodes = %lld\n", (ll)conf.ino_cnt);
    printf("Data block starts at %lld block\n", (ll)conf.data_st);
    printf("================================================================================\n");

    init_super_block();
    init_block_alloc_map();
    init_inode_alloc_map();
    init_inode_list();
    make_rootdir();

    bc_sync();
    printf("Device write complete\n");
    close(conf.fs_fd);
}
