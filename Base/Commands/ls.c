#include "ls.h"
#include "../Helpers/utils.h"
#include "../Helpers/directorylist.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

void printList(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory)
{
    //Check Current Directory
    if(tokens->size == 1)
    {
        //Just read cwd
        readDirectories(currentDirectory);
    }
    //Check DIRNAME
    else
    {
        //Find index of DIRNAME
        int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], DIRECTORY);
        //Check if index found the DIRNAME or not.
        if(index != -1)
        {
            //Calculate cluster value of DIRNAME
            char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusHI, 2);
            char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusLO, 2);
            strcat(clusterHI,clusterLOW);
            unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
            //Make list structure containing all files found in DIRNAME cluster.
            dirlist * lsDirectory = getDirectoryList(imgFile, clusterValHI);
            //Display to User
            readDirectories(lsDirectory);
            //Deallocate everything.
            free(clusterHI);
            free(clusterLOW);
            free_dirlist(lsDirectory);
        }
        else
        {
            printf("Directory not found.\n");
        }
    }
}