#ifndef _DIRENTRY_H
#define _DIRENTRY_H
#include <unistd.h>     //packed

struct DIRENTRY{
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth;
    unsigned char DIR_CrtTime[2];
    unsigned char DIR_CrtDate[2];
    unsigned char DIR_LstAccDate[2];
    unsigned char DIR_FstClusHI[2];
    unsigned char DIR_WrtTime[2];
    unsigned char DIR_WrtDate[2];
    unsigned char DIR_FstClusLO[2];
    unsigned char DIR_FileSize[4];
} __attribute__((packed));
typedef struct DIRENTRY DIRENTRY;

typedef struct {
	int size;
    unsigned int CUR_Clus;
	DIRENTRY **items;
} dirlist;

#define FILENAME 1
#define DIRECTORY 2
#define FILEORDIR 3
#define EMPTY 4

#endif