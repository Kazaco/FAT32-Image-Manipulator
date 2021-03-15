#include "help.h"
#include <stdio.h> 		//printf()

void printHelp()
{
    printf("List of Commands and Summary:\n");
    printf("1. exit - Ends this program.\n");
    printf("a. exit\n\n");
    printf("2. info - Prints the BIOS Paramter Block from the image\n");
    printf("a. info\n\n");
    printf("3. size - Prints the size of a file in bytes\n");
    printf("a. size <filename>\n\n");
    printf("4. ls - Prints the files located in a Directory\n");
    printf("a. ls\n");
    printf("b. ls <directory>\n\n");
    printf("5. cd - Changes current working directory.\n");
    printf("a. cd <directory>\n\n");
    printf("6. creat - Creates a new file\n");
    printf("a. creat <filename>\n\n");
    printf("7. mkdir - Creates a new directory\n");
    printf("a. mkdir <dirname>\n\n");
    printf("8. open - Opens a file for reading/writing based on flags\n");
    printf("a. open <filename> <r/w/rw/wr>\n\n");
    printf("9. close - Closes a file to prevent reading/writing to it\n");
    printf("a. close <filename>\n\n");
    printf("10. mv - Moves a file/directory to another folder or changes the name of a file\n");
    printf("a. mv <filename> <dirname> (moves file to dirname)\n");
    printf("b. mv <dirname> <dirname> (moves directory to dirname)\n");
    printf("c. mv <dirname> <newdirname> (If newdirname DNE, changes dirname's name)\n");
    printf("d. mv <filename> <newfilename> (If newfilename DNE, changes filename's name)\n\n");
    printf("11. rm - Deletes a file in current directory\n");
    printf("a. rm <filename>\n\n");
    printf("12. write - Writes to an existing file with a given string for N characters from lseek location. MUST BE OPENED WITH OPEN CMD!\n");
    printf("a. write <filename> <numChars> <string>\n");
    printf("ex. write HELLO 20 \"Hello Goodbye\"\n\n");
    printf("13. read - Outputs files contents to the screen for N characters from lseek location. MUST BE OPENED WITH OPEN COMMAND!\n");
    printf("a. read <filename> <numChars>\n");
    printf("ex. read HELLO 20\n\n");
    printf("14. lseek - Changes the offset location of a pointer in a given file or shows currently opened files\n");
    printf("a. lseek (shows currently opened files)\n");
    printf("b. lseek <open_file> <offset>\n");
    printf("15. cp - Copies a filename into a new file in current directory or into a new directory.\n");
    printf("a. cp <filename> <newfilename> (creates copy of file in cwd)\n");
    printf("b. cp <filename> <directory> (creates copy of file and places it into directory)\n\n");
    printf("16. rmdir - Deletes a Directory\n");
    printf("a. rmdir <dirname>\n");
}