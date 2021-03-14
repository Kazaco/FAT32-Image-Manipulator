#include "size.h"
#include "../Helpers/utils.h"
#include "../Helpers/directorylist.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

void printFileSize(tokenlist * tokens, dirlist * currentDirectory)
{
    //Find index of FILENAME
    int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILENAME);
    //Check if FILENAME was found or not.
    if(index != -1)
    {
        //Change unsigned int values to little endian to calculate file size for given FILENAME
        char * sizeStr = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FileSize, 4);
        unsigned int fileSize = (unsigned int)strtol(sizeStr, NULL, 16);
        printf("File %s: %i bytes\n", tokens->items[1], fileSize);
        free(sizeStr);
    }
    else
    {
        printf("File not found.\n");
    }
}