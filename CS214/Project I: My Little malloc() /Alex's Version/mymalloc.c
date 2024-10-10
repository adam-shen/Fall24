#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

// Define the size of the heap to be 4MB.
#define MEMSIZE 4096

// Union to ensure the heap is properly aligned for memory access.
static union {
    char bytes[MEMSIZE];
    double not_used;  // Ensures the array is aligned at addresses divisible by 8.
} heap;

// Define a structure for a memory block header.
struct header {
    size_t size;     // Size of the block (header + payload).
    int allocated;   // 1 if allocated, 0 if free.
};

// Forward declaration of the memory leak checker function.
static void leak_checker(void);

// Global variables for heap initialization state and pointer to the first block.
static int initialized = 0;
static struct header *first_header = (struct header *) heap.bytes;

// Initialize the heap, creating a single large free block.
static void initialize_heap(void) {
    first_header->size = MEMSIZE - sizeof(struct header); // Set initial block size.
    first_header->allocated = 0;  // Mark the block as free.
    atexit(leak_checker);  // Register the leak checker to run at program exit.
    initialized = 1;
    //printf("Heap initialized. Size of struct header: %zu bytes\n", sizeof(struct header)); was ysed for testing
}

// Custom malloc function that allocates memory from the heap.
void *mymalloc(size_t size, char *file, int line) {
    if (!initialized) {
        initialize_heap();  // Initialize the heap if it hasn't been initialized.
    }

    // Align the requested size to 8 bytes for proper alignment.
    size_t aligned_size = (size + 7) & ~7;
    struct header *current = first_header;

    // Iterate over the heap to find a suitable free block.
    while ((char *)current < heap.bytes + MEMSIZE) {
        // Check if the current block is free and large enough to accommodate the request.
        if (!current->allocated && current->size >= aligned_size) {
            // Split the block if there's enough space for another header.
            if (current->size >= aligned_size + sizeof(struct header)) {
                size_t remaining_size = current->size - aligned_size - sizeof(struct header);
                if (remaining_size > sizeof(struct header)) {
                    // Create a new header for the remaining free space.
                    struct header *new_header = (struct header *)((char *)current + sizeof(struct header) + aligned_size);
                    new_header->size = remaining_size;
                    new_header->allocated = 0;
                    current->size = aligned_size;  // Adjust the size of the allocated block.
                }
            }
            // Mark the block as allocated and return a pointer to its payload.
            current->allocated = 1;
            return (void *)((char *)current + sizeof(struct header));
        }
        // Move to the next block in the heap.
        current = (struct header *)((char *)current + sizeof(struct header) + current->size);
    }

    // If no suitable block is found, print an error and return NULL.
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

// Custom free function that releases memory back to the heap.
void myfree(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        return;  // Do nothing if a null pointer is passed.
    }

    // Get the header associated with the given pointer.
    struct header *header = (struct header *)((char *)ptr - sizeof(struct header));

    // Check if the block is already free (detect double free).
    if (header->allocated == 0) {
        fprintf(stderr, "Double free detected at %s:%d\n", file, line);
        exit(2);  // Exit the program if a double free is detected.
    }

    // Mark the block as free.
    header->allocated = 0;

    // Coalesce with the next block if it's free.
    struct header *next = (struct header *)((char *)header + sizeof(struct header) + header->size);
    if ((char *)next < heap.bytes + MEMSIZE && next->allocated == 0) {
        header->size += sizeof(struct header) + next->size;
    }

    // Coalesce with the previous block if it's free.
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

// Memory leak checker function that reports any unreleased blocks.
static void leak_checker(void) {
    struct header *current = first_header;
    int leak_count = 0;
    int total_leaked_size = 0;

    // Iterate over all blocks in the heap to detect leaks.
    while ((char *)current < heap.bytes + MEMSIZE) {
        if (current->allocated) {
            leak_count++;  // Count the number of allocated blocks.
            total_leaked_size += current->size;  // Sum up the sizes of leaked blocks.
        }
        current = (struct header *)((char *)current + sizeof(struct header) + current->size);
    }

    // Report the number of leaks, if any.
    if (leak_count > 0) {
        fprintf(stderr, "mymalloc: %d bytes leaked in %d objects.\n", total_leaked_size, leak_count);
    } else {
        fprintf(stderr, "mymalloc: No memory leaks detected.\n");
    }
}
