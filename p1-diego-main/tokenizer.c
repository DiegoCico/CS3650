#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

// Check if a character is a special shell token. 
static int is_special_token(char ch) {
    return (ch == '(' || ch == ')' || ch == '<' ||
            ch == '>' || ch == ';' || ch == '|');
}


 // Read a word with no special character
 // Returns the number of characters read. 
static int read_word(const char *input, char *output) {
    int i = 0;
    while (input[i] != '\0' &&
           !isspace((unsigned char)input[i]) &&
           !is_special_token(input[i])) 
    {
        output[i] = input[i];
        i++;
    }
    output[i] = '\0';
    return i;
}

 // Read a quoted string
 // Returns the total number of characters
static int read_quoted_string(const char *input, char *output) {
    int i = 0;
    while (input[i] != '\0' && input[i] != '"') {
        output[i] = input[i];
        i++;
    }
    output[i] = '\0';
    
    if (input[i] == '"') {
        i++;
    }
    return i;
}

void tokenize(const char *input, char tokens[][MAX_INPUT], int *numTokens) {
    int i = 0;
    int len = strlen(input);
    *numTokens = 0;

    while (i < len) {
        // no white spaces
        if (isspace((unsigned char)input[i])) {
            i++;
            continue;
        }

        // exeding maximun token
        if (*numTokens >= MAX_TOKENS) {
            fprintf(stderr, "Warning: too many tokens (max %d). Some will be truncated.\n", MAX_TOKENS);
            return;
        }

        if (input[i] == '"') {
            i++; 
            i += read_quoted_string(&input[i], tokens[*numTokens]);
        }
        else if (is_special_token(input[i])) {
            // check if it is a special characyer
            tokens[*numTokens][0] = input[i];
            tokens[*numTokens][1] = '\0';
            i++;
        }
        else {
            // read if it is a not special word
            i += read_word(&input[i], tokens[*numTokens]);
        }
        (*numTokens)++;
    }
}
