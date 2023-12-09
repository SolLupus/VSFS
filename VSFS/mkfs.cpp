#include "bitmap.h"
#include "fs.h"
#include "user.h"
#include <iostream>
#include <conio.h>
#include "user.cpp"

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
		strcpy_s(Cur_User_Name, "root");
		strcpy_s(Cur_Group_Name, "root");

		//获取主机名
		memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
		DWORD k = 100;
		GetComputerName((LPTSTR)Cur_Host_Name, &k);

		Root_Dir_Addr = Inode_StartAddr;
		Cur_Dir_Addr = Root_Dir_Addr;
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
		Root_Dir_Addr = Inode_StartAddr;
		Cur_Dir_Addr = Root_Dir_Addr;
		strcpy_s(Cur_Dir_Name, "/");

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
	superblock->bmapSize = (BLOCK_NUM - 8 + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	superblock->imapSize = (MAXFILE_NUM + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	superblock->imapStart = InodeBitmap_StartAddr;
	superblock->bmapStart = BlockBitmap_StartAddr;
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(superblock), 1, fw);

	//write Block_Bitmap to Virtual disk
	memset(bmap, 0, superblock->bmapSize);
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(bmap, sizeof(bmap), 1, fw);


	//write Inode_Bitmap to Virtual Disk
	memset(imap, 0, superblock->imapSize);
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(imap, sizeof(imap), 1, fw);

	fflush(fw);


	//Create inode for root_dictionary
	inode cur;

	int inoAddr = InodeAlloc();
	int blockAddr = BlockAlloc();

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
	cur.refCnt = 0;
	cur.dirBlock[0] = blockAddr;
	for (int i = 1; i < IPB; i++) {
		cur.dirBlock[i] = -1;
	}
	cur.IdirBlock = -1;
	cur.mode = MODE_DIR | DIR_DEF_PERMISSION;
	
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, INODE_SIZE, 1, fw);
	fflush(fw);
}

void Ready()	//登录系统前的准备工作,变量初始化+注册+安装
{
	//初始化变量
	nextUID = 0;
	nextGID = 0;
	isLogin = false;
	strcpy_s(Cur_User_Name, "root");
	strcpy_s(Cur_Group_Name, "root");

	//获取主机名
	memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
	DWORD k = 100;
	GetComputerName((LPWSTR)Cur_Host_Name, &k);

	//根目录inode地址 ，当前目录地址和名字
	Root_Dir_Addr = Inode_StartAddr;	//第一个inode地址
	Cur_Dir_Addr = Root_Dir_Addr;
	strcpy_s(Cur_Dir_Name, "/");


	char c;
	cout << "Do you want to format? [y/n] ";
	while (c = getch()) {
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

	// 初始化用户，创建目录及配置文件
	InitUser();

	//printf("载入文件系统……\n");
	if (!Install()) {
		cout << "Failed to install file system." << endl;
		return;
	}
	//printf("载入完成\n");
}