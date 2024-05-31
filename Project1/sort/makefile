all: MergesortSingle MergesortMulti
clean:
	rm *.o
MergesortSingle: MergesortSingle.o
	gcc -o MergesortSingle MergesortSingle.o
MergesortMulti: MergesortMulti.o
	gcc -o MergesortMulti MergesortMulti.o
MergesortSingle.o: MergesortSingle.c header.h
	gcc -c MergesortSingle.c
MergesortMulti.o: MergesortMulti.c header.h
	gcc -c MergesortMulti.c