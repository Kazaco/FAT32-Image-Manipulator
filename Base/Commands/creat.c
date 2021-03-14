#include "creat.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fat.h"
#include <stdio.h> 		//printf()

void createNewFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory)
{
    //Find index of DIRNAME
    int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILEORDIR);
    //Check if given DIRNAME is in our current directory
    if(index == -1)
    {
        createFile(imgFile, tokens->items[1], currentDirectory, 0, 0);
    }
    else
    {
        printf("Directory/File %s already exists.\n", tokens->items[1]);
    }
}