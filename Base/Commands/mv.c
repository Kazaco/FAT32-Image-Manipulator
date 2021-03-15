#include "mv.h"
#include "../Structs/bios.h"
#include "../Helpers/utils.h"
#include "../Helpers/directorylist.h"
#include "../Helpers/fat.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

//Allows the User to rename files/directories or move a file or directory into another directory.
dirlist * MoveFileOrDirectory(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory)
{
    int file = open(imgFile, O_WRONLY);
    unsigned int DataSector;

    //check if currentdir is root dir
    if(currentDirectory->CUR_Clus == 2 && strcmp(".", tokens->items[1]) == 0)
    {
        printf("No such file or directory\n");
    }
    //case TO exists as directory
    else if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2], 2) != -1)
    {
        int loc = -1;
        loc = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 3);
        if(loc != -1)
        {
            int loc1 = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2],2);
            char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc1]->DIR_FstClusHI, 2);
            char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[loc1]->DIR_FstClusLO, 2);
            strcat(clusterHI,clusterLOW);
            unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
            dirlist * to;
            if(clusterValHI == 0)
            {
                to = getDirectoryList(imgFile, BPB.RootClus);
            }else
            {
                to = getDirectoryList(imgFile, clusterValHI);
            }
            free(clusterHI);
            free(clusterLOW);
            //case FROM is a directory
            if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 2) != -1)
            {
                //makes a new dirlist for the found To directory
                //case the FROM is .. pointing to root directory
                if(currentDirectory->CUR_Clus == 2 && strcmp("..", tokens->items[1]) == 0)
                {
                    printf("No such file or directory\n");
                }
                else
                {
                    int index = dirlistIndexOfFileOrDirectory(to, tokens->items[1], 2);
                    //Check if given DIRNAME is in our current directory
                    if(index == -1)
                    {
                        createFile(imgFile,tokens->items[1],to,currentDirectory->CUR_Clus,1);
                        index = dirlistIndexOfFileOrDirectory(to, tokens->items[1], 2);
                        unsigned int fats[2];
                        unsigned int * fatsPtr;
                        fats[0] = index;
                        fatsPtr = findFatSectorInDir(imgFile, fats,to->CUR_Clus);
                        unsigned int FatSectorDirCluster = fatsPtr[1];
                        index = fatsPtr[0];
                        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        DataSector += (FatSectorDirCluster - 2) * 512;
                        DataSector += index * 32;
                        lseek(file, DataSector, SEEK_SET);
                        write(file,currentDirectory->items[loc],32);
                        fats[0] = loc;
                        fatsPtr = findFatSectorInDir(imgFile,fats,currentDirectory->CUR_Clus);
                        FatSectorDirCluster = fatsPtr[1];
                        loc = fatsPtr[0];
                        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        DataSector += (FatSectorDirCluster - 2) * 512;
                        DataSector += loc * 32;
                        lseek(file, DataSector, SEEK_SET);
                        //copy contents to new DIRENTRY
                        if(loc == currentDirectory->size -1)
                        {
                            intToASCIIStringWrite(imgFile,0,DataSector,0,1);
                            if(currentDirectory->CUR_Clus == 2)
                            {
                                currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                            }
                            else
                            {
                                currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                            }
                        }
                        else
                        {
                            intToASCIIStringWrite(imgFile,229,DataSector,0,1);
                            if(currentDirectory->CUR_Clus == 2)
                            {
                                currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                            }
                            else
                            {
                                currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                            }
                        }
                    }
                    else
                    {
                        printf("The name is already being used by another file %s\n", tokens->items[1]);
                    }
                }
            }
            //case FROM is a file
            else
            {
                int index = dirlistIndexOfFileOrDirectory(to, tokens->items[1], 1);
                //Check if given DIRNAME is in our current directory
                if(index == -1)
                {
                    createFile(imgFile,tokens->items[1],to,currentDirectory->CUR_Clus,0);
                    index = dirlistIndexOfFileOrDirectory(to, tokens->items[1], 1);
                    unsigned int fats[2];
                    unsigned int * fatsPtr;
                    fats[0] = index;
                    fatsPtr = findFatSectorInDir(imgFile, fats,to->CUR_Clus);
                    unsigned int FatSectorDirCluster = fatsPtr[1];
                    index = fatsPtr[0];
                    //Modify the Data Region
                    unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                    DataSector += (FatSectorDirCluster - 2) * 512;
                    //Offset for Empty Index Start
                    DataSector += index * 32;
                    lseek(file, DataSector, SEEK_SET);
                    write(file,currentDirectory->items[loc],32);
                    //Do math to calculate the FAT sector we should iterate to get
                    //the right data region we should modify
                    fats[0] = loc;
                    fatsPtr = findFatSectorInDir(imgFile,fats,currentDirectory->CUR_Clus);
                    FatSectorDirCluster = fatsPtr[1];
                    loc = fatsPtr[0];
                    //Modify the Data Region
                    DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                    DataSector += (FatSectorDirCluster - 2) * 512;
                    //Offset for Empty Index Start
                    DataSector += loc * 32;
                    lseek(file, DataSector, SEEK_SET);
                    //creat FROM inside TO
                    //copy contents to new DIRENTRY
                    if(loc == currentDirectory->size -1)
                    {
                        intToASCIIStringWrite(imgFile,0,DataSector,0,1);
                        if(currentDirectory->CUR_Clus == 2)
                        {
                            currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                        }
                        else
                        {
                            currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                        }
                    }
                    else
                    {
                        intToASCIIStringWrite(imgFile,229,DataSector,0,1);
                        if(currentDirectory->CUR_Clus == 2){
                            currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                        }
                        else
                        {
                            currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                        }
                    }
                }
                else
                {
                    printf("The name is already being used by another file  %s\n", tokens->items[1]);
                }
            }
            free_dirlist(to);
        }
        //case FROM DNE
        else
        {
            printf("No such file or directory\n");
        }
    }
    //case FROM and TO are files
    else if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 1) != -1 && dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2], 1) != -1)
    {
        printf("The name is already being used by another file\n");
    }
    //case TO is a file and FROM is a directory
    else if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 2) != -1 && dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2], 1) != -1)
    {
        printf("Cannot move Directory: invalid destination argument\n");
    }
    //case TO DNE
    else if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 3) != -1 && dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2], 3) == -1)
    {
        int loc = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1],3);
        int loc1 = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2],3);
        unsigned int fats[2];
        unsigned int * fatsPtr;
        fats[0] = loc;
        fatsPtr = findFatSectorInDir(imgFile,fats,currentDirectory->CUR_Clus);
        unsigned int FatSectorDirCluster = fatsPtr[1];
        loc = fatsPtr[0];
        //Modify the Data Region
        unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
        //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
        DataSector += (FatSectorDirCluster - 2) * 512;
        //Offset for Empty Index Start
        DataSector += loc * 32;
        lseek(file, DataSector, SEEK_SET);
        unsigned char name[11];
        strcpy(name, tokens->items[2]);
        strncat(name, "           ", 11 - strlen(tokens->items[2]));
        write(file,&name,11);
        //Updates current directory
        if(currentDirectory->CUR_Clus == 2)
        {
            currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
        }
        else
        {
            currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
        }
    }
    //case FROM  and TO DNE
    else
    {
        printf("No such file or directory\n");
    }
    close(file);
    return currentDirectory;
}