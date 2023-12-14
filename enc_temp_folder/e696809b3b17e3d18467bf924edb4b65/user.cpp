#include "user.h"
#include "fs.h"
#include "bitmap.h"
#include <conio.h>
#include "type.h"
#include <iostream>
#include <stdio.h>
#include <pthread.h>
using namespace std;

//InitUser finish
static void InitUser() //ok
{
	cout << "Please type the root password!" << endl;
	bool ok = false;
	char password1[100] = { 0 }, password2[100] = { 0 };
	do {
		cout << "First input ";
		inputPassword(password1);
		cout << "Second input ";
		inputPassword(password2);
		ok = (strcmp(password1, "") != 0 && strcmp(password1, password2) == 0);
		if (!ok)
			cout << "The password can not be empty, and the two entries must be equal" << endl;
		else
			break;
	} while (!ok);


	mkdir("/home");
	mkdir("/home/root");

	mkdir("/etc");

	char buf[100] = { 0 };

	sprintf(buf, "root:x:%d:%d\n", nextUID++, nextGID++);
	create("/etc/passwd", buf);

	char s_passwd[100] = { 0 };
	strcpy_s(s_passwd, "root:");
	strcat_s(s_passwd, password1);
	strcat_s(s_passwd, "\n");

	sprintf(buf, s_passwd);

	create("/etc/shadow", buf);
	chmod("/etc/shadow", 0660);

	sprintf(buf, "root::0:root\n");
	sprintf(buf + strlen(buf), "user::1:\n");
	create("/etc/group", buf);

	cd("/");
}

//login finish
static bool login(char username[]) //ok
{
	char password[100] = { 0 };

	if (strlen(username) >= MAX_NAME_SIZE) {
		cout << "The username is too long." << endl;
		return false;
	}
	if (isLogin == true) {
		cout << "You have already logged in." << endl;
		return false;
	}

	if (strcmp(username, "") == 0) {
		inputUsername(username);
		if (strcmp(username, "") == 0) {
			cout << "Username can not be empty." << endl;
			return false;
		}
	}

	inputPassword(password);
	if (strcmp(password, "") == 0) {
		cout << "Password can not be empty." << endl;
		return false;
	}

	if (check(username, password)) {
		isLogin = true;
		return true;
	}
	else {
		isLogin = false;
		return false;
	}
}

//logout finish
static void logout()
{
	if (isLogin == false) {
		cout << "You have not logged in yet!" << endl;
		return;
	}

	gotoRoot();
	isLogin = false;
}

