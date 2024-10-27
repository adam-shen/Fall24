#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

#define MEMSIZE 4096

//the memory array with 8-byte alignment
static union {
    char bytes[MEMSIZE];
    double not_used; //ensures 8-byte alignment
} heap;

struct node {
    int allocated; // 1 if allocated, 0 if free
    int size; 
};

static int initialized = 0;
static struct node *first_header = (struct node *) heap.bytes;

void leak_checker(void) {
    int leaks = 0, total_size = 0;
    struct node *current = first_header;

    while ((char *)current < heap.bytes + MEMSIZE) {
        if (current->allocated) {
            leaks++;
            total_size += current->size;
        }
        current = (struct node *)((char *)current + current->size);
    }

    if (leaks > 0) {
        fprintf(stderr, "mymalloc: %d bytes leaked in %d objects.\n", total_size, leaks);
    }
}

void initialize_heap(void) {
    first_header->allocated = 0;
    first_header->size = MEMSIZE;
    atexit(leak_checker); //register memory leak checker
    initialized = 1;
    //printf("Heap initialized. Size of struct header: %zu bytes\n", sizeof(struct header));
}

void *mymalloc(size_t size, char *file, int line) {
    if (!initialized) initialize_heap();

    //adjust size for alignment
    size = (size + 7) & ~7; //round up to the nearest multiple of 8

    struct node *current = first_header; //start searching from the first block in the heap

    while ((char *)current < heap.bytes + MEMSIZE) {
        if (!current->allocated && current->size >= size + sizeof(struct node)) {
            int remaining_size = current->size - size - sizeof(struct node);

            // split the chunk if there is enough space left
            if (remaining_size > sizeof(struct node)) {
                struct node *next = (struct node *)((char *)current + size + sizeof(struct node));
                next->allocated = 0; //mark the new block as free
                next->size = remaining_size;

                current->size = size + sizeof(struct node);
            }

            current->allocated = 1; //mark the current block as allocated
            return (char *)current + sizeof(struct node);
        }
        current = (struct node *)((char *)current + current->size);  //move to the next block by adding the size of the current block to its address
    }

    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (!ptr) return;

    //check if the pointer is within the bounds of the managed heap
    if ((char *)ptr < heap.bytes || (char *)ptr >= heap.bytes + MEMSIZE) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }

    struct node *current = (struct node *)((char *)ptr - sizeof(struct node));

    if (!current->allocated) {
        fprintf(stderr, "free: Double free or corruption (%s:%d)\n", file, line);
        exit(2);
    }

    current->allocated = 0; //mark the block as free

    //coalesce with the next block if it's free
    struct node *next = (struct node *)((char *)current + current->size);
    if ((char *)next < heap.bytes + MEMSIZE && !next->allocated) {
        current->size += next->size; //combine the current block size with the next block size
    }
}