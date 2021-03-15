#include "directorylist.h"
#include "../Structs/tokenlist.h"
#include "../Structs/bios.h"
#include "utils.h"
#include "parser.h"
#include "fat.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_RDONLY

//Create a struct to contain the information for each file
//from the data region in our FAT32 file system.
dirlist * new_dirlist()
{
    dirlist * dirs = (dirlist *) malloc(sizeof(dirlist));
	dirs->size = 0;
	dirs->items = (DIRENTRY **) malloc(sizeof(DIRENTRY *));
	return dirs;
}

//Deallocation of this struct.
void free_dirlist(dirlist * directories)
{
    int i = 0;
	for (i; i < directories->size; i++)
    {
        free(directories->items[i]);
    }
	free(directories);
}

//Obtain the directory list for the Nth cluster of the FAT
dirlist * getDirectoryList(const char * imgFile, unsigned int N)
{
    //Beginning Locations for FAT and Data Sector
    unsigned int FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
    unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
    //Offset Location for N in FAT (Root = 2, 16392)
    FatSector += N * 4;
    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
    DataSector += (N - 2) * 512;
    //Ending Vals (Use in next N calculation)
    unsigned int FatSectorEndianVal = 0;
    unsigned int DataSectorEndianVal = 0;
    //Reading Hex Values from the FAT and Data Sector
    tokenlist * hex;
    char * littleEndian;
    int flag = 0;
    //Store List of Directories in whatever folder given by user
    dirlist * dirs = new_dirlist();
    dirs->CUR_Clus = N;

    do
    {
        //Read the FAT until we are at the end of the chosen cluster (N). This will tell us
        //the data sectors we should go to in the data region of sector size 512.
        //We have already positioned ourselves in the *first* position with previous math.

        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSector, 4);
        //Obtain Endian string, so we can determine if this is the last time we should read
        //from the FAT and search the data region.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);

        //Deallocate hex and little Endian for FAT portion
        free_tokens(hex);
        free(littleEndian);

        //Read Hex at Data Sector Position. We do this 16 times b/c a file size is 32 and
        //512 / 32 is 16. There can be at most 16 files in 1 sector.
        int i = 0;
        for(i; i < 16; i++)
        {
            //Open the file, we already checked that it exists. Obtain the file descriptor
            int file = open(imgFile, O_RDONLY);
            //Go to offset position in file. ~SEEK_SET = Absolute position in document.
            lseek(file, DataSector, SEEK_SET);
            //Create structure for Directory List
            dirs->items = (DIRENTRY **) realloc(dirs->items, (dirs->size + 1) * sizeof(DIRENTRY));
            dirs->items[dirs->size] = malloc(sizeof(DIRENTRY));
            //Read from the file 'size' number of bits from decimal position given.
            //We'll convert those bit values into hex, and insert into our hex token list.
            read(file, dirs->items[dirs->size], 32);
            dirs->size += 1;
            DataSector += 32;
            close(file);
        }

        //Set up data for new loop, or  quit.
        //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
        if((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0)
        {
            //We have to loop again, reset FAT/Data regions.
            FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
            DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
            //New FAT Offset added
            FatSector += FatSectorEndianVal * 4;
            //New Data Sector Offset Added
            DataSector += (FatSectorEndianVal - 2) * 512;
        }
    } while ((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0);

    //Allocate Empty files already given by fat32.img
    int i = 0;
    for (i; i < dirs->size; i++)
    {
        //Test the files we found in dirs. Allocate FAT space to the file if it doesn't have a cluster value.
        //Empty File leading byte = E5 or 00
        //Note: uint8 0 = 0, uint8 E5 = 229
        if(dirs->items[i]->DIR_Name[0] != 0 && dirs->items[i]->DIR_Name[0] != 229)
        {
            //Check if LONGFILE is one of our entries.
            //LONGFILE Byte is not:
            // 1. ATTR_DIRECTORY 0x10 = 16
            // 2. ATTR_ARCHIVE 0x20 = 32
            if( (dirs->items[i]->DIR_Attr & 0x10) != 0 || (dirs->items[i]->DIR_Attr & 0x20) != 0)
            {
                //Only files can be unallocated.
                if((dirs->items[i]->DIR_Attr & 0x20) != 0)
                {
                    //Check if the file has 0 clusters allocated.
                    //Calculate cluster value of DIRNAME
                    char * clusterHI = littleEndianHexStringFromUnsignedChar(dirs->items[i]->DIR_FstClusHI, 2);
                    char * clusterLOW = littleEndianHexStringFromUnsignedChar(dirs->items[i]->DIR_FstClusLO, 2);
                    strcat(clusterHI,clusterLOW);
                    unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
                    free(clusterHI);
                    free(clusterLOW);
                    //We have to allocate
                    if(clusterValHI == 0)
                    {
                        unsigned int emptyFATArr[2];
                        unsigned int * emptyFATptr;
                        //Need to create FAT entry for this file.
                        int index = dirlistIndexOfFileOrDirectory(dirs, dirs->items[i]->DIR_Name, FILENAME);
                        //Read FAT from top until we find an empty item
                        //arrPtr[0] : FAT Sector Empty Entry Loc
                        //arrPtr[1] : FAT Sector Empty End
                        emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
                        intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                        //Modify directory that we allocated space to. Use findFatSectorInDir() to
                        //make sure we are modifying the right file.
                        unsigned int fats[2];
                        unsigned int * fatsPtr;
                        fats[0] = index;
                        fatsPtr = findFatSectorInDir(imgFile, fats, dirs->CUR_Clus);
                        unsigned int FatSectorDirCluster = fatsPtr[1];
                        index = fatsPtr[0];
                        //Modify the Data Region
                        unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                        DataSector += (FatSectorDirCluster - 2) * 512;
                        //Offset for Empty Index Start
                        DataSector += index * 32;
                        //Write Cluster number to file
                        //HI
                        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 20, 2, 2);
                        //LOW
                        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 26, 0, 2);
                        //Flag that we need to read again.
                        flag = 1;
                    }
                }
            }
        }
    }
    //Re-read the file again for the cluster values. Since we had to write to it for allocating space to empty files given.
    if(flag == 1)
    {
        getDirectoryList(imgFile, N);
    }
    else
    {
        return dirs;
    }
}

