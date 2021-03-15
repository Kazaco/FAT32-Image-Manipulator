
#ifndef _LSEEK_H
#define _LSEEK_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"
#include "../Structs/fileentry.h"

void seekNewPositonInOpenFile(tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles);
unsigned int seekFAT(tokenlist * tokens, dirlist * directories, filesList * curFiles, unsigned int index2);

#endif