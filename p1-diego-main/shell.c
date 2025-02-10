#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // for fork(), execvp(), chdir()
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tokenizer.h"  // Provides MAX_INPUT and the prototype for tokenize()

/* Global variable to hold the previous command line */
char *prevCommand = NULL;

/* Print help message for built-in commands */
void print_help() {
    printf("Built-in commands:\n");
    printf("  cd <dir>      : change directory\n");
    printf("  source <file> : execute commands from a script file\n");
    printf("  prev          : re-execute the previous command\n");
    printf("  help          : display this help message\n");
    printf("  exit          : exit the shell\n");
}

/* Run built-in commands. Return 1 if a built-in was executed, 0 otherwise */
int run_builtin(char **args) {
    if (args[0] == NULL)
        return 0;
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            char *home = getenv("HOME");
            if (home == NULL)
                home = "/";
            if (chdir(home) != 0)
                perror("chdir");
        } else {
            if (chdir(args[1]) != 0)
                perror("chdir");
        }
        return 1;
    }
    if (strcmp(args[0], "help") == 0) {
        print_help();
        return 1;
    }
    if (strcmp(args[0], "source") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "source: missing filename\n");
        } else {
            FILE *fp = fopen(args[1], "r");
            if (fp == NULL) {
                perror("source");
            } else {
                char line[MAX_INPUT];
                while (fgets(line, MAX_INPUT, fp) != NULL) {
                    line[strcspn(line, "\n")] = '\0';
                    int numTokens = 0;
                    char **tokens = tokenize(line, &numTokens);
                    if (numTokens > 0) {
                        pid_t pid = fork();
                        if (pid == 0) {
                            execvp(tokens[0], tokens);
                            fprintf(stderr, "%s: command not found\n", tokens[0]);
                            exit(EXIT_FAILURE);
                        } else if (pid > 0) {
                            int status;
                            waitpid(pid, &status, 0);
                        }
                    }
                    for (int j = 0; j < numTokens; j++)
                        free(tokens[j]);
                    free(tokens);
                }
                fclose(fp);
            }
        }
        return 1;
    }
    if (strcmp(args[0], "prev") == 0) {
        if (prevCommand == NULL) {
            fprintf(stderr, "prev: no previous command\n");
        } else {
            printf("Executing previous command: %s\n", prevCommand);
            int numTokens = 0;
            char **tokens = tokenize(prevCommand, &numTokens);
            pid_t pid = fork();
            if (pid == 0) {
                execvp(tokens[0], tokens);
                fprintf(stderr, "%s: command not found\n", tokens[0]);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            }
            for (int j = 0; j < numTokens; j++)
                free(tokens[j]);
            free(tokens);
        }
        return 1;
    }
    if (strcmp(args[0], "exit") == 0) {
        printf("Bye bye.\n");
        exit(0);
    }
    return 0;
}

/* Execute an external command with possible input (<) and output (>) redirection */
void execute_external(char **args) {
    char *infile = NULL;
    char *outfile = NULL;
    int i, j;
    int count = 0;
    while (args[count] != NULL)
        count++;
    char **new_args = malloc((count + 1) * sizeof(char *));
    if (!new_args) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    j = 0;
    for (i = 0; i < count; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (i + 1 < count) {
                infile = args[i + 1];
                i++;  // skip filename
            } else {
                fprintf(stderr, "Error: no input file specified\n");
                break;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (i + 1 < count) {
                outfile = args[i + 1];
                i++;  // skip filename
            } else {
                fprintf(stderr, "Error: no output file specified\n");
                break;
            }
        } else {
            new_args[j++] = args[i];
        }
    }
    new_args[j] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(new_args);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (infile != NULL) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) {
                perror("open infile");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (outfile != NULL) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open outfile");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(new_args[0], new_args);
        fprintf(stderr, "%s: command not found\n", new_args[0]);
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
    free(new_args);
}

/* Execute a command with a pipe (only one pipe supported) */
void execute_pipe(char **args) {
    int i, count = 0, pipeIndex = -1;
    while (args[count] != NULL)
        count++;
    for (i = 0; i < count; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipeIndex = i;
            break;
        }
    }
    if (pipeIndex == -1) {
        execute_external(args);
        return;
    }
    args[pipeIndex] = NULL;  // Split the array into two commands.
    char **left_args = args;
    char **right_args = &args[pipeIndex + 1];

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(left_args[0], left_args);
        fprintf(stderr, "%s: command not found\n", left_args[0]);
        exit(EXIT_FAILURE);
    }
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(right_args[0], right_args);
        fprintf(stderr, "%s: command not found\n", right_args[0]);
        exit(EXIT_FAILURE);
    }
    close(fd[0]);
    close(fd[1]);
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
}

/* Execute a single command segment (without ";" separation) */
void process_command(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            execute_pipe(args);
            return;
        }
        i++;
    }
    execute_external(args);
}

int main(int argc, char **argv) {
    printf("Welcome to mini-shell.\n"); // Print welcome message
    char line[MAX_INPUT];
    while (1) {
        printf("shell $ ");
        fflush(stdout);
        if (fgets(line, MAX_INPUT, stdin) == NULL) {
            printf("\nBye bye.\n");
            break;
        }
        /* Save command for "prev" built-in */
        if (prevCommand) {
            free(prevCommand);
        }
        prevCommand = strdup(line);
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0)
            continue;
        int numTokens = 0;
        char **tokens = tokenize(line, &numTokens);
        if (numTokens == 0) {
            free(tokens);
            continue;
        }
        int start = 0;
        while (start < numTokens) {
            int end = start;
            while (end < numTokens && strcmp(tokens[end], ";") != 0)
                end++;
            int cmdArgCount = end - start;
            char **cmdArgs = malloc((cmdArgCount + 1) * sizeof(char *));
            if (!cmdArgs) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            for (int k = 0; k < cmdArgCount; k++) {
                cmdArgs[k] = tokens[start + k];
            }
            cmdArgs[cmdArgCount] = NULL;
            if (cmdArgs[0] != NULL && run_builtin(cmdArgs) == 0) {
                process_command(cmdArgs);
            }
            free(cmdArgs);
            start = end + 1;
        }
        for (int k = 0; k < numTokens; k++)
            free(tokens[k]);
        free(tokens);
    }
    if (prevCommand)
        free(prevCommand);
    return 0;
}
