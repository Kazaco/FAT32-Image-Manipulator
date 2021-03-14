#include "write.h"
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

void writeToOpenFile(const char * imgFile, tokenlist * tokens, dirlist * currentDirectory, filesList * openFiles)
{
    //Check valid input for "STRING"
    int validString = 1;
    if(tokens->items[3][0] != '"' || tokens->items[tokens->size - 1][strlen(tokens->items[tokens->size - 1]) - 1] != '"')
    {
        printf("Invalid String Format: \"STRING\"\n");
        validString = -1;
    }
    //Check that the file is open and able to be written to, if it
    //get that index from the openFiles list.
    int openIndex = openFileIndex(openFiles, tokens, 2);
    if(openIndex != -1 && validString == 1)
    {
        //Calculate how much memory we should allocate for the string
        int letterCount = 0;
        int sentenceStart = 3;
        for(sentenceStart; sentenceStart < tokens->size; sentenceStart++)
        {
            letterCount += strlen(tokens->items[sentenceStart]);
            //Calculate for spaces as well
            if(sentenceStart != tokens->size - 1)
            {
                letterCount += 1;
            }
        }

        //Allocate for string and merge all token items for string.
        char * string = malloc(sizeof(char) * letterCount + 1);
        strcpy(string, "");
        sentenceStart = 3;
        for(sentenceStart; sentenceStart < tokens->size; sentenceStart++)
        {
            strcat(string, tokens->items[sentenceStart]);
            //Don't put a space after last word.
            if(sentenceStart != tokens->size - 1)
            {
                strcat(string, " ");
            }
        }

        //Check our allocation for the file
        int fileFATAllocation = 0;
        int fileDataAllocation = 0;

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
        //Check if lseek + size given by the user is greater then allocated space for chosen file. If it is
        // we need to extend the file before we write.
        int writeStartVal = openFiles->items[openIndex]->FILE_OFFSET;
        int writeEndVal = openFiles->items[openIndex]->FILE_OFFSET + atoi(tokens->items[2]);
        unsigned int emptyFATArr[2];
        unsigned int * emptyFATptr;
        unsigned int endClusterFATArr[2];
        unsigned int * endClusterFATptr;

        //This will only run if we don't have enough allocated space.
        while(writeEndVal > fileDataAllocation)
        {
            //Extend the file until fileDataAllocation > writeEndVal

            //Read FAT from top until we find an empty item
            //arrPtr[0] : FAT Sector Empty Entry Loc
            //arrPtr[1] : FAT Sector Empty End
            emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);

            // //endClusArr[0] : FAT Sector Clus End Loc
            // //endClusArr[1] : FAT Sector Clus End
            endClusterFATptr = findEndClusEntryInFAT(imgFile, openFiles->items[openIndex]->FILE_FstClus, endClusterFATArr);

            //Create new end for current directory cluster.
            // 268435448 = 0xF8FFFF0F (uint 32, little endian)
            intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
            //Connect old end to new end of cluster.
            intToASCIIStringWrite(imgFile, emptyFATptr[0], endClusterFATArr[1], 0, 4);

            //Iterate changes
            fileDataAllocation += 512;
        }

        //Writing to file
        //Beginning Locations for FAT and Data Sector
        unsigned int FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
        unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
        unsigned int bitsLeftToWrite = atoi(tokens->items[2]);
        //Offset Location for File Cluster in FAT
        FatSector += openFiles->items[openIndex]->FILE_FstClus * 4;
        //Offset Location for File Cluster in Data
        DataSector += (openFiles->items[openIndex]->FILE_FstClus - 2) * 512;
        //Ending Vals (Use in next cluster calculation)
        unsigned int FatSectorEndianVal = 0;
        //Reading Hex Values from the FAT and Data Sector
        tokenlist * hex;
        char * littleEndian;
        // Writing Flags / Logic
        int foundFirstWriteLoc = -1;
        //Subtract 2 to not include " character
        letterCount -= 2;
        int stringPosition = 1;

        do
        {
            //Read the FAT/Data Region until we have written SIZE characters to the file.
            //Read Hex at FatSector Position
            hex = getHex(imgFile, FatSector, 4);
            //Obtain Endian string, so we can determine if this is the last time we should read
            //from the FAT and search the data region.
            littleEndian = littleEndianHexStringFromTokens(hex);
            FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
            //Deallocate hex and little Endian for FAT portion
            free_tokens(hex);
            free(littleEndian);
            //Calculate whether we should write to the current FAT position, or move onto the next.
            //Assuming start of file is like an array
            if(writeStartVal >= 0 && writeStartVal < 512)
            {
                //We should write to this data region until the end.
                while(writeStartVal < 512 && bitsLeftToWrite != 0)
                {
                    //Open the file, we already checked that it exists. Obtain the file descriptor
                    int file = open(imgFile, O_WRONLY);
                    //Go to offset position in file. ~SEEK_SET = Absolute position in document.
                    lseek(file, DataSector + writeStartVal, SEEK_SET);
                    //What should we write to file.
                    if(letterCount == 0)
                    {
                        //Size is greater than string, so we write '/0' afterwards.
                        write(file,"\0", 1);
                    }
                    else
                    {
                        char letter = string[stringPosition];
                        char letterString[1] = {letter};
                        write(file, letterString, 1);
                        //Iterate
                        stringPosition++;
                        letterCount--;
                    }
                    close(file);

                    //Move pointer for writing.
                    writeStartVal++;
                    bitsLeftToWrite--;
                }
                //If we have to also look at the next block, from now on we'll always start at 0.
                writeStartVal = 0;
                foundFirstWriteLoc = 1;
            }
            //Go to the next FAT block, untill we have written SIZE characters to the file.
            if(bitsLeftToWrite != 0)
            {
                //We have to loop again, reset FAT/Data regions.
                FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                //New FAT Offset added
                FatSector += FatSectorEndianVal * 4;
                //New Data Sector Offset Added
                DataSector += (FatSectorEndianVal - 2) * 512;
                //Only change writeStart if we haven't found the first writing location.
                if(foundFirstWriteLoc != 1)
                {
                    //Move offset of write start and end.
                    writeStartVal -= 512;
                }
            }
        } while (bitsLeftToWrite != 0);

        //Deallocate String used.
        free(string);

        //Update lseek info (should now be offset + size)
        openFiles->items[openIndex]->FILE_OFFSET = openFiles->items[openIndex]->FILE_OFFSET + atoi(tokens->items[2]);
        //Modify the size values stored for file if we wrote beyond its current file size. Must change
        //program local data and the disk itself.
        if(writeEndVal > openFiles->items[openIndex]->FILE_SIZE)
        {
            // 1. Local Data (filelist)
            openFiles->items[openIndex]->FILE_SIZE = writeEndVal;

            // 2. Disk Data (Assuming open will delete itself when we change directories)
            //We know the file will exist because we are modifying it right now, so index wont ever = -1
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILENAME);
            //Retrieve the correct FAT we should should to modify the directory.
            unsigned int fats[2];
            unsigned int * fatsPtr;
            fats[0] = index;
            fatsPtr = findFatSectorInDir(imgFile, fats, currentDirectory->CUR_Clus);
            unsigned int FatSectorDirCluster = fatsPtr[1];
            index = fatsPtr[0];
            //Modify the Data Region
            unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
            //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
            DataSector += (FatSectorDirCluster - 2) * 512;
            //Offset for Empty Index Start
            DataSector += index * 32;
            //Modify size for file on disk
            intToASCIIStringWrite(imgFile, writeEndVal, DataSector + 28, 0, 4);

            //3. Local Data Current Directory
            //Delete the current directory so these changes will be made to local data.
            unsigned int N = currentDirectory->CUR_Clus;
            free_dirlist(currentDirectory);
            currentDirectory = getDirectoryList(imgFile, N);
        }
    }
}