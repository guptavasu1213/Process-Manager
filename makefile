CC=gcc
CFLAGS= -Wall -g 

all: macD

macD: macD.c
	$(CC) $(CFLAGS) -o macD macD.c 
testleaks:
	valgrind --leak-check=full --show-leak-kinds=all -v ./macD
checkp:
	/tmp/cmpt360/checkpatch.pl --no-tree --file --terse macD.c
