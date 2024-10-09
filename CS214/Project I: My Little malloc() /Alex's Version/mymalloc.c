#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

#define MEMSIZE 4096 * 4096 // 4MB

static union {
    char bytes[MEMSIZE];
    double not_used;  // ensures the array is aligned at addresses divisible by 8
} heap;

struct header {
    size_t size;     // size of the entire chunk (header + payload)
    int allocated;   // 1 if allocated, 0 if free
};

static void leak_checker(void);  // forward declaration of leak_checker

static int initialized = 0;
static struct header *first_header = (struct header *) heap.bytes;

static void initialize_heap(void) {
    first_header->size = MEMSIZE - sizeof(struct header);
    first_header->allocated = 0;
    atexit(leak_checker);  // register memory leak checker
    initialized = 1;
    //printf("Heap initialized. Size of struct header: %zu bytes\n", sizeof(struct header));
}

void *mymalloc(size_t size, char *file, int line) {
    if (!initialized) {
        initialize_heap();  // initialize heap
    }

    size_t aligned_size = (size + 7) & ~7;
    struct header *current = first_header;

    while ((char *)current < heap.bytes + MEMSIZE) {
        //printf("Inspecting block at %p with size %zu bytes, allocated: %d\n",
        //       (void *)current, current->size, current->allocated);
        
        if (!current->allocated && current->size >= aligned_size) {
            if (current->size >= aligned_size + sizeof(struct header)) {
                size_t remaining_size = current->size - aligned_size - sizeof(struct header);
                if (remaining_size > sizeof(struct header)) {
                    struct header *new_header = (struct header *)((char *)current + sizeof(struct header) + aligned_size);
                    new_header->size = remaining_size;
                    new_header->allocated = 0;
                    current->size = aligned_size;
                }
            }
            current->allocated = 1;
            return (void *)((char *)current + sizeof(struct header));
        }

        current = (struct header *)((char *)current + sizeof(struct header) + current->size);
    }

    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        return;
    }

    struct header *header = (struct header *)((char *)ptr - sizeof(struct header));

    if (header->allocated == 0) {
        fprintf(stderr, "Double free detected at %s:%d\n", file, line);
        exit(2);
    }

    header->allocated = 0;

    struct header *next = (struct header *)((char *)header + sizeof(struct header) + header->size);
    if ((char *)next < heap.bytes + MEMSIZE && next->allocated == 0) {
        header->size += sizeof(struct header) + next->size;
    }

    struct header *prev = first_header;
    struct header *prev_block = NULL;
    while ((char *)prev < (char *)header) {
        struct header *next_block = (struct header *)((char *)prev + sizeof(struct header) + prev->size);
        if (next_block == header) {
            prev_block = prev;
            break;
        }
        prev = next_block;
    }

    if (prev_block && prev_block->allocated == 0) {
        prev_block->size += sizeof(struct header) + header->size;
    }
}

static void leak_checker(void) {
    struct header *current = first_header;
    int leak_count = 0;
    int total_leaked_size = 0;

    while ((char *)current < heap.bytes + MEMSIZE) {
        if (current->allocated) {
            leak_count++;
            total_leaked_size += current->size;
        }
        current = (struct header *)((char *)current + sizeof(struct header) + current->size);
    }

    if (leak_count > 0) {
        fprintf(stderr, "mymalloc: %d bytes leaked in %d objects.\n", total_leaked_size, leak_count);
    } else {
        fprintf(stderr, "mymalloc: No memory leaks detected.\n");
    }
}
