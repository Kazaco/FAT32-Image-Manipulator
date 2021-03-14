#include "rm.h"
#include "../Structs/bios.h"
#include "../Helpers/utils.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fat.h"
#include "../Helpers/parser.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

//Remove file from current working directory.
dirlist * removeFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory)
{
    removeFileFromFATandDataRegion(imgFile, currentDirectory, tokens->items[1]);
    if (currentDirectory->CUR_Clus == 2) 
    {
        currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
    } 
    else 
    {
        currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
    }
    return currentDirectory;
}

//Delete FAT and Data Region attached to given file name.
void removeFileFromFATandDataRegion(const char * imgFile, dirlist * directory, const char * filename)
{
    //check if filename is within the directory
    int loc = -1;
    loc = dirlistIndexOfFileOrDirectory(directory,filename, 1);
    //case filename DNE or is a directory
    if(loc == -1){
        if(dirlistIndexOfFileOrDirectory(directory,filename,2) != -1){
            printf("File is a directory\n");
        }
        else{
            printf("File does not exist\n");
        }
    }
    else{
        //Beginning Locations for FAT and Data Sector
        unsigned int FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
        unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
        char * clusterHI = littleEndianHexStringFromUnsignedChar(directory->items[loc]->DIR_FstClusHI, 2);
        char * clusterLOW = littleEndianHexStringFromUnsignedChar(directory->items[loc]->DIR_FstClusLO, 2);
        strcat(clusterHI,clusterLOW);
        free(clusterLOW);
        unsigned int N = (unsigned int)strtol(clusterHI, NULL, 16);
        free(clusterHI);
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

            int i = 0;
            int file = open(imgFile, O_WRONLY);
            for(i; i < 512; i++)
            {
                //Go to offset position in file. ~SEEK_SET = Absolute position in document.
                lseek(file, DataSector, SEEK_SET);
                write(file, "\0", 1);
                DataSector += 1;
            }
            close(file);
            if(N != 0){
                intToASCIIStringWrite(imgFile,0,FatSector,0,4);
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
        unsigned int fats[2];
        unsigned int * fatsPtr;
        fats[0] = loc;
        fatsPtr = findFatSectorInDir(imgFile,fats,directory->CUR_Clus);
        unsigned int FatSectorDirCluster = fatsPtr[1];
        loc = fatsPtr[0];
        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
        DataSector += (FatSectorDirCluster - 2) * 512;
        DataSector += loc * 32;
        if(loc == directory->size -1){
            intToASCIIStringWrite(imgFile,0,DataSector,0,1);
            DataSector++;
            intToASCIIStringWrite(imgFile,0,DataSector,0,3);

        }
        else{
            intToASCIIStringWrite(imgFile,229,DataSector,0,1);
            DataSector++;
            intToASCIIStringWrite(imgFile,0,DataSector,0,3);
        }
    }
}