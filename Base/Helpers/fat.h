#ifndef _FAT_H
#define _FAT_H

unsigned int * findEmptyEntryInFAT(const char * imgFile, unsigned int * emptyArr);
unsigned int * findFatSectorInDir(const char * imgFile, unsigned int * fats, unsigned int clus);

#endif