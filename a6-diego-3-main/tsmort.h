#ifndef TMSORT_H
#define TMSORT_H

#include <stddef.h>

// Declaring the functions for testing and so they can be used in tmsort.c
void *threaded_merge_sort(void *args);
long *merge_sort(long nums[], int count);

#endif 
