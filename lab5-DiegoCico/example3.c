#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  pid_t pid;
  int x = 1;

  pid = fork();

  if (pid == 0) {
    printf("Running child forever pid = %d\n", getpid());
    while (1) { }
  } else {
    printf("Terminating parent pid = %d\n", getpid());
    exit(0);
  }

  return 0;
}
