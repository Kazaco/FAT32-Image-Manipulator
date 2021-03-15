#include "cp.h"
#include "../Structs/bios.h"
#include "../Helpers/parser.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fat.h"
#include "../Helpers/utils.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

dirlist * copyFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory)
{
    int loc = -1;
    int loc1 = -1;
    loc = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1],1);
    loc1 = dirlistIndexOfFileOrDirectory(currentDirectory,tokens->items[2],3);
    //Filename not in current directory
    if(loc == -1){
        printf("Specified filename does not exist.\n");
    }
    else if(strcmp(tokens->items[1],tokens->items[2]) == 0){
        printf("The name is already being used by another file %s\n", tokens->items[2]);
    }
        //Filename exists
    else{
        //TO DNE
        if(loc1 == -1){
            //Copy Filename in CWD and rename to TO
            unsigned int FatSector = 0;
            unsigned int DataSector = 0;
            unsigned int FatSector1 = 0;
            unsigned int DataSector1 = 0;
            FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
            DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
            FatSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec;
            DataSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
            unsigned int emptyFATArr[2];
            unsigned int emptyFATArr1[2];
            unsigned int * emptyFATptr;
            unsigned int endClusterFATArr[2];
            unsigned int * endClusterFATptr;
            unsigned int N = currentDirectory->CUR_Clus;
            //Check if there is an empty space in current directory.
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, "", 4);
            if(index != -1)
            {
                //We found an empty entry. We don't need to extend the FAT region for this cluster.
            }
            else
            {
                //No more empty entries in this directory, need to extend the FAT
                //Read FAT from top until we find an empty item
                //arrPtr[0] : FAT Sector Empty Entry Loc
                //arrPtr[1] : FAT Sector Empty End
                emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);

                //endClusArr[0] : FAT Sector Clus End Loc
                //endClusArr[1] : FAT Sector Clus End
                endClusterFATptr = findEndClusEntryInFAT(imgFile, currentDirectory->CUR_Clus, endClusterFATArr);

                //Create new end for current directory cluster.
                // 268435448 = 0xF8FFFF0F (uint 32, little endian)
                intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                //Connect old end to new end of cluster.
                intToASCIIStringWrite(imgFile, emptyFATptr[0], endClusterFATArr[1], 0, 4);
            }

            //Do the index calculation again, if we failed previously
            if(index == -1)
            {
                //Update directories list b/c we just extended it.
                free_dirlist(currentDirectory);
                currentDirectory = getDirectoryList(imgFile, N);

                //Find the empty entry.
                index = dirlistIndexOfFileOrDirectory(currentDirectory, "", 4);
            }
            emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
            char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusHI, 2);
            char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusLO, 2);
            strcat(clusterHI,clusterLOW);
            free(clusterLOW);
            unsigned int X = (unsigned int)strtol(clusterHI, NULL, 16);
            free(clusterHI);
            FatSector += X * 4;
            DataSector += (X - 2) * 512;
            FatSector1 += emptyFATptr[0] * 4;
            DataSector1 += (emptyFATptr[0] - 2) * 512;
            unsigned int FatSectorEndianVal = 0;
            unsigned int DataSectorEndianVal = 0;
            tokenlist * hex;
            char * littleEndian;
            unsigned int lo = emptyFATptr[0];
            unsigned int * emptyFATptr1;
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

                int file = open(imgFile, O_RDWR);
                lseek(file, DataSector, SEEK_SET);
                char clusData[512];
                strcpy(clusData, "");
                read(file, clusData,512);

                int i = 0;
                for(i; i < 512;i++){
                    lseek(file,DataSector1+i, SEEK_SET);
                    char letterString[1] = {clusData[i]};
                    write(file,letterString,1);
                }
                close(file);

                //Set up data for new loop, or  quit.
                //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
                if((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0)
                {
                    emptyFATptr1 = findEmptyEntryInFATNext(imgFile, emptyFATArr1);
                    intToASCIIStringWrite(imgFile, emptyFATptr1[0], emptyFATptr[1], 0, 4);
                    emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
                    emptyFATptr1 = emptyFATptr;
                    //We have to loop again, reset FAT/Data regions.
                    FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                    DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                    FatSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec;
                    DataSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                    //New FAT Offset added
                    FatSector += FatSectorEndianVal * 4;
                    //New Data Sector Offset Added
                    DataSector += (FatSectorEndianVal - 2) * 512;
                    FatSector1 += emptyFATptr[0] * 4;
                    DataSector1 += (emptyFATptr[0] - 2) * 512;
                }
                else
                {
                    intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                }
            } while ((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0);
            unsigned int fats[2];
            unsigned int * fatsPtr;
            fats[0] = index;
            fatsPtr = findFatSectorInDir(imgFile, fats, currentDirectory->CUR_Clus);
            unsigned int FatSectorDirCluster = fatsPtr[1];
            index = fatsPtr[0];

            //Modify the Data Region
            DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
            //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
            DataSector += (FatSectorDirCluster - 2) * 512;
            //Offset for Empty Index Start
            DataSector += index * 32;

            //Open the file, we already checked that it exists. Obtain the file descriptor
            int file = open(imgFile, O_WRONLY);
            //Go to offset position in file. ~SEEK_SET = Absolute position in document.
            lseek(file, DataSector, SEEK_SET);
            //Write name of file to disk
            unsigned char name[11];
            strcpy(name, tokens->items[2]);
            strncat(name, "           ", 11 - strlen(tokens->items[2]));
            lseek(file, DataSector, SEEK_SET);
            //Only copy over strlen to avoid garbage data.
            write(file, &name, 11);
            //Create file
            intToASCIIStringWrite(imgFile, 32, DataSector + 11, 0, 1);
            // Write cluster of file to disk
            //HI
            intToASCIIStringWrite(imgFile, lo, DataSector + 20, 2, 2);
            //LOW
            intToASCIIStringWrite(imgFile, lo, DataSector + 26, 0, 2);
            intToASCIIStringWrite(imgFile,(unsigned int)strtol(littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FileSize,4), NULL, 16) , DataSector + 28, 0, 4);

            close(file);
            currentDirectory = getDirectoryList(imgFile,currentDirectory->CUR_Clus);
        }
            //case TO already exits in CWD as a file
        else if((loc1 = dirlistIndexOfFileOrDirectory(currentDirectory,tokens->items[2],1)) != -1){
            printf("Cannot copy a file to another file.\n");
        }
            //case TO exists in CWD as a directory
        else{
            loc1 = dirlistIndexOfFileOrDirectory(currentDirectory,tokens->items[2],2);
            char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc1]->DIR_FstClusHI, 2);
            char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc1]->DIR_FstClusLO, 2);
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
            int same = dirlistIndexOfFileOrDirectory(to,tokens->items[1],3);
            if(same == -1){
                unsigned int FatSector = 0;
                unsigned int DataSector = 0;
                unsigned int FatSector1 = 0;
                unsigned int DataSector1 = 0;
                FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                FatSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec;
                DataSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                unsigned int emptyFATArr[2];
                unsigned int emptyFATArr1[2];
                unsigned int * emptyFATptr;
                unsigned int endClusterFATArr[2];
                unsigned int * endClusterFATptr;
                unsigned int N = to->CUR_Clus;
                //Check if there is an empty space in current directory.
                int index = dirlistIndexOfFileOrDirectory(to, "", 4);
                if(index != -1)
                {
                    //We found an empty entry. We don't need to extend the FAT region for this cluster.
                }
                else
                {
                    //No more empty entries in this directory, need to extend the FAT
                    //Read FAT from top until we find an empty item
                    //arrPtr[0] : FAT Sector Empty Entry Loc
                    //arrPtr[1] : FAT Sector Empty End
                    emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);

                    //endClusArr[0] : FAT Sector Clus End Loc
                    //endClusArr[1] : FAT Sector Clus End
                    endClusterFATptr = findEndClusEntryInFAT(imgFile, to->CUR_Clus, endClusterFATArr);

                    //Create new end for current directory cluster.
                    // 268435448 = 0xF8FFFF0F (uint 32, little endian)
                    intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                    //Connect old end to new end of cluster.
                    intToASCIIStringWrite(imgFile, emptyFATptr[0], endClusterFATArr[1], 0, 4);
                }

                //Do the index calculation again, if we failed previously
                if(index == -1)
                {
                    //Update directories list b/c we just extended it.
                    free_dirlist(to);
                    to = getDirectoryList(imgFile, N);

                    //Find the empty entry.
                    index = dirlistIndexOfFileOrDirectory(to, "", 4);
                }
                emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
                char * clusterH = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusHI, 2);
                char * clusterL = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FstClusLO, 2);
                strcat(clusterH,clusterL);
                free(clusterL);
                unsigned int X = (unsigned int)strtol(clusterH, NULL, 16);
                free(clusterH);
                FatSector += X * 4;
                DataSector += (X - 2) * 512;
                FatSector1 += emptyFATptr[0] * 4;
                DataSector1 += (emptyFATptr[0] - 2) * 512;
                unsigned int FatSectorEndianVal = 0;
                unsigned int DataSectorEndianVal = 0;
                tokenlist * hex;
                char * littleEndian;
                unsigned int lo = emptyFATptr[0];
                unsigned int * emptyFATptr1;

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
                    int file = open(imgFile, O_RDWR);
                    lseek(file, DataSector, SEEK_SET);
                    char clusData[512];
                    strcpy(clusData, "");
                    read(file, clusData,512);

                    int i = 0;
                    for(i; i < 512;i++){
                        lseek(file,DataSector1+i, SEEK_SET);
                        char letterString[1] = {clusData[i]};
                        write(file,letterString,1);
                    }
                    close(file);

                    //Set up data for new loop, or  quit.
                    //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
                    if((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0)
                    {

                        emptyFATptr1 = findEmptyEntryInFATNext(imgFile, emptyFATArr1);
                        intToASCIIStringWrite(imgFile, emptyFATptr1[0], emptyFATptr[1], 0, 4);
                        emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
                        emptyFATptr1 = emptyFATptr;
                        //We have to loop again, reset FAT/Data regions.
                        FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        FatSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec;
                        DataSector1 = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        //New FAT Offset added
                        FatSector += FatSectorEndianVal * 4;
                        //New Data Sector Offset Added
                        DataSector += (FatSectorEndianVal - 2) * 512;
                        FatSector1 += emptyFATptr[0] * 4;
                        DataSector1 += (emptyFATptr[0] - 2) * 512;
                    }
                    else
                    {
                        intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                        //This should be our last iteration. Do nothing.
                    }
                } while ((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0);
                unsigned int fats[2];
                unsigned int * fatsPtr;
                fats[0] = index;
                fatsPtr = findFatSectorInDir(imgFile, fats, to->CUR_Clus);
                unsigned int FatSectorDirCluster = fatsPtr[1];
                index = fatsPtr[0];

                //Modify the Data Region
                DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                DataSector += (FatSectorDirCluster - 2) * 512;
                //Offset for Empty Index Start
                DataSector += index * 32;

                //Open the file, we already checked that it exists. Obtain the file descriptor
                int file = open(imgFile, O_WRONLY);
                //Go to offset position in file. ~SEEK_SET = Absolute position in document.
                lseek(file, DataSector, SEEK_SET);
                //Write name of file to disk
                unsigned char name[11];
                strcpy(name, tokens->items[1]);
                strncat(name, "           ", 11 - strlen(tokens->items[1]));
                lseek(file, DataSector, SEEK_SET);
                //Only copy over strlen to avoid garbage data.
                write(file, &name, 11);
                //Create file
                intToASCIIStringWrite(imgFile, 32, DataSector + 11, 0, 1);
                // Write cluster of file to disk
                //HI
                intToASCIIStringWrite(imgFile, lo, DataSector + 20, 2, 2);
                //LOW
                intToASCIIStringWrite(imgFile, lo, DataSector + 26, 0, 2);
                intToASCIIStringWrite(imgFile,(unsigned int)strtol(littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc]->DIR_FileSize,4), NULL, 16) , DataSector + 28, 0, 4);
                close(file);
                to = getDirectoryList(imgFile,to->CUR_Clus);
            }
            else{
                printf("The name is already being used by another file %s\n", tokens->items[1]);
            }
        }
    }
    return currentDirectory;
}

unsigned int * findEmptyEntryInFATNext(const char * imgFile, unsigned int * emptyArr)
{
    //Reading hex from file.
    tokenlist * hex;
    char * littleEndian;
    //Read FAT from top until we find an empty item. We start at the root directory,
    //so offset will automatically be 2 when we start.
    unsigned int FatSectorEmptyEndianVal = 0;
    unsigned int FatSectorEmpty = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.RootClus * 4);
    unsigned int emptyEntryLoc = 2;
    int next = 0;

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
        else
        {
            FatSectorEmpty += 4;
            emptyEntryLoc += 1;
            next--;
        }
        
    } while (next != -1);

    //Return data
    emptyArr[0] = emptyEntryLoc;
    emptyArr[1] = FatSectorEmpty;
    return emptyArr;
}
