#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mymalloc.h"

long calculate_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
}

// Task 1: Allocate 120 1-byte blocks and immediately free them
void task1() {
    for (int i = 0; i < 120; i++) {
        char *ptr = (char *) mymalloc(1, __FILE__, __LINE__);
        myfree(ptr, __FILE__, __LINE__);
    }
}

// Task 2: Allocate 1 byte 120 times and free all 120 bytes
void task2() {
    char *ptrs[120];
    for (int i = 0; i < 120; i++) {
        ptrs[i] = (char *) mymalloc(1, __FILE__, __LINE__);
    }
    for (int i = 0; i < 120; i++) {
        myfree(ptrs[i], __FILE__, __LINE__);
    }
}

// Task 3: Allocate and free 240 times randomly
void task3() {
    char *ptrs[120];
    int allocated = 0;

    // Initialize the array to NULL
    for (int i = 0; i < 120; i++) {
        ptrs[i] = NULL;
    }

    // Perform 240 iterations of random allocation and deallocation
    for (int i = 0; i < 240; i++) {
        int action = rand() % 2;

        if (action == 0 && allocated < 120) {
            // Allocate 1 byte and store the pointer in the array
            ptrs[allocated++] = (char *) mymalloc(1, __FILE__, __LINE__);
        } else if (action == 1 && allocated > 0) {
            // Deallocate a previously allocated object
            int index = rand() % allocated;
            
            // Check that the pointer is valid before freeing
            if (ptrs[index] != NULL) {
                myfree(ptrs[index], __FILE__, __LINE__);
                ptrs[index] = NULL;
            }

            // Shuffle the array to fill the freed slot
            ptrs[index] = ptrs[--allocated];
            ptrs[allocated] = NULL;  // Set the last pointer to NULL
        }
    }

    // Deallocate any remaining allocated objects
    for (int i = 0; i < allocated; i++) {
        if (ptrs[i] != NULL) {
            myfree(ptrs[i], __FILE__, __LINE__);
            ptrs[i] = NULL;  // Set the pointer to NULL after freeing
        }
    }
}

// Task 4: Allocate 64 bytes 120 times and free them in reverse order
void task4() {
    char *ptrs[120];
    for (int i = 0; i < 120; i++) {
        ptrs[i] = (char *) mymalloc(64, __FILE__, __LINE__);
        // Optionally, check for allocation failure:
        if (!ptrs[i]) {
            fprintf(stderr, "Failed to allocate memory in task4\n");
            return;
        }
    }
    // Free in reverse order
    for (int i = 119; i >= 0; i--) {
        myfree(ptrs[i], __FILE__, __LINE__);
    }
}

// Task 5: Randomly allocate between 1 to 64 bytes and deallocate randomly
void task5() {
    char *ptrs[120];
    int sizes[120]; // Store the size of each allocation
    int allocated = 0;

    // Initialize pointers and sizes to NULL/0
    for (int i = 0; i < 120; i++) {
        ptrs[i] = NULL;
        sizes[i] = 0;
    }

    // Perform 240 iterations of random allocation and deallocation
    for (int i = 0; i < 240; i++) {
        int action = rand() % 2;

        if (action == 0 && allocated < 120) {
            // Randomly choose a size between 1 and 64 bytes
            int size = (rand() % 64) + 1;
            ptrs[allocated] = (char *) mymalloc(size, __FILE__, __LINE__);
            // Check for successful allocation
            if (ptrs[allocated]) {
                sizes[allocated++] = size;
            }
        } else if (action == 1 && allocated > 0) {
            // Deallocate a randomly chosen object
            int index = rand() % allocated;
            
            if (ptrs[index]) {
                myfree(ptrs[index], __FILE__, __LINE__);
                ptrs[index] = NULL;
            }

            // Move last allocated pointer to fill the gap
            ptrs[index] = ptrs[--allocated];
            sizes[index] = sizes[allocated];
        }
    }

    // Deallocate any remaining allocated objects
    for (int i = 0; i < allocated; i++) {
        if (ptrs[i]) {
            myfree(ptrs[i], __FILE__, __LINE__);
        }
    }
}

// Main method
int main() {
    struct timeval start, end;
    long total_time;

    // Task 1
    total_time = 0;
    for (int i = 0; i < 50; i++) {
        gettimeofday(&start, NULL);
        task1();
        gettimeofday(&end, NULL);
        total_time += calculate_time(start, end);
    }
    printf("Task 1 average time: %ld microseconds\n", total_time / 50);

    // Task 2
    total_time = 0;
    for (int i = 0; i < 50; i++) {
        gettimeofday(&start, NULL);
        task2();
        gettimeofday(&end, NULL);
        total_time += calculate_time(start, end);
    }
    printf("Task 2 average time: %ld microseconds\n", total_time / 50);
    printf("task 2 is over\n");

    // Task 3
    total_time = 0;
    printf("task 3 is starting\n");
    for (int i = 0; i < 50; i++) {
        gettimeofday(&start, NULL);
        task3();
        gettimeofday(&end, NULL);
        total_time += calculate_time(start, end);
    }
    printf("Task 3 average time: %ld microseconds\n", total_time / 50);

    // Task 4
    total_time = 0;
    printf("task 4 is starting\n");
    for (int i = 0; i < 50; i++) {
        gettimeofday(&start, NULL);
        task4();
        gettimeofday(&end, NULL);
        total_time += calculate_time(start, end);
    }
    printf("Task 4 average time: %ld microseconds\n", total_time / 50);

    // Task 5
    total_time = 0;
    printf("task 5 is starting\n");
    for (int i = 0; i < 50; i++) {
        gettimeofday(&start, NULL);
        task5();
        gettimeofday(&end, NULL);
        total_time += calculate_time(start, end);
    }
    printf("Task 5 average time: %ld microseconds\n", total_time / 50);

    return 0;
}