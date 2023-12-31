#include "fs.h"
#include "bitmap.h"
#include <iostream>
#include <pthread.h>
using namespace std;


//Atomic operation
//After-write for inode
void iput(_iobuf* fpoint,inode& in, int address) {
	in.mutex = in.mutex - 1;
	fseek(fpoint, address, SEEK_SET);
	fwrite(&in,INODE_SIZE,1, fpoint);
	fflush(fpoint);
}

//Pre-write for inode
void iget(_iobuf* fpoint,inode& in, int address) {
	in.mutex = in.mutex + 1;
	fseek(fpoint, address, SEEK_SET);
	fwrite(&in, INODE_SIZE, 1, fpoint);
	fflush(fpoint);
}


//After-write for fileEnt
void fput(_iobuf* fpoint,FileEnt (&fileEnt)[FILEENT_PER_BLOCK], int address, int position) {
	fileEnt[position].mutex = fileEnt[position].mutex - 1;
	fseek(fpoint, address, SEEK_SET);
	fwrite(&fileEnt, sizeof(fileEnt), 1, fpoint);
	fflush(fpoint);
}


//Pre-write for fileEnt 
void fget(_iobuf* fpoint,FileEnt (&fileEnt)[FILEENT_PER_BLOCK], int address, int position) {
	fileEnt[position].mutex = fileEnt[position].mutex + 1;
	fseek(fpoint, address, SEEK_SET);
	fwrite(&fileEnt, sizeof(fileEnt), 1, fpoint);
	fflush(fpoint);
}



// Allocate Inode
int InodeAlloc()
{
	int position;
	position = allocate_inode(superblock, imap);
	if (position == -1) {
		cout << "No free inode can be allocated." << endl;
		return -1;
	}
	else {
		fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
		fwrite(imap, sizeof(BLOCK_SIZE), 1, fw);
		fflush(fw);
		return Inode_StartAddr + position * INODE_SIZE;
	}
}

//Free Inode
bool InodeFree(int address)
{
	//�ж�
	if ((address - Inode_StartAddr) % INODE_SIZE != 0) {
		cout << "Address error, this location is not the starting position of inode." << endl;
		return false;
	}
	unsigned short inum = (address - Inode_StartAddr) / INODE_SIZE;
	free_inode(superblock, imap, inum);
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(imap, sizeof(BLOCK_SIZE), 1, fw);

	//claer inode
	inode tmp = { 0 };
	fseek(fw, address, SEEK_SET);
	fwrite(&tmp, INODE_SIZE, 1, fw);

	fflush(fw);

	return true;
}

//Allocate Block
int BlockAlloc() {
	int position;
	position = allocate_block(superblock, bmap);
	if (position == -1) {
		cout << "No free data block can be allocated" << endl;;
		return -1;
	}
	else {
		fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
		fwrite(bmap, sizeof(BLOCK_SIZE), 1, fw);
		fflush(fw);
		return Block_StartAddr + BLOCK_SIZE * position;
	}
}


//Free whole Block
bool BlockFree(int bnum) {
	if (!GET_BIT(bmap, bnum)) {
		cout << "This Block is not used." << endl;
		return false;
	}
	free_block(superblock, bmap, bnum);
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(bmap, sizeof(BLOCK_SIZE), 1, fw);

	char buffer[BLOCK_SIZE] = { 0 };
	fseek(fw, Block_StartAddr + bnum * BLOCK_SIZE, SEEK_SET);
	fwrite(buffer, sizeof(buffer), 1, fw);
	fflush(fw);
	return true;
}

//get substring
//example: get /home/ from /home/root
char* substring(char dir[],char name[]) {
	const char* found = strstr(dir, name);
	if (strcmp(dir, "/") == 0) {
		return dir;
	}
	else if (dir[strlen(dir)-1]=='/') {
		return dir;
	}
	if (found != nullptr) {
		size_t position = found - dir;		
		char* result = new char[position + 1];  
		strncpy(result, dir, position);  
		result[position] = '\0';  
		if (strlen(result) != 1 && result[position - 1] == '/') {
			result[position - 1] = '\0';
		}
		return result;
	}
	else {
		return "";
	}

	return "";
}

//parse path to inodeaddress
//example : get root indode address from /home/root
int extractPath(char path[]) {
	char pathCopy[1024];
	strcpy_s(pathCopy, path);
	inode tmp = { 0 };
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	int address=0;
	if (path[0] == '/') {
		if (strlen(path) == 1) {
			return Root_Dir_Addr;
		}
		fseek(fr, Root_Dir_Addr, SEEK_SET);
		fread(&tmp, sizeof(inode), 1, fr);
		fflush(fr);
		fseek(fr, tmp.dirBlock[0], SEEK_SET);
		fread(&fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
	}
	else if (strcmp(path, "") == 0) {
		return Cur_Dir_Addr;
	}
	else {
		fseek(fr, Cur_Dir_Addr, SEEK_SET);
		fread(&tmp, sizeof(inode), 1, fr);
		fflush(fr);
		fseek(fr, tmp.dirBlock[0], SEEK_SET);
		fread(&fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
	}
	char* token = strtok(pathCopy, "/");
	while (token != nullptr) {
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (token == ".")
				break;
			else if (strcmp(token,fileEnt[i].dir.name)==0) {
				if (((tmp.mode >> 9) & 1) == 1) {
					address = fileEnt[i].dir.iaddr;

					fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
					fread(&tmp, sizeof(inode), 1, fr);
					fflush(fr);
					fseek(fr, tmp.dirBlock[0], SEEK_SET);
					fread(&fileEnt, sizeof(fileEnt), 1, fr);
					fflush(fr);
				}
				else {
					address = fileEnt[i].dir.iaddr;
				}
			}
		}
		token = strtok(nullptr, "/");
	}
	if (address !=0)
		return address;
	else
		return -1;
}


//parse path to sub one
//example: from /home/root to /home
void truncatePath(char* basePath, const char* subPath) {
	char* found = strstr(basePath, subPath);

	if (!found) {
		return;
	}
	size_t newLen = found - basePath +strlen(subPath);

	basePath[newLen] = '\0';
}


//replace .. to specific path
//example: replace .. to home
char* replaceDoubleDot(char* original, const char* replacement) {
	char* found = strstr(original, "..");

	if (!found) {
		return original;
	}

	size_t replacement_len = strlen(replacement);
	size_t original_len = strlen(original);


	size_t new_len = original_len - 1 + replacement_len;
	char* new_str = (char*)malloc(new_len + 1);

	if (!new_str) {
		return original;
	}

	strncpy(new_str, original, found - original);

	strncpy(new_str + (found - original), replacement, replacement_len);


	strcpy(new_str + (found - original) + replacement_len, found + 2);

	return new_str;


	free(new_str);

	replaceDoubleDot(original + (found - original) + replacement_len, replacement);
}


//get the last name of path 
//example:  get passwd from /etc/passwd
char* extractLastPath(char path[]) {
	char pathCopy[1024];
	strcpy_s(pathCopy, path);


	char* token =strtok(pathCopy, "/");
	char* lastPathComponent = nullptr;

	while (token != nullptr) {
		lastPathComponent = token;
		token = strtok(nullptr, "/");
	}

	if (lastPathComponent != nullptr) {
		return lastPathComponent;
	}

	return "";
}

