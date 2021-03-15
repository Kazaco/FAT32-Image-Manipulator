
#ifndef _READ_H
#define _READ_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"
#include "../Structs/fileentry.h"

void readToOpenFile(const char * imgFile, tokenlist * tokens, filesList * openFiles);
char * readFAT(const char * imgFile, tokenlist * tokens, filesList * openFiles);

#endif