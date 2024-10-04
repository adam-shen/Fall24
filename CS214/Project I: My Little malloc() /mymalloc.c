#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"



#define MEMSIZE 4096 // 4KB

static union{ 
    char bytes [MEMSIZE];
    double not_used; //makes sure the array is aligned at addresses divisible by 8
} heap;

void *mymalloc(size_t size, char *file, int line)
{


    return NULL;
}

void myfree(void *ptr, char *file, int line)
{

}

void leak_check()
{
    

}

static void intialize(void) {
    
    atexit(lead_check());
    
}

static void print_heap(void) {
    
}

