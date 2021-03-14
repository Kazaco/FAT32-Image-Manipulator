#ifndef _DIRLIST_H
#define _DIRLIST_H
#include "../Structs/direntry.h"

dirlist * new_dirlist();
void free_dirlist(dirlist * directories);
dirlist * getDirectoryList(const char * imgFile, unsigned int N);
int dirlistIndexOfFileOrDirectory(dirlist * directories, const char * item, int flag);
void readDirectories(dirlist * readEntry);

#endif