// this is the client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // for socket()
#include <sys/un.h>     // for unix struct
#include <unistd.h>

int main(void) {

  printf("CLIENT!:\n");

  // socket
  int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  // always check for errors - if first fails it will use garbage as the socket
  if (socket_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // connect
  struct sockaddr_un sockstruct;
  sockstruct.sun_family = AF_UNIX; /* AF_UNIX */
  snprintf(sockstruct.sun_path, 108,
           "socket_fun"); // opens local socket named socket_fun

  // create destination address
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, 108, "da_udp_socket");

  // write
  char *msg = "I'm sending this, I hope you get it\n";

  int bytes_written =
      sendto(socket_fd, msg, strlen(msg), 0, (struct sockaddr *)&addr,
             sizeof(struct sockaddr_un));

  if (bytes_written == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  // close
  close(socket_fd);

  return 0;
}
