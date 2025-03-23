#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

void test1() {
    printf("Test 1: Allocating and freeing a single block\n");
    void *ptr = malloc(16);
    if (ptr) {
        printf("Allocation successful\n");
    } else {
        printf("Allocation failed\n");
    }
    free(ptr);
    printf("Memory freed\n\n");
}

void test2() {
    printf("Test 2: Allocating multiple blocks\n");
    void *ptr1 = malloc(32);
    void *ptr2 = malloc(64);
    if (ptr1 && ptr2) {
        printf("Multiple allocations successful\n");
    } else {
        printf("Allocation failed\n");
    }
    free(ptr1);
    free(ptr2);
    printf("Memory freed\n\n");
}

void test3() {
    printf("Test 3: Allocating, freeing, and reallocating\n");
    void *ptr1 = malloc(16);
    free(ptr1);
    void *ptr2 = malloc(16);
    if (ptr2) {
        printf("Reallocation successful\n");
    } else {
        printf("Reallocation failed\n");
    }
    free(ptr2);
    printf("Memory freed\n\n");
}

void test4() {
    printf("Test 4: Allocating zero bytes\n");
    void *ptr = malloc(0);
    if (ptr == NULL) {
        printf("Returned NULL as expected\n");
    } else {
        printf("Unexpected allocation\n");
        free(ptr);
    }
    printf("Test completed\n\n");
}

int main() {
    test1();
    test2();
    test3();
    test4();
    return 0;
}
