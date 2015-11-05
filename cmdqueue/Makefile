UNAME=$(shell uname)

CCFLAGS=-Wall -Wextra -Wno-unused-parameter -std=c99 -O3

all: main test

remake: clean all

%.o: %.c util.h cmdqueue.h list.h mycmdqueue.h
	gcc $(CCFLAGS) -I../ctest/ -c -o $@ $<

test: testmain.o main.o list.o mycmdqueue.o mytests.o
	gcc $(LDFLAGS) testmain.o cmdqueue.o list.o mycmdqueue.o mytests.o -o test -lpthread

main: main.o cmdqueue.o list.o mycmdqueue.o
	gcc $(LDFLAGS) main.o cmdqueue.o list.o mycmdqueue.o -o main -lpthread

clean:
	rm -f test main *.o

