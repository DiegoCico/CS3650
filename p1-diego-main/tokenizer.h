#ifndef TOKENIZER_H
#define TOKENIZER_H

#define MAX_INPUT 512
#define MAX_TOKENS 128

void tokenize(const char *input, char tokens[][MAX_INPUT], int *numTokens);

#endif 