//have bug in mkdir user dir ,the main reason is the global arg Cur_User_Name,to solve it should correct process it; **
static bool useradd(char username[])
{
	if (strcmp(Cur_User_Name, "root") != 0) {
		cout << "Permission denied." << endl;
		return false;
	}
	int passwd_Inode_Addr = -1;	//passwd inode address
	int shadow_Inode_Addr = -1;	//shadow inode address
	int group_Inode_Addr = -1;	//group inode address 
	inode passwd_Inode = { 0 };		//passwd inode
	inode shadow_Inode = { 0 };		//shadow inode
	inode group_Inode = { 0 };		//group inode

	char bak_Cur_User_Name[110];
	char bak_Cur_User_Name_2[110];
	char bak_Cur_User_Dir_Name[310];
	int bak_Cur_Dir_Addr;
	char bak_Cur_Dir_Name[310];
	char bak_Cur_Group_Name[310];
	char homeName[30];

	char passwd_buf[MAX_FILE_SIZE];
	char shadow_buf[MAX_FILE_SIZE];
	char group_buf[MAX_FILE_SIZE];



	inode cur = { 0 };
	strcpy_s(bak_Cur_User_Name, Cur_User_Name);
	strcpy_s(bak_Cur_User_Dir_Name, Cur_User_Dir_Name);
	bak_Cur_Dir_Addr = Cur_Dir_Addr;
	strcpy_s(bak_Cur_Dir_Name, Cur_Dir_Name);

	cd("/home");
	strcpy_s(bak_Cur_User_Name_2, Cur_User_Name);
	strcpy_s(bak_Cur_Group_Name, Cur_Group_Name);
	strcpy_s(Cur_User_Name, username); // 
	strcpy_s(Cur_Group_Name, "user");
	strcpy_s(homeName, "/home/");
	strcat_s(homeName, username);
	if (!mkdir(homeName)) {
		strcpy_s(Cur_User_Name, bak_Cur_User_Name_2);
		strcpy_s(Cur_Group_Name, bak_Cur_Group_Name);
		strcpy_s(Cur_User_Name, bak_Cur_User_Name);
		strcpy_s(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy_s(Cur_Dir_Name, bak_Cur_Dir_Name);

		cout << "Add user failure." << endl;
		return false;
	}
	strcpy_s(Cur_User_Name, bak_Cur_User_Name_2);
	strcpy_s(Cur_Group_Name, bak_Cur_Group_Name);

	cd("/etc");
	char passwd[100] = { 0 };
	inputPassword(passwd);
	fseek(fr, Cur_Dir_Addr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);

	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, "passwd") == 0) {
			passwd_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&passwd_Inode, sizeof(inode), 1, fr);
			strcpy_s(passwd_buf, fileEnt[i].buffer);
		}
		if (strcmp(fileEnt[i].dir.name, "shadow") == 0) {
			shadow_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&shadow_Inode, sizeof(inode), 1, fr);
			strcpy_s(shadow_buf, fileEnt[i].buffer);
		}
		if (strcmp(fileEnt[i].dir.name, "group") == 0) {
			group_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&group_Inode, sizeof(inode), 1, fr);
			strcpy_s(group_buf, fileEnt[i].buffer);
		}
	}
	fflush(fr);

	if (strstr(passwd_buf, username) != nullptr) {
		cout << "The user has already existed." << endl;

		strcpy_s(Cur_User_Name, bak_Cur_User_Name);
		strcpy_s(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy_s(Cur_Dir_Name, bak_Cur_Dir_Name);
		return false;
	}

	pthread_rwlock_wrlock(&passwd_Inode.lock);
	sprintf(passwd_buf + strlen(passwd_buf), "%s:x:%d:%d\n", username, nextUID++, 1);
	passwd_Inode.fileSize = strlen(passwd_buf);
	fseek(fw, passwd_Inode_Addr, SEEK_SET);
	fwrite(&passwd_Inode, INODE_SIZE, 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&passwd_Inode.lock);

	pthread_rwlock_wrlock(&shadow_Inode.lock);
	sprintf(shadow_buf + strlen(shadow_buf), "%s:%s\n", username, passwd);
	shadow_Inode.fileSize = strlen(shadow_buf);
	fseek(fw, shadow_Inode_Addr, SEEK_SET);
	fwrite(&shadow_Inode, INODE_SIZE, 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&shadow_Inode.lock);

	pthread_rwlock_wrlock(&group_Inode.lock);
	if (group_buf[strlen(group_buf) - 2] == ':')
		sprintf(group_buf + strlen(group_buf) - 1, "%s\n", username);
	else
		sprintf(group_buf + strlen(group_buf) - 1, ",%s\n", username);
	group_Inode.fileSize = strlen(group_buf);
	fseek(fw, group_Inode_Addr, SEEK_SET);
	fwrite(&group_Inode, INODE_SIZE, 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&group_Inode.lock);



	strcpy_s(Cur_User_Name, bak_Cur_User_Name);
	strcpy_s(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
	Cur_Dir_Addr = bak_Cur_Dir_Addr;
	strcpy_s(Cur_Dir_Name, bak_Cur_Dir_Name);

	cout << "Add user success." << endl;
	return true;
}

//didn't test yet
static bool userdel(char username[])
{
	if (strcmp(Cur_User_Name, "root") != 0) {
		cout << "Permission denied." << endl;
		return false;
	}
	if (strcmp(username, "root") == 0) {
		cout << "Unable to delete root user." << endl;
		return false;
	}
	int passwd_Inode_Addr = -1;	//passwd inode address
	int shadow_Inode_Addr = -1;	//shadow inode address
	int group_Inode_Addr = -1;	//group inode address 
	inode passwd_Inode = { 0 };		//passwd inode
	inode shadow_Inode = { 0 };		//shadow inode
	inode group_Inode = { 0 };		//group inode

	char bak_Cur_User_Name[110];
	char bak_Cur_User_Name_2[110];
	char bak_Cur_User_Dir_Name[310];
	int bak_Cur_Dir_Addr;
	char bak_Cur_Dir_Name[310];
	char bak_Cur_Group_Name[310];

	char passwd_buf[MAX_FILE_SIZE];
	char shadow_buf[MAX_FILE_SIZE];
	char group_buf[MAX_FILE_SIZE];



	inode cur = { 0 };
	strcpy_s(bak_Cur_User_Name, Cur_User_Name);
	strcpy_s(bak_Cur_User_Dir_Name, Cur_User_Dir_Name);
	bak_Cur_Dir_Addr = Cur_Dir_Addr;
	strcpy_s(bak_Cur_Dir_Name, Cur_Dir_Name);

	cd("/home");
	strcpy_s(bak_Cur_User_Name_2, Cur_User_Name);
	strcpy_s(bak_Cur_Group_Name, Cur_Group_Name);
	strcpy_s(Cur_User_Name, username);
	strcpy_s(Cur_Group_Name, "user");
	if (!mkdir(username)) {
		strcpy_s(Cur_User_Name, bak_Cur_User_Name_2);
		strcpy_s(Cur_Group_Name, bak_Cur_Group_Name);
		strcpy_s(Cur_User_Name, bak_Cur_User_Name);
		strcpy_s(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy_s(Cur_Dir_Name, bak_Cur_Dir_Name);

		cout << "Add user failure." << endl;
		return false;
	}
	strcpy_s(Cur_User_Name, bak_Cur_User_Name_2);
	strcpy_s(Cur_Group_Name, bak_Cur_Group_Name);

	cd("/etc");
	char passwd[100] = { 0 };
	inputPassword(passwd);
	fseek(fr, Cur_Dir_Addr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);

	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, "passwd") == 0) {
			passwd_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&passwd_Inode, sizeof(inode), 1, fr);
			strcpy_s(passwd_buf, fileEnt[i].buffer);
		}
		if (strcmp(fileEnt[i].dir.name, "shadow") == 0) {
			shadow_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&shadow_Inode, sizeof(inode), 1, fr);
			strcpy_s(shadow_buf, fileEnt[i].buffer);
		}
		if (strcmp(fileEnt[i].dir.name, "group") == 0) {
			group_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&group_Inode, sizeof(inode), 1, fr);
			strcpy_s(group_buf, fileEnt[i].buffer);
		}
	}
	fflush(fr);

	if (strstr(passwd_buf, username) != nullptr) {
		cout << "The user has already existed." << endl;
		strcpy_s(Cur_User_Name, bak_Cur_User_Name);
		strcpy_s(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy_s(Cur_Dir_Name, bak_Cur_Dir_Name);
		return false;
	}

	//update passwd infomation
	pthread_rwlock_wrlock(&passwd_Inode.lock);
	char* p1 = strstr(passwd_buf, username);
	*p1 = '\0';
	while ((*p1) != '\n')
		p1++;
	p1++;
	strcat_s(passwd_buf, p1);
	passwd_Inode.fileSize = strlen(passwd_buf);
	fseek(fw, passwd_Inode_Addr, SEEK_SET);
	fwrite(&passwd_Inode, INODE_SIZE, 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&passwd_Inode.lock);

	pthread_rwlock_wrlock(&shadow_Inode.lock);
	char* p2 = strstr(shadow_buf, username);
	*p2 = '\0';
	while ((*p2) != '\n')
		p2++;
	p2++;
	shadow_Inode.fileSize = strlen(shadow_buf);
	fseek(fw, shadow_Inode_Addr, SEEK_SET);
	fwrite(&shadow_Inode, INODE_SIZE, 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&shadow_Inode.lock);

	pthread_rwlock_wrlock(&group_Inode.lock);
	char* p3 = strstr(group_buf, username);
	*p3 = '\0';
	while ((*p3) != '\n' && (*p3) != ',')
		p3++;
	if ((*p3) == ',')
		p3++;
	strcat_s(group_buf, p3);
	group_Inode.fileSize = strlen(group_buf);
	fseek(fw, group_Inode_Addr, SEEK_SET);
	fwrite(&group_Inode, INODE_SIZE, 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&group_Inode.lock);

	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, "passwd") == 0) {
			strcpy_s(fileEnt[i].buffer, passwd_buf);
		}
		if (strcmp(fileEnt[i].dir.name, "shadow") == 0) {
			strcpy_s(fileEnt[i].buffer, shadow_buf);
		}
		if (strcmp(fileEnt[i].dir.name, "group") == 0) {
			strcpy_s(fileEnt[i].buffer, group_buf);
		}
	}
	fseek(fw, cur.dirBlock[0], SEEK_SET);
	fwrite(fileEnt, sizeof(fileEnt), 1, fw);
	fflush(fw);


	char* tmp = strcat("/home/", username);
	if (strcmp(Cur_Dir_Name, tmp) == 0) {
		cd("/");
		rm(tmp);
	}
	else
		rm(tmp);

	cout << "User has deleted." << endl;
	return true;
}

//have bug in parse .. ../ below function
//didn't support relative path
//chmod finish 
static void chmod(char name[], int pmode)
{
	if (strlen(name) > MAX_FILE_SIZE) {
		cout << "Name is too long" << endl;
		return;
	}
	char tmpName[1024], fileName[1024];
	int parinoaddr;
	int address = 0;
	inode fileinode = { 0 };
	if (strcmp(name, "") == 0||strcmp(name,".")==0) {
		strcpy(fileName, Cur_Dir_Name);
		parinoaddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy(fileName, Cur_Dir_Name);
		parinoaddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(fileName, extractLastPath(name));
		parinoaddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(fileName, extractLastPath(name));
		parinoaddr = extractPath(tmpName);
	}
	if (parinoaddr == -1) {
		cout << "No such file or dictionary." << endl;
		return;
	}
	inode cur = { 0 };
	fseek(fr, parinoaddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);
	int filemode;
	if (strcmp(Cur_User_Name, cur.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_Group_Name, cur.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	if (((cur.mode >> filemode >> 1) & 1) == 0) {
		cout << "Permission Denied" << endl;
		return;
	}

	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, fileName) == 0) {
			address = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&fileinode, sizeof(inode), 1, fr);
			fflush(fr);
			break;
		}
	}
	if (strcmp(fileinode.uname, Cur_User_Name) == 0) {
		pthread_rwlock_wrlock(&fileinode.lock);
		fileinode.mode = (fileinode.mode >> 9 << 9) | pmode;
		fseek(fw, address, SEEK_SET);
		fwrite(&fileinode, INODE_SIZE, 1, fw);
		fflush(fw);
		pthread_rwlock_unlock(&fileinode.lock);
		return;
	}
	else if (strcmp(fileinode.gname, Cur_Group_Name) == 0) {
		pthread_rwlock_wrlock(&fileinode.lock);
		fileinode.mode = (fileinode.mode >> 9 << 9) | pmode;
		fseek(fw, address, SEEK_SET);
		fwrite(&fileinode, INODE_SIZE, 1, fw);
		fflush(fw);
		pthread_rwlock_unlock(&fileinode.lock);
		return;
	}
	else {
		cout << "No Permission!" << endl;
		return;
	}
}

//touch finish
static void touch(char name[]) //ok
{
	int parinoAddr;
	char tmpName[1024], FileName[1024];
	if (strlen(name) > FILENAME_MAX) {
		cout << "File name too long." << endl;
		return;
	}
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0) {
		strcpy(FileName, Cur_Dir_Name);
		parinoAddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy(FileName, Cur_Dir_Name);
		parinoAddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(FileName, extractLastPath(name));
		parinoAddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(FileName, extractLastPath(name));
		parinoAddr = extractPath(tmpName);
	}
	if (parinoAddr == -1) {
		cout << "Can't touch file at Non-existent path." << endl;
		return;
	}
	inode cur = { 0 };
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);

	int filemode;
	if (strcmp(Cur_User_Name, cur.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_Group_Name, cur.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	if (((cur.mode >> filemode >> 1) & 1) == 0) {
		cout << "Permission Denied" << endl;
		return;
	}

	int newInodeAddress = InodeAlloc();
	if (newInodeAddress == -1) {
		cout << "No empty space." << endl;
		return;
	}
	else {
		int cnt = 1;
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (fileEnt[i].dir.iaddr == 0) {
				fileEnt[i].dir.iaddr = newInodeAddress;
				strcpy_s(fileEnt[i].dir.name, FileName);
				initFileEntLock(fileEnt[i]);
				break;
			}
			cnt = cnt + 1;
		}
		if (cnt == FILEENT_PER_BLOCK) {
			cout << "No empty space in that dir." << endl;
			return;
		}
		inode New = { 0 };
		New.inum = (newInodeAddress - Inode_StartAddr) / INODE_SIZE;
		New.dirBlock[0] = cur.dirBlock[0];
		for (int i = 1; i < IPB; i++) {
			New.dirBlock[i] = -1;
		}
		initLock(New);
		New.fileSize = 0;
		New.IdirBlock = -1;
		New.mode = MODE_FILE | FILE_DEF_PERMISSION;
		strcpy_s(New.uname, Cur_User_Name);
		strcpy_s(New.gname, Cur_Group_Name);
		New.atime = time(NULL);
		New.ctime = time(NULL);
		New.mtime = time(NULL);

		fseek(fw, newInodeAddress, SEEK_SET);
		fwrite(&New, INODE_SIZE, 1, fw);
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, INODE_SIZE, 1, fw);
		fseek(fw, cur.dirBlock[0], SEEK_SET);
		fwrite(fileEnt, sizeof(fileEnt), 1, fw);
		fflush(fw);
		return;
	}
}

