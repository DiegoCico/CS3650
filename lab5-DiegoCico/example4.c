#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int child_status;

  if (fork() == 0) {
    printf("Child runs!\n");
    exit(0);
  } else {
    printf("Hello from parent\n");
    wait(&child_status);  

    if (WIFEXITED(child_status)) {
      printf("Child exited with the following child_status %d\n", WEXITSTATUS(child_status));
    }
    printf("Child always is terminated by this point--parent resuming\n");
  }

  printf("This always prints last\n");
  return 0;
}
