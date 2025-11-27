#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX 1000000

int count = 0;

// step 1: Make the thread function
void *counter_thread(void *arg) {
  for (int i = 0; i < MAX; i++) {
    count++;
  }

  return NULL;
}

int main(void) {
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, counter_thread, NULL);

  pthread_t thread_id2;
  pthread_create(&thread_id2, NULL, counter_thread, NULL);

  pthread_join(thread_id2, NULL);
  pthread_join(thread_id2, NULL);

  printf("The answer is: %d\n", count);

  return 0;
}
