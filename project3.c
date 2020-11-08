#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy()
#include <fcntl.h>      //O_RDONLY

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

///////////////////////////////////
char *get_input(void);
tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
////////////////////////////////////
int file_exists(const char * filename);
void running(const char * imgFile);
tokenlist * getHex(const char * imgFile, int decStart, int size);
char * littleEndianHexString(tokenlist * hex);
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
    while(1)
    {
        //User Initial Input
        printf("> ");
        char *input = get_input();
        //Split User Input into Tokens
        tokenlist *tokens = get_tokens(input);
        int i = 0;
        for (i; i < tokens->size; i++) 
        {
            printf("token %d: (%s)\n", i, tokens->items[i]);
        }

        //Commands
        if(strcmp("exit", tokens->items[0]) == 0 && tokens->size == 1)
        {
            printf("Exit\n");
            free(input);
            break;
        }
        else if(strcmp("info", tokens->items[0]) == 0 && tokens->size == 1)
        {
            printf("=== Info ===\n");
            tokenlist * hex;
            char * littleEndian;

            //Calculate Bytes Per Sector
            hex = getHex(imgFile, 11, 2);
            littleEndian = littleEndianHexString(hex);
            BPB.BytsPerSec = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("Bytes Per Sector: %d\n", BPB.BytsPerSec);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");

            //Calculate Sectors per Cluster
            hex = getHex(imgFile, 13, 1);
            littleEndian = littleEndianHexString(hex);
            BPB.SecPerClus = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("Sectors per Cluster: %d\n", BPB.SecPerClus);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");

            //Calculate Reserved Sector Count
            hex = getHex(imgFile, 14, 2);
            littleEndian = littleEndianHexString(hex);
            BPB.RsvdSecCnt = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("Reserved Sector Count: %d\n", BPB.RsvdSecCnt);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");

            //Calculate number of FATs
            hex = getHex(imgFile, 16, 1);
            littleEndian = littleEndianHexString(hex);
            BPB.NumFATs = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("Number of FATs: %d\n", BPB.NumFATs);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");

            //Calculate total sectors
            hex = getHex(imgFile, 32, 4);
            littleEndian = littleEndianHexString(hex);
            BPB.TotSec32 = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("Total Sectors: %d\n", BPB.TotSec32);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");

            //Calculate FAT size
            hex = getHex(imgFile, 36, 4);
            littleEndian = littleEndianHexString(hex);
            BPB.FATSz32 = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("FAT size: %d\n", BPB.FATSz32);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");

            //Calculate Root Cluster
            hex = getHex(imgFile, 44, 4);
            littleEndian = littleEndianHexString(hex);
            BPB.RootClus = (unsigned int)strtol(littleEndian, NULL, 16);
            printf("Root Cluster: %d\n", BPB.RootClus);
            free_tokens(hex);
            free(littleEndian);
            printf("=======\n");
        }
        else if(strcmp("size", tokens->items[0]) == 0 && tokens->size == 2)
        {
            printf("Size\n");
        }
        else
        {
            printf("Invalid Command Given\n");
        }
        free(input);
        free_tokens(tokens);
    }
}

tokenlist * getHex(const char * imgFile, int decStart, int size)
{
    printf("getHex()\n");
    //C-String of Bit Values and Token List of Hex Values.
    unsigned char * bitArr = (unsigned char *) malloc(sizeof(unsigned char) * size);
    tokenlist * hex = new_tokenlist();

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

char * littleEndianHexString(tokenlist * hex)
{
    printf("littleEndianHexString()\n");
    //Allocate 2 * hex->size since we store 2 hexes at each item
    char * littleEndian = (char *) malloc(sizeof(char) * hex->size * 2);
    //Little Endian = Reading Backwards by 2
    int end = hex->size - 1;
    for(end; end >= 0; end--)
    {
        strcat(littleEndian, hex->items[end]);
    }
    printf("%s\n\n", littleEndian);
    return littleEndian;
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