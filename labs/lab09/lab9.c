/*
Questions to answer at top of client.c:
(You should not need to change the code in client.c)
1. What is the address of the server it is trying to connect to (IP address and
port number).

   127.0.0.1:8000

2. Is it UDP or TCP? How do you know?

   TCP , this is evident by the fact that the socket is created with SOCK_STREAM
within the socket() call

3. The client is going to send some data to the server. Where does it get this
data from? How can you tell in the code?

   Gets data from the standard input file stream. The read() call reads from
STDIN_FILENO into the buf variable writes the input data to the socket w/the
write() call

4. How does the client program end? How can you tell that in the code?

   Ends in the event of either reaching the end of the input, an input less than
1 byte being read or when an error occuers. You can tell from the loop condiion
which reads into num_read so long as the input is greater than 1 byte. In the
event of no error it closes the socket and exits w/ exit(EXIT_SUCCESS). But when
error occurs, it calls handle_error which terminates the program w/
exit(EXIT_FAILURE)
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUF_SIZE 64
#define ADDR "127.0.0.1"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int main() {
  struct sockaddr_in addr;
  int sfd;
  ssize_t num_read;
  char buf[BUF_SIZE];

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
    handle_error("inet_pton");
  }

  int res = connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (res == -1) {
    handle_error("connect");
  }

  while ((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 1) {
    if (write(sfd, buf, num_read) != num_read) {
      handle_error("write");
    }
    printf("Just sent %zd bytes.\n", num_read);
  }

  if (num_read == -1) {
    handle_error("read");
  }

  close(sfd);
  exit(EXIT_SUCCESS);
}

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 64
#define PORT 8000
#define LISTEN_BACKLOG 32

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// Shared counters for: total # messages, and counter of clients (used for
// assigning client IDs)
int total_message_count = 0;
int client_id_counter = 1;

// Mutexs to protect above global state.
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_info {
  int cfd;
  int client_id;
};

void *handle_client(void *arg) {
  struct client_info *client = (struct client_info *)arg;

  int client_socket_fd = client->cfd;
  int client_id = client->client_id;

  char message_buffer[BUF_SIZE];

  while (true) {
    ssize_t bytes_read = read(client_socket_fd, message_buffer, BUF_SIZE);

    if (bytes_read <= 0) {
      break;
    }

    // null terminates so message can be treated as string
    message_buffer[bytes_read] = '\0';

    int message_number = 0;

    pthread_mutex_lock(&count_mutex);
    {
      total_message_count++;

      message_number = total_message_count;
    }

    pthread_mutex_unlock(&count_mutex);

    printf("Msg # %d; Client ID %d: %s", message_number, client_id,
           message_buffer);
  }

  printf("Ending thread for client %d\n", client_id);

  close(client_socket_fd);
  free(client);

  return NULL;
}

int main() {
  struct sockaddr_in addr;

  int sfd;

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    handle_error("bind");
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    handle_error("listen");
  }

  for (;;) {
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);

    int client_socket_fd =
        accept(sfd, (struct sockaddr *)&client_address, &client_address_length);

    if (client_socket_fd == -1) {
      perror("accept");
      continue;
    }

    struct client_info *client_data = malloc(sizeof(struct client_info));
    if (client_data == NULL) {
      perror("malloc");
      close(client_socket_fd);
      continue;
    }

    pthread_mutex_lock(&client_id_mutex);
    {
      client_data->client_id = client_id_counter;
      client_id_counter++;
    }
    pthread_mutex_unlock(&client_id_mutex);

    client_data->cfd = client_socket_fd;

    printf("New client created! ID %d on socket FD %d\n",
           client_data->client_id, client_data->cfd);

    pthread_t thread_id;

    if (pthread_create(&thread_id, NULL, handle_client, client_data) != 0) {
      perror("pthread_create");
      close(client_socket_fd);
      free(client_data);

      continue;
    }

    pthread_detach(thread_id);
  }

  if (close(sfd) == -1) {
    handle_error("close");
  }

  return 0;
}
