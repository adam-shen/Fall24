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
        char *ptr = (char *) malloc(1);
        free(ptr);
    }
}

// Task 2: Allocate 1 byte 120 times and free all 120 bytes
void task2() {
    char *ptrs[120];
    for (int i = 0; i < 120; i++) {
        ptrs[i] = (char *) malloc(1);
    }
    for (int i = 0; i < 120; i++) {
        free(ptrs[i]);
    }
}

// Task 3: Allocate and free 240 times randomly
void task3() {
    char *ptrs[120];
    int allocated = 0;

    for (int i = 0; i < 120; i++) {
        ptrs[i] = NULL;
    }

    for (int i = 0; i < 240; i++) {
        int action = rand() % 2;

        if (action == 0 && allocated < 120) {
            ptrs[allocated++] = (char *) malloc(1);
        } else if (action == 1 && allocated > 0) {
            int index = rand() % allocated;
            free(ptrs[index]);
            ptrs[index] = ptrs[--allocated];
        }
    }

    for (int i = 0; i < allocated; i++) {
        free(ptrs[i]);
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

    // Task 3
    total_time = 0;
    for (int i = 0; i < 50; i++) {
        gettimeofday(&start, NULL);
        task3();
        gettimeofday(&end, NULL);
        total_time += calculate_time(start, end);
    }
    printf("Task 3 average time: %ld microseconds\n", total_time / 50);

    return 0;
}