#include "user.h"
#include "fs.h"
#include "bitmap.h"
#include <conio.h>
#include <iostream>
using namespace std;


void InitUser()
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
	strcat(s_passwd, password1);
	strcat(s_passwd, "\n");

	sprintf(buf, s_passwd);

	create("/etc/shadow", buf);
	chmod("/etc/shadow", 0660);

	sprintf(buf, "root::0:root\n");
	sprintf(buf + strlen(buf), "user::1:\n");
	create("/etc/group", buf);

	cd("/");
}

bool login(char username[])
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

	// 用户名
	if (strcmp(username, "") == 0) {
		inputUsername(username);
		// 判空
		if (strcmp(username, "") == 0) {
			cout << "Username can not be empty." << endl;
			return false;
		}
	}

	//输入用户密码
	inputPassword(password);
	// 判空
	if (strcmp(password, "") == 0) {
		cout << "Password can not be empty." << endl;
		return false;
	}

	if (check(username, password)) {	//核对用户名和密码
		isLogin = true;
		return true;
	}
	else {
		isLogin = false;
		return false;
	}
}


void logout()
{
	if (isLogin == false) {
		cout << "You have not logged in yet!" << endl;
		return;
	}

	gotoRoot();
	isLogin = false;
}

bool useradd(char username[])
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

	char passwd_buf[MAX_FILE_SIZE];
	char shadow_buf[MAX_FILE_SIZE];
	char group_buf[MAX_FILE_SIZE];



	inode cur = { 0 };
	strcpy(bak_Cur_User_Name, Cur_User_Name);
	strcpy(bak_Cur_User_Dir_Name, Cur_User_Dir_Name);
	bak_Cur_Dir_Addr = Cur_Dir_Addr;
	strcpy(bak_Cur_Dir_Name, Cur_Dir_Name);

	cd("/home");
	strcpy(bak_Cur_User_Name_2, Cur_User_Name);
	strcpy(bak_Cur_Group_Name, Cur_Group_Name);
	strcpy(Cur_User_Name, username);
	strcpy(Cur_Group_Name, "user");
	if (!mkdir(username)) {
		strcpy(Cur_User_Name, bak_Cur_User_Name_2);
		strcpy(Cur_Group_Name, bak_Cur_Group_Name);
		//恢复现场，回到原来的目录
		strcpy(Cur_User_Name, bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name, bak_Cur_Dir_Name);

		cout << "Add user failure." << endl;
		return false;
	}
	strcpy(Cur_User_Name, bak_Cur_User_Name_2);
	strcpy(Cur_Group_Name, bak_Cur_Group_Name);

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
	
	if (strstr(passwd_buf, username)!=NULL) {
		cout << "The user has already existed." << endl;

		strcpy(Cur_User_Name, bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name, bak_Cur_Dir_Name);
		return false;
	}
	pthread_rwlock_wrlock(&passwd_Inode.lock);
	sprintf(passwd_buf+strlen(passwd_buf), "%s:x:%d:%d\n", username, nextUID++, 1);
	passwd_Inode.fileSize = strlen(passwd_buf);
	fseek(fw, passwd_Inode_Addr, SEEK_SET);
	fwrite(&passwd_Inode, sizeof(inode), 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&passwd_Inode.lock);

	pthread_rwlock_wrlock(&shadow_Inode.lock);
	sprintf(shadow_buf+strlen(shadow_buf), "%s:%s\n", username, passwd);
	shadow_Inode.fileSize = strlen(shadow_buf);
	fseek(fw, shadow_Inode_Addr, SEEK_SET);
	fwrite(&shadow_Inode, sizeof(inode), 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&shadow_Inode.lock);

	pthread_rwlock_wrlock(&group_Inode.lock);
	if(group_buf[strlen(group_buf)-2] == ':')
		sprintf(group_buf + strlen(group_buf) - 1, "%s\n", username);
	else
		sprintf(group_buf + strlen(group_buf) - 1, ",%s\n", username);
	group_Inode.fileSize = strlen(group_buf);
	fseek(fw, group_Inode_Addr, SEEK_SET);
	fwrite(&group_Inode, sizeof(inode), 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&group_Inode.lock);



	strcpy(Cur_User_Name, bak_Cur_User_Name);
	strcpy(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
	Cur_Dir_Addr = bak_Cur_Dir_Addr;
	strcpy(Cur_Dir_Name, bak_Cur_Dir_Name);

	cout << "Add user success." << endl;
	return true;
}

