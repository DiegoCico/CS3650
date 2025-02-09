#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    int *arr[100];

    for (int i = 0; i < 100; i++) {
        arr[i] = malloc(sizeof(int));
    }

    for (int i = 0; i < 100; i += 2) {
        free(arr[i]); 
    }

    long *pl = malloc(sizeof(long));
    short int *pi = (short int *) pl;
    *pl = 21;
    printf("%ld\n", *pl);
    free(pl);
    *pl = 42;
    printf("%d\n", *pi);

    return 0;
}

