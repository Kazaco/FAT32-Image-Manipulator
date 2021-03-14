#include "../Structs/tokenlist.h"
#include "../Structs/bios.h"
#include "../Helpers/utils.h"
#include "../Helpers/parser.h"
#include <stdio.h> 		//printf(), NULL
#include <stdlib.h>     //free(), realloc()

void getBIOSParamBlock(const char * imgFile)
{
    tokenlist * hex;
    char * littleEndian;

    //Calculate Bytes Per Sector
    hex = getHex(imgFile, 11, 2);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.BytsPerSec = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);

    //Calculate Sectors per Cluster
    hex = getHex(imgFile, 13, 1);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.SecPerClus = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);

    //Calculate Reserved Sector Count
    hex = getHex(imgFile, 14, 2);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.RsvdSecCnt = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);

    //Calculate number of FATs
    hex = getHex(imgFile, 16, 1);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.NumFATs = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);

    //Calculate total sectors
    hex = getHex(imgFile, 32, 4);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.TotSec32 = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);

    //Calculate FAT size
    hex = getHex(imgFile, 36, 4);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.FATSz32 = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);

    //Calculate Root Cluster
    hex = getHex(imgFile, 44, 4);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.RootClus = (unsigned int)strtol(littleEndian, NULL, 16);
    free_tokens(hex);
    free(littleEndian);
}

void printFATInfo()
{
    printf("=== Info ===\n");
    printf("Bytes Per Sector: %d\n", BPB.BytsPerSec);
    printf("Sectors per Cluster: %d\n", BPB.SecPerClus);
    printf("Reserved Sector Count: %d\n", BPB.RsvdSecCnt);
    printf("Number of FATs: %d\n", BPB.NumFATs);
    printf("Total Sectors: %d\n", BPB.TotSec32);
    printf("FAT size: %d\n", BPB.FATSz32);
    printf("Root Cluster: %d\n", BPB.RootClus);
}