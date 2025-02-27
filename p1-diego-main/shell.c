#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tokenizer.h"

static char prevCommand[MAX_INPUT] = "";

// Searches PATH for 'file' and calls execv to run it. If 'file' has '/', use it directly.
static void runProgram(char *file, char *argv[]) {
    if (strchr(file, '/')) {
        execv(file, argv);
        perror("execv");
        _exit(1);
    }
    char *pathEnv = getenv("PATH");
    if (!pathEnv) {
        char *fallback = "/bin:/usr/bin";
        pathEnv = fallback;
    }
    char buf[2048];
    strncpy(buf, pathEnv, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (char *dir = strtok(buf, ":"); dir; dir = strtok(NULL, ":")) {
        char candidate[2048];
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, file);
        // execv loads and runs a new program from a path that we gave. If it works, doesn't returns.
        execv(candidate, argv);
    }
    fprintf(stderr, "%s: command not found\n", file);
    _exit(1);
}

// Sets up < and > redirection, then calls runProgram to start the command.
static void runRedirectExec(char *argv[]) {
    char *infile = NULL;
    char *outfile = NULL;
    char *newArgs[MAX_TOKENS];
    int i = 0, j = 0;
    while (argv[i]) {
        if (strcmp(argv[i], "<") == 0) {
            infile = argv[i + 1];
            i += 2;
        } else if (strcmp(argv[i], ">") == 0) {
            outfile = argv[i + 1];
            i += 2;
        } else {
            newArgs[j++] = argv[i++];
        }
    }
    newArgs[j] = NULL;
    if (infile) {
        int fd = open(infile, O_RDONLY);
        if (fd < 0) {
            perror("open infile");
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (outfile) {
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open outfile");
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (!newArgs[0]) {
        _exit(0);
    }
    runProgram(newArgs[0], newArgs);
    _exit(1);
}

// Forks a child to runRedirectExec. Parent waits for the child to finish.
static void runSimpleCommand(char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        runRedirectExec(argv);
        _exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

// Looks for '|'. If found, split into left/right, create a pipe, fork twice, and runRedirectExec in each.
static void runPipe(char *argv[]) {
    int pipeIndex = -1;
    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], "|") == 0) {
            pipeIndex = i;
            break;
        }
    }
    if (pipeIndex == -1) {
        runSimpleCommand(argv);
        return;
    }
    argv[pipeIndex] = NULL;
    char **left = argv;
    char **right = &argv[pipeIndex + 1];
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return;
    }
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        return;
    }
    if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        runRedirectExec(left);
        _exit(1);
    }
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        return;
    }
    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        runRedirectExec(right);
        _exit(1);
    }
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Checks if the first argument is exit, cd, or prev. If so, handles it and returns 1; otherwise 0.
static int checkBuiltIn(char *argv[]) {
    if (!argv[0]) return 0;
    if (strcmp(argv[0], "exit") == 0) {
        printf("Bye bye.\n");
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        char *dir = argv[1] ? argv[1] : getenv("HOME");
        if (!dir) dir = "/";
        if (chdir(dir) != 0) {
            perror("chdir");
        }
        return 1;
    }
    if (strcmp(argv[0], "prev") == 0) {
        if (!*prevCommand) return 1;
        char tokens2D[MAX_TOKENS][MAX_INPUT];
        int numPrev = 0;
        tokenize(prevCommand, tokens2D, &numPrev);
        if (numPrev > 0) {
            char *tmpArgs[MAX_TOKENS];
            for (int i = 0; i < numPrev; i++) {
                tmpArgs[i] = tokens2D[i];
            }
            tmpArgs[numPrev] = NULL;
            for (int i = 0; tmpArgs[i]; i++) {
                if (strcmp(tmpArgs[i], "|") == 0) {
                    runPipe(tmpArgs);
                    return 1;
                }
            }
            runSimpleCommand(tmpArgs);
        }
        return 1;
    }
    return 0;
}

// If it's not built-in, check for '|', then runPipe or runSimpleCommand.
static void runCommand(char *argv[]) {
    if (!checkBuiltIn(argv)) {
        for (int i = 0; argv[i]; i++) {
            if (strcmp(argv[i], "|") == 0) {
                runPipe(argv);
                return;
            }
        }
        runSimpleCommand(argv);
    }
}

int main(void) {
    printf("Welcome to mini-shell.\n");
    while (1) {
        printf("shell $ ");
        fflush(stdout);
        char line[MAX_INPUT];
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (!*line) continue;
        if (strcmp(line, "prev") != 0) {
            strncpy(prevCommand, line, sizeof(prevCommand) - 1);
            prevCommand[sizeof(prevCommand) - 1] = '\0';
        }
        char tokens2D[MAX_TOKENS][MAX_INPUT];
        int numTokens = 0;
        tokenize(line, tokens2D, &numTokens);
        if (numTokens == 0) continue;
        int start = 0;
        while (start < numTokens) {
            int end = start;
            while (end < numTokens && strcmp(tokens2D[end], ";") != 0) {
                end++;
            }
            if (end > start) {
                char *args[MAX_TOKENS];
                int c = 0;
                for (int i = start; i < end; i++) {
                    args[c++] = tokens2D[i];
                }
                args[c] = NULL;
                runCommand(args);
            }
            start = end + 1;
        }
    }
    return 0;
}
