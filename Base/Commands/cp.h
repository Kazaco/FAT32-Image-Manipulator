#ifndef _COPY_H
#define _COPY_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"

dirlist * copyFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory);
unsigned int * findEmptyEntryInFATNext(const char * imgFile, unsigned int * emptyArr);

#endif