
#ifndef _REMOVE_H
#define _REMOVE_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"

dirlist * removeFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory);
void removeFileFromFATandDataRegion(const char * imgFile, dirlist * directory, const char * filename);

#endif