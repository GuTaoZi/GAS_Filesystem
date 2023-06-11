#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bitmap.h"
#include "sfs.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef long long ll;

struct filesys_param
{
    int fs_fd;
    u64 iam_blk;
    u64 ino_blk;
    u64 bam_blk;
    u64 blk_cnt;
    u64 ino_cnt;
    u64 data_st;
} gas;

#define BAM_BLOCK_START 1
#define IAM_BLOCK_START (BAM_BLOCK_START + gas.bam_blk)
#define INODE_LIST_START (IAM_BLOCK_START + gas.iam_blk)
#define DATA_BLOCK_START (INODE_LIST_START + gas.ino_blk)
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
    sb->s_bam_blocks = gas.bam_blk;
    sb->s_iam_blocks = gas.iam_blk;
    sb->s_inode_blocks = gas.ino_blk;
    sb->s_nblocks = gas.blk_cnt;
    sb->s_ninodes = gas.ino_cnt;
    write_block(SUPER_BLOCK_NO, buffer);
}

static void init_block_alloc_map()
{
    printf("GAS: Initializing block alloc map ...\n");
    char *buffer = (char *)malloc(SFS_BLOCK_SIZE);
    u64 *map = (u64 *)buffer;
    int block = BAM_BLOCK_START;
    int preallocated = gas.data_st;
    int i;
    for (i = 1; i <= gas.bam_blk; i++)
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
        if (i == gas.bam_blk)
        {
            if (gas.blk_cnt != gas.bam_blk * BITS_PER_BLOCK)
            {
                int bits = (int)(gas.bam_blk * BITS_PER_BLOCK - gas.blk_cnt);

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
    for (i = 1; i <= gas.iam_blk; i++)
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
        if (i == gas.iam_blk)
        { // last IAM
            if (gas.ino_cnt != gas.iam_blk * BITS_PER_BLOCK)
            {
                int bits = (int)(gas.iam_blk * BITS_PER_BLOCK - gas.ino_cnt);

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
    for (i = 1; i <= gas.ino_blk; i++)
    {
        write_block(block, buffer);
        block++;
    }
}

int read_block(int blk_id, void *block)
{
    lseek(gas.fs_fd, blk_id * SFS_BLOCK_SIZE, SEEK_SET);
    return read(gas.fs_fd, block, SFS_BLOCK_SIZE);
}

int write_block(int blk_id, void *block)
{
    lseek(gas.fs_fd, blk_id * SFS_BLOCK_SIZE, SEEK_SET);
    return write(gas.fs_fd, block, SFS_BLOCK_SIZE);
}

struct blk_cache
{
    int dirty;
    int blk_id;
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
    struct blk_cache *it = bc_head;
    struct blk_cache *next;
    while (it)
    {
        next = it->next;
        if (it->dirty)
        {
            write_block(it->blk_id, it->block);
        }
        free(it);
        it = next;
    }
    bc_head = NULL;
}

struct blk_cache *bc_find(int blk_id)
{
    struct blk_cache *it = bc_head;
    while (it)
    {
        if (it->blk_id == blk_id)
            break;
        it = it->next;
    }
    return it;
}

void *bc_read(int blk_id)
{
    struct blk_cache *it = bc_find(blk_id);
    if (it)
    {
        return it->block;
    }
    it = malloc(sizeof(struct blk_cache));
    it->dirty = 0;
    it->blk_id = blk_id;
    it->next = NULL;
    read_block(blk_id, it->block);
    bc_insert(it);
    return it->block;
}

void bc_write(int blk_id, int sync)
{
    struct blk_cache *p = bc_find(blk_id);
    if (p)
    {
        if (sync)
        {
            write_block(p->blk_id, p->block);
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
    u64 n;
    int block = BAM_BLOCK_START;
    int i;
    for (i = 0; i < gas.bam_blk; i++)
    {
        u64 *map = (u64 *)bc_read(block);
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
    u64 n = bitmap_alloc_region(map, BITS_PER_BLOCK, SFS_ROOT_INO, 1);
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

void free_inode(u32 ino_id)
{
    u64 *map = (u64 *)bc_read(IAM_BLOCK_START);
    if (ino_id >= INODES_PER_BLOCK)
    {
        return;
    }
    bitmap_free_region(map, ino_id, 1);
    bc_write(IAM_BLOCK_START, 0);
}

struct sfs_inode *get_inode(u32 ino_id)
{
    struct sfs_inode *ino_list = (struct sfs_inode *)bc_read(INODE_LIST_START);
    if (ino_id >= INODES_PER_BLOCK)
    {
        return NULL;
    }
    return &ino_list[ino_id];
}

u32 new_inode(mode_t mode, int byte_size)
{
    int nblocks = (byte_size + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;
    u32 ino_id = allocate_inode();
    if (ino_id == INVALID_NO)
    {
        return ino_id;
    }
    struct sfs_inode *ino = get_inode(ino_id);
    if (ino == NULL)
    {
        printf("Cannot read inode\n");
        exit(1);
    }
    ino->i_blkaddr[0] = allocate_blk(nblocks);
    if (ino->i_blkaddr[0] == INVALID_NO)
    {
        free_inode(ino_id);
        return INVALID_NO;
    }
    ino->i_size = 0;
    ino->i_nlink = S_ISDIR(mode) ? 2 : 1;
    ino->i_uid = getuid();
    ino->i_gid = getgid();
    ino->i_mode = mode;
    ino->i_ctime = time(NULL);
    ino->i_atime = ino->i_ctime;
    ino->i_mtime = ino->i_ctime;
    bc_write(INODE_LIST_START, 0);
    return ino_id;
}

void dump_inode(struct sfs_inode *ino)
{
    printf("ino->i_blkaddr[0] = %d\n", ino->i_blkaddr[0]);
    printf("ino->i_size = %d\n", ino->i_size);
    printf("ino->i_mode = 0x%x\n", ino->i_mode);
}

void sfs_add_dentry(struct sfs_inode *ino, char *name, u32 new_ino)
{
    u32 blk_id;
    u32 offset;
    struct sfs_dir_entry *dp;

    if (ino->i_size >= SFS_BLOCK_SIZE)
    {
        printf("Error: Failed to create a dentry due to limited space.\n");
        printf("name = %s, new_ino = %d\n", name, new_ino);
        dump_inode(ino);
        bc_sync();
        exit(1);
    }

    blk_id = ino->i_blkaddr[0] + (ino->i_size / SFS_BLOCK_SIZE);
    offset = ino->i_size % SFS_BLOCK_SIZE;

    dp = (struct sfs_dir_entry *)((char *)bc_read(blk_id) + offset);
    strncpy(dp->de_name, name, SFS_MAX_NAME_LEN - 1);
    dp->de_name[SFS_MAX_NAME_LEN - 1] = '\0';
    dp->de_inode = new_ino;

    ino->i_size += sizeof(struct sfs_dir_entry);
    bc_write(blk_id, 0);
}

void make_rootdir()
{
    u32 ino_id = new_inode(S_IFDIR | 0755, 64 * sizeof(struct sfs_dir_entry));
    if (ino_id == INVALID_NO)
    {
        printf("Create root dir error\n");
        bc_sync();
        exit(1);
    }
    struct sfs_inode *ino = get_inode(ino_id);
    sfs_add_dentry(ino, ".", SFS_ROOT_INO);
    sfs_add_dentry(ino, "..", SFS_ROOT_INO);
}

int main(int argc, char *argv[])
{
    off_t size;

    if (argc < 2)
    {
        printf("Error: Too few command line parameter.\n");
        exit(1);
    }
    gas.fs_fd = open(argv[1], O_RDWR);
    if (gas.fs_fd < 0)
    {
        printf("Error: Failed to open the file.\n");
        exit(2);
    }
    printf("=============================================================================\n"
           "                                                                             \n"
           " ██████╗  █████╗ ███████╗███████╗██╗██╗     ███████╗███████╗██╗   ██╗███████╗\n"
           "██╔════╝ ██╔══██╗██╔════╝██╔════╝██║██║     ██╔════╝██╔════╝╚██╗ ██╔╝██╔════╝\n"
           "██║  ███╗███████║███████╗█████╗  ██║██║     █████╗  ███████╗ ╚████╔╝ ███████╗\n"
           "██║   ██║██╔══██║╚════██║██╔══╝  ██║██║     ██╔══╝  ╚════██║  ╚██╔╝  ╚════██║\n"
           "╚██████╔╝██║  ██║███████║██║     ██║███████╗███████╗███████║   ██║   ███████║\n"
           " ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚══════╝╚══════╝╚══════╝   ╚═╝   ╚══════╝\n"
           "                                                                             \n");

    size = lseek(gas.fs_fd, 0, SEEK_END);

    // Initialize file system configuration
    gas.blk_cnt = size / SFS_BLOCK_SIZE;
    gas.bam_blk = (gas.blk_cnt + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    gas.ino_blk = (gas.blk_cnt / 4) / INODES_PER_BLOCK;
    gas.ino_cnt = gas.ino_blk * INODES_PER_BLOCK;
    gas.iam_blk = (gas.ino_cnt + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    gas.data_st = 1 + gas.bam_blk + gas.iam_blk + gas.ino_blk;

    printf("===============================GAS File Sys Info================================\n");
    printf("Disk size = %lld\n", (ll)size);
    printf("Number of Blocks = %lld\n", (ll)gas.blk_cnt);
    printf("BAM blocks = %lld\n", (ll)gas.bam_blk);
    printf("IAM blocks = %lld\n", (ll)gas.iam_blk);
    printf("inode blocks = %lld\n", (ll)gas.ino_blk);
    printf("Number of inodes = %lld\n", (ll)gas.ino_cnt);
    printf("Data block starts at %lld block\n", (ll)gas.data_st);
    printf("================================================================================\n");

    init_super_block();
    init_block_alloc_map();
    init_inode_alloc_map();
    init_inode_list();
    make_rootdir();

    bc_sync();
    printf("Device write complete\n");
    close(gas.fs_fd);
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