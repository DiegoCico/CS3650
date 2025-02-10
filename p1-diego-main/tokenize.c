#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

int main(int argc, char **argv) {
    char input[MAX_INPUT];
    /* Do not print a prompt, so that output matches test expectations */
    if (fgets(input, MAX_INPUT, stdin) == NULL) {
        return 1;
    }
    input[strcspn(input, "\n")] = '\0';
    int numTokens = 0;
    char **tokens = tokenize(input, &numTokens);
    for (int i = 0; i < numTokens; i++) {
        printf("%s\n", tokens[i]);
        free(tokens[i]);
    }
    free(tokens);
    return 0;
}
