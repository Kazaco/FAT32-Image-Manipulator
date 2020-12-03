Project 3 README 

Names and Division of Labor:
Alexander Kostandarithes: lseek, read, write, and README
Karl Cooley: initial setup, exit, info, size, ls, creat, mkdir, open, close, and write
Ryan Goldberg:

Files and descriptions:
project3.c: Has all the required code to completed requested tasks for Project 3
makefile: will compile project3.c on command
fat32.img: The .img needed by project3.c
README: This
GIT Commit Log: The screenshot to show commits from members

How to compile and run: 
First untar with "tar -xvf project3_Kostandarithes_Cooley_Goldberg.tgz"
Type "make" in the terminal window, then run "./project3 fat32.img" to execute the menu.

makefile contents:
project3: project3.o
        gcc -o project3 project3.o
project3.o: project3.c
        gcc -c project3.c
clean:
        rm *.o project3



Bugs:
1. If you copy/mv LONGFILE multiple times there is a possibility the program will break.

Extra Credit: