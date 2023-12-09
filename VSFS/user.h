#ifndef USER_H
#define USER_H
#include<windows.h>
#include "type.h"
#include <iostream>
using namespace std;

bool login(char[]);											
void logout();												
bool useradd(char username[]);								
bool userdel(char username[]);								
void chmod(int parinoAddr, char name[], int pmode);			
void touch(int parinoAddr, char name[], char buf[]);		
bool mkdir(int parinoAddr, char name[]);					
bool rmdir(int parinoAddr, char name[]);					
bool rm(int parinoAddr, char name[]);						
void ls(int parinoaddr);									
void cd(int parinoaddr, char name[]);						
void vim(int parinoaddr, char name[], char buf[]);			
void clear();												
void pwd();													
void cat(int parinoAddr, char name[]);
void cmd(char cmdline[]);


void inputUsername(char username[]);							
void inputPassword(char passwd[]);								
void gotoxy(HANDLE hOut, int x, int y);							
void writeFile(inode fileInode, int fileInodeAddr, char buf[]);	
bool check(char username[], char passwd[]);						
void gotoRoot();												
void rmall(int parinoAddr);										
bool create(int parinoAddr, char name[], char buf[]);

void help();													

void CommandError(char command[]);
void Command_help();

void Useradd_help();
void Userdel_help();

void Login_help();
void Logout_help();

void Pwd_help();
void Mkdir_help();
void Rmdir_help();
void Cd_help();
void Ls_help();
void Chmod_help();

void Clear_help();
void Exit_help();

void Cat_help();
void Touch_help();
void Rm_help();
void Vi_help();

void Super_help();
void Inode_help();
void Block_help();

#endif // !USER_H
