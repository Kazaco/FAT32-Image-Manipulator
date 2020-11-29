#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

typedef struct {
	int size;
	char **items;
} tokenlist;

struct BIOS_Param_Block {
    unsigned int BytsPerSec;    //Bytes per sector
    unsigned int SecPerClus;    //Sectors per cluster
    unsigned int RsvdSecCnt;    //Reserved  region size
    unsigned int NumFATs;       //Number of FATs
    unsigned int TotSec32;      //Total sectors
    unsigned int FATSz32;       //FAT size
    unsigned int RootClus;      //Root cluster
} BPB;

///
struct DIRENTRY{
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth;
    unsigned char DIR_CrtTime[2];
    unsigned char DIR_CrtDate[2];
    unsigned char DIR_LstAccDate[2];
    unsigned char DIR_FstClusHI[2];
    unsigned char DIR_WrtTime[2];
    unsigned char DIR_WrtDate[2];
    unsigned char DIR_FstClusLO[2];
    unsigned char DIR_FileSize[4];
} __attribute__((packed));
typedef struct DIRENTRY DIRENTRY;

typedef struct {
	int size;
    unsigned int CUR_Clus;
	DIRENTRY **items;
} dirlist;

#define FILENAME 1
#define DIRECTORY 2
#define FILEORDIR 3
#define EMPTY 4

///
typedef struct {
    unsigned char FILE_Name[11];
    unsigned int FILE_FstClus;
    char FILE_Mode[3];
    unsigned int FILE_OFFSET;
    unsigned int FILE_SIZE;
} FILEENTRY;

typedef struct {
	int size;
	FILEENTRY **items;
} filesList;

