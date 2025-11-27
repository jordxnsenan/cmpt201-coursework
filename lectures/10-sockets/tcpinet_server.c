// this is the server
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // for socket()
#include <sys/un.h>     // for unix struct
#include <unistd.h>

int main(void) {
  printf("server\n");

  // socket
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  // always check for errors - if first fails it will use garbage as the socket
  if (socket_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // REMOVE the existing socket (if any)
  if (remove("socket_fun") == -1 && errno != ENOENT) {
    perror("remove");
    exit(EXIT_FAILURE);
  }
  // bind
  struct sockaddr_in sockstruct;
  memset(&sockstruct, 0, sizeof(struct sockaddr_in));
  sockstruct.sin_family = AF_INET; /* AF_UNIX */
  sockstruct.sin_port = htons(1042);
  sockstruct.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(socket_fd, (struct sockaddr *)&sockstruct,
           sizeof(struct sockaddr_in)) == -1) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // listen
  // accepts one connection at a time, back
  if (listen(socket_fd, 10) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // accept
  while (true) {
    printf("ACCEPTING NEW CLIENTS\n");
    int connected_fd = accept(socket_fd, NULL, NULL);

    if (connected_fd == -1) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    // read
    const int SIZE = 1024;
    char buff[SIZE];
    int bytes_read = 0;

    // keeps looping while client writes data - until port closes
    while ((bytes_read = read(connected_fd, buff, SIZE)) > 0) {
      write(STDOUT_FILENO, buff, bytes_read);
    }

    if (bytes_read == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    close(connected_fd);
  }

  // close
  close(socket_fd);

  return 0;
}
