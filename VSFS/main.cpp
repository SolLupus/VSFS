#include "fs.h"
#include<stdio.h>
#include <iostream>
#include "mkfs.cpp"
using namespace std;

//全局变量定义
const int Superblock_StartAddr = SUPERBLOCK_START;																	
const int InodeBitmap_StartAddr = INODE_BITMAP_START * BLOCK_SIZE;													
const int BlockBitmap_StartAddr = DATA_BITMAP_START * BLOCK_SIZE;							
const int Inode_StartAddr = INODE_TABLE_START * BLOCK_SIZE;								
const int Block_StartAddr = Inode_StartAddr + IBLOCK_NUM * BLOCK_SIZE;
const int File_Max_Size = MAX_FILE_SIZE;

FILE* fw;									
FILE* fr;									
SuperBlock* superblock = new SuperBlock;	
uint8_t* imap;						
uint8_t* bmap;						
char Sys_buffer[SYS_SIZE] = { 0 };				

bool isLogin;								
int Root_Dir_Addr;							
int Cur_Dir_Addr;							
char Cur_Dir_Name[310];						
char Cur_Host_Name[110];					
char Cur_User_Name[110];					
char Cur_Group_Name[110];					
char Cur_User_Dir_Name[310];				
int nextUID;								
int nextGID;								



int main() {
	char inputLine[100];

	InitSystem();

	if (isLogin == false) {
		cout << "-----------------------------------------" << endl;
		cout << "Wellcome to " << SYSNAME << endl;
		cout << "If you need help, please type 'help'." << endl;
		cout << "-----------------------------------------" << endl;
	}

	while (1) {

		if (isLogin == false) {
			cout << "$ ";
		}
		else {
			char* p;
			if ((p = strstr(Cur_Dir_Name, Cur_User_Dir_Name)) == NULL)	//没找到
				printf("[%s@%s %s]$ ", Cur_User_Name, Cur_Host_Name, Cur_Dir_Name);
			else
				printf("[%s@%s ~%s]$ ", Cur_User_Name, Cur_Host_Name, Cur_Dir_Name + strlen(Cur_User_Dir_Name));
		}

		fgets(inputLine,sizeof(inputLine), stdin);
		cmd(inputLine);
	}

	delete(superblock);
	superblock = NULL;
	fclose(fw);
	fclose(fr);

	return 0;
}