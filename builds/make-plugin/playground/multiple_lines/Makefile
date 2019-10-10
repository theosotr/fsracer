C=gcc
CFLAGS=-I.
OBJECTS = s1.o s2.o

.PHONY: obj clean all

all: main

main.o: main.c 
	$(CC) -c main.c $(CFLAGS)

# Missing s2.c
$(OBJECTS): s1.c s1.h s2.h
	$(CC) -c s1.c $(CFLAGS)
	$(CC) -c s2.c $(CFLAGS)

main: $(OBJECTS) main.o
	$(CC) $^ -o $@

clean:
	rm -rf *.o main foo/foo bar
