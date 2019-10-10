C=gcc

.PHONY: clean all nested 

all: main

main.o: main.c 
	$(CC) -c main.c $(CFLAGS)

nested: 
	$(MAKE) -C nested_dir

main: main.o nested
	$(CC) nested_dir/s1.o nested_dir/s2.o main.o -o main

clean:
	rm -rf main *.o
	$(MAKE) -C nested_dir clean
