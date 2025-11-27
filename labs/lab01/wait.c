#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork()");
    return 1;
  }

  if (pid > 0) {
    int wstatus = 0;

    if (waitpid(pid, &wstatus, 0) == -1) {
      perror("waitpid() failed");

      return 1;
    }

    if (WIFEXITED(wstatus)) {
      printf("Reason: %d\n", WEXITSTATUS(wstatus));
    }
  }

  if (pid == 0) {
    char *path = "/bin/ls";

    if (execl(path, path, "-a", "-l", NULL) == -1) {
      perror("execl() failed");
      return 1;
    }
  }
}