bool userdel(char username[])
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
	strcpy(bak_Cur_User_Name, Cur_User_Name);
	strcpy(bak_Cur_User_Dir_Name, Cur_User_Dir_Name);
	bak_Cur_Dir_Addr = Cur_Dir_Addr;
	strcpy(bak_Cur_Dir_Name, Cur_Dir_Name);

	cd("/home");
	strcpy(bak_Cur_User_Name_2, Cur_User_Name);
	strcpy(bak_Cur_Group_Name, Cur_Group_Name);
	strcpy(Cur_User_Name, username);
	strcpy(Cur_Group_Name, "user");
	if (!mkdir(username)) {
		strcpy(Cur_User_Name, bak_Cur_User_Name_2);
		strcpy(Cur_Group_Name, bak_Cur_Group_Name);
		//恢复现场，回到原来的目录
		strcpy(Cur_User_Name, bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name, bak_Cur_Dir_Name);

		cout << "Add user failure." << endl;
		return false;
	}
	strcpy(Cur_User_Name, bak_Cur_User_Name_2);
	strcpy(Cur_Group_Name, bak_Cur_Group_Name);

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

	if (strstr(passwd_buf, username) != NULL) {
		cout << "The user has already existed." << endl;

		strcpy(Cur_User_Name, bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name, bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name, bak_Cur_Dir_Name);
		return false;
	}
	pthread_rwlock_wrlock(&passwd_Inode.lock);
	char* p = strstr(passwd_buf, username);
	*p = '\0';
	while ((*p) != '\n')	//空出中间的部分
		p++;
	p++;
	strcat(passwd_buf, p);
	passwd_Inode.fileSize = strlen(passwd_buf);
	fseek(fw, passwd_Inode_Addr, SEEK_SET);
	fwrite(&passwd_Inode, sizeof(inode), 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&passwd_Inode.lock);

	pthread_rwlock_wrlock(&shadow_Inode.lock);
	char* p = strstr(shadow_buf, username);
	*p = '\0';
	while ((*p) != '\n')	//空出中间的部分
		p++;
	p++;
	shadow_Inode.fileSize = strlen(shadow_buf);
	fseek(fw, shadow_Inode_Addr, SEEK_SET);
	fwrite(&shadow_Inode, sizeof(inode), 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&shadow_Inode.lock);

	pthread_rwlock_wrlock(&group_Inode.lock);
	p = strstr(group_buf, username);
	*p = '\0';
	while ((*p) != '\n' && (*p) != ',')	//空出中间的部分
		p++;
	if ((*p) == ',')
		p++;
	strcat(group_buf, p);
	group_Inode.fileSize = strlen(group_buf);
	fseek(fw, group_Inode_Addr, SEEK_SET);
	fwrite(&group_Inode, sizeof(inode), 1, fw);
	fflush(fw);
	pthread_rwlock_unlock(&group_Inode.lock);


	
	char* tmp = strcat("/home/", username);
	if (strcmp(Cur_Dir_Name, tmp)) {
		cd("/");
		rm(tmp);
	}else
		rm(tmp);

	cout << "User has deleted." << endl;
	return true;
}

