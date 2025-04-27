/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: a dummy file system illustrating the concept of "inode"
 */

#ifdef MKFS
#include <stdio.h>
#include <sys/types.h>
#else
#include "egos.h"
#endif
#include "inode.h"
#include <stdlib.h>
#include <string.h>

#define DUMMY_DISK_OFFSET(ino, offset) ino * 128 + offset

#define DISK_SIZE (16UL * 1024UL * 1024UL)
#define BLOCK_NUM (FILE_SYS_DISK_SIZE / BLOCK_SIZE)

#define INODE_NUM_IN_BLOCK (BLOCK_SIZE / sizeof(struct inode))
#define ENTRY_NUM_IN_BLOCK (BLOCK_SIZE / sizeof(struct entry))
// 记录磁盘是否空闲
#define DISK_END -1
#define DISK_EMPTY -2
#define DISK_USED 1

static int inode_block_start = 1;
static int data_block_start = -1;
static int entry_block_start = -1;
static int inode_num = -1;


struct inode{
    int size, head;
};

struct entry{
    int next;
};



typedef uint block_num;

struct superblock{
    block_num n_inodeblocks;
    block_num free_list;        // 记录entry入口
    block_num block_satrt;      // 记录data开始块
};

struct inodeblock{
    struct inode inodes[INODE_NUM_IN_BLOCK];
};

struct freelistblock{
    struct entry fat_entry[BLOCK_SIZE / sizeof(struct entry)];
};

struct indirblock{
    block_num refs[BLOCK_SIZE / sizeof(block_num)];
};

union disk_block{
    block_t datablock;
    struct superblock _superblock;
    struct inodeblock _inodeblock;
    struct freelistblock _freelistblock;
    struct indirblock _indirblock;
};

enum block_type{
    INODE_BLOCK,
    ENTRY_BLOCK,
    DATA_BLOCK
};

int read_index(inode_intf below, int index, enum block_type type, block_t *buf){
    int idx = 0;
    int block_id = 0;
    if(type == INODE_BLOCK){
        block_id = (index / INODE_NUM_IN_BLOCK) + 1;
        idx = index % INODE_NUM_IN_BLOCK;
    }else if(type == ENTRY_BLOCK){
        block_id = (index / ENTRY_NUM_IN_BLOCK) + entry_block_start;
        idx = index % ENTRY_NUM_IN_BLOCK;
    }
    below->read(below, 0, block_id, buf);
    return idx;
}

int write_inode(inode_intf below, int ino, struct inode ino_info){
    block_t buf;
    memset(&buf, 0, BLOCK_SIZE);
    int block_id = (ino / INODE_NUM_IN_BLOCK) + 1;
    below->read(below, 0, block_id, &buf);
    struct inode* p = (void*)&buf;
    int idx = ino % INODE_NUM_IN_BLOCK;
    p[idx].size = ino_info.size;
    p[idx].head = ino_info.head;
    below->write(below, 0, block_id, &buf);
    return 0;
}

int write_entry(inode_intf below, int entry, int next){
    block_t buf;
    memset(&buf, 0, BLOCK_SIZE);
    int block_id = (entry / ENTRY_NUM_IN_BLOCK) + entry_block_start;
    below->read(below, 0, block_id, &buf);
    struct entry* p = (void*)&buf;
    int idx = entry % ENTRY_NUM_IN_BLOCK;
    p[idx].next = next;
    below->write(below, 0, block_id, &buf);
    return 0;
}

int add_block(inode_intf below, int head, int next){
    block_t buf;
    memset(&buf, 0, BLOCK_SIZE);
    struct entry *p = NULL;
    int idx = 0;
    while(1){
        idx = read_index(below, head, ENTRY_BLOCK, &buf);
        p = (void*)&buf;
        // 到达末尾
        if(head == p[idx].next){
            break;
        }
        head = p[idx].next;
    }
    write_entry(below, head, next);
}

