C=gcc
CFLAGS=-I.
#OBJECTS = main.o s1.o s2.o

.PHONY: obj clean all

all: main

main.o: main.c 
	$(CC) -c main.c $(CFLAGS)

s1.o: s1.c s1.h 
	$(CC) -c s1.c $(CFLAGS)

# Missing s1.c
s2.o: s2.h
	$(CC) -c s2.c $(CFLAGS)

main: s1.o s2.o main.o
	$(CC) $^ -o $@

clean:
	rm -rf *.o main 
