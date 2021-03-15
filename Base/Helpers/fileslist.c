#include "fileslist.h"
#include "utils.h"
#include <stdlib.h>     //free(), realloc()
#include <stdio.h> 		//printf()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

//Create a list containing all the files the user has opened for
//reading, writing, or both.
filesList * new_filesList()
{
    filesList * files = (filesList *) malloc(sizeof(filesList));
    files->size = 0;
    files->items = (FILEENTRY **) malloc(sizeof(FILEENTRY *));
    return files;
}

//Create an open file for read/write in our struct.
int createOpenFileEntry(filesList * openFiles, dirlist * directories, tokenlist * tokens, int index)
{
    //First Check that we aren't creating a duplicate entry.
    //Need the cluster number as well b/c files can have the same name in different directorys.
    char * clusterHI = littleEndianHexStringFromUnsignedChar(directories->items[index]->DIR_FstClusHI, 2);
    char * clusterLOW = littleEndianHexStringFromUnsignedChar(directories->items[index]->DIR_FstClusLO, 2);
    strcat(clusterHI, clusterLOW);
    unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
    free(clusterHI);
    free(clusterLOW);

    //Check cluster and name, if not found filesListIndex returns -1
    if(filesListIndex(openFiles, tokens->items[1]) == -1)
    {
        //Create a new entry in our open files list.
        openFiles->items = (FILEENTRY **) realloc(openFiles->items, (openFiles->size + 1) * sizeof(FILEENTRY));
        openFiles->items[openFiles->size] = malloc(sizeof(FILEENTRY));
        //Copy relevant data over:
        //1. Name
        strcpy(openFiles->items[openFiles->size]->FILE_Name, "");
        strcpy(openFiles->items[openFiles->size]->FILE_Name, tokens->items[1]);
        //2. Cluster Info
        openFiles->items[openFiles->size]->FILE_FstClus = clusterValHI;
        //3. Mode
        strcpy(openFiles->items[openFiles->size]->FILE_Mode, "");
        strcpy(openFiles->items[openFiles->size]->FILE_Mode, tokens->items[2]);
        //4. Offset
        openFiles->items[openFiles->size]->FILE_OFFSET = 0;
        //5. File Size
        char * sizeStr = littleEndianHexStringFromUnsignedChar(directories->items[index]->DIR_FileSize, 4);
        unsigned int fileSize = (unsigned int)strtol(sizeStr, NULL, 16);
        openFiles->items[openFiles->size]->FILE_SIZE = fileSize;
        free(sizeStr);
        //Iterate size of openFiles
        openFiles->size += 1;
        return 1;
    }
    else
    {
        //Found an entry with same name/cluster val
        return -1;
    }
}

//Print Files List
void readFilesList(filesList * openFiles)
{
    int i = 0;
    for (i; i < openFiles->size; i++)
    {
        printf("(open) %-11s : (mode) %-2s : (clus) %3i : (offset) %6i : (size) %6i\n", openFiles->items[i]->FILE_Name, openFiles->items[i]->FILE_Mode,
        openFiles->items[i]->FILE_FstClus, openFiles->items[i]->FILE_OFFSET, openFiles->items[i]->FILE_SIZE);
    }
}

//Check if given char * is already in our files list
int filesListIndex(filesList * openFiles, const char * item)
{
    int i = 0;
    int found = -1;
    for (i; i < openFiles->size; i++)
    {
        //Check if that item in our list.
        if(strncmp(openFiles->items[i]->FILE_Name, item, strlen(openFiles->items[i]->FILE_Name)) == 0 )
        {
            found = i;
            break;
        }
    }
    return found;
}

//Check if we are allowed to read, write, or both with our given file
int openFileIndex(filesList * files, tokenlist * tokens, int flag)
{
    //Flag:
    // 1 - READ
    // 2 - WRITE
    // 3 - READ OR WRITE

    //Check if given char * is in our given directory
    int i = 0;
    int index = -1;
    //Unlike in dirListIndex we don't need to extend the item string w/ spaces because
    //I already cut them off when opening the file and inserting them into the filesList
    for(i; i < files->size; i++)
    {
        //File was found in our list.
        if(strncmp(files->items[i]->FILE_Name, tokens->items[1], strlen(tokens->items[1])) == 0)
        {
            //Check that the flags of the files->items permits what we are trying to do.

            //Check if we are allowed to read the file
            if( (flag == 1 || flag == 3) && (strcmp(files->items[i]->FILE_Mode, "r") == 0 || strcmp(files->items[i]->FILE_Mode, "rw") == 0
            || strcmp(files->items[i]->FILE_Mode, "wr") == 0) )
            {
                index = i;
                return index;
            }
            //Check if we are allowed to write to the file
            else if( (flag == 2 || flag == 3) && (strcmp(files->items[i]->FILE_Mode, "w") == 0 || strcmp(files->items[i]->FILE_Mode, "rw") == 0
            || strcmp(files->items[i]->FILE_Mode, "wr") == 0) )
            {
                index = i;
                return index;
            }
            else
            {
                //Invalid use of function.
                printf("Filename given is not 'opened' for %s.\n", tokens->items[0]);
                return -1;
            }
        }
    }
    //Return index of file/directory/empty if it is found. Val = -1 if not found.
    printf("Filename given is not an 'open' file.\n");
    return -1;
}

//Deallocate list of open files
void free_filesList(filesList * openFiles)
{
    int i = 0;
	for (i; i < openFiles->size; i++)
    {
        free(openFiles->items[i]);
    }
	free(openFiles);
}