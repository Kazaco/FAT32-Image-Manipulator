
#ifndef _WRITE_H
#define _WRITE_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"
#include "../Structs/fileentry.h"

void writeToOpenFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles);

#endif