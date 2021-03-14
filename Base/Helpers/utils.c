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

//Function that converts string to a little endian string
char * littleEndianHexStringFromUnsignedChar(unsigned char * arr, int size)
{
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
    return littleEndian;
}

//Convert wanted integer (little endian) to its hex value. Assuming we aren't passing
//values greater than 4 million here.
void intToASCIIStringWrite(const char * imgFile, int value, unsigned int DataSector, int begin, int size)
{
    unsigned char hexString[9];
    sprintf(hexString, "%08x", value);

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

        //ASCII decimal value needed
        unsigned int decASCII = (unsigned int)strtol(hexByte, NULL, 16);
        unsigned char charASCII = (unsigned char ) decASCII;

        //Unsigned Char to array so we can write to file
        char stringASCII[2];
        strcpy(stringASCII, "");
        strncat(stringASCII, &charASCII, 1);

        //Open the file, we already checked that it exists. Obtain the file descriptor
        int file = open(imgFile, O_WRONLY);
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