#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymalloc.h"

#define MEMSIZE 4096
#define HEADERSIZE sizeof(struct node)
#define SMALL_ALLOC 32
#define LARGE_ALLOC 2048

void test_allocation_and_no_overlap() {
    printf("Testing allocation and no overlap...\n");
    char *a = malloc(SMALL_ALLOC);
    char *b = malloc(SMALL_ALLOC);

    if (a && b && (b - a >= SMALL_ALLOC)) {
        printf("PASS: Allocated blocks do not overlap.\n");
    } else {
        printf("FAIL: Allocated blocks overlap or allocation failed.\n");
    }

    if (a) free(a);
    if (b) free(b);
}

void test_double_free_detection() {
    printf("Testing double free detection...\n");
    char *a = malloc(SMALL_ALLOC);
    if (a) {
        free(a);
        // The second free should trigger an error message or exit.
        free(a);
    }
}

void test_large_allocation() {
    printf("Testing large allocation...\n");
    char *large = malloc(LARGE_ALLOC);

    if (large) {
        printf("PASS: Large allocation succeeded.\n");
        free(large);
    } else {
        printf("FAIL: Large allocation failed.\n");
    }
}

void test_out_of_bounds_free() {
    printf("Testing out-of-bounds free...\n");
    char *a = malloc(SMALL_ALLOC);
    if (a) {
        // Attempting to free a pointer outside the managed heap.
        char *invalid_ptr = a + MEMSIZE;
        free(invalid_ptr);
    }
    if (a) free(a);
}

void test_leak_detection() {
    printf("Testing leak detection...\n");

    // Allocate some blocks without freeing them.
    malloc(SMALL_ALLOC);
    malloc(SMALL_ALLOC);
    // The leak checker should identify leaked memory when the program exits.
}

int main() {
    test_allocation_and_no_overlap();
    test_double_free_detection();
    test_large_allocation();
    test_out_of_bounds_free();
    test_leak_detection();

    return 0;
}