//mkdir finish
static bool mkdir(char name[])  //ok
{
	char tmpName[1024], dirName[1024];
	int parinoAddr;
	if (strlen(name) > MAX_FILE_NAME) {
		cout << "The directory name is too long." << endl;
		return false;
	}
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0) {
		strcpy(dirName, Cur_Dir_Name);
		parinoAddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy(dirName, Cur_Dir_Name);
		parinoAddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(dirName, extractLastPath(name));
		parinoAddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(dirName, extractLastPath(name));
		parinoAddr = extractPath(tmpName);
	}
	if (parinoAddr == -1) {
		cout << "Can't mkdir at Non-existent path." << endl;
		return false;
	}
	inode cur = { 0 };
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt curFileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(curFileEnt, sizeof(curFileEnt), 1, fr);
	fflush(fr);
	int filemode;
	if (strcmp(Cur_User_Name, cur.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_Group_Name, cur.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	if (((cur.mode >> filemode >> 1) & 1) == 0) {
		cout << "Permission Denied" << endl;
		return false;
	}
	//new dirent
	int blockAddress = BlockAlloc();
	int newInodeAddress = InodeAlloc();
	if (blockAddress != -1 && newInodeAddress != -1) {
		Dirent dir = { 0 };
		strcpy_s(dir.name, dirName);
		dir.iaddr = newInodeAddress;
		inode New = { 0 };
		New.inum = (newInodeAddress - Inode_StartAddr) / INODE_SIZE;
		New.dirBlock[0] = blockAddress;
		for (int i = 1; i < IPB; i++) {
			New.dirBlock[i] = -1;
		}
		initLock(New);
		New.fileSize = 0;
		New.IdirBlock = -1;
		New.mode = MODE_DIR | DIR_DEF_PERMISSION;
		strcpy_s(New.uname, Cur_User_Name);
		strcpy_s(New.gname, Cur_Group_Name);
		New.atime = time(NULL);
		New.ctime = time(NULL);
		New.mtime = time(NULL);
		

		//check whehter there is same name file
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (strcmp(curFileEnt[i].dir.name, dirName) == 0) {
				cout << "File of this name is already exist!" << endl;
				return false;
			}
		}

		//update current dictionary information
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (strcmp(curFileEnt[i].dir.name, "") == 0) {
				curFileEnt[i].dir = dir;
				strcpy_s(curFileEnt[i].buffer, "");
				break;
			}
		}
		fseek(fw, cur.dirBlock[0], SEEK_SET);
		fwrite(curFileEnt, sizeof(curFileEnt), 1, fw);
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, INODE_SIZE, 1, fw);

		//create new dictionary information
		FileEnt newFileEnt[FILEENT_PER_BLOCK] = { 0 };
		newFileEnt[0].dir = dir;
		strcpy_s(newFileEnt[0].buffer, "");
		initFileEntLock(newFileEnt[0]);
		newFileEnt[1].dir = curFileEnt[0].dir;
		strcpy_s(newFileEnt[1].dir.name, "..");
		initFileEntLock(newFileEnt[1]);
		fseek(fw, New.dirBlock[0], SEEK_SET);
		fwrite(newFileEnt, sizeof(newFileEnt), 1, fw);
		fseek(fw, dir.iaddr, SEEK_SET);
		fwrite(&New, INODE_SIZE, 1, fw);

		fflush(fw);
		return true;
	}
	else {
		return false;
	}

}

//rm finish
static bool rm(char name[])
{
	char tmpName[1024], fileName[1024];
	int parinoaddr;
	int address = 0;
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0) {
		strcpy_s(fileName, Cur_Dir_Name);
		parinoaddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy_s(fileName, Cur_Dir_Name);
		parinoaddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(fileName, extractLastPath(name));
		parinoaddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(fileName, extractLastPath(name));
		parinoaddr = extractPath(tmpName);
	}
	if (parinoaddr == -1) {
		cout << "No such file or dictionary." << endl;
		return false;
	}
	inode empty = { 0 };
	FileEnt emptyfile = { 0 };
	FileEnt tmp = { 0 };
	inode fileinode = { 0 };
	//get current dir inode
	inode cur = { 0 };
	fseek(fr, parinoaddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);
	//get fileinode
	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, fileName) == 0) {
			address = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&fileinode, sizeof(inode), 1, fr);
			fflush(fr);
			break;
		}
	}
	int filemode;
	if (strcmp(Cur_User_Name, fileinode.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_Group_Name, fileinode.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	if (((fileinode.mode >> filemode >> 1) & 1) == 0) {
		cout << "Permission Denied" << endl;
		return false;
	}

	if (((fileinode.mode >> 9) & 1) == 0) { //file
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (strcmp(fileEnt[i].dir.name, fileName)==0) {
				fseek(fw, cur.dirBlock[0] + i * sizeof(FileEnt), SEEK_SET);
				fwrite(&emptyfile, sizeof(FileEnt), 1, fw);
				fseek(fw, address, SEEK_SET);
				fwrite(&empty, INODE_SIZE, 1, fw);
				fflush(fw);
				free_inode(superblock, imap, (address - Inode_StartAddr) / INODE_SIZE);
				break;
			}
		}
		return true;
	}
	else {						//dictionary
		int start = cur.dirBlock[0];
		int diraddress = 0;
		FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
		FileEnt emptydir[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, start, SEEK_SET);
		fread(fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (strcmp(fileEnt[i].dir.name, fileName) == 0) {
				diraddress = fileEnt[i].dir.iaddr;
				fseek(fw, fileEnt[i].dir.iaddr, SEEK_SET);
				fwrite(&empty, INODE_SIZE, 1, fw);
				fileEnt[i] = {0};
				free_inode(superblock, imap, (fileEnt[i].dir.iaddr - Inode_StartAddr) / INODE_SIZE);
			}
		}
		fseek(fw, diraddress, SEEK_SET);
		fwrite(&emptydir, sizeof(emptydir), 1, fw);
		fseek(fw, start, SEEK_SET);
		fwrite(&fileEnt, sizeof(fileEnt), 1, fw);
		fflush(fw);
		free_block(superblock, bmap, (start - Block_StartAddr) / BLOCK_SIZE);
		return true;
	}
	return false;
}

//ls finish
static void ls(char name[])
{
	int parinoaddr;
	char tmpName[1024], dirName[1024];
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0) {
		strcpy_s(dirName, Cur_Dir_Name);
		parinoaddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy_s(dirName, Cur_Dir_Name);
		parinoaddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(dirName, extractLastPath(name));
		parinoaddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(dirName, extractLastPath(name));
		parinoaddr = extractPath(tmpName);
	}
	if (parinoaddr == -1) {
		cout << "No such path." << endl;
		return;
	}
	//get current inode
	inode cur = { 0 };
	fseek(fr, parinoaddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);

	//get current fileEnt
	FileEnt curFileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(curFileEnt, sizeof(curFileEnt), 1, fr);
	fflush(fr);

	//set file mode:6-owner ,3-group,0-other
	int filemode;
	if (strcmp(Cur_User_Name, cur.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_Group_Name, cur.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	if (((cur.mode >> filemode >> 2) & 1) == 0) {
		cout << "Permission Denied" << endl;
		return;
	}

	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		Dirent dir = curFileEnt[i].dir;
		inode tmp = { 0 };
		fseek(fr, dir.iaddr, SEEK_SET);
		fread(&tmp, sizeof(inode), 1, fr);
		fflush(fr);
		if (strcmp(dir.name, "") == 0)
			continue;
		if (((tmp.mode >> 9) & 1) == 1) {
			printf("d");
		}
		else {
			printf("-");
		}

		//for time loaded
		tm* ptr;
		ptr = gmtime(&tmp.mtime);

		//file privilege information
		int t = 8;
		while (t >= 0) {
			if (((tmp.mode >> t) & 1) == 1) {
				if (t % 3 == 2)	printf("r");
				if (t % 3 == 1)	printf("w");
				if (t % 3 == 0)	printf("x");
			}
			else {
				printf("-");
			}
			t--;
		}
		printf(" ");

		printf("%s ", tmp.uname);
		printf("%s\t", tmp.gname);
		printf("%d B\t", tmp.fileSize);
		printf("%d.%d.%d %02d:%02d:%02d  ", 1900 + ptr->tm_year, ptr->tm_mon + 1, ptr->tm_mday, (8 + ptr->tm_hour) % 24, ptr->tm_min, ptr->tm_sec);
		if (i == 0)
			printf(".");
		else
			printf("%s", dir.name);	//filename
		printf("\n");
	}
}