///////////////////////////////////
char *get_input(void);
tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
////////////////////////////////////
dirlist *new_dirlist(void);
void free_dirlist(dirlist * directories);
dirlist * getDirectoryList(const char * imgFile, unsigned int N);
void readDirectories(dirlist * directories);
int dirlistIndexOfFileOrDirectory(dirlist * directories, const char * item, int flag);
////////////////////////////////////
filesList *new_filesList(void);
void free_filesList(filesList * openFiles);
int createOpenFileEntry(filesList * openFiles, dirlist * directories, tokenlist * tokens, int index);
void readFilesList(filesList * openFiles);
int filesListIndex(filesList * openFiles, const char * item);
////////////////////////////////////
int file_exists(const char * filename);
void running(const char * imgFile);
tokenlist * getHex(const char * imgFile, int decStart, int size);
char * littleEndianHexStringFromTokens(tokenlist * hex);
char * littleEndianHexStringFromUnsignedChar(unsigned char * arr, int size);
char * bigEndianHexString(tokenlist * hex);
void getBIOSParamBlock(const char * imgFile);
////////////////////////////////////
void createFile(const char * imgFile, const char * filename, dirlist * directories, unsigned int previousCluster, int flag);
void intToASCIIStringWrite(const char * imgFile, int value, unsigned int DataSector, int begin, int size);
unsigned int * findEmptyEntryInFAT(const char * imgFile, unsigned int * emptyArr);
unsigned int * findEndClusEntryInFAT(const char * imgFile, unsigned int clusStart, unsigned int * endClusArr);
unsigned int * findFatSectorInDir(const char* imgFile, unsigned int * fats, unsigned int clus);
////////////////////////////////////
int openFileIndex(filesList * files, tokenlist * tokens, int flag);

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

        //User inputted nothing
        if(tokens->size == 0)
        {
            //Do nothing.
        }
        //Commands
        else if(strcmp("exit", tokens->items[0]) == 0 && tokens->size == 1)
        {
            printf("Exit\n");
            free(input);
            free_dirlist(currentDirectory);
            free_filesList(openFiles);
            break;
        }
        else if(strcmp("info", tokens->items[0]) == 0 && tokens->size == 1)
        {
            printf("=== Info ===\n");
            printf("Bytes Per Sector: %d\n", BPB.BytsPerSec);
            printf("Sectors per Cluster: %d\n", BPB.SecPerClus);
            printf("Reserved Sector Count: %d\n", BPB.RsvdSecCnt);
            printf("Number of FATs: %d\n", BPB.NumFATs);
            printf("Total Sectors: %d\n", BPB.TotSec32);
            printf("FAT size: %d\n", BPB.FATSz32);
            printf("Root Cluster: %d\n", BPB.RootClus);
        }
        else if(strcmp("size", tokens->items[0]) == 0 && tokens->size == 2)
        {
            //Find index of FILENAME
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILENAME);
            //Check if FILENAME was found or not.
            if(index != -1)
            {
                //Change unsigned int values to little endian to calculate file size for given FILENAME
                char * sizeStr = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FileSize, 4);
                unsigned int fileSize = (unsigned int)strtol(sizeStr, NULL, 16);
                printf("File %s: %i bytes\n", tokens->items[1], fileSize);
                free(sizeStr);
            }
            else
            {
                printf("File not found.\n");
            }
        }
        else if(strcmp("ls", tokens->items[0]) == 0 && (tokens->size == 1 || tokens->size == 2) )
        {
            //Check Current Directory
            if(tokens->size == 1)
            {
                //Just read cwd
                readDirectories(currentDirectory);
            }
            //Check DIRNAME
            else
            {
                //Find index of DIRNAME
                int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], DIRECTORY);
                //Check if index found the DIRNAME or not.
                if(index != -1)
                {
                    //Calculate cluster value of DIRNAME
                    char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusHI, 2);
                    char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusLO, 2);
                    strcat(clusterHI,clusterLOW);
                    unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
                    //unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
                    //Make list structure containing all files found in DIRNAME cluster.
                    dirlist * lsDirectory = getDirectoryList(imgFile, clusterValHI);
                    //Display to User
                    readDirectories(lsDirectory);
                    //Deallocate everything.
                    free(clusterHI);
                    free(clusterLOW);
                    free_dirlist(lsDirectory);
                }
                else
                {
                    printf("Directory not found.\n");
                }
            }
        }
        else if(strcmp("cd", tokens->items[0]) == 0 && tokens->size == 2)
        {
            //Find index of DIRNAME
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], DIRECTORY);
            //Check if given DIRNAME is in our current directory
            if(index != -1)
            {
                //Calculate cluster value of DIRNAME
                char * clusterHI = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusHI, 2);
                char * clusterLOW = littleEndianHexStringFromUnsignedChar(currentDirectory->items[index]->DIR_FstClusLO, 2);
                strcat(clusterHI,clusterLOW);
                unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
                //unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
                //free the CWD
                free_dirlist(currentDirectory);
                //case for CD to root directory
                if(clusterValHI == 0){
                    currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                }
                //case for cd to any other directory
                else{
                    currentDirectory = getDirectoryList(imgFile, clusterValHI);
                }
                free(clusterHI);
                free(clusterLOW);   
            }
            else
            {
                printf("Directory not found.\n");
            }
        }
        else if(strcmp("creat", tokens->items[0]) == 0 && tokens->size == 2)
        {
            //Find index of DIRNAME
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILENAME);
            //Check if given DIRNAME is in our current directory
            if(index == -1)
            {
                createFile(imgFile, tokens->items[1], currentDirectory, 0, 0);
            }
            else
            {
                printf("File %s already exists.\n", tokens->items[1]);
            }
        }
        else if(strcmp("mkdir", tokens->items[0]) == 0 && tokens->size == 2)
        {
            //Find index of DIRNAME
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], DIRECTORY);
            //Check if given DIRNAME is in our current directory
            if(index == -1)
            {
                createFile(imgFile, tokens->items[1], currentDirectory, 0, 1);
            }
            else
            {
                printf("Directory %s already exists.\n", tokens->items[1]);
            }
        }
        else if(strcmp("open", tokens->items[0]) == 0 && tokens->size == 3)
        {
            //Find index of FILENAME
            int index = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], FILENAME);
            //Check if given FILENAME is in our current directory
            if(index != -1)
            {
                //Check if dealing with read-only file.
                if( ((currentDirectory->items[index]->DIR_Attr & 0x10) != 0) )
                {
                    //READ-ONLY (HAVE NOT BEEN ABLE TO TEST)
                    if(strcmp("r", tokens->items[2]) == 0)
                    {
                        if(createOpenFileEntry(openFiles, currentDirectory, tokens, index) == 1)
                        {
                            readFilesList(openFiles);
                        }
                        else
                        {
                            printf("File given is already open.\n");
                        }
                    }
                    else
                    {
                        //Can't do anything with the file.
                        printf("File given is read-only.\n");
                    }
                }
                else
                {
                    //Can read or write to file.
                    if(strcmp("r", tokens->items[2]) == 0 || strcmp("w", tokens->items[2]) == 0 
                    || strcmp("rw", tokens->items[2]) == 0 || strcmp("wr", tokens->items[2]) == 0)
                    {
                        if(createOpenFileEntry(openFiles, currentDirectory, tokens, index) == 1)
                        {
                            readFilesList(openFiles);
                        }
                        else
                        {
                            printf("File given is already open.\n");
                        }
                    }
                    else
                    {
                        printf("Invalid flag given.\n");
                    }
                }
            }
            else
            {
                printf("File not found.\n");
            }
        }
        else if(strcmp("close", tokens->items[0]) == 0 && tokens->size == 2)
        {
            //Find index of FILENAME in openFiles
            int index = filesListIndex(openFiles, tokens->items[1]);
            //Check if FILENAME was found or not.
            if(index != -1)
            {
                //Create a new filesList b/c deleting items from dynamically allocated in c can't be simple.
                filesList * newList = new_filesList();
                //Create a new entry list 
                newList->items = (FILEENTRY **) realloc(newList->items, (openFiles->size - 1) * sizeof(FILEENTRY));
                
                //Copy all items over that don't have FILENAME.
                int i = 0;
                for(i; i < openFiles->size; i++)
                {
                    //Copy everything over except the FILENAME given
                    if(i != index)
                    {
                        //Create a new entry in our open files list.
                        newList->items[newList->size] = malloc(sizeof(FILEENTRY));
                        //1. Name
                        strcpy(newList->items[newList->size]->FILE_Name, "");
                        strcpy(newList->items[newList->size]->FILE_Name, openFiles->items[i]->FILE_Name);
                        //2. Cluster Info
                        newList->items[newList->size]->FILE_FstClus = openFiles->items[i]->FILE_FstClus;
                        //3. Mode
                        strcpy(newList->items[newList->size]->FILE_Mode, "");
                        strcpy(newList->items[newList->size]->FILE_Mode, openFiles->items[i]->FILE_Mode);
                        //4. Offset
                        newList->items[newList->size]->FILE_OFFSET = openFiles->items[i]->FILE_OFFSET;
                        //5. File Size
                        newList->items[newList->size]->FILE_SIZE = openFiles->items[i]->FILE_SIZE;
                        //Iterate
                        newList->size += 1;
                    }
                }
                //Delete previous fileList and replace it.
                free_filesList(openFiles);
                openFiles = newList;
                readFilesList(openFiles);
            }
            else
            {
                printf("File given is not open.\n");
            }
        }
        else if(strcmp("mv", tokens->items[0]) == 0 && tokens->size == 3){
            int file = open(imgFile, O_WRONLY);
            unsigned int DataSector;
            //check if currentdir is root dir
            if(currentDirectory->CUR_Clus == 2 && strcmp(".", tokens->items[1]) == 0)
            {
                printf("No such file or directory\n");
            }
            //case TO exists as directory
            else if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2], 2) != -1){
                int loc = -1;
                loc = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 3);
                if(loc != -1){
                    int loc1 = dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2],2);
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
                    //case FROM is a directory
                    if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 2) != -1){
                        //makes a new dirlist for the found To directory
                        //case the FROM is .. pointing to root directory
                        if(currentDirectory->CUR_Clus == 2 && strcmp("..", tokens->items[1]) == 0)
                        {
                            printf("No such file or directory\n");
                        }
                        else{
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
                                if(loc == currentDirectory->size -1){
                                    intToASCIIStringWrite(imgFile,0,DataSector,0,1);
                                    if(currentDirectory->CUR_Clus == 2){
                                        currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                                    }
                                    else{
                                        currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                                    }
                                }
                                else{
                                    intToASCIIStringWrite(imgFile,229,DataSector,0,1);
                                    if(currentDirectory->CUR_Clus == 2){
                                        currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                                    }
                                    else{
                                        currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                                    }
                                }
                            }
                            else
                            {
                                printf("The name is already being used by another file\n", tokens->items[1]);
                            }
                        }
                    }
                    //case FROM is a file
                    else{
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
                            if(loc == currentDirectory->size -1){
                                intToASCIIStringWrite(imgFile,0,DataSector,0,1);
                                if(currentDirectory->CUR_Clus == 2){
                                    currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                                }
                                else{
                                    currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                                }
                            }
                            else{
                                intToASCIIStringWrite(imgFile,229,DataSector,0,1);
                                if(currentDirectory->CUR_Clus == 2){
                                    currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                                }
                                else{
                                    currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                                }
                            }
                        }
                        else
                        {
                            printf("The name is already being used by another file\n", tokens->items[1]);
                        }
                    }
                    free_dirlist(to);
                }
                //case FROM DNE
                else{
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
            else if(dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[1], 3) != -1 && dirlistIndexOfFileOrDirectory(currentDirectory, tokens->items[2], 3) == -1){
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
                if(currentDirectory->CUR_Clus == 2){
                    currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                }
                else{
                    currentDirectory = getDirectoryList(imgFile, currentDirectory->CUR_Clus);
                }
            }
            //case FROM  and TO DNE
            else{
                printf("No such file or directory\n");
            }
            close(file);
        }
        else if(strcmp("write", tokens->items[0]) == 0 && tokens->size == 4)
        {
            //Check if we have a file open
            printf("Write\n");
            //Check that the file is open and able to be written to.
            readFilesList(openFiles);
            //Check that the file is open and able to be written to, if it
            //get that index from the openFiles list.
            int openIndex = openFileIndex(openFiles, tokens, 2);
            if(openIndex != -1)
            {
                //Info we need to use to iterate the FAT and data region.
                printf("We will write to the file\n");

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

                printf("Current File FAT Allocation: %i\n", fileFATAllocation);
                printf("Current File Data Region Allocation: %i\n", fileDataAllocation);

                //Check if lseek + size given by the user is greater then allocated space for chosen file. If it is
                // we need to extend the file before we write.
                int writeStartVal = openFiles->items[openIndex]->FILE_OFFSET; 
                int writeEndVal = openFiles->items[openIndex]->FILE_OFFSET + atoi(tokens->items[2]);
                printf("writeStartVal: %i\n", writeStartVal);
                printf("writeEndVal: %i\n", writeEndVal);
                if(writeEndVal > fileDataAllocation)
                {
                    //We need to extend the current file.
                    printf("Extend the file first!");

                    // unsigned int emptyFATArr[2];
                    // unsigned int * emptyFATptr;
                    // unsigned int endClusterFATArr[2];
                    // unsigned int * endClusterFATptr;

                    // //No more empty entries in this directory, need to extend the FAT
                    // printf("Must create a new FAT entry\n");

                    // //Read FAT from top until we find an empty item
                    // //arrPtr[0] : FAT Sector Empty Entry Loc
                    // //arrPtr[1] : FAT Sector Empty End
                    // emptyFATptr = findEmptyEntryInFAT(imgFile, emptyFATArr);

                    // //endClusArr[0] : FAT Sector Clus End Loc
                    // //endClusArr[1] : FAT Sector Clus End
                    // endClusterFATptr = findEndClusEntryInFAT(imgFile, currentDirectory->CUR_Clus, endClusterFATArr);

                    // //Create new end for current directory cluster.
                    // // 268435448 = 0xF8FFFF0F (uint 32, little endian)
                    // intToASCIIStringWrite(imgFile, 268435448, emptyFATptr[1], 0, 4);
                    // //Connect old end to new end of cluster.
                    // intToASCIIStringWrite(imgFile, emptyFATptr[0], endClusterFATArr[1], 0, 4);
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
                printf("Fat Sector Start: %i\n", FatSector);
                printf("Data Region Start: %i\n", DataSector);
                //Ending Vals (Use in next cluster calculation)
                unsigned int FatSectorEndianVal = 0;
                //Reading Hex Values from the FAT and Data Sector
                tokenlist * hex;
                char * littleEndian;
                // Writing Flags / Logic
                int foundFirstWriteLoc = -1;

                do
                {
                    //Read the FAT/Data Region until we have written SIZE characters to the file.

                    //Read Hex at FatSector Position
                    hex = getHex(imgFile, FatSector, 4);
                    //Obtain Endian string, so we can determine if this is the last time we should read
                    //from the FAT and search the data region.
                    littleEndian = littleEndianHexStringFromTokens(hex);
                    FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
                    printf("FAT Endian Val: %i\n", FatSectorEndianVal);
                    //Deallocate hex and little Endian for FAT portion
                    free_tokens(hex);
                    free(littleEndian);

                    //Calculate whether we should write to the current FAT position, or move onto the next.
                    //Assuming start of file is like an array
                    if(writeStartVal >= 0 && writeStartVal < 512)
                    {
                        //We should write to this data region until the end.
                        printf("Writing....\n");
                        while(writeStartVal < 512 && bitsLeftToWrite != 0)
                        {
                            printf("Writing At Position: %i\n", writeStartVal);

                            //Open the file, we already checked that it exists. Obtain the file descriptor
                            int file = open(imgFile, O_WRONLY);
                            //Go to offset position in file. ~SEEK_SET = Absolute position in document.
                            lseek(file, DataSector + writeStartVal, SEEK_SET);
                            write(file,"J",1);
                            close(file);

                            //Move pointer for writing.
                            writeStartVal++;
                            bitsLeftToWrite--;
                        }
                        //If we have to also look at the next block, from now on we'll always start at 0.
                        printf("Bits left: %i\n", bitsLeftToWrite);
                        writeStartVal = 0;
                        foundFirstWriteLoc = 1;
                    }

                    //Go to the next FAT block, untill we have written SIZE characters to the file.
                    if(bitsLeftToWrite != 0)
                    {
                        printf("Need to move to next FAT block.\n");

                        //We have to loop again, reset FAT/Data regions.
                        FatSector = BPB.RsvdSecCnt * BPB.BytsPerSec;
                        DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
                        //New FAT Offset added
                        FatSector += FatSectorEndianVal * 4;
                        //New Data Sector Offset Added
                        DataSector += (FatSectorEndianVal - 2) * 512;
                        //New Offset for FAT
                        printf("New FAT sector: %i\n", FatSector);
                        printf("New Data sector: %i\n", DataSector);

                        //Only change writeStart if we haven't found the first writing location.
                        if(foundFirstWriteLoc != 1)
                        {
                            //Move offset of write start and end.
                            writeStartVal -= 512;
                            writeEndVal -= 512;
                        }
                    }
                    
                } while (bitsLeftToWrite != 0);
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

int openFileIndex(filesList * files, tokenlist * tokens, int flag)
{
    //Flag:
    // 1 - READ
    // 2 - WRITE

    //Check if given char * is in our given directory
    int i = 0;
    int index = -1;
    //Unlike in dirListIndex we don't need to extend the item string w/ spaces because
    //I already cut them off when opening the file and inserting them into the filesList
    for(i; i < files->size; i++)
    {
        //File was found in our list.
        if(strncmp(files->items[i]->FILE_Name, tokens->items[1], strlen(tokens->items[1])) == 0)
        {
            printf("Found\n");
            //Check that the flags of the files->items permits what we are trying to do.
            printf("File Mode: %s s\n", files->items[i]->FILE_Mode);

            //Check if we are allowed to read the file
            if(flag == 1 && (strcmp(files->items[i]->FILE_Mode, "r") == 0 || strcmp(files->items[i]->FILE_Mode, "rw") == 0
            || strcmp(files->items[i]->FILE_Mode, "wr") == 0) )
            {
                index = i;
                return index;
            }
            //Check if we are allowed to write to the file
            else if(flag == 2 && (strcmp(files->items[i]->FILE_Mode, "w") == 0 || strcmp(files->items[i]->FILE_Mode, "rw") == 0
            || strcmp(files->items[i]->FILE_Mode, "wr") == 0) )
            {
                index = i;
                return index;
            }
            else
            {
                //Invalid use of function.
                printf("Filename given is not 'opened' for %s.\n", tokens->items[0]);
                return -1;
            }
        }
    }
    //Return index of file/directory/empty if it is found. Val = -1 if not found.
    printf("Filename given is not an 'open' file.\n");
    return -1;
}

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
        printf("We good\n");
    }
    else
    {
        //No more empty entries in this directory, need to extend the FAT
        printf("Must create a new FAT entry\n");

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
    printf("Data Region to Search: %i\n", FatSectorDirCluster);

    //Modify the Data Region
    unsigned int DataSector = BPB.RsvdSecCnt * BPB.BytsPerSec + (BPB.NumFATs * BPB.FATSz32 * BPB.BytsPerSec);
    //Offset Location for N in Data (Root = 2, 1049600 : 3 = 1050112 ...)
    DataSector += (FatSectorDirCluster - 2) * 512;
    printf("Main Data Sector Start: %i\n", DataSector);
    //Offset for Empty Index Start
    DataSector += index * 32;
    printf("Main Data Sector Start + Offset: %i\n", DataSector);

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

void intToASCIIStringWrite(const char * imgFile, int value, unsigned int DataSector, int begin, int size)
{
    //Convert wanted integer (little endian) to its hex value. Assuming you aren't passing 
    //values greater than 4 million here.
    unsigned char hexString[9];
    sprintf(hexString, "%08x", value);
    printf("int: %i\n", value);
    printf("hex: %s\n", hexString);

    //Read this hex string string in little endian format.
    // OFFSET MATH:
    // AA BB CC DD - HEX wanted on HxD in little endian
    // begin = 0 -> AA
    // begin = 1 -> BB
    // begin = 2 -> CC
    // begin = 3 -> DD
    // 
    // Example:
    // begin = 0, size = 1 -> AA
    // begin = 2, size = 2 -> CC DD
    int i = 7 - (begin * 2);
    for(i; i >= 0; i -= 2)
    {
        //Initialize byte to be empty.
        char hexByte[3];
        strcpy(hexByte, "");

        //Copy 2 hex values over
        hexByte[2] = '\0';
        hexByte[1] = hexString[i];
        hexByte[0] = hexString[i - 1];
        printf("Hex %i: %s\n", i, hexByte);

        //ASCII decimal value needed
        unsigned int decASCII = (unsigned int)strtol(hexByte, NULL, 16);
        printf("ASCII Decimal: %i\n", decASCII);
        unsigned char charASCII = (unsigned char ) decASCII;
        printf("ASCII Char: %c\n", charASCII);

        //Unsigned Char to array so we can write to file
        char stringASCII[2];
        strcpy(stringASCII, "");
        strncat(stringASCII, &charASCII, 1);

        //Open the file, we already checked that it exists. Obtain the file descriptor
        int file = open(imgFile, O_WRONLY);
        printf("file: %i\n", file);
        //Go to offset position in file. ~SEEK_SET = Absolute position in document.
        lseek(file, DataSector, SEEK_SET);
        //Read from the file 'size' number of bits from decimal position given.
        //We'll convert those bit values into hex, and insert into our hex token list.
        write(file, &stringASCII, 1);
        close(file);
        //Iterate
        DataSector++;
        size--;

        //Read smaller numbers
        if(size == 0)
        {
            break;
        }
    }
}

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
    } while (FatSectorEmptyEndianVal != 0);

    //Return data
//    printf("arr[0] : FAT Sector Empty Entry Loc: %i\n", emptyEntryLoc);
//    printf("arr[1] : FAT Sector Empty End: %i\n\n", FatSectorEmpty);
    emptyArr[0] = emptyEntryLoc;
    emptyArr[1] = FatSectorEmpty;
    return emptyArr;
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
    printf("Cluster Num: %i\n", clusterStart);
    printf("FAT Sector Clus Start: %i\n", FatSectorEndClus);
    printf("FAT Sector Clus Loc: %i\n", FatSectorEndClusLoc);
    do
    {
        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSectorEndClus, 4);
        //Obtain Endian string, so we can determine if this is the last time we should read
        //from the FAT and search the data region.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEndClusEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
        printf("FAT Endian End Val: %i\n", FatSectorEndClusEndianVal);
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
            printf("New FAT sector end: %i\n", FatSectorEndClus);
        }
        else
        {
            //This should be our last iteration. Do nothing.
            printf("Last Time!\n");
        }
    
    } while ((FatSectorEndClusEndianVal < 268435448 || FatSectorEndClusEndianVal > 4294967295) && FatSectorEndClusEndianVal != 0);

    printf("endClusArr[0] : FAT Sector Clus End Loc: %i\n", FatSectorEndClusLoc);
    printf("endClusArr[1] : FAT Sector Clus End: %i\n", FatSectorEndClus);
    endClusArr[0] = FatSectorEndClusLoc;
    endClusArr[1] = FatSectorEndClus;
    return endClusArr;
}

