# FAT32-Image-Manipulator

## Divison of Labor
| Name | Sections |
| --- | --- |
| Alexander Kostandarithes | lseek, read, write, and README |
| Karl Cooley | initial setup, exit, info, size, ls, creat, mkdir, open, close, write, debugging, and reformatted code. |
| Ryan Goldberg | cd, mv, rm, cp, rmdir |
  
## How to compile and run:  
```
Type "make" in the "Base" folder
Type "./project3 fat32.img" to execute the program.  
```
  
## Bugs/Noted Behaviors:
1. If you copy LONGFILE it will take a while to finish properly.
2. If you open directory GREEN it will take awhile to allocate all the empty files 
to allow for correct behavior with write/read/rm/cp/ect.
3. Reading characters from a file can be misleading to actually how many characters
there are inbetween items. Null characters aren't printed to the screen as spaces so items
may be farther away in the Data Region than shown to user.
4. Write/Read do move the file offset when used (as required by spec). However, this isn't 
made clear to the user when using the program.
  
## Screenshot
![Image of Utility](https://i.imgur.com/I0kRBfX.png)
