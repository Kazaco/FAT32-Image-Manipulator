
project3: main.o ./Helpers/utils.o ./Helpers/parser.o ./Helpers/directorylist.o ./Helpers/fileslist.o ./Helpers/fat.o ./Commands/info.o ./Commands/size.o ./Commands/ls.o ./Commands/cd.o ./Commands/creat.o ./Commands/mkdir.o ./Commands/open.o ./Commands/close.o ./Commands/mv.o ./Commands/rm.o ./Commands/write.o ./Commands/read.o ./Commands/lseek.o ./Commands/cp.o ./Commands/rmdir.o ./Commands/help.o
	gcc -o project3 main.o ./Helpers/utils.o ./Helpers/parser.o ./Helpers/directorylist.o ./Helpers/fileslist.o ./Helpers/fat.o ./Commands/info.o ./Commands/size.o ./Commands/ls.o ./Commands/cd.o ./Commands/creat.o ./Commands/mkdir.o ./Commands/open.o ./Commands/close.o ./Commands/mv.o ./Commands/rm.o ./Commands/write.o ./Commands/read.o ./Commands/lseek.o ./Commands/cp.o ./Commands/rmdir.o ./Commands/help.o

main.o: main.c
	gcc -c main.c

utils.o: ./Helpers/utils.c ./Helpers/utils.h
	gcc -c ./Helpers/utils.c -o ./Helpers/utils.o

parser.o: ./Helpers/parser.c ./Helpers/parser.h
	gcc -c ./Helpers/parser.c -o ./Helpers/parser.o

directorylist.o: ./Helpers/directorylist.c ./Helpers/directorylist.h
	gcc -c ./Helpers/directorylist.c -o ./Helpers/directorylist.o

fileslist.o: ./Helpers/fileslist.c ./Helpers/fileslist.h
	gcc -c ./Helpers/fileslist.c -o ./Helpers/fileslist.o

fat.o: ./Helpers/fat.c ./Helpers/fat.h
	gcc - c ./Helpers/fat.c -o ./Helpers/fat.o

info.o: ./Commands/info.c ./Commands/info.h
	gcc -c ./Commands/info.c -o ./Commands/info.o

size.o: ./Commands/size.c ./Commands/size.h
	gcc -c ./Commands/size.c -o ./Commands/size.o

ls.o: ./Commands/ls.c ./Commands/ls.h
	gcc -c ./Commands/ls.c -o ./Commands/ls.o

cd.o: ./Commands/cd.c ./Commands/cd.h
	gcc -c ./Commands/cd.c -o ./Commands/cd.o

creat.o: ./Commands/creat.c ./Commands/creat.h
	gcc -c ./Commands/creat.c -o ./Commands/creat.o

mkdir.o: ./Commands/mkdir.c ./Commands/mkdir.h
	gcc -c ./Commands/mkdir.c -o ./Commands/mkdir.o

open.o: ./Commands/open.c ./Commands/open.h
	gcc -c ./Commands/open.c -o ./Commands/open.o

close.o: ./Commands/close.c ./Commands/close.h
	gcc -c ./Commands/close.c -o ./Commands/close.o

mv.o: ./Commands/mv.c ./Commands/mv.h
	gcc -c ./Commands/mv.c -o ./Commands/mv.o

rm.o: ./Commands/rm.c ./Commands/rm.h
	gcc -c ./Commands/rm.c -o ./Commands/rm.o

write.o: ./Commands/write.c ./Commands/write.h
	gcc -c ./Commands/write.c -o ./Commands/write.o

read.o: ./Commands/read.c ./Commands/read.h
	gcc -c ./Commands/read.c -o ./Commands/read.o

lseek.o: ./Commands/lseek.c ./Commands/lseek.h
	gcc -c ./Commands/lseek.c -o ./Commands/lseek.o

cp.o: ./Commands/cp.c ./Commands/cp.h
	gcc -c ./Commands/cp.c -o ./Commands/cp.o

rmdir.o: ./Commands/rmdir.c ./Commands/rmdir.h
	gcc -c ./Commands/rmdir.c -o ./Commands/rmdir.o

help.o: ./Commands/help.c ./Commands/help.h
	gcc -c ./Commands/help.c -o ./Commands/help.o

clean:
	rm *.o ./Helpers/*.o ./Commands/*.o project3
