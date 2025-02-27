#include <stdio.h>
#include <string.h>
#include "tokenizer.h" 

int main(void) {
    char input[MAX_INPUT];
    char tokens[MAX_TOKENS][MAX_INPUT];
    int numTokens = 0;

    // Read one line from stdin into input 
    if (!fgets(input, sizeof(input), stdin)) {
        return 1;
    }

    // Remove any trailing from the input
    input[strcspn(input, "\n")] = '\0';

    // writes each token into a 2D array of tokens
    tokenize(input, tokens, &numTokens);
    for (int i = 0; i < numTokens; i++) {
        printf("%s\n", tokens[i]);
    }

    return 0;
}
