.PHONY: clean all

all: s1.o s2.o

s1.o: s1.c s1.h 
	$(CC) -c s1.c 

# Missing s2.c
s2.o: s2.h
	$(CC) -c s2.c 

clean:
	rm -rf *.o
