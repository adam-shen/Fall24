CC = gcc
CFLAGS = -std=c99 -Wall -Wvla -fsanitize=address,undefined
TARGETS = memgrind

all: $(TARGETS)

memgrind: memgrind.o mymalloc.o
	$(CC) $(CFLAGS) -o $@ $^

mymalloc.o: mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -c mymalloc.c

memgrind.o: memgrind.c mymalloc.h
	$(CC) $(CFLAGS) -c memgrind.c

clean:
	rm -f *.o $(TARGETS)
