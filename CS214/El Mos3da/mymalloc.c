#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

#define MEMLENGTH 4096

static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;

typedef struct {
    size_t size;
    int is_free;
} BlockHeader;

static int is_initialized = 0;

static void leak_checker(void);  // Forward declaration of leak_checker

void initialize_heap() {
    BlockHeader *first_block = (BlockHeader *)heap.bytes;
    first_block->size = MEMLENGTH - sizeof(BlockHeader);
    first_block->is_free = 1;
    is_initialized = 1;
    atexit(detect_leaks);
}

void *mymalloc(size_t size, char *file, int line) {
    if (!is_initialized) {
        initialize_heap();
    }
    
    // Round up size to nearest multiple of 8
    size = (size + 7) & ~7;
    size_t total_size = size + sizeof(BlockHeader);
    
    BlockHeader *current_block = (BlockHeader *)heap.bytes;
    
    // Search for a free block that fits the size
    while ((char *)current_block < heap.bytes + MEMLENGTH) {
        if (current_block->is_free && current_block->size >= size) {
            // Split the block if the remaining size is sufficient for a new block
            if (current_block->size >= total_size + sizeof(BlockHeader)) {
                BlockHeader *new_block = (BlockHeader *)((char *)current_block + total_size);
                new_block->size = current_block->size - total_size;
                new_block->is_free = 1;
                current_block->size = size;
            }
            current_block->is_free = 0;
            return (char *)current_block + sizeof(BlockHeader);
        }
        current_block = (BlockHeader *)((char *)current_block + current_block->size + sizeof(BlockHeader));
    }
    
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (ptr == NULL || (char *)ptr < heap.bytes + sizeof(BlockHeader) || (char *)ptr >= heap.bytes + MEMLENGTH) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }
    
    BlockHeader *block_to_free = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    
    if (block_to_free->is_free) {
        fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
        exit(2);
    }
    
    block_to_free->is_free = 1;
    
    // Coalesce with next block if free
    BlockHeader *next_block = (BlockHeader *)((char *)block_to_free + block_to_free->size + sizeof(BlockHeader));
    if ((char *)next_block < heap.bytes + MEMLENGTH && next_block->is_free) {
        block_to_free->size += next_block->size + sizeof(BlockHeader);
    }
}

void detect_leaks() {
    size_t total_size = 0;
    int count = 0;
    BlockHeader *current_block = (BlockHeader *)heap.bytes;
    
    while ((char *)current_block < heap.bytes + MEMLENGTH) {
        if (!current_block->is_free) {
            total_size += current_block->size;
            count++;
        }
        current_block = (BlockHeader *)((char *)current_block + current_block->size + sizeof(BlockHeader));
    }
    
    if (count > 0) {
        fprintf(stderr, "mymalloc: %zu bytes leaked in %d objects.\n", total_size, count);
    }
}