int alloc_entry(inode_intf below){
    block_t buf;
    // printf("alloc_entry entry_block_start = %d, data_block_start = %d\n", entry_block_start, data_block_start);
    for(int i = entry_block_start; i < data_block_start; i++){
        memset(&buf, 0, BLOCK_SIZE);
        below->read(below, 0, i, &buf);
        struct entry *p = (void *)&buf;
        for(int j = 0; j < ENTRY_NUM_IN_BLOCK; j++){
            // printf("alloc_entry p[j].next = %d\n", p[j].next);
            if(p[j].next == DISK_EMPTY){
                p[j].next = j + (i - entry_block_start) * ENTRY_NUM_IN_BLOCK;
                // 将齐写回
                below->write(below, 0, i, &buf);
                return j + (i - entry_block_start) * ENTRY_NUM_IN_BLOCK; 
            }
        }
    }
    return -1;
}


void read_super(inode_intf below, block_t *_block){
    // printf("read_super\n");
    if(inode_num == -1){        // 读取超级块，初始化数据
        // printf("read_super2\n");
        below->read(below, 0, 0, _block);
        struct superblock *super = (void *)_block;
        entry_block_start = super->free_list;
        data_block_start = super->block_satrt;
        inode_num = super->n_inodeblocks;
        if(inode_num == -1){
            // FATAL("read super block failed!");
        }
        memset(_block, 0, BLOCK_SIZE);
    }
}

int get_offset(inode_intf below, int head, int offset){
    int block_id = head;
    block_t *buf = malloc(BLOCK_SIZE);
    int idx;
    int start = -1, end = -1;
    while(offset){
        if(block_id >= end || end == -1){
            // 读取fat_entry
            memset(buf, 0, BLOCK_SIZE);
            idx = read_index(below, block_id, ENTRY_BLOCK, buf);
            start = block_id - (block_id % ENTRY_NUM_IN_BLOCK);
            end = start + ENTRY_NUM_IN_BLOCK;
        }else{
            idx = block_id % ENTRY_NUM_IN_BLOCK;
        }
        block_id = ((struct entry *)buf)[idx].next;
        offset--;
    }
    free(buf);
    return block_id;
}



int mydisk_read(inode_intf self, uint ino, uint offset, block_t* block) {
    /* Student's code goes here (File System). */

    /* Replace the code below with your own file system read logic. */

    inode_intf below = self->state;
    block_t _block;
    memset(&_block, 0, BLOCK_SIZE);
    read_super(below, &_block);
    if(ino > inode_num){
        return -1;
    }

    // 找到对应的inode节点， 并读取head
    int idx = read_index(below, ino, INODE_BLOCK, &_block);
    struct inode *inode_p = (void *)&_block;
    int head = inode_p[idx].head;
    int block_id = head;
    // while(offset){
    //     // 读取fat_entry
    //     memset(&_block, 0, BLOCK_SIZE);
    //     idx = read_index(below, block_id, ENTRY_BLOCK, &_block);
    //     block_id = ((struct entry *)&_block)[idx].next;
    //     offset--;
    // }
    block_id = get_offset(below, head, offset);
    // INFO("block_id = %d", block_id);

    return below->read(below, 0, block_id + data_block_start, block);
    /* Student's code ends here. */
}

int mydisk_write(inode_intf self, uint ino, uint offset, block_t* block) {
    /* Student's code goes here (File System). */
    // printf("mydisk_write, offset = %d\n", offset);
    /* Replace the code below with your own file system write logic. */
    inode_intf below = self->state;
    block_t _block;
    memset(&_block, 0, BLOCK_SIZE);
    read_super(below, &_block);
    if(ino > inode_num){
        return -1;
    }
    int idx = read_index(below, ino, INODE_BLOCK, &_block);
    struct inode *p = (void*)&_block;
    int entry = 0;
    // 需要申请磁盘
    if(p[idx].size <= offset){
        p[idx].size++;
        entry = alloc_entry(below);
        if(p[idx].head == -1){
            // printf("entry %d\n", entry);
            p[idx].head = entry;
        }
        write_inode(below, ino, p[idx]);
        add_block(below, p[idx].head, entry);
    }
    int head = p[idx].head;
    // printf("mydisk_write, head = %d\n", head + data_block_start);
    // while(offset){
    //     memset(&_block, 0, BLOCK_SIZE);
    //     idx = read_index(below, head, ENTRY_BLOCK, &_block);
    //     head = ((struct entry *)&_block)[idx].next;
    //     offset--;
    // }
    head = get_offset(below, head, offset);
    // printf("mydisk_write, block num = %d\n", head + data_block_start);
   
    return below->write(below, 0, head + data_block_start, block);
    /* Student's code ends here. */
}

