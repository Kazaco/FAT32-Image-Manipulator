
project3: project3.o
	gcc -o project3 project3.o
project3.o: project3.c
	gcc -c project3.c
clean:
	rm *.o project3
