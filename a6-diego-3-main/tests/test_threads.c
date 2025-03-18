#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "../tmsort.c"

void test_thread_usage() {
    printf("Testing multi-threaded merge sort...\n");

    setenv("MSORT_THREADS", "4", 1); 

    long arr[] = {10, 3, 5, 7, 1, 8, 2, 6};
    int n = sizeof(arr) / sizeof(arr[0]);

    long *sorted = merge_sort(arr, n);

    for (int i = 0; i < n - 1; i++) {
        assert(sorted[i] <= sorted[i + 1]); 
    }

    free(sorted);
    printf("Threaded sorting test passed.\n");
}

int main() {
    test_thread_usage();
    return 0;
}
