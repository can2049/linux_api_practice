#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int MAX_EVENTS = 10;
constexpr int BUFFER_SIZE = 1024;
constexpr int PORT = 8080;

// Function to set a socket to non-blocking mode
static void set_nonblocking(int sockfd) {
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl F_GETFL");
    exit(EXIT_FAILURE);
  }

  if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl F_SETFL");
    exit(EXIT_FAILURE);
  }
}

int main() {
  int server_fd, client_fd, epoll_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  struct epoll_event ev, events[MAX_EVENTS];
  char buffer[BUFFER_SIZE];
  int nfds, i, n;

  // Create TCP socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Set socket options - allow reuse of address/port
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // Bind socket to port
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  // Start listening for connections
  if (listen(server_fd, SOMAXCONN) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d...\n", PORT);

  // Create epoll instance
  // The argument to epoll_create is a hint for the kernel about the number of
  // file descriptors to monitor. It's ignored in modern kernels (since 2.6.8),
  // but must be > 0.
  if ((epoll_fd = epoll_create1(0)) == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  // Add the server socket to the epoll interest list
  // We're interested in read events (EPOLLIN) on the server socket (new
  // connections)
  ev.events = EPOLLIN | EPOLLET;  // Edge-triggered mode
  ev.data.fd = server_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
    perror("epoll_ctl: server_fd");
    exit(EXIT_FAILURE);
  }

  // Main event loop
  while (1) {
    // Wait for events (indefinitely, no timeout)
    // Returns the number of file descriptors ready for I/O
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    printf("epoll_wait returned %d events\n", nfds);

    // Process all events
    for (i = 0; i < nfds; i++) {
      // Check for error conditions first
      if (events[i].events & (EPOLLERR | EPOLLHUP)) {
        fprintf(stderr, "epoll error on fd %d\n", events[i].data.fd);
        close(events[i].data.fd);
        continue;
      }

      // Handle new connection on server socket
      if (events[i].data.fd == server_fd) {
        // Accept all pending connections (important for edge-triggered mode)
        while (1) {
          client_fd =
              accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
          if (client_fd == -1) {
            // No more connections to accept
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              perror("accept");
              break;
            }
          }

          // Print client info
          char client_ip[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
                    sizeof(client_ip));
          printf("New connection from %s:%d\n", client_ip,
                 ntohs(client_addr.sin_port));

          // Set client socket to non-blocking
          set_nonblocking(client_fd);

          // Add client socket to epoll interest list
          // We monitor for read events (EPOLLIN) and also hang-up events
          // (EPOLLRDHUP)
          ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
          ev.data.fd = client_fd;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl: client_fd");
            close(client_fd);
          }
        }
      }
      // Handle data from client
      else {
        // With edge-triggered mode, we must read all available data
        int done = 0;
        while (!done) {
          ssize_t count = read(events[i].data.fd, buffer, BUFFER_SIZE);
          if (count == -1) {
            // No more data to read
            if (errno != EAGAIN) {
              perror("read");
              done = 1;
            }
            break;
          } else if (count == 0) {
            // EOF - client disconnected
            printf("Client disconnected (fd=%d)\n", events[i].data.fd);
            close(events[i].data.fd);
            done = 1;
            break;
          }

          // Process the data we read
          printf("Received %zd bytes from fd %d: %.*s\n", count,
                 events[i].data.fd, (int)count, buffer);

          // Echo back to client
          if (write(events[i].data.fd, buffer, count) == -1) {
            perror("write");
            close(events[i].data.fd);
            done = 1;
          }
        }
      }
    }
  }

  // Cleanup (though we never reach here in this simple example)
  close(server_fd);
  close(epoll_fd);
  return 0;
}