//cd  privilege-need **
static void cd(char name[])
{
	if (strcmp(name, "") == 0) {
		cout << "The argument can't be empty." << endl;
		return;
	}
	if (strcmp(name, ".") == 0) {
		return;
	}
	if (strcmp(name, "/") == 0) {
		Cur_Dir_Addr = Root_Dir_Addr;
		strcpy(Cur_Dir_Name, "/");
		return;
	}
	if (strcmp(name, "~") == 0) {
		strcpy(name, Cur_User_Dir_Name);
	}
	char tmpName[1024], dirName[1024];
	strcpy_s(tmpName, name);
	strcpy_s(dirName, extractLastPath(name));
	strcpy_s(tmpName, substring(tmpName, dirName));
	int parinoaddr = extractPath(tmpName);

	if (parinoaddr != -1) {
		inode cur = { 0 };
		FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, parinoaddr, SEEK_SET);
		fread(&cur, sizeof(inode), 1, fr);
		fflush(fr);
		if (((cur.mode >> 9) & 1) == 0) {
			cout << "Please input dir instead of file as argument";
			return;
		}
		fseek(fr, cur.dirBlock[0], SEEK_SET);
		fread(fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (strcmp(dirName, fileEnt[i].dir.name) == 0) {
				if (strcmp(tmpName, "/"))
					strcpy_s(Cur_Dir_Name, strcat(tmpName, "/"));
				else
					strcpy_s(Cur_Dir_Name, tmpName);
				strcat(Cur_Dir_Name, fileEnt[i].dir.name);
				Cur_Dir_Addr = fileEnt[i].dir.iaddr;
				return;
			}
		}
	}
	else {
		printf("cd %s : No such file or directory.\n", name);
		return;
	}
}

//vim finish
static void vim(char name[], char buf[])
{
	memset(buf, 0, sizeof(buf));
	int maxlen = 0;
	int position = 0;
	bool isExist = false;
	int fileInodeAddr;
	char tmpName[30], FileName[30];
	int parinoAddr;
	if (strcmp(name, "") == 0) {
		strcpy_s(FileName, Cur_Dir_Name);
		parinoAddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy_s(FileName, Cur_Dir_Name);
		parinoAddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(FileName, extractLastPath(name));
		parinoAddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(FileName, extractLastPath(name));
		parinoAddr = extractPath(tmpName);
	}
	if (parinoAddr == -1) {
		cout << "Can't do operation at Non-existent path." << endl;
		return;
	}
	inode cur = { 0 };
	inode fileinode = { 0 };
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	int filemode;
	if (strcmp(Cur_User_Name, cur.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_User_Name, cur.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);
	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, FileName) == 0) {
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&fileinode, sizeof(inode), 1, fr);
			fflush(fr);
			if (((fileinode.mode >> 9) & 1) == 0) {	//find the file ,go into edit mode
				pthread_rwlock_wrlock(&fileEnt[i].lock);
				fileInodeAddr = fileEnt[i].dir.iaddr;
				position = i;
				isExist = true;
				goto label;
			}
		}
	}

