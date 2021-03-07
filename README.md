# FAT32-Image-Manipulator

## Divison of Labor
| Name | Sections |
| --- | --- |
| Alexander Kostandarithes | lseek, read, write, and README |
| Karl Cooley | initial setup, exit, info, size, ls, creat, mkdir, open, close, write, debugging, and updated README |
| Ryan Goldberg | cd, mv, rm, cp, rmdir |

## Files 
| Name | Description |
| --- | --- |
| project3.c | Has all the required code to completed requested tasks for Project 3 |
| makefile | will compile project3.c on command |
| fat32.img | The .img needed by project3.c   |
| GitLog.docx | The screenshot to show commits from members   |
| run.txt | Shows how to open fat32.img on linux os |
  
## How to compile and run:  
```
First untar with "tar -xvf project3_Kostandarithes_Cooley_Goldberg.tgz"  
Type "make" in the terminal window
Type "./project3 fat32.img" to execute the menu.  
```

## makefile contents:  
```
project3: project3.o  
        gcc -o project3 project3.o  
project3.o: project3.c  
        gcc -c project3.c  
clean:  
        rm *.o project3  
```
  
## Bugs/Noted Behaviors:
1. If you copy LONGFILE it will take a while to finish properly
2. If you open directory GREEN it will take awhile to allocate all the empty files 
to allow for correct behavior with write/read/rm/cp/ect.
3. Reading characters from a file can be misleading to actually how many characters
there are inbetween items. Null characters aren't printed to the screen as spaces so items
may be farther away in the Data Region than shown to user.
4. Write/Read do move the file offset when used (as required by spec). However, this isn't 
made clear to the user when using the program.
  
## Extra Credit:  
rmdir is operational  

## Screenshot
![Image of Utility](https://i.imgur.com/I0kRBfX.png)
