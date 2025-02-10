#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"  // For MAX_INPUT and prototype for tokenize()

int main(int argc, char **argv) {
    char input[MAX_INPUT];
    printf("Enter a command to tokenize:\n");
    if (fgets(input, MAX_INPUT, stdin) == NULL) {
        perror("fgets");
        return 1;
    }
    input[strcspn(input, "\n")] = '\0';

    int numTokens = 0;
    char **tokens = tokenize(input, &numTokens);
    for (int i = 0; i < numTokens; i++) {
        printf("Token %d: %s\n", i, tokens[i]);
        free(tokens[i]);
    }
    free(tokens);
    return 0;
}
