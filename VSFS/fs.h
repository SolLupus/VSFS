#ifndef FS_H
#define FS_H

#include<windows.h>
#include "type.h"


#define MAGIC 0x10203040
#define SYSNAME "So1VFSF.sys"

//Block
#define BLOCK_SIZE 1024*4    //Size of Block
#define BLOCK_NUM 64		//total number of block


//INODE and INODE_TABLE
#define INODE_SIZE 256		//Max size of inode 256B
#define IBLOCK_NUM 5		//IBLOCK 5

//Inode Per Block
#define IPB (BLOCK_SIZE/INODE_SIZE)		//一个磁盘可以存储16个inode
#define MAXFILE_NUM IPB*IBLOCK_NUM
#define INODE_TABLE_START 3				//Start Position 3*BLOCKSIZE

//File
#define MAX_FILE_SIZE 256 


#define INODE_BITMAP_START 1
#define DATA_BITMAP_START 2
#define SUPERBLOCK_START 0
#define SUPERBLOCK_SIZE (BLOCK_SIZE)




// 块位图的设置和获取宏
#define BITS_PER_BYTE 8
#define SET_BIT(bitmap, bit) ((bitmap)[(bit) / BITS_PER_BYTE] |= (1 << ((bit) % BITS_PER_BYTE)))
#define GET_BIT(bitmap, bit) ((bitmap)[(bit) / BITS_PER_BYTE] & (1 << ((bit) % BITS_PER_BYTE)))



struct SuperBlock
{
	uint magic;				//Magic Number
	uint size;				//Size of FileSystem
	uint nBlocks;			//Total number of block 
	uint nInodes;			//Total number of inode
	uint blockStart;		//block start
	uint inodeStart;		//inode_table start 
	uint bmapSize;			// (nBlocks + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	uint imapSize;			// (nInodes + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	uint imapStart;			//inode bitmap start address
	uint bmapStart;			//data bitmap start address
};

struct inode{
	uint type;			//dir or file
	uint mode;			//Privilege For File r-read w-write x-exec
	uint inum;			//inode number 
	uint fileSize;		//Size of File
	uint refCnt;		//Reference to this file

	char uname[20];
	char gname[20];

	time_t atime;		//access time
	time_t ctime;		//create time
	time_t mtime;		//modify time

	uint dirBlock[IPB];	//refs to physical block position
};


#define DIR_SIZE 28

struct dirent {				//32B
	uint inum;				//inode number
	char name[DIR_SIZE];	//Name of dirent
};

#endif