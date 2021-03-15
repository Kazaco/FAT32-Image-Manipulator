
#ifndef _FILELIST_H
#define _FILELIST_H
#include "../Structs/tokenlist.h"
#include "../Structs/fileentry.h"
#include "../Structs/direntry.h"

filesList * new_filesList();
int createOpenFileEntry(filesList * openFiles, dirlist * directories, tokenlist * tokens, int index);
void readFilesList(filesList * openFiles);
int filesListIndex(filesList * openFiles, const char * item);
int openFileIndex(filesList * files, tokenlist * tokens, int flag);
void free_filesList(filesList * openFiles);

#endif