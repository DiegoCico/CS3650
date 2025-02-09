#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid;
    int status;

    // Command 1: ps aux (macOS alternative to ps -uf)
    pid = fork();
    if (pid == 0) {
        char *args[] = {"/bin/ps", "aux", NULL};
        execve(args[0], args, NULL);
        perror("execve");
        exit(1);
    } else {
        wait(&status);
    }

    // Command 2: echo --help
    pid = fork();
    if (pid == 0) {
        char *args[] = {"/bin/echo", "--help", NULL};
        execve(args[0], args, NULL);
        perror("execve");
        exit(1);
    } else {
        wait(&status);
    }

    // Command 3: nl -b a example1.c
    pid = fork();
    if (pid == 0) {
        char *args[] = {"/usr/bin/nl", "-b", "a", "example1.c", NULL};
        execve(args[0], args, NULL);
        perror("execve");
        exit(1);
    } else {
        wait(&status);
    }

    return 0;
}
