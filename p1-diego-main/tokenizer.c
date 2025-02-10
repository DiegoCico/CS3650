#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

/* 
 * tokenize() splits the input string into tokens.
 * Special tokens: ( ) < > ; | 
 * Quoted strings (inside double quotes) are taken as a single token.
 */
char **tokenize(const char *input, int *numTokens) {
    int tokensCapacity = 10;
    int tokensSize = 0;
    char **tokens = malloc(tokensCapacity * sizeof(char *));
    if (!tokens) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    enum { STATE_NONE, STATE_IN_WORD, STATE_IN_QUOTE } state = STATE_NONE;
    const char *start = NULL;  /* Pointer to the beginning of the current token */
    char *token = NULL;
    int i = 0;
    int len = strlen(input);

    while (i < len) {
        char c = input[i];
        switch (state) {
            case STATE_NONE:
                if (isspace(c)) {
                    i++;  /* Skip whitespace */
                } else if (c == '"') {
                    state = STATE_IN_QUOTE;
                    start = input + i + 1;  /* Skip the opening quote */
                    i++;
                } else if (c == '(' || c == ')' || c == '<' ||
                           c == '>' || c == ';' || c == '|') {
                    /* Single-character token */
                    token = malloc(2);
                    if (!token) {
                        perror("malloc");
                        exit(EXIT_FAILURE);
                    }
                    token[0] = c;
                    token[1] = '\0';
                    if (tokensSize >= tokensCapacity) {
                        tokensCapacity *= 2;
                        tokens = realloc(tokens, tokensCapacity * sizeof(char *));
                        if (!tokens) {
                            perror("realloc");
                            exit(EXIT_FAILURE);
                        }
                    }
                    tokens[tokensSize++] = token;
                    i++;
                } else {
                    state = STATE_IN_WORD;
                    start = input + i;
                    i++;
                }
                break;

            case STATE_IN_WORD:
                if (isspace(c) || c == '(' || c == ')' || c == '<' ||
                    c == '>' || c == ';' || c == '|' || c == '"') {
                    int tokenLen = (input + i) - start;
                    token = malloc(tokenLen + 1);
                    if (!token) {
                        perror("malloc");
                        exit(EXIT_FAILURE);
                    }
                    strncpy(token, start, tokenLen);
                    token[tokenLen] = '\0';
                    if (tokensSize >= tokensCapacity) {
                        tokensCapacity *= 2;
                        tokens = realloc(tokens, tokensCapacity * sizeof(char *));
                        if (!tokens) {
                            perror("realloc");
                            exit(EXIT_FAILURE);
                        }
                    }
                    tokens[tokensSize++] = token;
                    state = STATE_NONE;
                    /* Do not increment i here; the special character will be processed next */
                } else {
                    i++;
                }
                break;

            case STATE_IN_QUOTE:
                if (c == '"') {
                    int tokenLen = (input + i) - start;
                    token = malloc(tokenLen + 1);
                    if (!token) {
                        perror("malloc");
                        exit(EXIT_FAILURE);
                    }
                    strncpy(token, start, tokenLen);
                    token[tokenLen] = '\0';
                    if (tokensSize >= tokensCapacity) {
                        tokensCapacity *= 2;
                        tokens = realloc(tokens, tokensCapacity * sizeof(char *));
                        if (!tokens) {
                            perror("realloc");
                            exit(EXIT_FAILURE);
                        }
                    }
                    tokens[tokensSize++] = token;
                    state = STATE_NONE;
                    i++;  /* Skip the closing quote */
                } else {
                    i++;
                }
                break;
        }
    }

    /* If ended in the middle of a word, flush it */
    if (state == STATE_IN_WORD) {
        int tokenLen = (input + i) - start;
        token = malloc(tokenLen + 1);
        if (!token) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strncpy(token, start, tokenLen);
        token[tokenLen] = '\0';
        if (tokensSize >= tokensCapacity) {
            tokensCapacity *= 2;
            tokens = realloc(tokens, tokensCapacity * sizeof(char *));
            if (!tokens) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
        tokens[tokensSize++] = token;
    }

    tokens = realloc(tokens, tokensSize * sizeof(char *));
    *numTokens = tokensSize;
    return tokens;
}
