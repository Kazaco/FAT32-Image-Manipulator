#ifndef _OPEN_H
#define _OPEN_H
#include "../Structs/tokenlist.h"
#include "../Structs/direntry.h"
#include "../Structs/fileentry.h"

void openFileForReadOrWrite(tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles);

#endif