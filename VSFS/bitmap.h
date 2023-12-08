#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include "fs.h"
#include "type.h"

#define BITS_PER_BYTE 8

#define SET_BIT(bitmap, bit) ((bitmap)[(bit) / BITS_PER_BYTE] |= (1 << ((bit) % BITS_PER_BYTE)))
#define GET_BIT(bitmap, bit) ((bitmap)[(bit) / BITS_PER_BYTE] & (1 << ((bit) % BITS_PER_BYTE)))
extern uint8_t* imap;		//bitmap of inode
extern uint8_t* bmap;		//bitmap of block

int allocate_block(struct SuperBlock* sb, uint8_t* bmap);
void free_block(struct SuperBlock* sb, uint8_t* bmap, uint blockIndex);
void print_block_bitmap(uint8_t* bmap, uint bmapSize);

int allocate_inode(struct SuperBlock* sb, uint8_t* imap);
void free_inode(struct SuperBlock* sb, uint8_t* imap, uint InodeIndex);
void print_inode_bitmap(uint8_t* imap, uint imapSize);


#endif // !BITMAP_H