void chmod(char name[], int pmode)
{
	if (strlen(name) > MAX_FILE_SIZE) {
		cout << "Name is too long"<<endl;
		return;
	}
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		cout << "Operation error." << endl;
		return;
	}
	int parinoaddr = extractPath(name);
	char* fileName = extractLastPath(name);
	if (parinoaddr == -1 ) {
		cout << "Non-existent Target.";
		return;
	}
	inode cur = { 0 };
	fseek(fr, parinoaddr, SEEK_SET);
	fread(&cur, sizeof(cur), 1, fr);
	fflush(fr);
	
	if (cur.uname == Cur_User_Name) {
		pthread_rwlock_wrlock(&cur.lock);
		cur.mode = (cur.mode >> 9 << 9) | pmode;
		fseek(fw, parinoaddr, SEEK_SET);
		fwrite(&cur, sizeof(cur), 1, fw);
		pthread_rwlock_unlock(&cur.lock);
		return;
	}
	else if (cur.gname == Cur_Group_Name) {
		pthread_rwlock_wrlock(&cur.lock);
		cur.mode = (cur.mode >> 9 << 9) | pmode;
		fseek(fw, parinoaddr, SEEK_SET);
		fwrite(&cur, sizeof(cur), 1, fw);
		pthread_rwlock_unlock(&cur.lock);
		return;
	}
	else {
		cout << "No Permission!" << endl;
		return;
	}
}

void touch(char name[])
{
	if (strlen(name) > FILENAME_MAX) {
		cout << "File name too long." << endl;
		return;
	}
	char tmpName[30], FileName[30];
	strcpy_s(tmpName, name);
	strcpy_s(FileName, extractLastPath(name));
	strcat(tmpName, "/..");
	int parinoAddr = extractPath(tmpName);
	if (parinoAddr == -1) {
		cout << "Can't touch file at Non-existent path." << endl;
		return ;
	}
	
	int newInodeAddress = InodeAlloc();
	if (newInodeAddress == -1) {
		cout << "No empty space." << endl;
		return;
	}
	else {
		inode cur = { 0 };
		fseek(fr, parinoAddr, SEEK_SET);
		fread(&cur, sizeof(cur), 1, fr);
		fflush(fr);
		FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, cur.dirBlock[0], SEEK_SET);
		fread(fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
		int cnt=1;
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
		New.refCnt = 0;
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
		fwrite(fileEnt, sizeof(fileEnt), 1, fw);
		fflush(fw);
		return;
	}
}

bool mkdir(char name[])
{
	if (strlen(name) > MAX_FILE_NAME) {
		cout << "The directory name is too long." << endl;
		return false;
	}
	char tmpName[30],dirName[30];
	strcpy_s(tmpName, name);
	strcpy_s(dirName, extractLastPath(name));
	strcat(tmpName, "/..");
	int parinoAddr = extractPath(tmpName);
	if (parinoAddr == -1) {
		cout << "Can't mkdir at Non-existent path." << endl;
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
		New.refCnt = 0;
		New.IdirBlock = -1;
		New.mode = MODE_DIR | DIR_DEF_PERMISSION;
		strcpy_s(New.uname, Cur_User_Name);
		strcpy_s(New.gname, Cur_Group_Name);
		New.atime = time(NULL);
		New.ctime = time(NULL);
		New.mtime = time(NULL);

		//get current inode
		inode cur = { 0 };
		fseek(fr, parinoAddr, SEEK_SET);
		fread(&cur, sizeof(inode), 1, fr);
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

		cur.refCnt = cur.refCnt + 1;

		//get current file entry
		FileEnt curFileEnt[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, cur.dirBlock[0], SEEK_SET);
		fread(curFileEnt, sizeof(curFileEnt), 1, fr);
		fflush(fr);

		//check whehter there is same name file
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (strcmp(curFileEnt[i].dir.name, dirName) == 0) {
				cout << "File of this name is already exist!" << endl;
				return false;
			}
		}

		//update current dictionary information
		curFileEnt[cur.refCnt].dir = dir;
		strcpy_s(curFileEnt[cur.refCnt].buffer, 0);
		fseek(fw, cur.dirBlock[0], SEEK_SET);
		fwrite(curFileEnt, sizeof(curFileEnt), 1, fw);
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(inode), 1, fw);

		//create new dictionary information
		FileEnt newFileEnt[FILEENT_PER_BLOCK] = { 0 };
		newFileEnt[0].dir = dir;
		strcpy_s(newFileEnt[0].buffer, 0);
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

