#include "lseek.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fileslist.h"
#include <stdio.h> 		//printf()

void seekNewPositonInOpenFile(tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles)
{
    if (tokens->size < 3)
    {
        printf("ERROR: requires <filename><offset> parameters \n");
    }
    else if(tokens->size > 3)
    {
        printf("ERROR: requires <filename><offset> parameters \n");
    }
    //assuming a file needs to be opened to allow a lseek operation
    else
    {
        //check if file is open in either read/write mode
        if (openFileIndex(openFiles, tokens, 3) == -1)
        {
            //file isnt open at all!
            printf("ERROR: File must be opened in either read/write mode before seeking \n");
        }
        else
        {
            //check for read first
            //if this fails, then file is write open
            int index = openFileIndex(openFiles, tokens, 1);
            if (index == -1)
            {
                index = openFileIndex(openFiles, tokens, 2);
            }
            unsigned int offset = seekFAT(tokens, currentDirectory, openFiles, index);
            if (offset != -1)
            {
                openFiles->items[index]->FILE_OFFSET = offset;
                printf("New offset for file %s: %u\n", tokens->items[1], openFiles->items[index]->FILE_OFFSET);
            }
        }
    }
}

unsigned int seekFAT(tokenlist * tokens, dirlist * directories, filesList * curFiles, unsigned int index2)
{
	//first check if file exists
	int index = dirlistIndexOfFileOrDirectory(directories, tokens->items[1], 1);
	if(createOpenFileEntry(curFiles, directories, tokens, index) == 0){printf("issue\n"); return -1;}
	if (index == -1)
	{
		printf("ERROR: File does not exist!\n");
		return -1;
	}
	//check next if it is a directory
 	else if (dirlistIndexOfFileOrDirectory(directories, tokens->items[1], 2) > 0)
	{
		printf("ERROR: File is a directory!\n");
		return -1;
	}
	else
	{
		unsigned int fileSize = curFiles->items[index2]->FILE_SIZE;
		unsigned int OFFSET = (unsigned int)strtol(tokens->items[2], NULL, 10);
		unsigned int currentPos = curFiles->items[index2]->FILE_OFFSET;
		//if we are trying to seek more than filesize, do not allow
		if (OFFSET > fileSize)
		{
			printf("ERROR: Offset has attempted to exceed current file size\n");
			return -1;
		}
		else if (currentPos == OFFSET)
		{
			printf("Parameter given for offset is the same as current file offset.\n");
			return -1;
		}
		else
		{
			return OFFSET;
		}
	}
	//if you are here, something went wrong
	printf("ERROR: You discovered a bug \n");
	return -1;
}