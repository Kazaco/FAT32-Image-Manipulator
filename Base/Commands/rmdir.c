#include "rmdir.h"
#include "../Structs/bios.h"
#include "../Helpers/parser.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fat.h"
#include "../Helpers/utils.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()

dirlist * removeDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory)
{
    int loc = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1],3);
    if( loc == -1){
        printf("DIRNAME does not exist\n");
    }
    else{
        loc = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1],2);
        if(loc == -1){
            printf("DIRNAME is not a directory\n");
        }
        else{
            char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusHI, 2);
            char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusLO, 2);
            strcat(clusterHI,clusterLOW);
            unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
            dirlist * to;
            if(clusterValHI == 0){
                to = getDirectoryList(imgFile, BPB.RootClus);
            }else{
                to = getDirectoryList(imgFile, clusterValHI);
            }
            free(clusterHI);
            free(clusterLOW);
            int empty = dirlistIndexOfFileOrDirectory(to,"",4);
            if(empty != 2){
                printf("DIRNAME is not empty\n");
            }
            else{

                unsigned int FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                char * clusterH = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusHI, 2);
                char * clusterL = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusLO, 2);
                strcat(clusterH,clusterL);
                free(clusterL);
                unsigned int N = (unsigned int)strtol(clusterH, NULL, 16);
                free(clusterH);
                //Offset Location for N in FAT (Root = 2, 16392)
                FatSector += N * 4;
                //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                DataSector += (N - 2) * 512;
                //Ending Vals (Use in next N calculation)
                unsigned int FatSectorEndianVal = 0;
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
                fatsPtr = findFatSectorInDir(imgFile,fats,currentDirectory->CUR_Clus);
                unsigned int FatSectorDirCluster = fatsPtr[1];
                loc = fatsPtr[0];
                DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                DataSector += (FatSectorDirCluster - 2) * 512;
                DataSector += loc * 32;
                if(loc == currentDirectory->size -1){
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
            if(currentDirectory->CUR_Clus == 2){
                currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
            }
            else{
                currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
            }
        }
    }
    //Return updated directory with folder deleted.
    return currentDirectory;
}