label:

	int cnt = 0;
	system("cls");

	int winx, winy, curx, cury;

	HANDLE handle_out;                        
	CONSOLE_SCREEN_BUFFER_INFO screen_info;        
	COORD pos = { 0, 0 };                         

	if (isExist) {	//file exist got into edit mode

		//privilege check
		if (((fileinode.mode >> filemode >> 2) & 1) == 0) {
			cout << "Permission denied." << endl;
			return;
		}

		printf("%s", fileEnt[position].buffer);
		maxlen = strlen(fileEnt[position].buffer);
	}

	handle_out = GetStdHandle(STD_OUTPUT_HANDLE);    
	GetConsoleScreenBufferInfo(handle_out, &screen_info);   
	winx = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
	winy = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
	curx = screen_info.dwCursorPosition.X;
	cury = screen_info.dwCursorPosition.Y;


	int mode = 0;	
	unsigned char c;
	while (1) {
		if (mode == 0) {	
			c = _getch();

			if (c == 'i' || c == 'a') {	
				if (c == 'a') {
					curx++;
					if (curx == winx) {
						curx = 0;
						cury++;
					}
				}

				if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
					if (cury % (winy - 1) == winy - 2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes);
					int i;
					for (i = 0; i < winx - 1; i++)
						printf(" ");
					Gotoxy(handle_out, 0, cury + 1);

					printf("-- INSERT --");
					Gotoxy(handle_out, 0, cury);
				}
				else {
					Gotoxy(handle_out, 0, winy - 1);
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); 
					int i;
					for (i = 0; i < winx - 1; i++)
						printf(" ");
					Gotoxy(handle_out, 0, winy - 1);
					printf("-- INSERT --");
					Gotoxy(handle_out, curx, cury);
				}

				Gotoxy(handle_out, curx, cury);
				mode = 1;


			}
			else if (c == ':') {
				if (cury - winy + 2 > 0)
					Gotoxy(handle_out, 0, cury + 1);
				else
					Gotoxy(handle_out, 0, winy - 1);
				_COORD pos;
				if (cury - winy + 2 > 0)
					pos.X = 0, pos.Y = cury + 1;
				else
					pos.X = 0, pos.Y = winy - 1;
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); 
				int i;
				for (i = 0; i < winx - 1; i++)
					printf(" ");

				if (cury - winy + 2 > 0)
					Gotoxy(handle_out, 0, cury + 1);
				else
					Gotoxy(handle_out, 0, winy - 1);
				printf(":");

				char pc;
				int tcnt = 1;	
				while (c = _getch()) {
					if (c == '\r') {
						break;
					}
					else if (c == '\b') {
						tcnt--;
						if (tcnt == 0)
							break;
						printf("\b");
						printf(" ");
						printf("\b");
						continue;
					}
					pc = c;
					printf("%c", pc);
					tcnt++;
				}
				if (pc == 'q') {
					buf[cnt] = '\0';
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); 
					system("cls");
					break;
				}
				else {
					if (cury - winy + 2 > 0)
						Gotoxy(handle_out, 0, cury + 1);
					else
						Gotoxy(handle_out, 0, winy - 1);
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); 
					int i;
					for (i = 0; i < winx - 1; i++)
						printf(" ");

					if (cury - winy + 2 > 0)
						Gotoxy(handle_out, 0, cury + 1);
					else
						Gotoxy(handle_out, 0, winy - 1);
					printf("--  Command Error --");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); 
					Gotoxy(handle_out, curx, cury);
				}
			}
			else if (c == 27) {	//ESC,shift to command mode
				Gotoxy(handle_out, 0, winy - 1);
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes);
				int i;
				for (i = 0; i < winx - 1; i++)
					printf(" ");
				Gotoxy(handle_out, curx, cury);

			}

		}
		else if (mode == 1) {

			Gotoxy(handle_out, winx / 4 * 3, winy - 1);
			int i = winx / 4 * 3;
			while (i < winx - 1) {
				printf(" ");
				i++;
			}
			if (cury > winy - 2)
				Gotoxy(handle_out, winx / 4 * 3, cury + 1);
			else
				Gotoxy(handle_out, winx / 4 * 3, winy - 1);
			printf("[Row: %d, Column: %d]", curx == -1 ? 0 : curx, cury);
			Gotoxy(handle_out, curx, cury);

			c = _getch();
			if (c == 27) {
				mode = 0;
				Gotoxy(handle_out, 0, winy - 1);
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes);
				int i;
				for (i = 0; i < winx - 1; i++)
					printf(" ");
				continue;
			}
			else if (c == '\b') {
				if (cnt == 0)
					continue;
				printf("\b");
				printf(" ");
				printf("\b");
				curx--;
				cnt--;
				if (buf[cnt] == '\n') {
					if (cury != 0)
						cury--;
					int k;
					curx = 0;
					for (k = cnt - 1; buf[k] != '\n' && k >= 0; k--)
						curx++;
					Gotoxy(handle_out, curx, cury);
					printf(" ");
					Gotoxy(handle_out, curx, cury);
					if (cury - winy + 2 >= 0) {
						Gotoxy(handle_out, curx, 0);
						Gotoxy(handle_out, curx, cury + 1);
						SetConsoleTextAttribute(handle_out, screen_info.wAttributes);
						int i;
						for (i = 0; i < winx - 1; i++)
							printf(" ");
						Gotoxy(handle_out, 0, cury + 1);
						printf("-- INSERT --");

					}
					Gotoxy(handle_out, curx, cury);

				}
				else
					buf[cnt] = ' ';
				continue;
			}
			else if (c == 224) {
				c = _getch();
				if (c == 75) {
					if (cnt != 0) {
						cnt--;
						curx--;
						if (buf[cnt] == '\n') {
							if (cury != 0)
								cury--;
							int k;
							curx = 0;
							for (k = cnt - 1; buf[k] != '\n' && k >= 0; k--)
								curx++;
						}
						Gotoxy(handle_out, curx, cury);
					}
				}
				else if (c == 77) {	
					cnt++;
					if (cnt > maxlen)
						maxlen = cnt;
					curx++;
					if (curx == winx) {
						curx = 0;
						cury++;

						if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
							if (cury % (winy - 1) == winy - 2)
								printf("\n");
							SetConsoleTextAttribute(handle_out, screen_info.wAttributes);
							int i;
							for (i = 0; i < winx - 1; i++)
								printf(" ");
							Gotoxy(handle_out, 0, cury + 1);
							printf("-- INSERT --");
							Gotoxy(handle_out, 0, cury);
						}

					}
					Gotoxy(handle_out, curx, cury);
				}
				continue;
			}
			if (c == '\r') {
				printf("\n");
				curx = 0;
				cury++;

				if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
					if (cury % (winy - 1) == winy - 2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); 
					int i;
					for (i = 0; i < winx - 1; i++)
						printf(" ");
					Gotoxy(handle_out, 0, cury + 1);
					printf("-- INSERT --");
					Gotoxy(handle_out, 0, cury);
				}

				buf[cnt++] = '\n';
				if (cnt > maxlen)
					maxlen = cnt;
				continue;
			}
			else {
				printf("%c", c);
			}
			curx++;
			if (curx == winx) {
				curx = 0;
				cury++;

				if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
					if (cury % (winy - 1) == winy - 2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes);
					int i;
					for (i = 0; i < winx - 1; i++)
						printf(" ");
					Gotoxy(handle_out, 0, cury + 1);
					printf("-- INSERT --");
					Gotoxy(handle_out, 0, cury);
				}

				buf[cnt++] = '\n';
				if (cnt > maxlen)
					maxlen = cnt;
				if (cury == winy) {
					printf("\n");
				}
			}
			buf[cnt++] = c;
			if (cnt > maxlen)
				maxlen = cnt;
		}
		else {
		}
	}
	if (isExist) {
		if (((fileinode.mode >> filemode >> 1) & 1) == 1) {	
			strcpy_s(fileEnt[position].buffer, buf);
			fileinode.fileSize = strlen(buf);
			fileinode.mtime = time(NULL);
			fseek(fw, fileEnt[position].dir.iaddr, SEEK_SET);
			fwrite(&fileinode, sizeof(fileinode), 1, fw);
			fseek(fw, cur.dirBlock[0], SEEK_SET);
			fwrite(fileEnt, sizeof(fileEnt), 1, fw);
			fseek(fw, parinoAddr, SEEK_SET);
			fwrite(&cur, sizeof(inode), 1, fw);
			fflush(fw);
			pthread_rwlock_unlock(&fileEnt[position].lock);
		}
		else {	
			cout << "Permission denied." << endl;
			pthread_rwlock_unlock(&fileEnt[position].lock);
		}

	}
	else {	
		if (((cur.mode >> filemode >> 1) & 1) == 1) {
			create(name, buf);
		}
		else {
			cout << "Permission denied." << endl;
			return;
		}
	}
}


//clear finish
static void clear()
{
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	system("cls");
#else
	system("clear");
#endif
}

//clear finish
static void pwd()
{
	printf("%s\n", Cur_Dir_Name);
}

//cat finish
static void cat(char name[])
{
	int parinoaddr;
	char tmpName[1024], fileName[1024];
	if (strcmp(name, "") == 0||strcmp(name,".")==0) {
		strcpy_s(fileName, Cur_Dir_Name);
		parinoaddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		strcpy_s(fileName, Cur_Dir_Name);
		parinoaddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(fileName, extractLastPath(name));
		parinoaddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(fileName, extractLastPath(name));
		parinoaddr = extractPath(tmpName);
	}
	if (parinoaddr == -1) {
		cout << "No such file" << endl;
		return;
	}
	inode cur = { 0 };
	fseek(fr, parinoaddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	pthread_rwlock_rdlock(&(cur.lock));

	int filemode;
	if (strcmp(Cur_User_Name, cur.uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_Group_Name, cur.gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	if (((cur.mode >> filemode >> 2) & 1) == 0) {
		cout << "Permission Denied" << endl;
		pthread_rwlock_unlock(&(cur.lock));
		return;
	}

	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);
	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, fileName) == 0) {
			pthread_rwlock_wrlock(&fileEnt[i].lock);
			printf("%s\n", fileEnt[i].buffer);
			pthread_rwlock_unlock(&fileEnt[i].lock);
			break;
		}
	}
	pthread_rwlock_unlock(&(cur.lock));
	return;
}


