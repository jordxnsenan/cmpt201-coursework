
/*
 * Section 5:
 *
 * Explain the use of non-blocking sockets in this lab.
 *
 * How are sockets made non-blocking?
 * with a call to set_non_blocking() function which utilizes fcntl to add
0_NONBLOCK flag to
 * the file descriptor
 *
 * What sockets are made non-blocking?
 * The listening socket (sfd) within run_acceptor() and each client socket
within run_client()
 *
 * Why are these sockets made non-blocking? What purpose does it serve?
 * The listening socket is non-blocking so accept() doesnt block forever + s/*
Questions to answer at top of server.c:
(You should not need to change client.c)

Understanding the Client:
1. How is the client sending data to the server? What protocol?
Using TCP and by writing bytes over that socket

2. What data is the client sending to the server?
1024 byte sized messages that contain strings

Understanding the Server:
1. Explain the argument that the `run_acceptor` thread is passed as an argument.
pointer to an acceptor_args struct which contains the shared list, a run flag +
the list mutex

2. How are received messages stored?
they are copied into dynamically allocated memory and added to a linked list

3. What does `main()` do with the received messages?
waits for all of the messages to arrive and then iterates through the o that the
acceptor thread can continuously check aargs.rum and shut down cleanly

The client sockets are non-blocking to ensure read() does not block forever and
so that the client threads can notice when run flag is set to false and exit
instead of being stuck.

*/
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define PORT 8001
#define LISTEN_BACKLOG 32
#define MAX_CLIENTS 4
#define NUM_MSG_PER_CLIENT 5

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

struct list_node {
  struct list_node *next;
  void *data;
};

struct list_handle {
  struct list_node *last;
  volatile uint32_t count;
};

struct client_args {
  atomic_bool run;

  int cfd;
  struct list_handle *list_handle;
  pthread_mutex_t *list_lock;
};

struct acceptor_args {
  atomic_bool run;

  struct list_handle *list_handle;
  pthread_mutex_t *list_lock;
};

int init_server_socket() {
  struct sockaddr_in addr;

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
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

  return sfd;
}

// Set a file descriptor to non-blocking mode
void set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl F_GETFL");
    exit(EXIT_FAILURE);
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl F_SETFL");
    exit(EXIT_FAILURE);
  }
}

void add_to_list(struct list_handle *list_handle, struct list_node *new_node) {
  struct list_node *last_node = list_handle->last;
  last_node->next = new_node;
  list_handle->last = last_node->next;
  list_handle->count++;
}

int collect_all(struct list_node head) {
  struct list_node *node = head.next; // get first node after head
  uint32_t total = 0;

  while (node != NULL) {
    printf("Collected: %s\n", (char *)node->data);
    total++;

    // Free node and advance to next item
    struct list_node *next = node->next;
    free(node->data);
    free(node);
    node = next;
  }

  return total;
}

static void *run_client(void *args) {
  struct client_args *cargs = (struct client_args *)args;
  int cfd = cargs->cfd;
  set_non_blocking(cfd);

  char msg_buf[BUF_SIZE];

  while (cargs->run) {
    ssize_t bytes_read = read(cfd, &msg_buf, BUF_SIZE);
    if (bytes_read == -1) {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        perror("Problem reading from socket!\n");
        break;
      }
    } else if (bytes_read > 0) {
      // Create node with data
      struct list_node *new_node = malloc(sizeof(struct list_node));
      new_node->next = NULL;
      new_node->data = malloc(BUF_SIZE);
      memcpy(new_node->data, msg_buf, BUF_SIZE);

      struct list_handle *list_handle = cargs->list_handle;

      // TODO: Safely use add_to_list to add new_node to the list
      pthread_mutex_t *list_lock = cargs->list_lock;

      pthread_mutex_lock(list_lock);

      { add_to_list(list_handle, new_node); }

      pthread_mutex_unlock(list_lock);
    }
  }

  if (close(cfd) == -1) {
    perror("client thread close");
  }
  return NULL;
}

static void *run_acceptor(void *args) {
  int sfd = init_server_socket();
  set_non_blocking(sfd);

  struct acceptor_args *aargs = (struct acceptor_args *)args;
  pthread_t threads[MAX_CLIENTS];
  struct client_args client_args[MAX_CLIENTS];

  printf("Accepting clients...\n");

  uint16_t num_clients = 0;
  while (aargs->run) {
    if (num_clients < MAX_CLIENTS) {
      int cfd = accept(sfd, NULL, NULL);
      if (cfd == -1) {
        if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
          handle_error("accept");
        }
      } else {
        printf("Client connected!\n");

        client_args[num_clients].cfd = cfd;
        client_args[num_clients].run = true;
        client_args[num_clients].list_handle = aargs->list_handle;
        client_args[num_clients].list_lock = aargs->list_lock;
        num_clients++;

        // TODO: Create a new thread to handle the client
        int index_current_client = num_clients - 1;

        if (pthread_create(&threads[index_current_client], NULL, run_client,
                           &client_args[index_current_client]) != 0) {
          perror("pthread_create");

          // If thread creation fails, stop this client
          client_args[index_current_client].run = false;

          if (close(client_args[index_current_client].cfd) == -1) {
            perror("closing client");
          }
        }
      }
    }
  }

  printf("Not accepting any more clients!\n");

  // Shutdown and cleanup

  // signals each client to stop
  for (int i = 0; i < num_clients; i++) {
    client_args[i].run = false;
  }

  for (int i = 0; i < num_clients; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("pthread_join");
    }
  }

  if (close(sfd) == -1) {
    perror("closing server socket");
  }
  return NULL;
}

int main() {
  pthread_mutex_t list_mutex;
  pthread_mutex_init(&list_mutex, NULL);

  // List to store received messages
  // - Do not free list head (not dynamically allocated)
  struct list_node head = {NULL, NULL};
  struct list_node *last = &head;
  struct list_handle list_handle = {
      .last = &head,
      .count = 0,
  };

  pthread_t acceptor_thread;
  struct acceptor_args aargs = {
      .run = true,
      .list_handle = &list_handle,
      .list_lock = &list_mutex,
  };
  pthread_create(&acceptor_thread, NULL, run_acceptor, &aargs);

  // TODO: Wait until enough messages are received
  unsigned int target = MAX_CLIENTS * NUM_MSG_PER_CLIENT;

  while (true) {
    pthread_mutex_lock(&list_mutex);
    unsigned int current = list_handle.count;
    pthread_mutex_unlock(&list_mutex);

    if (current >= target) {
      break;
    }

    sleep(1000);
  }

  aargs.run = false;
  pthread_join(acceptor_thread, NULL);

  if (list_handle.count != MAX_CLIENTS * NUM_MSG_PER_CLIENT) {
    printf("Not enough messages were received!\n");
    return 1;
  }

  int collected = collect_all(head);
  printf("Collected: %d\n", collected);
  if (collected != list_handle.count) {
    printf("Not all messages were collected!\n");
    return 1;
  } else {
    printf("All messages were collected!\n");
  }

  pthread_mutex_destroy(&list_mutex);

  return 0;
}
