#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // for fork(), execvp()
#include <sys/types.h>
#include <sys/wait.h>
#include "tokenizer.h"  // Provides MAX_INPUT and prototype for tokenize().

// Include the tokenize() implementation so that shell.o contains its definition.
#include "tokenize.c"

int main(int argc, char **argv) {
    char line[MAX_INPUT];

    printf("Welcome to mini-shell.\n");

    while (1) {
        // Print the prompt.
        printf("shell $ ");
        fflush(stdout);

        // Read a line from standard input.
        if (fgets(line, MAX_INPUT, stdin) == NULL) {
            printf("\nBye bye.\n");
            break;
        }

        // Remove the trailing newline.
        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 0)
            continue;

        // Tokenize the input.
        int numTokens = 0;
        char **tokens = tokenize(line, &numTokens);
        if (numTokens == 0) {
            free(tokens);
            continue;
        }

        // Process commands separated by semicolons.
        int start = 0;
        while (start < numTokens) {
            int end = start;
            while (end < numTokens && strcmp(tokens[end], ";") != 0) {
                end++;
            }

            if (end - start > 0) {
                int cmdArgCount = end - start;
                // Create a NULL-terminated array of arguments.
                char **cmdArgs = malloc((cmdArgCount + 1) * sizeof(char *));
                if (!cmdArgs) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                for (int i = 0; i < cmdArgCount; i++) {
                    cmdArgs[i] = tokens[start + i];
                }
                cmdArgs[cmdArgCount] = NULL;

                // If the command is "exit", then exit the shell.
                if (strcmp(cmdArgs[0], "exit") == 0) {
                    free(cmdArgs);
                    for (int i = 0; i < numTokens; i++) {
                        free(tokens[i]);
                    }
                    free(tokens);
                    printf("Bye bye.\n");
                    exit(0);
                }

                // Fork a child process to execute the command.
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    free(cmdArgs);
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    execvp(cmdArgs[0], cmdArgs);
                    fprintf(stderr, "%s: command not found\n", cmdArgs[0]);
                    exit(EXIT_FAILURE);
                } else {
                    int status;
                    waitpid(pid, &status, 0);
                }
                free(cmdArgs);
            }
            start = end + 1;
        }

        // Free all tokens.
        for (int i = 0; i < numTokens; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }

    return 0;
}
