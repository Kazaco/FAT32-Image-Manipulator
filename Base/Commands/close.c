#include "close.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fileslist.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

filesList * closeFileForReadOrWrite(tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles)
{
    //Find index of FILENAME in openFiles
    int index = filesListIndex(openFiles, tokens->items[1]);
    //Check if FILENAME was found or not.
    if(index != -1)
    {
        //Create a new filesList b/c deleting items from dynamically allocated in c can't be simple.
        filesList * newList = new_filesList();
        //Create a new entry list
        newList->items = (FILEENTRY **) realloc(newList->items, (openFiles->size - 1) * sizeof(FILEENTRY));

        //Copy all items over that don't have FILENAME.
        int i = 0;
        for(i; i < openFiles->size; i++)
        {
            //Copy everything over except the FILENAME given
            if(i != index)
            {
                //Create a new entry in our open files list.
                newList->items[newList->size] = malloc(sizeof(FILEENTRY));
                //1. Name
                strcpy(newList->items[newList->size]->FILE_Name, "");
                strcpy(newList->items[newList->size]->FILE_Name, openFiles->items[i]->FILE_Name);
                //2. Cluster Info
                newList->items[newList->size]->FILE_FstClus = openFiles->items[i]->FILE_FstClus;
                //3. Mode
                strcpy(newList->items[newList->size]->FILE_Mode, "");
                strcpy(newList->items[newList->size]->FILE_Mode, openFiles->items[i]->FILE_Mode);
                //4. Offset
                newList->items[newList->size]->FILE_OFFSET = openFiles->items[i]->FILE_OFFSET;
                //5. File Size
                newList->items[newList->size]->FILE_SIZE = openFiles->items[i]->FILE_SIZE;
                //Iterate
                newList->size += 1;
            }
        }
        //Delete previous fileList and return replaced list.
        free_filesList(openFiles);
        readFilesList(newList);
        return newList;
    }
    else
    {
        printf("File given is not open.\n");
        return openFiles;
    }
}