#include "cd.h"
#include "../Structs/bios.h"
#include "../Helpers/utils.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fileslist.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

void changeDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles)
{
    //Find index of DIRNAME
    int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], DIRECTORY);
    //Check if given DIRNAME is in our current directory
    if(index != -1)
    {
        //Calculate cluster value of DIRNAME
        char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusHI, 2);
        char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusLO, 2);
        strcat(clusterHI,clusterLOW);
        unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
        //free the CWD
        free_dirlist(currentDirectory);

        //case for CD to root directory
        if(clusterValHI == 0){
            currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
        }
        //case for cd to any other directory
        else{
            currentDirectory = getDirectoryList(imgFile, clusterValHI);
        }
        //Free the open filelist if they switch directories
        free(openFiles);
        openFiles = new_filesList();

        //Deallocate
        free(clusterHI);
        free(clusterLOW);
    }
    else
    {
        printf("Directory not found.\n");
    }
}