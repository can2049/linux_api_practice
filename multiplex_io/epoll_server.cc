#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#define PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

// Set socket to non-blocking
int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
  int listen_sock, conn_sock, epoll_fd;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t cli_len = sizeof(cli_addr);

  // Create listening socket
  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Set reuse address
  int opt = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Bind to localhost:8080
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on all interfaces
  serv_addr.sin_port = htons(PORT);

  if (bind(listen_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  if (listen(listen_sock, SOMAXCONN) < 0) {
    perror("listen");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  std::cout << "Server is running on 127.0.0.1:8080..." << std::endl;

  // Create epoll instance
  epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_create1");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  struct epoll_event ev, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = listen_sock;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) < 0) {
    perror("epoll_ctl: listen_sock");
    close(listen_sock);
    close(epoll_fd);
    exit(EXIT_FAILURE);
  }

  while (true) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
      perror("epoll_wait");
      break;
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == listen_sock) {
        // Accept new connection
        conn_sock = accept(listen_sock, (struct sockaddr*)&cli_addr, &cli_len);
        if (conn_sock < 0) {
          perror("accept");
          continue;
        }

        set_nonblocking(conn_sock);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_sock;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) < 0) {
          perror("epoll_ctl: conn_sock");
          close(conn_sock);
        }
      } else {
        // Handle data from client
        int client_fd = events[n].data.fd;
        char buffer[BUFFER_SIZE];
        ssize_t count = read(client_fd, buffer, sizeof(buffer) - 1);

        if (count <= 0) {
          // Client disconnected or error
          if (count == 0)
            std::cout << "Client disconnected." << std::endl;
          else
            perror("read");

          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
          close(client_fd);
        } else {
          buffer[count] = '\0';  // Null-terminate

          // Get client information
          struct sockaddr_in client_addr;
          socklen_t client_len = sizeof(client_addr);
          getpeername(client_fd, (struct sockaddr*)&client_addr, &client_len);
          char client_ip[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,
                    sizeof(client_ip));
          int client_port = ntohs(client_addr.sin_port);

          std::cout << "Received from client [" << client_ip << ":"
                    << client_port << "]: " << buffer;
          write(client_fd, buffer, count);  // Echo back
        }
      }
    }
  }

  close(listen_sock);
  close(epoll_fd);
  return 0;
}
