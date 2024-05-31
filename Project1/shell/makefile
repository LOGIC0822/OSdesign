all : shell
clean :
	rm *.o
shell: shell.o
	gcc -o shell shell.o
shell.o: shell.c header.h
	gcc -c shell.c

