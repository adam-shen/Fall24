#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "mymalloc.h"

void workload1() {
    for (int i = 0; i < 120; i++) {
        void *ptr = malloc(1);
        free(ptr);
    }
}

void workload2() {
    void *pointers[120];
    for (int i = 0; i < 120; i++) {
        pointers[i] = malloc(1);
    }
    for (int i = 0; i < 120; i++) {
        free(pointers[i]);
    }
}

void workload3() {
    void *pointers[120];
    int allocated = 0;
    for (int i = 0; i < 120; i++) {
        if (rand() % 2 && allocated < 120) {
            pointers[allocated++] = malloc(1);
        } else if (allocated > 0) {
            free(pointers[--allocated]);
        }
    }
    while (allocated > 0) {
        free(pointers[--allocated]);
    }
}

int main() {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < 50; i++) {
        workload1();
        workload2();
        workload3();
        // Add two more workloads if needed.
    }
    
    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds*1e-6;
    
    printf("Average time: %f seconds\n", elapsed / 50);
    
    return 0;
}
