#ifndef USER_H
#define USER_H
#include <windows.h>

bool login(char[]);											
void logout();												
bool useradd(char username[]);								
bool userdel(char username[]);
void rename(char name[], char newName[]);
void mv(char name[], char newPath[]);
void cp(char name[], char newPath[]);
void chmod(char name[], int pmode);			
void touch(char name[]);		
bool mkdir(char name[]);										
bool rm(char name[]);						
void ls(char name[]);
void cd(char name[]);						
void vim(char name[], char buf[]);			
void clear();												
void pwd();													
void cat(char name[]);
void cmd(char cmdline[]);
bool su(char username[]);
void rename(char name[], char newName[]);
void mv(char name[], char newPath[]);
void cp(char name[], char newPath[]);


void inputUsername(char username[]);							
void inputPassword(char passwd[]);								
void Gotoxy(HANDLE hOut, int x, int y);							
bool check(char username[], char passwd[]);						
void gotoRoot();																					
bool create(char name[], char buf[]);

void help();													

void CommandError(char command[]);
void Command_help();

void Useradd_help();
void Userdel_help();

void Login_help();
void Logout_help();
void Su_help();
void Pwd_help();
void Mkdir_help();
void Cd_help();
void Ls_help();
void Chmod_help();
void Mv_help();
void Clear_help();
void Exit_help();
void Cp_help();
void Cat_help();
void Touch_help();
void Rm_help();
void Vim_help();
void Rename_help();
#endif // !USER_H