bool rm(char name[])
{
	int parinoaddr = extractPath(name);
	if (parinoaddr == -1) {
		cout << "No such file or dictionary." << endl;
		return false;
	}
	char* fileName = extractLastPath(name);
	//get current inode
	inode cur = { 0 };
	fseek(fr, parinoaddr, SEEK_SET);
	fread(&cur, sizeof(inode), 1, fr);
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

	if (((cur.mode >> 9) & 1) == 0) { //file
		FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, cur.dirBlock[0], SEEK_SET);
		fread(fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (fileEnt[i].dir.name == fileName) {
				fseek(fw, cur.dirBlock[0]+i*sizeof(FileEnt), SEEK_SET);
				fwrite(0, sizeof(FileEnt), 1, fw);
				fseek(fw, parinoaddr, SEEK_SET);
				fwrite(0, sizeof(INODE_SIZE), 1, fw);
				fflush(fw);
				free_inode(superblock, imap, (parinoaddr - Inode_StartAddr) / INODE_SIZE);
				break;
			}
		}
		return true;
	}
	else {						//dictionary
		int start = cur.dirBlock[0];
		FileEnt fileEnt[FILEENT_PER_BLOCK] = { 0 };
		fseek(fr, start, SEEK_SET);
		fread(fileEnt, sizeof(fileEnt), 1, fr);
		fflush(fr);
		for (int i = 0; i < FILEENT_PER_BLOCK; i++) {
			if (fileEnt[i].dir.name != "..") {
				fseek(fw, fileEnt[i].dir.iaddr, SEEK_SET);
				fwrite(0, sizeof(INODE_SIZE), 1, fw);
				free_inode(superblock, imap, (fileEnt[i].dir.iaddr - Inode_StartAddr) / INODE_SIZE);
			}
		}
		fseek(fw, start, SEEK_SET);
		fwrite(0, sizeof(fileEnt), 1, fw);
		fflush(fw);
		free_block(superblock, bmap, (start - Block_StartAddr) / BLOCK_SIZE);
		return true;
	}
	return false;
}

void ls(char name[])
{
	int parinoaddr = extractPath(name);
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
		if (dir.name == "0")
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

		printf("%d ", tmp.refCnt);	
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

void cd(char name[])
{
	if (strlen(name) > MAX_FILE_NAME) {
		cout << "The directory name is too long." << endl;
		return;
	}
	if (strcmp(name, "")) {
		cout << "The argument can't be empty." << endl;
		return;
	}
	if (strcmp(name, ".")) {
		return;
	}
	int parinoaddr = extractPath(name);
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
		strcpy_s(Cur_Dir_Name, fileEnt[0].dir.name);
		Cur_Dir_Addr = cur.dirBlock[0];
		return;
	}
	else {
		printf("cd %s : No such file or directory.\n", name);
		return;
	}
}

void vim(char name[], char buf[])	//todo
{
	if (strlen(name) >= MAX_NAME_SIZE) {
		cout << "The filename is too long." << endl;
		return;
	}


	memset(buf, 0, sizeof(buf));
	int maxlen = 0;
	int position = 0;
	bool isExist=false;
	int fileInodeAddr;
	char tmpName[30], FileName[30];
	strcpy_s(tmpName, name);
	strcpy_s(FileName, extractLastPath(name));
	strcat(tmpName, "/..");
	int parinoAddr = extractPath(tmpName);
	if (parinoAddr == -1) {
		cout << "Can't do operation at Non-existent path." << endl;
		return ;
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
			if (((fileinode.mode >> 9) & 1) == 0) {	//是文件且重名，打开这个文件，并编辑	
				pthread_rwlock_wrlock(&fileEnt[i].lock);
				fileInodeAddr = fileEnt[i].dir.iaddr;
				position = i;
				isExist = true;
				goto label;
			}
		}
	}

