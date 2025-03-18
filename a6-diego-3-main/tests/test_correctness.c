#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../tmsort.c"

void test_large_input() {
    printf("Testing sorting with a large input size...\n");

    int n = 10000;
    long *arr = malloc(n * sizeof(long));

    for (int i = 0; i < n; i++) arr[i] = rand() % 100000;

    long *sorted = merge_sort(arr, n);

    for (int i = 0; i < n - 1; i++) {
        assert(sorted[i] <= sorted[i + 1]);  
    }

    free(arr);
    free(sorted);
    printf("Large input sorting test passed.\n");
}

int main() {
    test_large_input();
    return 0;
}
