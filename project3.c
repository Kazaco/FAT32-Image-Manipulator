#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy()
#include <fcntl.h>      //O_RDONLY

typedef struct {
	int size;
	char **items;
} tokenlist;

typedef struct {
    unsigned int BytsPerSec;    //Bytes per sector
    unsigned int SecPerClus;    //Sectors per cluster
    unsigned int RsvdSecCnt;    //Reserved  region size
    unsigned int NumFATs;       //Number of FATs
    unsigned int FATSz32;       //FAT size
    unsigned int RootClus;      //Root cluster
    unsigned int TotSec32;      //Total sectors
} BPB;

char *get_input(void);
tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
////////////////////////////////////
int file_exists(const char * filename);
void running(const char * imgFile);
char * readHexToString(const char * imgFile, int decStart, int size);

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
            printf("Info\n");
            //Calculate size for BytsPerSec
            char * result = readHexToString(imgFile, 11, 2);
        }
        else if(strcmp("size", tokens->items[0]) == 0 && tokens->size == 2)
        {
            printf("Size\n");
            //Open the file, we already checked that it exists. Obtain the file descriptor
			// int file = open(imgFile, O_RDONLY);
            // int offset = lseek(file, 17008, SEEK_SET);

            // unsigned char arr[16];
            // read(file, arr, 16);
            // int i = 0;
            // for(i; i < 16; i++)
            // {
            //     printf("%x ", arr[i]);
            // }
            // printf("\n");
            // close(file);
        }
        else
        {
            printf("Invalid Command Given\n");
        }
        free(input);
        free_tokens(tokens);
    }
}

char * readHexToString(const char * imgFile, int decStart, int size)
{
    unsigned char * bitArr = malloc(size);
    char * returnStr;

    //Open the file, we already checked that it exists. Obtain the file descriptor
	int file = open(imgFile, O_RDONLY);

    //Go to offset position in file. ~SEEK_SET = Absolute position in document.
    int offset = lseek(file, decStart, SEEK_SET);
    printf("%d\n", offset);
    //Read from the file 'size' number of bits from decimal position given.
    int i = 0;
    read(file, bitArr, size);
    for(i; i < size; i++)
    {
        //Need a to reinsert a leading zero.
        printf("%02x ", bitArr[i]);
    }
    printf("\n");

    //Little Endian Fix
    int j = size - 1;
    for(j; j >= 0; j--)
    {
        printf("%02x ", bitArr[j]);
    }
    printf("\n");

    close(file);
    //int number = (int)strtol(str, NULL, 16);
    free(bitArr);

    return "result";
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