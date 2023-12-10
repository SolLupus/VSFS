#include "user.h"
#include "fs.h"
#include "bitmap.h"
#include <conio.h>



void InitUser()  //modify
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
	create(Cur_Dir_Addr, "passwd", buf);

	char s_passwd[100] = { 0 };
	strcpy_s(s_passwd, "root:");
	strcat(s_passwd, password1);
	strcat(s_passwd, "\n");

	sprintf(buf, s_passwd);

	create(Cur_Dir_Addr, "shadow", buf);
	chmod("/etc/shadow", 0660);

	sprintf(buf, "root::0:root\n");
	sprintf(buf + strlen(buf), "user::1:\n");
	create(Cur_Dir_Addr, "group", buf);

	cd("/");
}

bool login(char username[]) //todo
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

	//回到根目录
	gotoRoot();
	isLogin = false;
}

bool useradd(char username[]) //todo
{
	return false;
}

bool userdel(char username[]) //todo
{
	return false;
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

void touch(char name[]) //todo
{
	if (strlen(name) > FILENAME_MAX) {
		cout << "File name too long." << endl;
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
		newFileEnt[1].dir = curFileEnt[0].dir;
		strcpy_s(newFileEnt[1].dir.name, "..");
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
			pthread_rwlock_rdlock(&fileEnt[i].lock);
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

void gotoxy(HANDLE hOut, int x, int y)
{
	COORD pos;
	pos.X = x;            
	pos.Y = y;        
	SetConsoleCursorPosition(hOut, pos);
}


bool check(char username[], char passwd[]) //todo
{
	return false;
}

void gotoRoot()
{
	memset(Cur_User_Name, 0, sizeof(Cur_User_Name));		//clear current username
	memset(Cur_User_Dir_Name, 0, sizeof(Cur_User_Dir_Name));	//clear current User Dic name
	Cur_Dir_Addr = Root_Dir_Addr;	//set current dir address as root dir address
	strcpy(Cur_Dir_Name, "/");		//set current dir as "/"
}


void WriteFile(inode fileInode, int fileInodeAddr, char buf[]) //todo to be determined
{
}


bool create(int parinoAddr, char name[], char buf[])//todo
{
	return false;
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