#ifndef _FILEENTRY_H
#define _FILEENTRY_H

typedef struct {
    unsigned char FILE_Name[11];
    unsigned int FILE_FstClus;
    char FILE_Mode[3];
    unsigned int FILE_OFFSET;
    unsigned int FILE_SIZE;
} FILEENTRY;

typedef struct {
	int size;
	FILEENTRY **items;
} filesList;

#endif