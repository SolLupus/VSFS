#ifndef FS_H
#define FS_H

#include<windows.h>
#include "type.h"
#include <cstdio>
#include <cstdint>
#include <pthread.h>

#define MAGIC 0x10203040
#define FILESYSNAME "So1_VFSF.sys"
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
#define MAX_FILE_SIZE 224
#define FILEENT_PER_BLOCK 16
#define MAX_FILE_NAME 20

#define INODE_BITMAP_START 1
#define DATA_BITMAP_START 2
#define SUPERBLOCK_START 0
#define SUPERBLOCK_SIZE (BLOCK_SIZE)

//File privilege
#define MODE_DIR	01000					
#define MODE_FILE	00000					
#define OWNER_R	4<<6						
#define OWNER_W	2<<6						
#define OWNER_X	1<<6						
#define GROUP_R	4<<3						
#define GROUP_W	2<<3						
#define GROUP_X	1<<3						
#define OTHERS_R	4						
#define OTHERS_W	2						
#define OTHERS_X	1						
#define FILE_DEF_PERMISSION 0664			
#define DIR_DEF_PERMISSION	0755			

//User information
#define MAX_NAME_SIZE 20


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
	int mutex;			//whether there is write operation 0-none 1-yes
	uint mode;			//Privilege For File r-read w-write x-exec
	uint inum;			//inode number 
	uint fileSize;		//Size of File

	char uname[20];		// user Name
	char gname[20];		// group Name

	time_t atime;		//access time
	time_t ctime;		//create time
	time_t mtime;		//modify time

	uint dirBlock[IPB];	//refs to physical block position
	uint IdirBlock;
};


#define DIR_SIZE 28

struct Dirent {				//32B
	uint iaddr;				//address of relate inode number
	char name[DIR_SIZE];	//Name of dirent
};

struct FileEnt {				//256B     dirent(32B) + file_content(224B);
	Dirent dir;
	char buffer[MAX_FILE_SIZE];
	int mutex;						//whether there is write operation 0-none 1-yes
};


//global variable defination
extern const int Superblock_StartAddr;		
extern const int InodeBitmap_StartAddr;		
extern const int BlockBitmap_StartAddr;		
extern const int Inode_StartAddr;			
extern const int Block_StartAddr;			
extern const int File_Max_Size;				
extern const int Sum_Size;					

// user information
extern bool isLogin;						//user state fot login
extern int nextUID;							//next uid
extern int nextGID;							//next group id
extern int Root_Dir_Addr;					//address of root inode
extern int Cur_Dir_Addr;					//current dictionary
extern char Cur_Dir_Name[1024];				//current dictionaryName
extern char Cur_Host_Name[110];				//current hostName
extern char Cur_User_Name[110];				//current userName
extern char Cur_Group_Name[110];			//current groupName
extern char Cur_User_Dir_Name[310];			//current user dictonary name

extern FILE* fw;							//file write pointer
extern FILE* fr;							//file read pointer
extern SuperBlock* superblock;				//super block pointer 

extern char Sys_buffer[SYS_SIZE];			//64M

int BlockAlloc();							//allocate empty block
int InodeAlloc();							//allocate empty inode
bool BlockFree(int address);				//free block
bool InodeFree(int address);				//free inode
int extractPath(char path[]);				//path parser -> return target inodeaddress
char* extractLastPath(char path[]);
void iput(_iobuf* fpoint,inode& in, int address);
void iget(_iobuf* fpoint,inode& in, int address);
void fput(_iobuf* fpoint, FileEnt(&fileEnt)[FILEENT_PER_BLOCK], int address,int position);
void fget(_iobuf* fpoint, FileEnt(&fileEnt)[FILEENT_PER_BLOCK], int address,int position);
char* replaceDoubleDot(char* original, const char* replacement);
void truncatePath(char* basePath, const char* subPath);
char* substring(char dir[], char name[]);

void Help_NotLogin();
bool Install();
bool Format();
#endif FS_H