CFLAGS=-Wall -Wextra -Wno-unused -Wno-unused-parameter -std=c99

HEADERS=\
    cmdqueue/list.h \
    ctest/ctest.h

SOURCES=\
    list_test.c \
    cmdqueue/list.c \
    main.c

test: $(HEADERS) $(SOURCES)
	gcc $(CFLAGS) $(SOURCES) -o test

clean:
	rm -f *.o test


