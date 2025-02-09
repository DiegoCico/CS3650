#include <stdio.h>
#include <stdlib.h>

extern long crunch(long a, long b);

int main(int argc, char *argv[]) {
    // Check if exactly two arguments
    if (argc != 3) {
        printf("Two arguments required.\n");
        return 1;
    }

    // Convert the arguments to long integers
    long num1 = strtol(argv[1], NULL, 10);
    long num2 = strtol(argv[2], NULL, 10);

    // Call the crunch function
    long result = crunch(num1, num2);

    // Print the appropriate output based on the result
    if (result < 0) {
        printf("hat\n");
    } else if (result == 0) {
        printf("tea\n");
    } else {
        printf("beer\n");
    }

    return 0;
}

