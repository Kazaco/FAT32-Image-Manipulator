
project3: main.o ./Helpers/utils.o ./Helpers/parser.o ./Commands/info.o 
	gcc -o project3 main.o ./Helpers/utils.o ./Helpers/parser.o ./Commands/info.o	

main.o: main.c
	gcc -c main.c

utils.o: ./Helpers/utils.c ./Helpers/utils.h
	gcc -c ./Helpers/utils.c -o ./Helpers/utils.o

parser.o: ./Helpers/parser.c ./Helpers/parser.h
	gcc -c ./Helpers/parser.c -o ./Helpers/parser.o

info.o: ./Commands/info.c ./Commands/info.h
	gcc -c ./Commands/info.c -o ./Commands/info.o


clean:
	rm *.o ./Helpers/*.o ./Commands/*.o project3
