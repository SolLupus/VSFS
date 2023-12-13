#include "bitmap.h"
#include <stdio.h>

// block bitmap operation
//uint8_t *bmap = (uint8_t *)malloc(sb.bmapSize);
int allocate_block(SuperBlock* sb, uint8_t* bmap)
{
    for (uint i = 0; i < sb->nBlocks; ++i)
    {
        if (!GET_BIT(bmap, i))
        {
            SET_BIT(bmap, i);
            return i;
        }
    }
    // 没有可用的块
    return -1;
}

void free_block(SuperBlock* sb, uint8_t* bmap, uint blockIndex)
{
    if (blockIndex < sb->nBlocks)
    {
        SET_BIT(bmap, blockIndex);
    }
}

void print_block_bitmap(uint8_t* bmap, uint bmapSize)
{
    printf("Block Bitmap:\n");
    for (uint i = 0; i < bmapSize; ++i)
    {
        printf("%02X ", bmap[i]);
    }
    printf("\n");
}


//Inode bitmap operation

int allocate_inode(SuperBlock* sb, uint8_t* imap)
{
    for (uint i = 0; i < sb->nInodes; ++i)
    {
        if (!GET_BIT(imap, i))
        {
            SET_BIT(imap, i);
            return i;
        }
    }
    // 没有可用的块
    return -1;
}

void free_inode(SuperBlock* sb, uint8_t* imap, uint InodeIndex)
{
    if (InodeIndex < sb->nInodes)
    {
        SET_BIT(imap, InodeIndex);
    }
}

void print_inode_bitmap(uint8_t* imap, uint imapSize) {
    printf("Block Bitmap:\n");
    for (uint i = 0; i < imapSize; ++i)
    {
        printf("%02X ", imap[i]);
    }
    printf("\n");
}