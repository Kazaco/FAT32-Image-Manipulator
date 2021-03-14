
project3: main.o ./Helpers/utils.o ./Helpers/parser.o ./Helpers/directorylist.o ./Helpers/fileslist.o ./Helpers/fat.o ./Commands/info.o ./Commands/size.o ./Commands/ls.o ./Commands/cd.o ./Commands/creat.o
	gcc -o project3 main.o ./Helpers/utils.o ./Helpers/parser.o ./Helpers/directorylist.o ./Helpers/fileslist.o ./Helpers/fat.o ./Commands/info.o ./Commands/size.o ./Commands/ls.o ./Commands/cd.o ./Commands/creat.o

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

clean:
	rm *.o ./Helpers/*.o ./Commands/*.o project3
