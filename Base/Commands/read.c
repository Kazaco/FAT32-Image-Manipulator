#include "read.h"
#include "../Structs/bios.h"
#include "../Helpers/utils.h"
#include "../Helpers/parser.h"
#include "../Helpers/fileslist.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

void readToOpenFile(const char * imgFile, tokenlist * tokens, filesList * openFiles)
{
    if(tokens->size < 3)
    {
        printf("ERROR: requires <filename><size> parameters \n");
    }
    else if(tokens->size > 3)
    {
        printf("ERROR: requires <filename><offset> parameters \n");
    }
    else
    {
        char * result = readFAT(imgFile, tokens, openFiles);
        if (result != NULL)
        {
            printf("%s\n", result);
        }
        free(result);
    }
}

char * readFAT(const char * imgfile, tokenlist * tokens, filesList * openFiles)
{
	int openIndex = openFileIndex(openFiles, tokens, 1);
	if (openIndex == -1) {printf("ERROR: File does not exist or needs to be in read mode\n"); return NULL;}
	unsigned int readSize = atoi(tokens->items[2]);
	char * returnString = malloc(sizeof(char) * readSize + 1);
    strcpy(returnString, "");

	//Check our allocation for the file
	int fileFATAllocation = 0;
    int fileDataAllocation = 0;

	int readStartVal = openFiles->items[openIndex]->FILE_OFFSET;
    int readEndVal = openFiles->items[openIndex]->FILE_OFFSET + atoi(tokens->items[2]);
	if (readStartVal + atoi(tokens->items[2]) > openFiles->items[openIndex]->FILE_SIZE)
    {
        readEndVal = openFiles->items[openIndex]->FILE_SIZE;
    }
	//Check how many FAT/data regions blocks are allocated for the given file. First check modulo
    //to know how we should calculate edge cases.
    if(openFiles->items[openIndex]->FILE_SIZE % BPB.BytsPerSec == 0 && openFiles->items[openIndex]->FILE_SIZE != 0)
    {
        //Completely filled data region in last block.
        fileFATAllocation = openFiles->items[openIndex]->FILE_SIZE / BPB.BytsPerSec;
        fileDataAllocation = fileFATAllocation * 512;
    }
    else
    {
        //Partially filled data region in last block.
        fileFATAllocation = (openFiles->items[openIndex]->FILE_SIZE / BPB.BytsPerSec) + 1;
        fileDataAllocation = fileFATAllocation * 512;
    }

    unsigned int FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
    unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
    unsigned int bitsLeftToWrite = atoi(tokens->items[2]);
    FatSector += openFiles->items[openIndex]->FILE_FstClus * 4;
    DataSector += (openFiles->items[openIndex]->FILE_FstClus - 2) * 512;
    unsigned int FatSectorEndianVal = 0;
    unsigned int bitsLeftToRead = atoi(tokens->items[2]);
    unsigned int ReadPos = 0;
    tokenlist * hex;
    char * littleEndian;

    do 
    {
        hex = getHex(imgfile, FatSector, 4);
        //Obtain Endian string, so we can determine if this is the last time we should read
        //from the FAT and search the data region.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
        //Deallocate hex and little Endian for FAT portion
        free_tokens(hex);
        free(littleEndian);
        if (readStartVal >= 0 && readStartVal < 512)
        {
            //while within data section, read
            while (readStartVal < 512 && bitsLeftToRead != 0)
            {
                char readboi;
                int file = open(imgfile, O_RDONLY);
                lseek(file, DataSector + readStartVal, SEEK_SET);
                read(file, &readboi, sizeof(char));
                strncat(returnString, &readboi, 1);
                ReadPos++;
                readStartVal++;
                bitsLeftToRead--;
                close(file);
            }
        }

        if (bitsLeftToRead != 0)
        {
            FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
            DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
            //New FAT Offset added
            FatSector += FatSectorEndianVal * 4;
            //New Data Sector Offset Added
            DataSector += (FatSectorEndianVal - 2) * 512;
            //New Offset for FAT
            readStartVal -= 512;
        }
    } while(bitsLeftToRead != 0);

    //Update lseek info (should now be offset + size)
    openFiles->items[openIndex]->FILE_OFFSET = readEndVal;
	return returnString;
}