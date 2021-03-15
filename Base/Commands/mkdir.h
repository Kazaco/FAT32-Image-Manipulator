#ifndef _MKDIR_H
#define _MKDIR_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"

void createNewDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory);

#endif