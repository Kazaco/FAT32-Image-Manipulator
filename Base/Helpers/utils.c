#include "utils.h"
#include "parser.h"
#include <stdio.h> 		//printf()
#include <stdlib.h>     //free(), realloc()
#include <string.h>     //strchr(), memcpy(), strcpy(), strcat()
#include <fcntl.h>      //O_RDONLY
#include <unistd.h>     //lseek

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

//Function that converts tokenlist values to a little endian string
char * littleEndianHexStringFromTokens(tokenlist * hex)
{
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
    return littleEndian;
}

//Function that reads a file for hexadecimal characters and inserts
//them into a tokenlist.
tokenlist * getHex(const char * imgFile, int decStart, int size)
{
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
        add_token(hex, buffer);
    }
    //Close working file and deallocate working array.
    close(file);
    free(bitArr);
    //Tokenlist of hex values.
    return hex;
}