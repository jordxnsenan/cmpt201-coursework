#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

void *thread_func(void *arg) {
  char *msg = (char *)arg;

  // print the received string and thread ID
  pthread_t thread_id = pthread_self();

  printf("Thread Received: %s ... Thread ID: %d\n", msg, thread_id);

  return (void *)strlen(msg);
}

int main(void) {
  pthread_t thread;
  void *ret_val;
  const char *msg = "Hello from pthread";

  // create new thread
  if (pthread_create(&thread, NULL, thread_func, (void *)msg) != 0) {
    printf("ERROR: pthread_create failed");
    return 1;
  }
}
