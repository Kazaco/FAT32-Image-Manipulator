
#ifndef _FILELIST_H
#define _FILELIST_H
#include "../Structs/fileentry.h"

filesList * new_filesList();
void free_filesList(filesList * openFiles);

#endif