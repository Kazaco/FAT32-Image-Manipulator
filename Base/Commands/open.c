#include "open.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fileslist.h"
#include <stdio.h> 		//printf()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

void openFileForReadOrWrite(tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles)
{
    //Find index of FILENAME
    int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILENAME);
    //Check if given FILENAME is in our current directory
    if(index != -1)
    {
        //Check if dealing with read-only file.
        if((currentDirectory->items[index]->DIR_Attr & 0x10) != 0)
        {
            //READ-ONLY (HAVE NOT BEEN ABLE TO TEST)
            if(strcmp("r", tokens->items[2]) == 0)
            {
                if(createOpenFileEntry(openFiles, currentDirectory, tokens, index) == 1)
                {
                    readFilesList(openFiles);
                }
                else
                {
                    printf("File given is already open.\n");
                }
            }
            else
            {
                //Can't do anything with the file.
                printf("File given is read-only.\n");
            }
        }
        else
        {
            //Can read or write to file.
            if(strcmp("r", tokens->items[2]) == 0 || strcmp("w", tokens->items[2]) == 0
            || strcmp("rw", tokens->items[2]) == 0 || strcmp("wr", tokens->items[2]) == 0)
            {
                if(createOpenFileEntry(openFiles, currentDirectory, tokens, index) == 1)
                {
                    readFilesList(openFiles);
                }
                else
                {
                    printf("File given is already open.\n");
                }
            }
            else
            {
                printf("Invalid flag given.\n");
            }
        }
    }
    else
    {
        printf("File not found.\n");
    }
}