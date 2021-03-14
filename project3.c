#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

////////////////////////////////////
filesList *new_filesList(void);
void free_filesList(filesList * openFiles);
int createOpenFileEntry(filesList * openFiles, dirlist * directories, tokenlist * tokens, int index);
void readFilesList(filesList * openFiles);
int filesListIndex(filesList * openFiles, const char * item);
////////////////////////////////////
void createFile(const char * imgFile, const char * filename, dirlist * directories, unsigned int previousCluster, int flag);
void intToASCIIStringWrite(const char * imgFile, int value, unsigned int DataSector, int begin, int size);
unsigned int * findEmptyEntryInFAT(const char * imgFile, unsigned int * emptyArr);
unsigned int * findEndClusEntryInFAT(const char * imgFile, unsigned int clusStart, unsigned int * endClusArr);
unsigned int * findFatSectorInDir(const char* imgFile, unsigned int * fats, unsigned int clus);
unsigned int * findEmptyEntryInFATNext(const char * imgFile, unsigned int * emptyArr);
void removeFile(const char * imgFile, dirlist * directory, const char * filename);
int openFileIndex(filesList * files, tokenlist * tokens, int flag);
char * readFAT(tokenlist*, dirlist*, const char*, filesList*);
unsigned int seekFAT(tokenlist*, dirlist*, const char *, filesList*, unsigned int);


int main(int argc, char *argv[])
{
    //Only valid way to start program is with format: project3 fat32.img
    if(argc == 2)
    {
        //In-case the user puts an invalid file to search.
        if(!file_exists(argv[1]))
        {
            printf("Invalid File Given: %s\n", argv[1]);
            return -1;
        }
        //Let the User run as many commands they want on the file given.
        running(argv[1]);
    }
    else
    {
        //Invalid User Input
        printf("Invalid Format Given: project3 fat32.img\n");
        return -1;
    }
    return 0;
}

