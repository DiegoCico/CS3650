#ifndef TOKENIZER_H
#define TOKENIZER_H

/* Maximum input size (must be at least 255 characters) */
#define MAX_INPUT 512

/* 
 * Prototype for the tokenize() function.
 * Given an input string, tokenize() returns an array of tokens.
 * The number of tokens is stored in *numTokens.
 */
char **tokenize(const char *input, int *numTokens);

#endif /* TOKENIZER_H */
