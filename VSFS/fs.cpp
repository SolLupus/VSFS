#include "fs.h"
#include "bitmap.h"
#include <iostream>
using namespace std;



void initLock(inode node) {
	pthread_rwlock_init(&(node.lock), NULL);
}
//ReadLock
//read pthread_rwlock_rdlock(&(node.lock));
// ...
//pthread_rwlock_unlock(&(node.lock));

//∂¡∂¡‘ –Ì°¢∂¡–¥ª•≥‚°¢–¥–¥ª•≥‚
//WriteLock
//pthread_rwlock_wrlock(&(node.lock));
// ...
//pthread_rwlock_unlock(&(node.lock));

void destroyLock(inode node) {
	pthread_rwlock_destroy(&(node.lock));
}

void initFileEntLock(FileEnt fileEnt)
{
	pthread_rwlock_init(&(fileEnt.lock), NULL);
}

void destroyFileEntLock(FileEnt fileEnt) {
	pthread_rwlock_destroy(&(fileEnt.lock));
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
		fwrite(imap, sizeof(imap), 1, fw);
		fflush(fw);
		return Inode_StartAddr + position * INODE_SIZE;
	}
}

//Free Inode
bool InodeFree(int address)
{
	//≈–∂œ
	if ((address - Inode_StartAddr) % INODE_SIZE != 0) {
		cout << "Address error, this location is not the starting position of inode." << endl;
		return false;
	}
	unsigned short inum = (address - Inode_StartAddr) / INODE_SIZE;
	free_inode(superblock, imap, inum);
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(imap, sizeof(imap), 1, fw);

	//claer inode
	inode tmp = { 0 };
	fseek(fw, address, SEEK_SET);
	fwrite(&tmp, sizeof(tmp), 1, fw);

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
		fwrite(bmap, sizeof(bmap), 1, fw);
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
	fwrite(bmap, sizeof(bmap), 1, fw);

	char buffer[BLOCK_SIZE] = {0};
	fseek(fw, Block_StartAddr+bnum*BLOCK_SIZE, SEEK_SET);
	fwrite(buffer, sizeof(buffer), 1, fw);
	fflush(fw);
	return true;
}


//parse path to inodeaddress
int extractPath(char path[]) {
	char* pathCopy = new char[strlen(path) + 1];
	strcpy(pathCopy, path);
	inode tmp = { 0 };
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };

	if (path[0] = '/') {
		fseek(fr, Root_Dir_Addr, SEEK_SET);
		fread(&tmp, sizeof(inode), 1, fr);
		fflush(fr);
		fseek(fr, tmp.dirBlock[0], SEEK_SET);
		fread(&fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
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
			else if (token == "..") {
				if (Cur_Dir_Addr == Root_Dir_Addr)
					break;
				else {
					fseek(fr, Cur_Dir_Addr, SEEK_SET);
					fread(&tmp, sizeof(inode), 1, fr);
					fflush(fr);
					fseek(fr, tmp.dirBlock[0], SEEK_SET);
					fread(&fileEnt, sizeof(fileEnt), 1, fr);
					fflush(fr);
				}
			}
			else if (token == fileEnt[i].dir.name) {
				if (((tmp.mode >> 9) & 1) == 1) {
					fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
					fread(&tmp, sizeof(inode), 1, fr);
					fflush(fr);
					fseek(fr, tmp.dirBlock[0], SEEK_SET);
					fread(&fileEnt, sizeof(fileEnt), 1, fr);
					fflush(fr);
				}
				else {
					return fileEnt[i].dir.iaddr;
				}
			}
		}
		token = strtok(nullptr, "/");
	}
	cout << "Can't  find such a file.";
	return -1;
}



char* extractLastPath(char path[]) {
	char* pathCopy = new char[strlen(path) + 1];
	std::strcpy(pathCopy, path);

	//  π”√ strtok ∑÷∏Ó¬∑æ∂◊÷∑˚¥Æ
	char* token = std::strtok(pathCopy, "/");
	char* lastPathComponent = nullptr;

	while (token != nullptr) {
		lastPathComponent = token;
		token = std::strtok(nullptr, "/");
	}

	if (lastPathComponent != nullptr) {
		return lastPathComponent;

		delete[] pathCopy;
	}
}