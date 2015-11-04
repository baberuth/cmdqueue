UNAME=$(shell uname)

CCFLAGS=-Wall -Wextra -Wno-unused-parameter -std=c99 -O3

all: main

remake: clean all

%.o: %.c util.h cmdqueue.h list.h
	gcc $(CCFLAGS) -c -o $@ $<

main: main.o cmdqueue.o list.o
	gcc $(LDFLAGS) main.o cmdqueue.o list.o -o main

clean:
	rm -f main *.o

