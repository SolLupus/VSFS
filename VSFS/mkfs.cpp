#include "bitmap.h"
#include "fs.h"
#include <iostream>
#include <exception>

using namespace std;

void InitSystem() {
	//Virtual Disk does not exist
	if ((fr = fopen(FILESYSNAME, "rb")) == NULL) {

		fw = fopen(FILESYSNAME, "wb");
		if (fw == NULL) {
			cout << "Virtual disk file open failure." << endl;
			return;	//打开文件失败
		}
		fr = fopen(FILESYSNAME, "rb");

		//初始化变量
		nextUID = 0;
		nextGID = 0;
		isLogin = false;
		strcpy(Cur_User_Name, "root");
		strcpy(Cur_Group_Name, "root");

		//获取主机名
		memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
		DWORD k = 100;
		GetComputerName((LPTSTR)Cur_Host_Name, &k);

		Root_Dir_Addr = Inode_StartAddr;
		Cur_Dir_Addr = Root_Dir_Addr;
		strcpy(Cur_Dir_Name, "/");

		cout << "-----------------------------------------" << endl;
		cout << "The file system is being formatted......" << endl;
		if (!Format()) {
			cout << "Formatting failure!" << endl;
			getchar();
			return;
		}
		cout << "Format complete." << endl;

		InitUser();

		if (!Install()) {
			cout << "Failed to install file system." << endl;
			return;
		}
	}
	//Virtual Disk already exist
	else {
		fread(Sys_buffer, SYS_SIZE, 1, fr);

		fw = fopen(FILESYSNAME, "wb");
		if (fw == NULL) {
			cout << "Virtual disk file failed to open." << endl;
			return;
		}
		fwrite(Sys_buffer, SYS_SIZE, 1, fw);


		nextUID = 0;
		nextGID = 0;
		isLogin = false;
		strcpy(Cur_User_Name, "root");
		strcpy(Cur_Group_Name, "root");

		memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
		DWORD k = 100;
		GetComputerName((LPTSTR)Cur_Host_Name, &k);
		Root_Dir_Addr = Inode_StartAddr;
		Cur_Dir_Addr = Root_Dir_Addr;
		strcpy(Cur_Dir_Name, "/");

		if (!Install()) {
			cout << "Failed to install file system." << endl;
			return;
		}
	}
}

bool Install() {
	fseek(fr, Superblock_StartAddr, SEEK_SET);
	fread(superblock, sizeof(SuperBlock), 1, fr);

	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(imap, sizeof(imap), 1, fr);

	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(bmap, sizeof(bmap), 1, fr);
	return true;
}



bool Format() {

	//Super BLock init
	superblock->magic = MAGIC;
	superblock->size = SYS_SIZE;
	superblock->nBlocks = BLOCK_NUM;
	superblock->nInodes = MAXFILE_NUM;
	superblock->blockStart = Block_StartAddr;
	superblock->inodeStart = Inode_StartAddr;
	superblock->bmapSize = (BLOCK_NUM + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	superblock->imapSize = (MAXFILE_NUM + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	superblock->imapStart = InodeBitmap_StartAddr;
	superblock->bmapStart = BlockBitmap_StartAddr;
	superblock->emptyBlock = BLOCK_NUM - 8;
	superblock->emptyInode = MAXFILE_NUM;
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(superblock), 1, fw);

	//write Block_Bitmap to Virtual disk
	bmap = (uint8_t*)malloc(superblock->bmapSize);
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(bmap, sizeof(bmap), 1, fw);


	//write Inode_Bitmap to Virtual Disk
	imap = (uint8_t*)malloc(superblock->imapSize);
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(imap, sizeof(imap), 1, fw);

	fflush(fw);


	//init inode
}