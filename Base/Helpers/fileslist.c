#include "fileslist.h"
#include <stdlib.h>     //free(), realloc()

//Create a list containing all the files the user has opened for
//reading, writing, or both.
filesList * new_filesList(void)
{
    filesList * files = (filesList *) malloc(sizeof(filesList));
    files->size = 0;
    files->items = (FILEENTRY **) malloc(sizeof(FILEENTRY *));
    return files;
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