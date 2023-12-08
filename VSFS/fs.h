#ifndef FS_H
#define FS_H

#include<windows.h>
#include "type.h"
#include <cstdio>
#include <cstdint>


#define MAGIC 0x10203040
#define FILESYSNAME "So1VFSF.sys"
#define SYSNAME "So1's VSVF"


//Block
#define BLOCK_SIZE 1024*4    //Size of Block
#define BLOCK_NUM 64		//total number of block
#define SYS_SIZE (BLOCK_SIZE*BLOCK_NUM)

//INODE and INODE_TABLE
#define INODE_SIZE 256		//Max size of inode 256B
#define IBLOCK_NUM 5		//IBLOCK 5

//Inode Per Block
#define IPB (BLOCK_SIZE/INODE_SIZE)		//Inode per Iblock in  inode table
#define MAXFILE_NUM IPB*IBLOCK_NUM
#define INODE_TABLE_START 3				//Start Position 3*BLOCKSIZE

//File
#define MAX_FILE_SIZE 256 


#define INODE_BITMAP_START 1
#define DATA_BITMAP_START 2
#define SUPERBLOCK_START 0
#define SUPERBLOCK_SIZE (BLOCK_SIZE)

struct SuperBlock
{
	uint magic;				//Magic Number
	uint size;				//Size of FileSystem
	uint nBlocks;			//Total number of block 
	uint nInodes;			//Total number of inode
	uint emptyBlock;
	uint emptyInode;
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

	char uname[20];		// user Name
	char gname[20];		// group Name

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

//全局变量定义
extern const int Superblock_StartAddr;		
extern const int InodeBitmap_StartAddr;		
extern const int BlockBitmap_StartAddr;		
extern const int Inode_StartAddr;			
extern const int Block_StartAddr;			
extern const int File_Max_Size;				
extern const int Sum_Size;					

// 用户
extern bool isLogin;						//user state fot login
extern int nextUID;							//next uid
extern int nextGID;							//next group id
extern int Root_Dir_Addr;					//address of root inode
extern int Cur_Dir_Addr;					//current dictionary
extern char Cur_Dir_Name[310];				//current dictionaryName
extern char Cur_Host_Name[110];				//current hostName
extern char Cur_User_Name[110];				//current userName
extern char Cur_Group_Name[110];			//current groupName
extern char Cur_User_Dir_Name[310];			//current user dictonary name

extern FILE* fw;							//file write pointer
extern FILE* fr;							//file read pointer
extern SuperBlock* superblock;				//super block pointer 

extern char Sys_buffer[SYS_SIZE];				//64M
#endif