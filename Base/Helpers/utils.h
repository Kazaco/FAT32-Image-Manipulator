#ifndef _UTILS_H
#define _UTILS_H
#include "../Structs/tokenlist.h"

int file_exists(const char * filename);
tokenlist * getHex(const char * imgFile, int decStart, int size);
char * littleEndianHexStringFromTokens(tokenlist * hex);
char * littleEndianHexStringFromUnsignedChar(unsigned char * arr, int size);
void intToASCIIStringWrite(const char * imgFile, int value, unsigned int DataSector, int begin, int size);

#endif