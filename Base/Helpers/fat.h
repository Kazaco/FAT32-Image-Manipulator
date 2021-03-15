#ifndef _FAT_H
#define _FAT_H
#include "../Structs/direntry.h"

unsigned int * findEmptyEntryInFAT(const char * imgFile, unsigned int * emptyArr);
unsigned int * findFatSectorInDir(const char * imgFile, unsigned int * fats, unsigned int clus);
void createFile(const char * imgFile, const char * filename, dirlist * directories, unsigned int previousCluster, int flag);
unsigned int * findEndClusEntryInFAT(const char * imgFile, unsigned int clusterStart, unsigned int * endClusArr);

#endif