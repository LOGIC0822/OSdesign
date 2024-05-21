all : MyCopy ForkCopy PipeCopy
clean:
	rm *.o
MyCopy: MyCopy.o 
	gcc -o MyCopy MyCopy.o
MyCopy.o: MyCopy.c header.h
	gcc -c MyCopy.c
ForkCopy: ForkCopy.o
	gcc -o ForkCopy ForkCopy.o
ForkCopy.o: ForkCopy.c header.h
	gcc -c ForkCopy.c
PipeCopy: PipeCopy.o
	gcc -o PipeCopy PipeCopy.o
PipeCopy.o: PipeCopy.c header.h
	gcc -c PipeCopy.c
