#ifndef _RMDIR_H
#define _RMDIR_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"

dirlist * removeDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory);

#endif