//Obtain the index of a file if it is in our current directory.
int dirlistIndexOfFileOrDirectory(dirlist * directories, const char * item, int flag)
{
    //Input Flags
    //1 - File
    //2 - Directory
    //3 - Either File or Directory
    //4 - Empty

    //Check if given char * is in our given directory. Modify input string
    //with spaces to compare with items in the FAT32.
    int i = 0;
    int found = -1;
    unsigned char name[11];
    strcpy(name, item);
    int j = strlen(item);
    for(j; j < 11; j++)
    {
        strcat(name, " ");
    }

    //Searching our current directory.
    for(i; i < directories->size; i++)
    {
        //Compare only up to only strlen(item).
        if(strncmp(directories->items[i]->DIR_Name, name, strlen(name)) == 0 )
        {
            //Checking that the item is a directory.
            if( ((directories->items[i]->DIR_Attr & 0x10) != 0) && (flag == 2 || flag == 3))
            {
                //Found directory.
                found = i;
                break;
            }
            //Checking that the item is a file.
            else if( ((directories->items[i]->DIR_Attr & 0x20) != 0) && (flag == 1 || flag == 3))
            {
                //Found file.
                found = i;
                break;
            }
        }

        //Empty Entry
        if((directories->items[i]->DIR_Name[0] == 0 || directories->items[i]->DIR_Name[0] == 229) && flag == 4)
        {
            found = i;
            break;
        }
    }
    //Return index of file/directory/empty if it is found. Val = -1 if not found.
    return found;
}

//Show all files in a directory that arent lead by a longfile byte.
void readDirectories(dirlist * readEntry)
{
    //Reading Directory
    int i = 0;
    for (i; i < readEntry->size; i++)
    {
        //Test the files we found in readEntry.
        //If its empty, we don't want to show it to
        //the user.

        //Empty File leading byte = E5 or 00
        //Note: uint8 0 = 0, uint8 E5 = 229
        if(readEntry->items[i]->DIR_Name[0] != 0 && readEntry->items[i]->DIR_Name[0] != 229)
        {
            //Check if LONGFILE is one of our entries.
            //LONGFILE Byte is not:
            // 1. ATTR_DIRECTORY 0x10 = 16
            // 2. ATTR_ARCHIVE 0x20 = 32
            if( (readEntry->items[i]->DIR_Attr & 0x10) != 0 || (readEntry->items[i]->DIR_Attr & 0x20) != 0)
            {
                if((readEntry->items[i]->DIR_Attr & 0x10) != 0)
                {
                    printf("(dir) %s\n", readEntry->items[i]->DIR_Name);
                }
                else
                {
                    printf("(file) %s\n", readEntry->items[i]->DIR_Name);
                }
            }
        }
    }
}