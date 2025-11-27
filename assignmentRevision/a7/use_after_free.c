#include <stdio.h>
#include <stdlib.h>

void free_memory(int *ptr) {
  free(ptr); // Free the memory
}

int main() {
  int *ptr = (int *)malloc(sizeof(int));

  if (ptr == NULL) {
    perror("Memory allocation failed");
    return 1;
  }

  *ptr = 42;

  free_memory(ptr);
  printf("Value after freeing: %d\n", *ptr); // Accessing freed memory

  return 0;
}
