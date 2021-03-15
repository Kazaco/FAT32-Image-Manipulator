#ifndef _CLOSE_H
#define _CLOSE_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"
#include "../Structs/fileentry.h"

filesList * closeFileForReadOrWrite(tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles);

#endif