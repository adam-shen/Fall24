#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

#define MEMSIZE 4096  // 4KB

static union {
    char bytes[MEMSIZE];
    double not_used;  // Ensures the array is aligned at addresses divisible by 8
} heap;

struct header {
    size_t size;     // Size of the entire chunk (header + payload)
    int allocated;   // 1 if allocated, 0 if free
};

static void leak_checker(void);  // Forward declaration of leak_checker

static int initialized = 0;
static struct header *first_header = (struct header *) heap.bytes;

static void initialize_heap(void) {
    first_header->size = MEMSIZE - sizeof(struct header);
    first_header->allocated = 0;
    atexit(leak_checker);  // Register memory leak checker
    initialized = 1;
}

void *mymalloc(size_t size, char *file, int line) {
    if (!initialized) {
        initialize_heap();  // Initialize heap
    }

    // Align the requested size to 8-byte boundary
    size_t aligned_size = (size + 7) & ~7;

    struct header *current = first_header;  // Start from the first block
    while ((char *)current < heap.bytes + MEMSIZE) {
        if (!current->allocated && current->size >= aligned_size) {  // Find a free block large enough
            size_t remaining_size = current->size - aligned_size - sizeof(struct header);
            if (remaining_size > sizeof(struct header)) {  // Split block if remaining space is large enough
                struct header *new_header = (struct header *)((char *)current + sizeof(struct header) + aligned_size);
                new_header->size = remaining_size;
                new_header->allocated = 0;
                current->size = aligned_size;
            }
            current->allocated = 1;  // Mark as allocated
            return (void *)((char *)current + sizeof(struct header));  // Return pointer to data portion
        }
        current = (struct header *)((char *)current + sizeof(struct header) + current->size);  // Move to next block
    }

    // If no suitable block found, output error message
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        return;
    }

    struct header *header = (struct header *)((char *)ptr - sizeof(struct header));
    header->allocated = 0;  // Mark block as free

    printf("Freeing block at %p (size: %zu) from %s:%d\n", ptr, header->size, file, line);

    // Coalesce with the next block if it is free
    struct header *next = (struct header *)((char *)header + sizeof(struct header) + header->size);
    if ((char *)next < heap.bytes + MEMSIZE && !next->allocated) {
        header->size += sizeof(struct header) + next->size;  // Merge with next block
        printf("Coalesced with next block at %p (new size: %zu)\n", next, header->size);
    }

    // Coalesce with the previous block if it is free
    struct header *prev = first_header;
    while ((char *)prev < (char *)header) {
        struct header *next_block = (struct header *)((char *)prev + sizeof(struct header) + prev->size);
        if (next_block == header && !prev->allocated) {
            prev->size += sizeof(struct header) + header->size;  // Merge with previous block
            printf("Coalesced with previous block at %p (new size: %zu)\n", prev, prev->size);
            break;
        }
        prev = next_block;
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