//cmd finish
static void cmd(char cmdline[])
{
	char inputCommand[100] = { 0 };
	char inputParameter[100] = { 0 };
	char inputParameter2[100] = { 0 };
	char buf[MAX_FILE_SIZE] = { 0 };	//最大10K
	int tmp = 0;
	int i = 0;
	sscanf(cmdline, "%s%s%s", inputCommand, inputParameter, inputParameter2);

	if (isLogin == false) {
		// non-login type command
		if (strcmp(inputCommand, "login") == 0) {
			if (strcmp(inputParameter, "--help") == 0) {
				Login_help();
			}
			else {
				login(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "help") == 0)
			Help_NotLogin();
		else if (strcmp(inputCommand, "clear") == 0)
			clear();
		else if (strcmp(inputCommand, "exit") == 0)
			exit(0);
		else if (strcmp(inputCommand, "") == 0)
			return;
		else {
			printf("%s: command not found...\n", inputCommand);
			cout << "If you need help, please type 'help'." << endl;
		}
	}
	else {
		//login-type command
		if (strcmp(inputCommand, "login") == 0) {
			if (strcmp(inputParameter, "--help") == 0) {
				Login_help();
			}
			else {
				login(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "logout") == 0) {
			if (strcmp(inputCommand, "--help") == 0) {
				Logout_help();
			}
			else {
				logout();
			}
		}
		else if (strcmp(inputCommand, "useradd") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Useradd_help();
			}
			else {
				useradd(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "userdel") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Userdel_help();
			}
			else {
				userdel(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "cat") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Cat_help();
			}
			else {
				cat(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "touch") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Touch_help();
			}
			else {
				touch(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "rm") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Rm_help();
			}
			else {
				rm(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "vim") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Vim_help();
			}
			else {
				vim(inputParameter, buf);
			}
		}
		else if (strcmp(inputCommand, "chmod") == 0) {
			if (strcmp(inputParameter, "--help") == 0) {
				Chmod_help();
			}
			else if (strcmp(inputParameter, "") == 0 || strcmp(inputParameter2, "") == 0) {
				CommandError(inputCommand);
			}
			else {
				tmp = 0;
				for (size_t i = 0; inputParameter2[i]; i++)
					tmp = tmp * 8 + inputParameter2[i] - '0';
				chmod(inputParameter, tmp);
			}
		}
		else if (strcmp(inputCommand, "ls") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				ls(".");
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Ls_help();
			}
			else
			{
				ls(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "cd") == 0) {
			if (strcmp(inputParameter, "--help") == 0) {
				Cd_help();
			}
			else {
				cd(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "pwd") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				pwd();
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Pwd_help();
			}
			else {
				CommandError(inputCommand);
			}
		}
		else if (strcmp(inputCommand, "mkdir") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Mkdir_help();
			}
			else {
				mkdir(inputParameter);
			}
		}
		else if (strcmp(inputCommand, "help") == 0)
			help();
		else if (strcmp(inputCommand, "clear") == 0)
			clear();
		else if (strcmp(inputCommand, "exit") == 0)
			exit(0);
		else if (strcmp(inputCommand, "") == 0)
			return;
		else {
			printf("%s: command not found...\n", inputCommand);
			cout << "If you need help, please type 'help'." << endl;
		}
	}
}

//inputUsername finish
static void inputUsername(char username[])
{
	printf("username: ");
	scanf("%s", username);	//Username
}


//inputPassword finish
static void inputPassword(char passwd[])
{
	int len = 0;
	char c;
	fflush(stdin);	//clean input buffer
	printf("password: ");
	while (c = getchar()) {
		if (c == '\n') {
			passwd[len] = '\0';
			fflush(stdin);
			break;
		}
		else if (c == '\b') {
			if (len != 0) {
				len--;
			}
		}
		else {	//密码字符
			passwd[len++] = c;
		}
	}
}


//Gotoxy finish
static void Gotoxy(HANDLE hOut, int x, int y)
{
	COORD pos;
	pos.X = x;
	pos.Y = y;
	SetConsoleCursorPosition(hOut, pos);
}

//check finish
static bool check(char username[], char passwd[]) //todebug
{
	int passwd_Inode_Addr = -1;	//passwd inode address
	int shadow_Inode_Addr = -1;	//shadow inode address
	int group_Inode_Addr = -1;	//group inode address 
	inode passwd_Inode = { 0 };		//passwd inode
	inode shadow_Inode = { 0 };		//shadow inode
	inode group_Inode = { 0 };		//group inode

	char passwd_buf[256] = { 0 };
	char shadow_buf[256] = { 0 };
	char group_buf[256] = { 0 };
	char tmpbuf[256] = { 0 };


	inode cur = { 0 };

	cd("/etc");
	fseek(fr, Cur_Dir_Addr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
	fflush(fr);
	FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
	fseek(fr, cur.dirBlock[0], SEEK_SET);
	fread(fileEnt, sizeof(fileEnt), 1, fr);
	fflush(fr);

	for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
		if (strcmp(fileEnt[i].dir.name, "passwd") == 0) {
			passwd_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&passwd_Inode, sizeof(inode), 1, fr);
			strcpy_s(passwd_buf, fileEnt[i].buffer);
		}
		if (strcmp(fileEnt[i].dir.name, "shadow") == 0) {
			shadow_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&shadow_Inode, sizeof(inode), 1, fr);
			strcpy_s(shadow_buf, fileEnt[i].buffer);
		}
		if (strcmp(fileEnt[i].dir.name, "group") == 0) {
			group_Inode_Addr = fileEnt[i].dir.iaddr;
			fseek(fr, fileEnt[i].dir.iaddr, SEEK_SET);
			fread(&group_Inode, sizeof(inode), 1, fr);
			strcpy_s(group_buf, fileEnt[i].buffer);
		}
	}
	fflush(fr);

	if (strstr(passwd_buf, username) == nullptr) {
		cout << "User does not exist." << endl;
		cd("/");
		return false;
	}

	char* p = { 0 };
	if ((p = strstr(shadow_buf, username)) == nullptr) {
		cout << "The user does not exist in the shadow file." << endl;
		cd("/");	//回到根目录
		return false;
	}
	while ((*p) != ':') {
		p++;
	}
	p++;
	int j = 0;
	while ((*p) != '\n') {
		tmpbuf[j++] = *p;
		p++;
	}
	tmpbuf[j] = '\0';
	if (strcmp(tmpbuf, passwd) == 0) {	//密码正确，登陆
		strcpy_s(Cur_User_Name, username);
		if (strcmp(username, "root") == 0)
			strcpy_s(Cur_Group_Name, "root");	//当前登陆用户组名
		else
			strcpy_s(Cur_Group_Name, "user");	//当前登陆用户组名
		char tmp[25];
		strcpy_s(tmp, "/home/");
		strcat_s(tmp, username);
		cd(tmp);
		strcpy_s(Cur_User_Dir_Name, Cur_Dir_Name);	//复制当前登陆用户目录名
		return true;
	}
	else {
		cout << "Password is not correct." << endl;
		cd("/");	//回到根目录
		return false;
	}
}

