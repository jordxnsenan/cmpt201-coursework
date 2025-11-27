#include <stdio.h>     // for printf()
#include <sys/types.h> // for pid_t
#include <unistd.h>    // for fork() and exec()

int main(void) {
  // find out if parent or child
  pid_t pid = fork();

  // parent
  if (pid > 0) {
    printf("parent..\n");

    const char *path = "/bin/ls";

    execl(path, path, "-a", (char *)NULL);
  }

  else if (pid == 0) {
    printf("Child..\n");

    char *path = "/bin/ls";

    char *args[] = {path, "-a", "-l", "-h", NULL};

    execv(path, args);
  }

  else {
    printf("fork failed..\n");
  }

  return 0;
}
