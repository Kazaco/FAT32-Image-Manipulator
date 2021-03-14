#ifndef _MOVE_H
#define _MOVE_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"

dirlist * MoveFileOrDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory);

#endif