#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"



#define MEMSIZE 4096 // 4KB

static union{ 
    char bytes [MEMSIZE];
    double not_used; //makes sure the array is aligned at addresses divisible by 8
} heap;

struct header {
    size_t size;     // Size of the entire chunk (header + payload)
    int allocated;   // 1 if allocated, 0 if free
};

typedef struct header Header;

static int initialized = 0;

void *mymalloc(size_t size, char *file, int line)
{
    if (!initialized) {
    initialize();
    }

    size = (size + 7) & ~7; // Round up to nearest multiple of 8 and adds 8 bytes for the header




    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);

    return NULL;
}

void myfree(void *ptr, char *file, int line)
{

}

void leak_check()
{
    

}

static void intialize(void) {
    
    initialized = 1;

    atexit(lead_check());
    
}

static void print_heap(void) {
    
}

