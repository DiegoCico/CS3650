#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../tmsort.c"

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void test_performance() {
    printf("Testing sorting performance...\n");

    int n = 50000;
    long *arr = malloc(n * sizeof(long));

    for (int i = 0; i < n; i++) arr[i] = rand() % 100000;

    double start = get_time();
    long *sorted = merge_sort(arr, n);
    double end = get_time();

    printf("Sorting %d elements took %f seconds.\n", n, end - start);

    free(arr);
    free(sorted);
    printf("Performance test completed.\n");
}

int main() {
    test_performance();
    return 0;
}