label:

	//初始化vi
	int cnt = 0;
	system("cls");	//清屏

	int winx, winy, curx, cury;

	HANDLE handle_out;                              //定义一个句柄  
	CONSOLE_SCREEN_BUFFER_INFO screen_info;         //定义窗口缓冲区信息结构体  
	COORD pos = { 0, 0 };                             //定义一个坐标结构体

	if (isExist) {	//文件已存在，进入编辑模式，先输出之前的文件内容

		//权限判断。判断文件是否可读
		if (((fileinode.mode >> filemode >> 2) & 1) == 0) {
			//不可读
			cout << "Permission denied." << endl;
			return;
		}

		printf("%c", fileEnt[position].buffer);
		maxlen = strlen(fileEnt[position].buffer);
	}

	//获得输出之后的光标位置
	handle_out = GetStdHandle(STD_OUTPUT_HANDLE);   //获得标准输出设备句柄  
	GetConsoleScreenBufferInfo(handle_out, &screen_info);   //获取窗口信息  
	winx = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
	winy = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
	curx = screen_info.dwCursorPosition.X;
	cury = screen_info.dwCursorPosition.Y;


	//进入vi
	//先用vi读取文件内容

	int mode = 0;	//vi模式，一开始是命令模式
	unsigned char c;
	while (1) {
		if (mode == 0) {	//命令行模式
			c = getch();

			if (c == 'i' || c == 'a') {	//插入模式
				if (c == 'a') {
					curx++;
					if (curx == winx) {
						curx = 0;
						cury++;
					}
				}

				if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
					//超过这一屏，向下翻页
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
					//显示 "插入模式"
					Gotoxy(handle_out, 0, winy - 1);
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
				//system("color 09");//设置文本为蓝色
				if (cury - winy + 2 > 0)
					Gotoxy(handle_out, 0, cury + 1);
				else
					Gotoxy(handle_out, 0, winy - 1);
				_COORD pos;
				if (cury - winy + 2 > 0)
					pos.X = 0, pos.Y = cury + 1;
				else
					pos.X = 0, pos.Y = winy - 1;
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
				int i;
				for (i = 0; i < winx - 1; i++)
					printf(" ");

				if (cury - winy + 2 > 0)
					Gotoxy(handle_out, 0, cury + 1);
				else
					Gotoxy(handle_out, 0, winy - 1);

				WORD att = BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY; // 文本属性
				FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//控制台部分着色 
				SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN);	//设置文本颜色
				printf(":");

				char pc;
				int tcnt = 1;	//命令行模式输入的字符计数
				while (c = getch()) {
					if (c == '\r') {	//回车
						break;
					}
					else if (c == '\b') {	//退格，从命令条删除一个字符 
						//SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
					//buf[maxlen] = '\0'; 
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
					system("cls");
					break;	//vi >>>>>>>>>>>>>> 退出出口
				}
				else {
					if (cury - winy + 2 > 0)
						Gotoxy(handle_out, 0, cury + 1);
					else
						Gotoxy(handle_out, 0, winy - 1);
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
					int i;
					for (i = 0; i < winx - 1; i++)
						printf(" ");

					if (cury - winy + 2 > 0)
						Gotoxy(handle_out, 0, cury + 1);
					else
						Gotoxy(handle_out, 0, winy - 1);
					SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN);	//设置文本颜色
					FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//控制台部分着色
					printf("--  Command Error --");
					//getch();
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
					Gotoxy(handle_out, curx, cury);
				}
			}
			else if (c == 27) {	//ESC，命令行模式，清状态条
				Gotoxy(handle_out, 0, winy - 1);
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
				int i;
				for (i = 0; i < winx - 1; i++)
					printf(" ");
				Gotoxy(handle_out, curx, cury);

			}

		}
		else if (mode == 1) {	//插入模式

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

			c = getch();
			if (c == 27) {	// ESC，进入命令模式
				mode = 0;
				//清状态条
				Gotoxy(handle_out, 0, winy - 1);
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
				int i;
				for (i = 0; i < winx - 1; i++)
					printf(" ");
				continue;
			}
			else if (c == '\b') {	//退格，删除一个字符
				if (cnt == 0)	//已经退到最开始
					continue;
				printf("\b");
				printf(" ");
				printf("\b");
				curx--;
				cnt--;	//删除字符
				if (buf[cnt] == '\n') {
					//要删除的这个字符是回车，光标回到上一行
					if (cury != 0)
						cury--;
					int k;
					curx = 0;
					for (k = cnt - 1; buf[k] != '\n' && k >= 0; k--)
						curx++;
					Gotoxy(handle_out, curx, cury);
					printf(" ");
					Gotoxy(handle_out, curx, cury);
					if (cury - winy + 2 >= 0) {	//翻页时
						Gotoxy(handle_out, curx, 0);
						Gotoxy(handle_out, curx, cury + 1);
						SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
			else if (c == 224) {	//判断是否是箭头
				c = getch();
				if (c == 75) {	//左箭头
					if (cnt != 0) {
						cnt--;
						curx--;
						if (buf[cnt] == '\n') {
							//上一个字符是回车
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
				else if (c == 77) {	//右箭头
					cnt++;
					if (cnt > maxlen)
						maxlen = cnt;
					curx++;
					if (curx == winx) {
						curx = 0;
						cury++;

						if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
							//超过这一屏，向下翻页
							if (cury % (winy - 1) == winy - 2)
								printf("\n");
							SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
			if (c == '\r') {	//遇到回车
				printf("\n");
				curx = 0;
				cury++;

				if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
					//超过这一屏，向下翻页
					if (cury % (winy - 1) == winy - 2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
			//移动光标
			curx++;
			if (curx == winx) {
				curx = 0;
				cury++;

				if (cury > winy - 2 || cury % (winy - 1) == winy - 2) {
					//超过这一屏，向下翻页
					if (cury % (winy - 1) == winy - 2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
			//记录字符 
			buf[cnt++] = c;
			if (cnt > maxlen)
				maxlen = cnt;
		}
		else {	//其他模式
		}
	}
	if (isExist) {	//如果是编辑模式
		//将buf内容写回文件的磁盘块

		if (((fileinode.mode >> filemode >> 1) & 1) == 1) {	//可写
			strcpy_s(fileEnt[position].buffer, buf);
			fileinode.fileSize = strlen(buf);
			fileinode.mtime = time(NULL);
			fseek(fw, fileEnt[position].dir.iaddr, SEEK_SET);
			fwrite(&fileinode, sizeof(fileinode), 1, fw);
			fseek(fw, cur.dirBlock[0], SEEK_SET);
			fwrite(fileEnt, sizeof(fileEnt), 1, fw);
			fflush(fw);
			pthread_rwlock_unlock(&fileEnt[position].lock);
		}
		else {	//不可写
			cout << "Permission denied." << endl;
			pthread_rwlock_unlock(&fileEnt[position].lock);
		}

	}
	else {	//是创建文件模式
		if (((cur.mode >> filemode >> 1) & 1) == 1) {
			//可写。可以创建文件
			create(name, buf);	//创建文件
		}
		else {
			cout << "Permission denied." << endl;
			return;
		}
	}
}

void clear()
{
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	system("cls");
#else
	system("clear");
#endif
}

void pwd()
{
	printf("%s\n", Cur_Dir_Name);
}

void cat(char name[])
{
	int parinoaddr = extractPath(name);
	if (parinoaddr == -1) {
		cout << "No such file" << endl;
		return;
	}
	char* fileName = extractLastPath(name);
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
		if (fileEnt[i].dir.name == fileName) {
			pthread_rwlock_wrlock(&fileEnt[i].lock);
			printf("%s\n", fileEnt[i].buffer);
			pthread_rwlock_unlock(&fileEnt[i].lock);
			break;
		}
	}
	pthread_rwlock_unlock(&(cur.lock));
	return;
} 

void cmd(char cmdline[])
{
	char inputCommand[100] = { 0 };
	char inputParameter[100] = { 0 };
	char inputParameter2[100] = { 0 };
	char buf[MAX_FILE_SIZE] = { 0 };	//最大10K
	int tmp = 0;
	int i = 0;
	sscanf(cmdline, "%s%s%s", inputCommand, inputParameter, inputParameter2);

	if (isLogin == false) {
		// 还未登录只能使用login、注册、Help（未登录的Help）、exit、clear
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
		// 已经登录，可以使用全部命令
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
				cat(Cur_Dir_Addr, inputParameter);
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
				touch(Cur_Dir_Addr, inputParameter, buf);
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
				rm(Cur_Dir_Addr, inputParameter);
			}
		}
		else if (strcmp(inputCommand, "vi") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Vim_help();
			}
			else {
				vim(Cur_Dir_Addr, inputParameter, buf);
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
				chmod(Cur_Dir_Addr, inputParameter, tmp);
			}
		}
		else if (strcmp(inputCommand, "ls") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				ls(Cur_Dir_Addr);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Ls_help();
			}
			else {
				CommandError(inputCommand);
			}
		}
		else if (strcmp(inputCommand, "cd") == 0) {
			if (strcmp(inputParameter, "--help") == 0) {
				Cd_help();
			}
			else {
				cd(Cur_Dir_Addr, inputParameter);
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
				mkdir(Cur_Dir_Addr, inputParameter);
			}
		}
		else if (strcmp(inputCommand, "rmdir") == 0) {
			if (strcmp(inputParameter, "") == 0 || strcmp(inputParameter, ".") == 0 || strcmp(inputParameter, "..") == 0) {
				CommandError(inputCommand);
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Rmdir_help();
			}
			else {
				rmdir(Cur_Dir_Addr, inputParameter);
			}
		}
		else if (strcmp(inputCommand, "format") == 0) {
			if (strcmp(inputParameter, "") == 0) {
				if (strcmp(Cur_User_Name, "root") != 0) {
					cout << "Permission denied." << endl;
					return;
				}
				else {
					Ready();
					logout();
				}
			}
			else if (strcmp(inputParameter, "--help") == 0) {
				Rmdir_help();
			}
			else {
				CommandError(inputCommand);
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

void inputUsername(char username[])
{
	printf("username: ");
	scanf("%s", username);	//Username
}

void inputPassword(char passwd[])
{
	int len = 0;
	char c;
	fflush(stdin);	//clean input buffer
	printf("password: ");
	while (c = getch()) {
		if (c == '\r') {
			passwd[len] = '\0';
			fflush(stdin);
			printf("\n");
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

void Gotoxy(HANDLE hOut, int x, int y)
{
	COORD pos;
	pos.X = x;            
	pos.Y = y;        
	SetConsoleCursorPosition(hOut, pos);
}


bool check(char username[], char passwd[])
{
	int passwd_Inode_Addr = -1;	//passwd inode address
	int shadow_Inode_Addr = -1;	//shadow inode address
	int group_Inode_Addr = -1;	//group inode address 
	inode passwd_Inode = { 0 };		//passwd inode
	inode shadow_Inode = { 0 };		//shadow inode
	inode group_Inode = { 0 };		//group inode

	char passwd_buf[256];
	char shadow_buf[256];
	char group_buf[256];
	char tmpbuf[256];


	inode cur = { 0 };

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

	if (strstr(passwd_buf, username) == NULL) {
		cout << "User does not exist." << endl;
		cd("/");
		return false;
	}

	char* p;
	if ((p == strstr(shadow_buf, username)) == NULL) {
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
		strcpy(Cur_User_Name, username);
		if (strcmp(username, "root") == 0)
			strcpy_s(Cur_Group_Name, "root");	//当前登陆用户组名
		else
			strcpy_s(Cur_Group_Name, "user");	//当前登陆用户组名
		char tmp[25];
		strcpy_s(tmp,"/home/");
		strcat(tmp, username);
		cd(tmp); 
		strcpy(Cur_User_Dir_Name, Cur_Dir_Name);	//复制当前登陆用户目录名
		return true;
	}
	else {
		cout << "Password is not correct." << endl;
		cd("/");	//回到根目录
		return false;
	}
}	

void gotoRoot()
{
	memset(Cur_User_Name, 0, sizeof(Cur_User_Name));		//clear current username
	memset(Cur_User_Dir_Name, 0, sizeof(Cur_User_Dir_Name));	//clear current User Dic name
	Cur_Dir_Addr = Root_Dir_Addr;	//set current dir address as root dir address
	strcpy(Cur_Dir_Name, "/");		//set current dir as "/"
}


bool create(char name[], char buf[])
{
	if (strlen(name) > FILENAME_MAX) {
		cout << "File name too long." << endl;
		return false;
	}
	char tmpName[30], FileName[30];
	strcpy_s(tmpName, name);
	strcpy_s(FileName, extractLastPath(name));
	strcat(tmpName, "/..");
	int parinoAddr = extractPath(tmpName);
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
		fread(&cur, sizeof(cur), 1, fr);
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
		New.refCnt = 0;
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
		fwrite(fileEnt, sizeof(fileEnt), 1, fw);
		fflush(fw);
		return true;
	}
}


void Help_NotLogin() {
	Command_help();
	Login_help();
	Clear_help();
	Exit_help();
}


void help()
{
	Command_help();

	Useradd_help();
	Userdel_help();

	Login_help();
	Logout_help();

	Cat_help();
	Touch_help();
	Rm_help();
	Vi_help();
	Chmod_help();

	Ls_help();
	Cd_help();
	Pwd_help();
	Mkdir_help();
	Rmdir_help();

	Clear_help();
	Exit_help();
}


void Command_help() {
	cout << "Command format : 'command [Necessary parameter] ([Unnecessary parameter])'" << endl;
}
void Useradd_help() {
	cout << "useradd [username] : Add user" << endl;
}
void Userdel_help() {
	cout << "userdel [username] : Delete users" << endl;
}
void Login_help() {
	cout << "login ([username]): Login MFS" << endl;
}
void Logout_help() {
	cout << "logout : Exit the current user" << endl;
}
void Cat_help() {
	cout << "cat [filename] : Output file content" << endl;
}
void Rm_help() {
	cout << "rm [filename] : Remove file" << endl;
}
void Mkdir_help() {
	cout << "mkdir [directoryName] : Create subdirectory" << endl;
}
void Rmdir_help() {
	cout << "rmdir [directoryName] : Delete subdirectory" << endl;
}
void Cd_help() {
	cout << "cd [directoryName] : Change current directory" << endl;
}
void Ls_help() {
	cout << "ls : List the file directory" << endl;
}
void Pwd_help() {
	cout << "pwd : List the current directory name" << endl;
}
void Chmod_help() {
	cout << "chmod [filename] [permissions] : Change the file permissions" << endl;
}
void Clear_help() {
	cout << "clear : Clear the terminal" << endl;
}
void Exit_help() {
	cout << "exit : Exit the MFS" << endl;
}
void Touch_help() {
	cout << "touch [filename] : Create a new empty file" << endl;
}
void Vim_help() {
	cout << "vim [filename] : Remove file" << endl;
}
void CommandError(char command[])
{
	printf("%s: missing operand\n", command);
	printf("Try '%s --help' for more information.\n", command);
}