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

        //Commands
        if(strcmp("exit", tokens->items[0]) == 0 && tokens->size == 1)
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
                    unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
                    unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
                    //Make list structure containing all files found in DIRNAME cluster.
                    dirlist * lsDirectory = getDirectoryList(imgFile, clusterValHI + clusterValLOW);
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
                unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
                unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
                //free the CWD
                free_dirlist(currentDirectory);
                //case for CD to root directory
                if(clusterValLOW == 0){
                    currentDirectory = getDirectoryList(imgFile, BPB.RootClus);
                }
                //case for cd to any other directory
                else{
                    currentDirectory = getDirectoryList(imgFile, clusterValHI + clusterValLOW);
                }
                free(clusterHI);
                free(clusterLOW);   
            }
            else
            {
                printf("Directory not found.\n");
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

                printf("Size! %i\n\n", openFiles->size);
                int i = 0;
                for(i; i < openFiles->size; i++)
                {
                    //Copy everything over except the FILENAME given
                    if(i != index)
                    {
                        printf("Copied! %i\n\n", index);
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
                printf("Copied!\n");
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
        else
        {
            printf("Invalid Command Given\n");
        }
        free(input);
        free_tokens(tokens);
    }
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
    char * bigEndian;
    //Store List of Directories in whatever folder given by user
    dirlist * dirs = new_dirlist();

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
        }
        else
        {
            //Empty File stored in readEntry
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
    for(i; i < directories->size; i++)
    {
        //Compare only up to only strlen(item) b/c there will be spaces left from
        //reading it directly from the .img file. 
        if(strncmp(directories->items[i]->DIR_Name, item, strlen(item)) == 0 )
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
    unsigned int clusterValHI = (unsigned int)strtol(clusterHI, NULL, 16);
    unsigned int clusterValLOW = (unsigned int)strtol(clusterLOW, NULL, 16);
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
        openFiles->items[openFiles->size]->FILE_FstClus = clusterValHI + clusterValLOW;
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
    printf("getHex()\n");
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
        printf("%s ", buffer);
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
    printf("littleEndianHexStringFromTokens()\n");
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
    printf("%s\n\n", littleEndian);
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