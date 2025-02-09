#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  pid_t pid;
  int x = 1;

  pid = fork();

  if (pid == 0) {
    printf("Terminating Child pid = %d\n", getpid());
    exit(0);
  } else {
    printf("Running parent forever pid = %d\n", getpid());
    while (1) { }
  }

  return 0;
}