unsigned int * findFatSectorInDir(const char * imgFile, unsigned int * fats, unsigned int clus){
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
        else
        {
            //This should be our last iteration. Do nothing.
            printf("Last Time!\n");
        }
    }
    fats[1] =  FatSectorDirCluster;
    return fats;
}
//////////////////////////////////////////////////////
// Directory List Logic //////////////
//////////////////////////////////////////////////////
dirlist *new_dirlist(void)
{
    dirlist * dirs = (dirlist *) malloc(sizeof(dirlist));
	dirs->size = 0;
	dirs->items = (DIRENTRY **) malloc(sizeof(DIRENTRY *));
	return dirs;
}

void free_dirlist(dirlist * directories)
{
    int i = 0;
	for (i; i < directories->size; i++)
    {
        free(directories->items[i]);
    }
	free(directories);
}

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
    //Store List of Directories in whatever folder given by user
    dirlist * dirs = new_dirlist();
    dirs->CUR_Clus = N;
    printf("Current Cluster: %i\n", dirs->CUR_Clus);

    do
    {
        //Read the FAT until we are at the end of the chosen cluster (N). This will tell us
        //the data sectors we should go to in the data region of sector size 512.

        //We have already positioned ourselves in the *first* position with previous math.
        printf("FAT Sector Start: %i\n", FatSector);

        //Read Hex at FatSector Position
        hex = getHex(imgFile, FatSector, 4);
        //Obtain Endian string, so we can determine if this is the last time we should read
        //from the FAT and search the data region.
        littleEndian = littleEndianHexStringFromTokens(hex);
        FatSectorEndianVal = (unsigned int)strtol(littleEndian, NULL, 16);
        printf("FAT Endian Val: %i\n", FatSectorEndianVal);
        //Deallocate hex and little Endian for FAT portion
        free_tokens(hex);
        free(littleEndian);

        printf("Data Sector Start: %i\n", DataSector);
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
            //New Offset for FAT
            printf("New FAT sector: %i\n", FatSector);
            printf("New Data sector: %i\n", DataSector);
        }
        else
        {
          //This should be our last iteration. Do nothing.
          printf("Last Time!\n");
        }
    } while ((FatSectorEndianVal < 268435448 || FatSectorEndianVal > 4294967295) && FatSectorEndianVal != 0);
    
    return dirs;
}

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
            else
            {
                printf("File we don't care about.\n");
            }
        }
        else
        {
            //Empty File stored in readEntry
            printf("Empty.\n");
        }
    }
}
int dirlistIndexOfFileOrDirectory(dirlist * directories, const char * item, int flag)
{
    //Input Flags
    //1 - File
    //2 - Directory
    //3 - Either File or Directory
    //4 - Empty
    //Check if given char * is in our given directory
    int i = 0;
    int found = -1;
    unsigned char name[11];
    strcpy(name, item);
    strncat(name, "           ", 11 - strlen(item));
    for(i; i < directories->size; i++)
    {
        //Compare only up to only strlen(item) b/c there will be spaces left from
        //reading it directly from the .img file. 
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

/////////////////////////////////////////////////////
// Open Files Logic
/////////////////////////////////////////////////////
filesList * new_filesList(void)
{
    filesList * files = (filesList *) malloc(sizeof(filesList));
    files->size = 0;
    files->items = (FILEENTRY **) malloc(sizeof(FILEENTRY *));
    return files;
}

void free_filesList(filesList * openFiles)
{
    int i = 0;
	for (i; i < openFiles->size; i++)
    {
        free(openFiles->items[i]);
    }
	free(openFiles);
}

int createOpenFileEntry(filesList * openFiles, dirlist * directories, tokenlist * tokens, int index)
{
    //First Check that we aren't creating a duplicate entry.
    //Need the cluster number as well b/c files can have the same name in different directorys.
    char * clusterHI = littleEndianHexStringFromUnsignedChar(directories->items[index]->DIR_FstClusHI, 2);
    char * clusterLOW = littleEndianHexStringFromUnsignedChar(directories->items[index]->DIR_FstClusLO, 2);
    strcat(clusterHI, clusterLOW);
    unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
    //unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
    free(clusterHI);
    free(clusterLOW);
    //Check cluster and name, if not found filesListIndex returns -1
    if(filesListIndex(openFiles, tokens->items[1]) == -1)
    {
        //Create a new entry in our open files list.
        openFiles->items = (FILEENTRY **) realloc(openFiles->items, (openFiles->size + 1) * sizeof(FILEENTRY));
        openFiles->items[openFiles->size] = malloc(sizeof(FILEENTRY));
        //Copy relevant data over:
        //1. Name
        strcpy(openFiles->items[openFiles->size]->FILE_Name, "");
        strcpy(openFiles->items[openFiles->size]->FILE_Name, tokens->items[1]);
        //2. Cluster Info
        openFiles->items[openFiles->size]->FILE_FstClus = clusterValHI;
        //3. Mode
        strcpy(openFiles->items[openFiles->size]->FILE_Mode, "");
        strcpy(openFiles->items[openFiles->size]->FILE_Mode, tokens->items[2]);
        //4. Offset
        openFiles->items[openFiles->size]->FILE_OFFSET = 0;
        //5. File Size
        char * sizeStr = littleEndianHexStringFromUnsignedChar(directories->items[index]->DIR_FileSize, 4);
        unsigned int fileSize = (unsigned int)strtol(sizeStr, NULL, 16);
        openFiles->items[openFiles->size]->FILE_SIZE = fileSize;
        free(sizeStr);
        //Iterate size of openFiles
        openFiles->size += 1;
        return 1;
    }
    else
    {
        //Found an entry with same name/cluster val
        return -1;
    }
}

void readFilesList(filesList * openFiles)
{
    //Reading Files List
    int i = 0;
    for (i; i < openFiles->size; i++)
    {
        printf("(open) %-11s : (mode) %-2s : (clus) %3i : (offset) %6i : (size) %6i\n", openFiles->items[i]->FILE_Name, openFiles->items[i]->FILE_Mode,
        openFiles->items[i]->FILE_FstClus, openFiles->items[i]->FILE_OFFSET, openFiles->items[i]->FILE_SIZE);
    }
}

int filesListIndex(filesList * openFiles, const char * item)
{
    //Check if given char * is in our given files list
    int i = 0;
    int found = -1;
    for (i; i < openFiles->size; i++)
    {
        //Check if we that item in our list.
        if(strncmp(openFiles->items[i]->FILE_Name, item, strlen(item)) == 0 )
        {
            found = i;
            break;
        }
    }
    return found;
}
//////////////////////////////////////////////////////
// Parsing Hex/Unsigned Char Values    //////////////
//////////////////////////////////////////////////////
tokenlist * getHex(const char * imgFile, int decStart, int size)
{
   // printf("getHex()\n");
    //C-String of Bit Values and Token List of Hex Values.
    unsigned char * bitArr = malloc(sizeof(unsigned char) * size + 1);
    tokenlist * hex = new_tokenlist();

    //Initialize to get rid of garbage data
    strcpy(bitArr, "");
    //Open the file, we already checked that it exists. Obtain the file descriptor
    int file = open(imgFile, O_RDONLY);
    //Go to offset position in file. ~SEEK_SET = Absolute position in document.
    lseek(file, decStart, SEEK_SET);
    //Read from the file 'size' number of bits from decimal position given.
    //We'll convert those bit values into hex, and insert into our hex token list.
    int i = 0;
    char buffer[3];
    read(file, bitArr, size);
    for(i; i < size; i++)
    {
        //Create hex string using input. Size should always be 3
        //for 2 bits and 1 null character. 
        snprintf(buffer, 3, "%02x", bitArr[i]);
        //printf("%s ", buffer);
        add_token(hex, buffer);
    }
    printf("\n");
    //Close working file and deallocate working array.
    close(file);
    free(bitArr);
    //Tokenlist of hex values.
    return hex;
}

char * littleEndianHexStringFromTokens(tokenlist * hex)
{
    //printf("littleEndianHexStringFromTokens()\n");
    //Allocate 2 * hex->size since we store 2 hexes at each item
    char * littleEndian = malloc(sizeof(char) * hex->size * 2 + 1);
    //Initialize to get rid of garbage data
    strcpy(littleEndian, "");
    //Little Endian = Reading Backwards by 2
    int end = hex->size - 1;
    for(end; end >= 0; end--)
    {
        strcat(littleEndian, hex->items[end]);
    }
    //printf("%s\n\n", littleEndian);
    return littleEndian;
}

char * littleEndianHexStringFromUnsignedChar(unsigned char * arr, int size)
{
    printf("littleEndianHexStringFromUnsignedChar()\n");
    //Allocate 2 * hex->size since we store 2 hexes at each item
    char * littleEndian = malloc(sizeof(char) * size * 2 + 1);
    //Initialize to get rid of garbage data
    strcpy(littleEndian, "");
    //Little Endian = Reading Backwards by 2
    char buffer[3];
    int end = size - 1;
    for(end; end >= 0; end--)
    {
        snprintf(buffer, 3, "%02x", arr[end]);
        strcat(littleEndian, buffer);
    }
    printf("%s\n\n", littleEndian);
    return littleEndian;
}

//CURRENTLY UNUSED
char * bigEndianHexString(tokenlist * hex)
{
    printf("bigEndianHexString()\n");
    //Allocate 2 * hex->size since we store 2 hexes at each item
    char * bigEndian = malloc(sizeof(char) * hex->size * 2 + 1);
    //Initialize to get rid of garbage data
    strcpy(bigEndian, "");
    //Read hex forwards
    int begin = 0;
    for(begin; begin < hex->size; begin++)
    {
        strcat(bigEndian, hex->items[begin]);
    }
    printf("%s\n\n", bigEndian);
    return bigEndian;
}

void getBIOSParamBlock(const char * imgFile)
{
    printf("=== Info ===\n");
    tokenlist * hex;
    char * littleEndian;

    //Calculate Bytes Per Sector
    hex = getHex(imgFile, 11, 2);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.BytsPerSec = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("Bytes Per Sector: %d\n", BPB.BytsPerSec);
    free_tokens(hex);
    free(littleEndian);
    printf("=======\n");

    //Calculate Sectors per Cluster
    hex = getHex(imgFile, 13, 1);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.SecPerClus = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("Sectors per Cluster: %d\n", BPB.SecPerClus);
    free_tokens(hex);
    free(littleEndian);
    printf("=======\n");

    //Calculate Reserved Sector Count
    hex = getHex(imgFile, 14, 2);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.RsvdSecCnt = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("Reserved Sector Count: %d\n", BPB.RsvdSecCnt);
    free_tokens(hex);
    free(littleEndian);
    printf("=======\n");

    //Calculate number of FATs
    hex = getHex(imgFile, 16, 1);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.NumFATs = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("Number of FATs: %d\n", BPB.NumFATs);
    free_tokens(hex);
    free(littleEndian);
    printf("=======\n");

    //Calculate total sectors
    hex = getHex(imgFile, 32, 4);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.TotSec32 = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("Total Sectors: %d\n", BPB.TotSec32);
    free_tokens(hex);
    free(littleEndian);
    printf("=======\n");

    //Calculate FAT size
    hex = getHex(imgFile, 36, 4);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.FATSz32 = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("FAT size: %d\n", BPB.FATSz32);
    free_tokens(hex);
    free(littleEndian);
    printf("=======\n");

    //Calculate Root Cluster
    hex = getHex(imgFile, 44, 4);
    littleEndian = littleEndianHexStringFromTokens(hex);
    BPB.RootClus = (unsigned int)strtol(littleEndian, NULL, 16);
    printf("Root Cluster: %d\n", BPB.RootClus);
    free_tokens(hex);
    free(littleEndian);
}

//Function that attempts to open specified file and returns 1 if successful
int file_exists(const char * filename)
{
    FILE * file;
    if(file = fopen(filename,"r"))
    {
        fclose(file);
        return 1;
    }
    return 0;
}
//////////////////////////////////////////////////////
// Parsing Input: Taken from Project #1 //////////////
//////////////////////////////////////////////////////
tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **) malloc(sizeof(char *));
	tokens->items[0] = NULL;
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);
	tokens->size += 1;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

tokenlist *get_tokens(char *input)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	int i = 0;
	for (i; i < tokens->size; i++)
        free(tokens->items[i]);

	free(tokens);
}