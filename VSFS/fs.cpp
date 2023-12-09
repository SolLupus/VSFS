#include "fs.h"
#include "bitmap.h"
#include <iostream>
using namespace std;

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
	//еп╤о
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