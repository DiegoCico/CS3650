#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  pid_t pid;
  int x = 1;

  pid = fork();

  if (pid == 0) {
    printf("child: x = %d\n", ++x);
    return 0;  
  }

  printf("parent: x = %d\n", --x);

  return 0;
}
