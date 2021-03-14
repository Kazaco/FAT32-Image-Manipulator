#include "fat.h"
#include "../Structs/tokenlist.h"
#include "../Structs/bios.h"
#include "utils.h"
#include "parser.h"
#include <stdio.h> 		//NULL
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

unsigned int * findEmptyEntryInFAT(const char * imgFile, unsigned int * emptyArr)
{
    //Reading hex from file.
    tokenlist * hex;
    char * littleEndian;

    //Read FAT from top until we find an empty item. We start at the root directory,
    //so offset will automatically be 2 when we start.
    unsigned int FatSectorEmptyEndianVal = 0;
    unsigned int FatSectorEmpty = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.RootClus * 4);
    unsigned int emptyEntryLoc = 2;

    do
    {
        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSectorEmpty, 4);
        //Obtain Endian string, so we can determine if this is an empty entry.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEmptyEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
        //Deallocate hex and little Endian for FAT portion
        free(littleEndian);
        free_tokens(hex);
        //Iterate
        if(FatSectorEmptyEndianVal != 0)
        {
            //Iterate
            FatSectorEmpty += 4;
            emptyEntryLoc += 1;
        }
    } while (FatSectorEmptyEndianVal != 0);

    //Return data
    emptyArr[0] = emptyEntryLoc;
    emptyArr[1] = FatSectorEmpty;
    return emptyArr;
}

unsigned int * findFatSectorInDir(const char * imgFile, unsigned int * fats, unsigned int clus)
{
    tokenlist * hex;
    char * littleEndian;
    int loc = fats[0];

    //Do math to calculate the FAT sector we should iterate to get
    //the right data region we should modify.
    int FATIterateNum = 0;
    while(loc > 15)
    {
        loc -= 16;
        FATIterateNum++;
    }
    fats[0] = loc;

    //Beginning Locations for FAT and Data Sector
    unsigned int FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
    unsigned int FatSectorEndianVal = 0;
    unsigned int FatSectorDirCluster = clus;
    // //Offset Location for N in FAT (Root = 2, 16392)
    FatSector += clus * 4;
    //Need to iterate thorugh FAT again if empty folder is in another FAT entry other than the first.
    while(FATIterateNum != 0)
    {
        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSector, 4);
        //Obtain Endian string, so we can determine if this is the last time we should read
        //from the FAT and search the data region.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
        //Deallocate hex and little Endian for FAT portion
        free_tokens(hex);
        free(littleEndian);

        //Set up data for new loop, or  quit.
        //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
        if(FATIterateNum != 0)
        {
            //Need to move in FAT again.
            FATIterateNum--;
            //Move Dir Cluster we need to look at.
            FatSectorDirCluster = FatSectorEndianVal;
            //We have to loop again in the FAT
            FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
            //New FAT Offset added
            FatSector += FatSectorEndianVal * 4;
        }
    }
    fats[1] =  FatSectorDirCluster;
    return fats;
}