//gotoRoot finish
static void gotoRoot()
{
	memset(Cur_User_Name, 0, sizeof(Cur_User_Name));		//clear current username
	memset(Cur_User_Dir_Name, 0, sizeof(Cur_User_Dir_Name));	//clear current User Dic name
	Cur_Dir_Addr = Root_Dir_Addr;	//set current dir address as root dir address
	strcpy_s(Cur_Dir_Name, "/");		//set current dir as "/"
}

//create finish
static bool create(char name[], char buf[])
{
	if (strlen(name) > FILENAME_MAX) {
		cout << "File name too long." << endl;
		return false;
	}
	int parinoAddr;
	char tmpName[30], FileName[30];
	if (strcmp(name, "") == 0) {
		parinoAddr = Cur_Dir_Addr;
	}
	else if (strcmp(name, "/") == 0) {
		parinoAddr = Root_Dir_Addr;
	}
	else if (strstr(name, "/") == nullptr) {
		strcpy_s(FileName, extractLastPath(name));
		parinoAddr = Cur_Dir_Addr;
	}
	else {
		strcpy_s(tmpName, name);
		strcpy_s(FileName, extractLastPath(name));
		parinoAddr = extractPath(tmpName);
	}
	if (parinoAddr == -1) {
		cout << "Can't touch file at Non-existent path." << endl;
		return false;
	}

	int newInodeAddress = InodeAlloc();
	if (newInodeAddress == -1) {
		cout << "No empty space." << endl;
		return false;
	}
	else {
		inode cur = { 0 };
		fseek(fr, parinoAddr, SEEK_SET);
		fread(&cur, sizeof(inode), 1, fr);
		fflush(fr);
		FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, cur.dirBlock[0], SEEK_SET);
		fread(fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
		int cnt = 1;
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (fileEnt[i].dir.iaddr == 0) {
				fileEnt[i].dir.iaddr = newInodeAddress;
				strcpy_s(fileEnt[i].dir.name, FileName);
				strcpy_s(fileEnt[i].buffer, buf);
				break;
			}
			cnt = cnt + 1;
		}
		if (cnt == FILEENT_PER_BLOCK) {
			cout << "No empty space in that dir." << endl;
			return false;
		}
		inode New = { 0 };
		New.inum = (newInodeAddress - Inode_StartAddr) / INODE_SIZE;
		New.dirBlock[0] = cur.dirBlock[0];
		for (int i = 1; i < IPB; i++) {
			New.dirBlock[i] = -1;
		}
		initLock(New);
		New.fileSize = strlen(buf);
		New.IdirBlock = -1;
		New.mode = MODE_FILE | FILE_DEF_PERMISSION;
		strcpy_s(New.uname, Cur_User_Name);
		strcpy_s(New.gname, Cur_Group_Name);
		New.atime = time(NULL);
		New.ctime = time(NULL);
		New.mtime = time(NULL);

		fseek(fw, newInodeAddress, SEEK_SET);
		fwrite(&New, INODE_SIZE, 1, fw);
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, INODE_SIZE, 1, fw);
		fseek(fw, cur.dirBlock[0], SEEK_SET);
		fwrite(fileEnt, sizeof(fileEnt), 1, fw);
		fflush(fw);
		return true;
	}
}


static void Help_NotLogin() {
	Command_help();
	Login_help();
	Clear_help();
	Exit_help();
}

//help finish
static void help()
{
	Command_help();

	Useradd_help();
	Userdel_help();

	Login_help();
	Logout_help();

	Cat_help();
	Touch_help();
	Rm_help();
	Vim_help();
	Chmod_help();

	Ls_help();
	Cd_help();
	Pwd_help();
	Mkdir_help();
	Rmdir_help();

	Clear_help();
	Exit_help();
}


static void Command_help() {
	cout << "Command format : 'command [Necessary parameter] ([Unnecessary parameter])'" << endl;
}
static void Useradd_help() {
	cout << "useradd [username] : Add user" << endl;
}
static void Userdel_help() {
	cout << "userdel [username] : Delete users" << endl;
}
static void Login_help() {
	cout << "login ([username]): Login VSFS" << endl;
}
static void Logout_help() {
	cout << "logout : Exit the current user" << endl;
}
static void Cat_help() {
	cout << "cat [filename] : Output file content" << endl;
}
static void Rm_help() {
	cout << "rm [filename] : Remove file" << endl;
}
static void Mkdir_help() {
	cout << "mkdir [directoryName] : Create subdirectory" << endl;
}
static void Rmdir_help() {
	cout << "rmdir [directoryName] : Delete subdirectory" << endl;
}
static void Cd_help() {
	cout << "cd [directoryName] : Change current directory" << endl;
}
static void Ls_help() {
	cout << "ls : List the file directory" << endl;
}
static void Pwd_help() {
	cout << "pwd : List the current directory name" << endl;
}
static void Chmod_help() {
	cout << "chmod [filename] [permissions] : Change the file permissions" << endl;
}
static void Clear_help() {
	cout << "clear : Clear the terminal" << endl;
}
static void Exit_help() {
	cout << "exit : Exit the VSFS" << endl;
}
static void Touch_help() {
	cout << "touch [filename] : Create a new empty file" << endl;
}
static void Vim_help() {
	cout << "vim [filename] : Remove file" << endl;
}
static void CommandError(char command[])
{
	printf("%s: missing operand\n", command);
	printf("Try '%s --help' for more information.\n", command);
}