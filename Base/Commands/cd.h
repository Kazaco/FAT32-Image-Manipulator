#ifndef _CD_H
#define _CD_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"
#include "../Structs/fileentry.h"

void changeDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles);

#endif