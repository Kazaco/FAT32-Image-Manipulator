#include "fat.h"
#include "../Structs/tokenlist.h"
#include "../Structs/bios.h"
#include "utils.h"
#include "parser.h"
#include "directorylist.h"
#include <stdio.h> 		//NULL
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_WRONLY
#include <unistd.h>     //lseek

//Find an empty locate in the FAT (File Allocation Table) for insertion.
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
    //Offset Location for N in FAT (Root = 2, 16392)
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

//Create a new file entry in the FAT and in the Data Region with given filename
void createFile(const char * imgFile, const char * filename, dirlist * directories, unsigned int previousCluster, int flag)
{
    tokenlist * hex;
    char * littleEndian;
    //NEED COPY of this value in-case we need to delete currentdirectory list!
    int N = directories->CUR_Clus;
    unsigned int emptyFATArr[2];
    unsigned int * emptyFATptr;
    unsigned int endClusterFATArr[2];
    unsigned int * endClusterFATptr;

    //Check if there is an empty space in current directory.
    int index = dirlistIndexOfFileOrDirectory(directories, "", 4);
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
        endClusterFATptr = findEndClusEntryInFAT(imgFile, directories->CUR_Clus, endClusterFATArr);

        //Create new end for current directory cluster.
        // 268435448 = 0xF8FFFF0F (uint 32, little endian)
        intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
        //Connect old end to new end of cluster.
        intToASCIIStringWrite(imgFile, emptyFATptr[0], endClusterFATArr[1], 0, 4);
    }

    //Do the index calculation again, if we failed previously.
    if(index == -1)
    {
        //Update directories list b/c we just extended it.
        free_dirlist(directories);
        directories = getDirectoryList(imgFile, N);
        //Find the empty entry.
        index = dirlistIndexOfFileOrDirectory(directories, "", 4);
    }

    unsigned int fats[2];
    unsigned int * fatsPtr;
    fats[0] = index;
    fatsPtr = findFatSectorInDir(imgFile, fats, directories->CUR_Clus);
    unsigned int FatSectorDirCluster = fatsPtr[1];
    index = fatsPtr[0];

    //Modify the Data Region
    unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
    DataSector += (FatSectorDirCluster - 2) * 512;
    //Offset for Empty Index Start
    DataSector += index * 32;

    //Allocate for file cluster in FAT
    emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
    intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
    //Open the file, we already checked that it exists. Obtain the file descriptor
    int file = open(imgFile, O_WRONLY);
    //Go to offset position in file. ~SEEK_SET = Absolute position in document.
    lseek(file, DataSector, SEEK_SET);
    //Write name of file to disk
    unsigned char name[11];
    strcpy(name, filename);
    strncat(name, "           ", 11 - strlen(filename));
    lseek(file, DataSector, SEEK_SET);
    //Only copy over strlen to avoid garbage data.
    write(file, &name, 11);
    close(file);
    // Write type of file to disk
    if(flag == 0)
    {
        //Create file
        intToASCIIStringWrite(imgFile, 32, DataSector + 11, 0, 1);
        // Write cluster of file to disk
        //HI
        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 20, 2, 2);
        //LOW
        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 26, 0, 2);
    }
    else if(flag == 1)
    {
        //Create directory
        intToASCIIStringWrite(imgFile, 16, DataSector + 11, 0, 1);
        // Write cluster of file to disk
        //HI
        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 20, 2, 2);
        //LOW
        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 26, 0, 2);
        //Made changes to local directory list
        free_dirlist(directories);
        directories = getDirectoryList(imgFile, N);

        //Find directory we just created.
        int newIndex = dirlistIndexOfFileOrDirectory(directories, filename, 2);
        //Calculate cluster value of DIRNAME
        char * clusterHI = littleEndianHexStringFromUnsignedChar(directories->items[newIndex]->DIR_FstClusHI, 2);
        char * clusterLOW = littleEndianHexStringFromUnsignedChar(directories->items[newIndex]->DIR_FstClusLO, 2);
        strcat(clusterHI,clusterLOW);
        unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
        //unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
        free(clusterHI);
        free(clusterLOW);

        //Create list of items
        dirlist * newDirItems = getDirectoryList(imgFile, clusterValHI);
        readDirectories(newDirItems);
        createFile(imgFile, ".", newDirItems, 0, 2);
        createFile(imgFile, "..", newDirItems, directories->CUR_Clus, 3);

        // Write cluster of file to disk
        //HI
        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 20, 2, 2);
        //LOW
        intToASCIIStringWrite(imgFile, emptyFATptr[0], DataSector + 26, 0, 2);
        free_dirlist(newDirItems);
    }
    else if(flag == 2)
    {
        //Create . Entry
        //Create directory
        intToASCIIStringWrite(imgFile, 16, DataSector + 11, 0, 1);
        // Write cluster of file to disk
        //HI
        intToASCIIStringWrite(imgFile, directories->CUR_Clus, DataSector + 20, 2, 2);
        //LOW
        intToASCIIStringWrite(imgFile, directories->CUR_Clus, DataSector + 26, 0, 2);
    }
    else if(flag == 3)
    {
        //Create .. Entry
        //Create directory
        intToASCIIStringWrite(imgFile, 16, DataSector + 11, 0, 1);
        // Write cluster of file to disk
        //HI
        intToASCIIStringWrite(imgFile, previousCluster, DataSector + 20, 2, 2);
        //LOW
        intToASCIIStringWrite(imgFile, previousCluster, DataSector + 26, 0, 2);
    }

    //Make changes to local directory list
    free_dirlist(directories);
    directories = getDirectoryList(imgFile, N);
}

unsigned int * findEndClusEntryInFAT(const char * imgFile, unsigned int clusterStart, unsigned int * endClusArr)
{
    //Reading hex from file.
    tokenlist * hex;
    char * littleEndian;
    //Find the end of current directory cluster.
    unsigned int FatSectorEndClusEndianVal = 0;
    unsigned int FatSectorEndClus = BPB.RsvdSecCnt * BPB.BytsPerSec + (clusterStart * 4);
    unsigned int FatSectorEndClusLoc = clusterStart;

    do
    {
        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSectorEndClus, 4);
        //Obtain Endian string, so we can determine if this is the last time we should read
        //from the FAT and search the data region.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEndClusEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
        //Deallocate hex and little Endian for FAT portion
        free_tokens(hex);
        free(littleEndian);

        //Set up data for new loop, or  quit.
        //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
        if((FatSectorEndClusEndianVal < 268435448 || FatSectorEndClusEndianVal > 4294967295) && FatSectorEndClusEndianVal != 0)
        {
            //We have to loop again in the FAT
            FatSectorEndClus = BPB.RsvdSecCnt * BPB.BytsPerSec;
            //New FAT Offset added
            FatSectorEndClus += FatSectorEndClusEndianVal * 4;
        }
    } while ((FatSectorEndClusEndianVal < 268435448 || FatSectorEndClusEndianVal > 4294967295) && FatSectorEndClusEndianVal != 0);
    //Returning last valid cluster value
    endClusArr[0] = FatSectorEndClusLoc;
    endClusArr[1] = FatSectorEndClus;
    return endClusArr;
}