#include "bitmap.h"
#include "fs.h"
#include "user.h"
#include <iostream>
#include <conio.h>
#include "user.cpp"
#include <stdio.h>
#include <pthread.h>

using namespace std;

//InitSystem finish
static void InitSystem() {
	//Virtual Disk does not exist
	if ((fr = fopen(FILESYSNAME, "rb")) == NULL) {

		fw = fopen(FILESYSNAME, "wb");
		if (fw == NULL) {
			cout << "Virtual disk file open failure." << endl;
			return;	
		}
		fr = fopen(FILESYSNAME, "rb");

		//init variable
		nextUID = 0;
		nextGID = 0;
		isLogin = false;
		strcpy_s(Cur_User_Name, "root");
		strcpy_s(Cur_Group_Name, "root");

		//get hostname
		memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
		DWORD k = 100;
		GetComputerName((LPTSTR)Cur_Host_Name, &k);
		strcpy_s(Cur_Dir_Name, "/");

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
		strcpy_s(Cur_User_Name, "root");
		strcpy_s(Cur_Group_Name, "root");

		memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
		DWORD k = 100;
		GetComputerName((LPTSTR)Cur_Host_Name, &k);
		Root_Dir_Addr = Inode_StartAddr+INODE_SIZE;
		Cur_Dir_Addr = Root_Dir_Addr;
		strcpy_s(Cur_Dir_Name, "/");

		if (!Install()) {
			cout << "Failed to install file system." << endl;
			return;
		}
	}
}

//Install finish
static bool Install() {
	fseek(fr, Superblock_StartAddr, SEEK_SET);
	fread(superblock, sizeof(SuperBlock), 1, fr);
	fflush(fr);
	imap = new uint8_t[BLOCK_SIZE];
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(imap, BLOCK_SIZE, 1, fr);

	bmap = new uint8_t[BLOCK_SIZE];
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(bmap, BLOCK_SIZE, 1, fr);
	fflush(fr);
	return true;
}


//Format finish
static bool Format() {

	//Super BLock init
	superblock->magic = MAGIC;
	superblock->size = SYS_SIZE;
	superblock->nBlocks = BLOCK_NUM;
	superblock->nInodes = MAXFILE_NUM;
	superblock->blockStart = Block_StartAddr;
	superblock->inodeStart = Inode_StartAddr;
	superblock->bmapSize = (BLOCK_NUM - 8 + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	superblock->imapSize = (MAXFILE_NUM + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	superblock->imapStart = InodeBitmap_StartAddr;
	superblock->bmapStart = BlockBitmap_StartAddr;
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	//write Block_Bitmap to Virtual disk
	bmap = (uint8_t*)malloc(superblock->bmapSize);
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(bmap, BLOCK_SIZE, 1, fw);


	//write Inode_Bitmap to Virtual Disk
	imap = (uint8_t*)malloc(superblock->imapSize);
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(imap, BLOCK_SIZE, 1, fw);

	fflush(fw);


	//Create inode for root_dictionary
	inode cur;

	int inoAddr = InodeAlloc();
	int blockAddr = BlockAlloc();
	Root_Dir_Addr = inoAddr;
	Cur_Dir_Addr = Root_Dir_Addr;
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	Dirent dirent = { 0 };
	strcpy_s(dirent.name, "/");
	dirent.iaddr = inoAddr;
	fileEnt[0].dir = dirent;
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(fileEnt, sizeof(fileEnt), 1, fw);

	cur.inum = 0;
	cur.atime = time(NULL);
	cur.ctime = time(NULL);
	cur.mtime = time(NULL);
	cur.fileSize = 0;
	strcpy_s(cur.uname, Cur_User_Name);
	strcpy_s(cur.gname, Cur_Group_Name);
	cur.dirBlock[0] = blockAddr;
	for (int i = 1; i < IPB; i++) {
		cur.dirBlock[i] = -1;
	}
	cur.IdirBlock = -1;
	cur.mode = MODE_DIR | DIR_DEF_PERMISSION;
	initLock(cur);
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, INODE_SIZE, 1, fw);
	fflush(fw);
	return true;
}

//didn't use it
static void Ready()	
{
	nextUID = 0;
	nextGID = 0;
	isLogin = false;
	strcpy_s(Cur_User_Name, "root");
	strcpy_s(Cur_Group_Name, "root");

	memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
	DWORD k = 100;
	GetComputerName((LPWSTR)Cur_Host_Name, &k);

	Root_Dir_Addr = Inode_StartAddr;	
	Cur_Dir_Addr = Root_Dir_Addr;
	strcpy_s(Cur_Dir_Name, "/");


	char c;
	cout << "Do you want to format? [y/n] ";
	while (c = getchar()) {
		fflush(stdin);
		if (c == 'y') {
			printf("\n");
			cout << endl << "The file system is being formatted......" << endl;
			if (!Format()) {
				cout << "Format failur." << endl;
				return;
			}
			cout << "Format complete." << endl;
			break;
		}
		else if (c == 'n') {
			printf("\n");
			break;
		}
	}

	// init user
	InitUser();

	if (!Install()) {
		cout << "Failed to install file system." << endl;
		return;
	}
}