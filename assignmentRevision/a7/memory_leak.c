#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_user_input() {
  // Allocate a buffer
  size_t buffer_size = 100;
  char *buffer = (char *)malloc(buffer_size * sizeof(char));

  // Check if the allocation was successful
  if (buffer == NULL) {
    perror("malloc() failed");
    return NULL;
  }

  // Read a user input
  printf("Enter your input: ");
  if (fgets(buffer, buffer_size, stdin) == NULL) {
    perror("fgets() failed");
    return NULL;
  }

  // Return the buffer that contains the user input
  return buffer;
}

int main() {
  char *input = read_user_input();

  if (input != NULL) {
    printf("You entered: %s\n", input);
    free(input);
  }

  return 0;
}
