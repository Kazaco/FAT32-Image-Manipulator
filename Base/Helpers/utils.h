#ifndef _UTILS_H
#define _UTILS_H
#include "../Structs/tokenlist.h"

int file_exists(const char * filename);
char * littleEndianHexStringFromTokens(tokenlist * hex);
tokenlist * getHex(const char * imgFile, int decStart, int size);

#endif