//Let User run commands on given .img file
void running(const char * imgFile)
{
    //Get BIOS INFO before moving around disk.
    getBIOSParamBlock(imgFile);
    //Make the User Start in the Root
		dirlist * currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
		//Let the user have a container to interact w/ files no matter where they are in file system.
		filesList * openFiles = new_filesList();
    printf("=== FAT32 File System ===\n");
    while(1)
    {
        //User Initial Input
        printf("> ");
        char *input = get_input();
        //Split User Input into Tokens
        tokenlist *tokens = get_tokens(input);
        else if(strcmp("write", tokens->items[0]) == 0 && tokens->size >= 4)
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

                // printf("Current File FAT Allocation: %i\n", fileFATAllocation);
                // printf("Current File Data Region Allocation: %i\n", fileDataAllocation);

                //Check if lseek + size given by the user is greater then allocated space for chosen file. If it is
                // we need to extend the file before we write.
                int writeStartVal = openFiles->items[openIndex]->FILE_OFFSET;
                int writeEndVal = openFiles->items[openIndex]->FILE_OFFSET + atoi(tokens->items[2]);
                unsigned int emptyFATArr[2];
                unsigned int * emptyFATptr;
                unsigned int endClusterFATArr[2];
                unsigned int * endClusterFATptr;
                // printf("writeStartVal: %i\n", writeStartVal);
                // printf("writeEndVal: %i\n", writeEndVal);

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
                    // printf("New Data Region Allocation: %i\n", fileDataAllocation);
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
                // printf("Fat Sector Start: %i\n", FatSector);
                // printf("Data Region Start: %i\n", DataSector);
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
                    // printf("FAT Endian Val: %i\n", FatSectorEndianVal);
                    //Deallocate hex and little Endian for FAT portion
                    free_tokens(hex);
                    free(littleEndian);

                    //Calculate whether we should write to the current FAT position, or move onto the next.
                    //Assuming start of file is like an array
                    if(writeStartVal >= 0 && writeStartVal < 512)
                    {
                        //We should write to this data region until the end.
                        // printf("Writing....\n");
                        while(writeStartVal < 512 && bitsLeftToWrite != 0)
                        {
                            //printf("Writing At Position: %i\n", writeStartVal);

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
                        // printf("Bits left: %i\n", bitsLeftToWrite);
                        writeStartVal = 0;
                        foundFirstWriteLoc = 1;
                    }

                    //Go to the next FAT block, untill we have written SIZE characters to the file.
                    if(bitsLeftToWrite != 0)
                    {
                        // printf("Need to move to next FAT block.\n");

                        //We have to loop again, reset FAT/Data regions.
                        FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        //New FAT Offset added
                        FatSector += FatSectorEndianVal * 4;
                        //New Data Sector Offset Added
                        DataSector += (FatSectorEndianVal - 2) * 512;
                        //New Offset for FAT
                        // printf("New FAT sector: %i\n", FatSector);
                        // printf("New Data sector: %i\n", DataSector);

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
                // printf("Old: %i\n", openFiles->items[openIndex]->FILE_OFFSET);
                openFiles->items[openIndex]->FILE_OFFSET = openFiles->items[openIndex]->FILE_OFFSET + atoi(tokens->items[2]);
                // printf("New: %i\n", openFiles->items[openIndex]->FILE_OFFSET);

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
                    // printf("Data Region to Search: %i\n", FatSectorDirCluster);
                    //Modify the Data Region
                    unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                    DataSector += (FatSectorDirCluster - 2) * 512;
                    // printf("Main Data Sector Start: %i\n", DataSector);
                    //Offset for Empty Index Start
                    DataSector += index * 32;
                    // printf("Main Data Sector Start + Offset: %i\n", DataSector);
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
        else if(strcmp("cp", tokens->items[0]) == 0 && tokens->size == 4) {
            printf("Not implemented\n");
        }
        else if(strcmp("cp", tokens->items[0]) == 0 && tokens->size >= 3) {

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
                        // printf("We good\n");
                    }
                    else
                    {
                        //No more empty entries in this directory, need to extend the FAT
                        //printf("Must create a new FAT entry\n");

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
                    // intToASCIIStringWrite(imgFile, emptyFATptr1[0], emptyFATptr[1], 0, 4);
                    // emptyFATptr = emptyFATptr1;
                    do
                    {
                        //Read the FAT until we are at the end of the chosen cluster (N). This will tell us
                        //the data sectors we should go to in the data region of sector size 512.

                        //We have already positioned ourselves in the *first* position with previous math.
                        //printf("FAT Sector Start: %i\n", FatSector);
                        //Read Hex at FatSector Position
                        hex = getHex(imgFile, FatSector, 4);
                        //Obtain Endian string, so we can determine if this is the last time we should read
                        //from the FAT and search the data region.
                        littleEndian = littleEndianHexStringFromTokens(hex);
                        FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
                        // printf("FAT Endian Val: %i\n", FatSectorEndianVal);
                        //Deallocate hex and little Endian for FAT portion
                        free_tokens(hex);
                        free(littleEndian);
                        int file = open(imgFile, O_RDWR);
                        lseek(file, DataSector, SEEK_SET);
                        char clusData[512];
                        strcpy(clusData, "");
                        read(file, clusData,512);
                        //printf("Cluster Data: %s\n",clusData);
                        //printf("clus 2: %d\n",emptyFATptr[0]);
                        //printf("Fat 2: %d\n",emptyFATptr[1]);
                        int i = 0;
                        for(i; i < 512;i++){
                            lseek(file,DataSector1+i, SEEK_SET);
                            char letterString[1] = {clusData[i]};
                            write(file,letterString,1);
                        }
                        close(file);


                        //printf("Fat Sector Start: %i\n", FatSector);
                        //Set up data for new loop, or  quit.
                        //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
                        if((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0)
                        {
                            emptyFATptr1 = findEmptyEntryInFATNext(imgFile, emptyFATArr1);
                            intToASCIIStringWrite(imgFile, emptyFATptr1[0], emptyFATptr[1], 0, 4);
                            emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
                            emptyFATptr1 = emptyFATptr;
                            //printf("Next clus 2: %d\n",emptyFATptr[0]);
                            // printf("Next Fat 2: %d\n",emptyFATptr[1]);
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
                            //New Offset for FAT
                            // printf("New FAT sector: %i\n", FatSector);
                            // printf("New Data sector: %i\n", DataSector);
                            // printf("New Data1 sector: %i\n", DataSector1);
                        }
                        else
                        {
                            intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                            //This should be our last iteration. Do nothing.
                            //printf("Last Time!\n");
                        }
                    } while ((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0);
                    unsigned int fats[2];
                    unsigned int * fatsPtr;
                    fats[0] = index;
                    fatsPtr = findFatSectorInDir(imgFile, fats, currentDirectory->CUR_Clus);
                    unsigned int FatSectorDirCluster = fatsPtr[1];
                    index = fatsPtr[0];
                    // printf("Data Region to Search: %i\n", FatSectorDirCluster);

                    //Modify the Data Region
                    DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                    DataSector += (FatSectorDirCluster - 2) * 512;
                    //printf("Main Data Sector Start: %i\n", DataSector);
                    //Offset for Empty Index Start
                    DataSector += index * 32;
                    //printf("Main Data Sector Start + Offset: %i\n", DataSector);

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
                            //printf("We good\n");
                        }
                        else
                        {
                            //No more empty entries in this directory, need to extend the FAT
                            // printf("Must create a new FAT entry\n");

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
                        unsigned int * emptyFATptr1;// = findEmptyEntryInFAT(imgFile, emptyFATArr1);
//                        intToASCIIStringWrite(imgFile, emptyFATptr1[0], emptyFATptr[1], 0, 4);
//                        emptyFATptr = emptyFATptr1;
                        do
                        {
                            //Read the FAT until we are at the end of the chosen cluster (N). This will tell us
                            //the data sectors we should go to in the data region of sector size 512.

                            //We have already positioned ourselves in the *first* position with previous math.
                            //printf("FAT Sector Start: %i\n", FatSector);
                            //Read Hex at FatSector Position
                            hex = getHex(imgFile, FatSector, 4);
                            //Obtain Endian string, so we can determine if this is the last time we should read
                            //from the FAT and search the data region.
                            littleEndian = littleEndianHexStringFromTokens(hex);
                            FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
                            //printf("FAT Endian Val: %i\n", FatSectorEndianVal);
                            //Deallocate hex and little Endian for FAT portion
                            free_tokens(hex);
                            free(littleEndian);
                            int file = open(imgFile, O_RDWR);
                            lseek(file, DataSector, SEEK_SET);
                            char clusData[512];
                            strcpy(clusData, "");
                            read(file, clusData,512);
                            //printf("Cluster Data: %s\n",clusData);
                            //printf("clus 2: %d\n",emptyFATptr[0]);
                            // printf("Fat 2: %d\n",emptyFATptr[1]);
                            int i = 0;
                            for(i; i < 512;i++){
                                lseek(file,DataSector1+i, SEEK_SET);
                                char letterString[1] = {clusData[i]};
                                write(file,letterString,1);
                            }
                            close(file);


                            //printf("Fat Sector Start: %i\n", FatSector);
                            //Set up data for new loop, or  quit.
                            //RANGE: Cluster End: 0FFFFFF8 -> FFFFFFFF or empty (same for while loop end)
                            if((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0)
                            {

                                emptyFATptr1 = findEmptyEntryInFATNext(imgFile, emptyFATArr1);
                                intToASCIIStringWrite(imgFile, emptyFATptr1[0], emptyFATptr[1], 0, 4);
                                emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);
                                emptyFATptr1 = emptyFATptr;
                                // printf("Next clus 2: %d\n",emptyFATptr[0]);
                                //printf("Next Fat 2: %d\n",emptyFATptr[1]);
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
                                //New Offset for FAT
                                //printf("New FAT sector: %i\n", FatSector);
                                // printf("New Data sector: %i\n", DataSector);
                                //printf("New Data1 sector: %i\n", DataSector1);
                            }
                            else
                            {
                                intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                                //This should be our last iteration. Do nothing.
                                //printf("Last Time!\n");
                            }
                        } while ((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0);
                        unsigned int fats[2];
                        unsigned int * fatsPtr;
                        fats[0] = index;
                        fatsPtr = findFatSectorInDir(imgFile, fats, to->CUR_Clus);
                        unsigned int FatSectorDirCluster = fatsPtr[1];
                        index = fatsPtr[0];
                        //printf("Data Region to Search: %i\n", FatSectorDirCluster);

                        //Modify the Data Region
                        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
                        DataSector += (FatSectorDirCluster - 2) * 512;
                        //printf("Main Data Sector Start: %i\n", DataSector);
                        //Offset for Empty Index Start
                        DataSector += index * 32;
                        //printf("Main Data Sector Start + Offset: %i\n", DataSector);

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
        }
        else if (strcmp("read", tokens->items[0]) == 0)
        {
            if (tokens->size < 3)
            {
                printf("ERROR: requires <filename><size> parameters \n");
            }
            else if(tokens->size > 3)
            {
                printf("ERROR: requires <filename><offset> parameters \n");
            }
            else
            {
                char * result = readFAT(tokens, currentDirectory, imgFile, openFiles);
                if (result != NULL)
                {
                    printf("%s\n", result);
                }
                free(result);
            }
        }
        else if (strcmp("lseek", tokens->items[0]) == 0)
        {
            if (tokens->size < 3)
            {
                printf("ERROR: requires <filename><offset> parameters \n");
            }
            else if(tokens->size > 3)
            {
                printf("ERROR: requires <filename><offset> parameters \n");
            }
            //assuming a file needs to be opened to allow a lseek operation
            else
            {
                //check if file is open in either read/write mode
                if (openFileIndex(openFiles, tokens, 3) == -1)
                {
                    //file isnt open at all!
                    printf("ERROR: File must be opened in either read/write mode before seeking \n");
                }
                else
                {
                    //check for read first
                    //if this fails, then file is write open
                    int index = openFileIndex(openFiles, tokens, 1);
                    if (index == -1)
                    {
                        index = openFileIndex(openFiles, tokens, 2);
                    }
                    unsigned int offset = seekFAT(tokens, currentDirectory, imgFile, openFiles, index);
                    if (offset != -1)
                    {
                        openFiles->items[index]->FILE_OFFSET = offset;
                        printf("New offset for file %s: %u\n", tokens->items[1], openFiles->items[index]->FILE_OFFSET);
                    }
                }
            }
        }
        else if(strcmp("rmdir", tokens->items[0]) == 0 && tokens->size >= 2)
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
                        //printf("Current Cluster: %i\n", directory->CUR_Clus);
                        do
                        {
                            //Read the FAT until we are at the end of the chosen cluster (N). This will tell us
                            //the data sectors we should go to in the data region of sector size 512.

                            //We have already positioned ourselves in the *first* position with previous math.
                            //printf("FAT Sector Start: %i\n", FatSector);

                            //Read Hex at FatSector Position
                            hex = getHex(imgFile, FatSector, 4);
                            //Obtain Endian string, so we can determine if this is the last time we should read
                            //from the FAT and search the data region.
                            littleEndian = littleEndianHexStringFromTokens(hex);
                            FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
                            //printf("FAT Endian Val: %i\n", FatSectorEndianVal);
                            //Deallocate hex and little Endian for FAT portion
                            free_tokens(hex);
                            free(littleEndian);

                            //printf("Fat Sector Start: %i\n", FatSector);
                            //int i = 0;
                            //int file = open(imgFile, O_WRONLY);
                            //for(i; i < 512; i++)
                            //{
                                //Go to offset position in file. ~SEEK_SET = Absolute position in document.
                                //lseek(file, DataSector, SEEK_SET);
                                //write(file, "\0", 1);
                                //DataSector += 1;
                            //}
                            //close(file);
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
                                //New Offset for FAT
                                //printf("New FAT sector: %i\n", FatSector);
                                //printf("New Data sector: %i\n", DataSector);
                            }
                            else
                            {
                                //This should be our last iteration. Do nothing.
                                //printf("Last Time!\n");
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
                        //printf("New Data sector: %i\n", DataSector);
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
        }
        else
        {
            printf("Invalid Command Given\n");
        }

        free(input);
        free_tokens(tokens);
    }
}
unsigned int seekFAT(tokenlist * tokens, dirlist * directories, const char * imgFile, filesList * curFiles, unsigned int index2){
	//first check if file exists
	int index = dirlistIndexOfFileOrDirectory(directories, tokens->items[1], 1);
	if(createOpenFileEntry(curFiles, directories, tokens, index) == 0){printf("issue\n"); return -1;}
	if (index == -1)
	{
		printf("ERROR: File does not exist!\n");
		return -1;
	}
	//check next if it is a directory
 	else if (dirlistIndexOfFileOrDirectory(directories, tokens->items[1], 2) > 0)
	{
		printf("ERROR: File is a directory!\n");
		return -1;
	}
	//else
	else
	{
		unsigned int fileSize = curFiles->items[index2]->FILE_SIZE;
		//printf("File size: %u\n", fileSize);
		unsigned int OFFSET = (unsigned int)strtol(tokens->items[2], NULL, 10);
	//	printf("Requested file offset: %u\n", OFFSET);
		unsigned int currentPos = curFiles->items[index2]->FILE_OFFSET;
		//printf("Current position: %u\n", currentPos);
		//if we are trying to seek more than filesize, do not allow
		if (OFFSET > fileSize)
		{
			printf("ERROR: Offset has attempted to exceed current file size\n");
			return -1;
		}
		else if (currentPos == OFFSET)
		{
			printf("Parameter given for offset is the same as current file offset.\n");
			return -1;
		}
		else
		{
			return OFFSET;
		}
	}
	//if you are here, soemthing went wrong
	printf("ERROR: You discovered a bug \n");
	return -1;
}

char * readFAT(tokenlist*tokens, dirlist*directories, const char*imgfile, filesList*openFiles)
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
    //printf("readStartVal: %i\n", readStartVal);
    //printf("readEndVal: %i\n", readEndVal);
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
    //printf("Current File FAT Allocation: %i\n", fileFATAllocation);
    //printf("Current File Data Region Allocation: %i\n", fileDataAllocation);

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
        //printf("FAT Endian Val: %i\n", FatSectorEndianVal);
        //Deallocate hex and little Endian for FAT portion
        free_tokens(hex);
        free(littleEndian);
        if (readStartVal >= 0 && readStartVal < 512)
        {
            //while within data section, read
            //printf("Reading...\n");
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
            //printf("New FAT sector: %i\n", FatSector);
            //printf("New Data sector: %i\n", DataSector);
        }
    } while(bitsLeftToRead != 0);

    //Update lseek info (should now be offset + size)
    //printf("Old: %i\n", openFiles->items[openIndex]->FILE_OFFSET);
    openFiles->items[openIndex]->FILE_OFFSET = readEndVal;
    //printf("New: %i\n", openFiles->items[openIndex]->FILE_OFFSET);

	return returnString;
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
//    printf("FAT Sector Empty Start: %i\n", FatSectorEmpty);
    do
    {
        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSectorEmpty, 4);
        //Obtain Endian string, so we can determine if this is an empty entry.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEmptyEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
//        printf("FAT Endian Empty Val: %i\n", FatSectorEmptyEndianVal);
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
//    printf("arr[0] : FAT Sector Empty Entry Next Loc: %i\n", emptyEntryLoc);
//    printf("arr[1] : FAT Sector Empty Next End: %i\n\n", FatSectorEmpty);
    emptyArr[0] = emptyEntryLoc;
    emptyArr[1] = FatSectorEmpty;
    return emptyArr;
}
