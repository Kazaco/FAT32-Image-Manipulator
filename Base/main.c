#include "Structs/bios.h"
#include "Structs/direntry.h"
#include "Helpers/utils.h"
#include "Helpers/parser.h"
#include "Helpers/directorylist.h"
#include "Helpers/fileslist.h"
#include "Commands/info.h"
#include "Commands/size.h"
#include "Commands/ls.h"
#include "Commands/cd.h"
#include "Commands/creat.h"
#include "Commands/mkdir.h"
#include "Commands/open.h"
#include "Commands/close.h"
#include "Commands/mv.h"
#include "Commands/rm.h"
#include "Commands/write.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy()

int main(int argc, char *argv[])
{
    //Only valid way to start program is with format: project3 fat32.img
    if(argc == 2)
    {
        //In-case the user puts an invalid file to search.
        if(!file_exists(argv[1]))
        {
            printf("Invalid File Given: %s\n", argv[1]);
            return -1;
        }

        //Valid FAT32 File System, set-up Program
        //before user interaction.
        const char * imgFile = argv[1];
        //Make the User Start in the Root
        getBIOSParamBlock(imgFile);
		dirlist * currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
		//Let the user have a container to interact w/ files no matter where they are in file system.
		filesList * openFiles = new_filesList();

        printf("=== FAT32 File System ===\n");
        while(1)
        {
            //User Initial Input
            printf("> ");
            char *input = get_input();
            //Split User Input into Tokens
            tokenlist *tokens = get_tokens(input);

            //Commands
            if(tokens->size == 0)
            {
                //Do nothing.
            }
            else if(strcmp("exit", tokens->items[0]) == 0 && tokens->size == 1)
            {
                free(input);
                free_dirlist(currentDirectory);
                free_filesList(openFiles);
                break;
            }
            else if(strcmp("info", tokens->items[0]) == 0 && tokens->size == 1)
            {
                printFATInfo();
            }
            else if(strcmp("size", tokens->items[0]) == 0 && tokens->size == 2)
            {
                printFileSize(tokens, currentDirectory);
            }
            else if(strcmp("ls", tokens->items[0]) == 0 && (tokens->size == 1 || tokens->size == 2) )
            {
                printList(imgFile, tokens, currentDirectory);
            }
            else if(strcmp("cd", tokens->items[0]) == 0 && tokens->size == 2)
            {
                changeDirectory(imgFile, tokens, currentDirectory, openFiles);
            }
            else if(strcmp("creat", tokens->items[0]) == 0 && tokens->size == 2)
            {
                createNewFile(imgFile, tokens, currentDirectory);
            }
            else if(strcmp("mkdir", tokens->items[0]) == 0 && tokens->size == 2)
            {
                createNewDirectory(imgFile, tokens, currentDirectory);
            }
            else if(strcmp("open", tokens->items[0]) == 0)
            {
                if(tokens->size == 3)
                {
                    openFileForReadOrWrite(tokens, currentDirectory, openFiles);
                }
                else
                {
                    printf("Invalid Format: open <filename> <r/w/rw/wr>\n");
                }
            }
            else if(strcmp("close", tokens->items[0]) == 0)
            {
                if(tokens->size == 2)
                {
                    openFiles = closeFileForReadOrWrite(tokens, currentDirectory, openFiles);
                }
                else
                {
                    printf("Invalid Format: close <filename>\n");
                }
            }
            else if(strcmp("mv", tokens->items[0]) == 0 && tokens->size == 3)
            {
                currentDirectory = MoveFileOrDirectory(imgFile, tokens, currentDirectory);
            }
            else if(strcmp("rm", tokens->items[0]) == 0 && tokens->size == 2) 
            {
                currentDirectory = removeFile(imgFile, tokens, currentDirectory);
            }
            else if(strcmp("write", tokens->items[0]) == 0 && tokens->size >= 4)
            {
                writeToOpenFile(imgFile, tokens, currentDirectory, openFiles);
            }
            else
            {
                printf("Invalid Command\n");
            }
            free(input);
            free_tokens(tokens);
        }
    }
    else
    {
        //Invalid User Input
        printf("Invalid Format Given: project3 fat32.img\n");
        return -1;
    }
    return 0;
}