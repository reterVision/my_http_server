CC = gcc
LD = gcc

all: CC += -c -Wall
all: gd

debug: CC += -DDEBUG -g -c
debug: gd

gd: gcd.o gmalloc.o
	$(LD) -o gd gcd.o gmalloc.o

gcd.o: gcd.c gmalloc.c
	$(CC) gcd.c gmalloc.c

clean:
	rm -rf *.o gd
