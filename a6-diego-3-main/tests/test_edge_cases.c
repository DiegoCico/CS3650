#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../tmsort.c"

void test_empty_array() {
    printf("Testing sorting on an empty array...\n");
    long arr[] = {};
    long *sorted = merge_sort(arr, 0);
    assert(sorted != NULL);
    free(sorted);
    printf("Empty array test passed.\n");
}

void test_single_element() {
    printf("Testing sorting with a single element...\n");
    long arr[] = {42};
    long *sorted = merge_sort(arr, 1);
    assert(sorted[0] == 42);
    free(sorted);
    printf("Single element test passed.\n");
}

void test_identical_elements() {
    printf("Testing sorting with identical elements...\n");
    long arr[] = {7, 7, 7, 7, 7};
    long *sorted = merge_sort(arr, 5);
    for (int i = 0; i < 4; i++) {
        assert(sorted[i] == sorted[i + 1]);
    }
    free(sorted);
    printf("Identical elements test passed.\n");
}

int main() {
    test_empty_array();
    test_single_element();
    test_identical_elements();
    return 0;
}
