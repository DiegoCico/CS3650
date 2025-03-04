#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char **argv) {
  /* Dynamically allocate some amount of memory */
  char *message = (char *) malloc(sizeof(char) * 12);

  /* Check if memory allocation failed (i.e. we do not have enough bytes */
  if (NULL == message) {
    printf("Allocation failed\n");
    exit(0);
  }

  /* If it did not fail, then copy a string into our allocated memory.
   * Function signature: char *strcpy(char *dest, const char *src); 
   */
  strcpy(message, "Hello, Bob\n");

  printf("%s", message);

  /* Lets now free our memory */
  free(message);

  /* If we are working with data, it can sometimes be nice to zero everything 
   * out. You'll notice the parameters are a little flipped here. This is asking
   * for '57' blocks of memory that are the sizeof(int). So we are getting 57 
   * sizeof(int) pieces of memory). 
   */
  int *numberData = (int *) calloc(57, sizeof(int));
  int i;

  /* Let's confirm it is all zeroed out memory */
  for (i = 0; i < 57; ++i) {
    assert(numberData[i] == 0);
    printf("%d ", numberData[i]);
  }
  printf("\n");

  /* Lets free our data */
  free(numberData);

  return 0;
}