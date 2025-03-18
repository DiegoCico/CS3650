#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../tmsort.c"

void test_basic_sort() {
    printf("Testing basic sorting...\n");

    long arr[] = {5, 2, 9, 1, 5, 6};
    int n = sizeof(arr) / sizeof(arr[0]);

    long *sorted = merge_sort(arr, n);

    for (int i = 0; i < n - 1; i++) {
        assert(sorted[i] <= sorted[i + 1]); 
    }

    free(sorted);
    printf("Basic sorting test passed.\n");
}

int main() {
    test_basic_sort();
    return 0;
}
