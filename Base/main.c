#include "Helpers/utils.h"
#include "Helpers/parser.h"
#include "Commands/info.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()

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
        getBIOSParamBlock(imgFile);

        //MISSING
        //Make the User Start in the Root
		//dirlist * currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
		//Let the user have a container to interact w/ files no matter where they are in file system.
		//filesList * openFiles = new_filesList();

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
                // free_dirlist(currentDirectory);
                // free_filesList(openFiles);
                break;
            }
            else if(strcmp("info", tokens->items[0]) == 0 && tokens->size == 1)
            {
                printFATInfo();
            }
            else if(strcmp("ls", tokens->items[0]) == 0 && (tokens->size == 1 || tokens->size == 2))
            {
                printLSCommand();
            }
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