int mydisk_getsize(inode_intf self, uint ino) {
    /* Student's code goes here (File System). */

    /* Get the size of inode #ino. */
#ifdef MKFS
    fprintf(stderr, "mydisk_getsize not implemented");
    while (1);
#else
    FATAL("mydisk_getsize not implemented");
#endif
    /* Student's code ends here. */
}

int mydisk_setsize(inode_intf self, uint ino, uint nblocks) {
    /* Student's code goes here (File System). */

    /* Set the size of inode #ino to nblocks. */
#ifdef MKFS
    fprintf(stderr, "mydisk_setsize not implemented");
    while (1);
#else
    FATAL("mydisk_setsize not implemented");
#endif
    /* Student's code ends here. */
}

inode_intf mydisk_init(inode_intf below, uint below_ino) {
    // INFO("mydisk_init!!!");
    inode_intf self = malloc(sizeof(struct inode_store));
    self->getsize   = mydisk_getsize;
    self->setsize   = mydisk_setsize;
    self->read      = mydisk_read;
    self->write     = mydisk_write;
    self->state     = below;
    return self;
}

int mydisk_create(inode_intf below, uint below_ino, uint ninodes) {

    // INFO("mydisk_create!!!");
    /* Student's code goes here (File System). */
    // inode_num = ninodes;
    block_num start = 1;
    /* Initialize the on-disk data structures for your file system. */
    union disk_block superblock;
    superblock._superblock.n_inodeblocks = ninodes;
    
    // 计算记录inode需要几个磁盘块
    int block_num = (ninodes + INODE_NUM_IN_BLOCK - 1) / INODE_NUM_IN_BLOCK;
    // printf("inode block num = %d\n", block_num);
    union disk_block *inode_blocks = malloc(block_num * sizeof(union disk_block));
    // 初始化inode
    for(int i = 0; i < block_num; i++){
        for(int j = 0; j < INODE_NUM_IN_BLOCK; j++){
            inode_blocks[i]._inodeblock.inodes[j].size = 0;
            inode_blocks[i]._inodeblock.inodes[j].head = -1;
        }
        below->write(below, below_ino, start + i, (void*)&inode_blocks[i]);
    }
    start += block_num;
    // 记录fat_entry开始块
    superblock._superblock.free_list = start;
    // entry_block_start = start;
    // 初始化fat_entry
    block_num = (BLOCK_NUM + ENTRY_NUM_IN_BLOCK - 1) / ENTRY_NUM_IN_BLOCK;
    // printf("entry block num = %d\n", block_num);
    union disk_block *entry_blocks = malloc(block_num * sizeof(union disk_block));
    for(int i = 0; i < block_num; i++){
        for(int j = 0; j < ENTRY_NUM_IN_BLOCK; j++){
            if(start + i * ENTRY_NUM_IN_BLOCK + j < BLOCK_NUM){
                entry_blocks[i]._freelistblock.fat_entry[j].next = DISK_EMPTY;
            }else{
                entry_blocks[i]._freelistblock.fat_entry[j].next = DISK_USED;
            }
        }
        // printf("mydisk_create, start + i = %d\n", start + i);
        below->write(below, below_ino, start + i, (void*)&entry_blocks[i]);
    }
    
    start += block_num;     // 真正记录数据块的编号(FAT_ENTRY结束)
    superblock._superblock.block_satrt = start;
    // data_block_start = start;
    // 写入超级块
    below->write(below, below_ino, 0, (void*)&superblock);
    free(entry_blocks);
    free(inode_blocks);
    /* Student's code ends here. */

    return 0;
}
