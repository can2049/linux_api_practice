#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TIMEOUT_MS 5000  // 5 second timeout
#define MAX_FDS 2        // Number of file descriptors to monitor

int main(int argc, char *argv[]) {
  struct pollfd fds[MAX_FDS];
  int ret, i;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <file1> <file2>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Open two files for reading (could be pipes, sockets, etc.)
  for (i = 0; i < MAX_FDS; i++) {
    fds[i].fd = open(argv[i + 1], O_RDONLY | O_NONBLOCK);
    if (fds[i].fd == -1) {
      fprintf(stderr, "Failed to open %s: %s\n", argv[i + 1], strerror(errno));
      exit(EXIT_FAILURE);
    }

    // Set the events we're interested in (readable without blocking)
    fds[i].events = POLLIN;
  }

  printf("Waiting for data on files (timeout in %d ms)...\n", TIMEOUT_MS);

  // Wait for events
  ret = poll(fds, MAX_FDS, TIMEOUT_MS);
  if (ret == -1) {
    perror("poll");
    exit(EXIT_FAILURE);
  }

  if (ret == 0) {
    printf("Timeout occurred! No data after %d ms.\n", TIMEOUT_MS);
    exit(EXIT_SUCCESS);
  }

  // Check which file descriptors have events
  for (i = 0; i < MAX_FDS; i++) {
    if (fds[i].revents & POLLIN) {
      printf("File %d (%s) is ready for reading\n", i, argv[i + 1]);

      // Demonstrate reading some data
      char buf[256];
      ssize_t bytes_read = read(fds[i].fd, buf, sizeof(buf) - 1);
      if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        printf("Read %zd bytes: %s\n", bytes_read, buf);
      }
    }

    // You can check for other events too
    if (fds[i].revents & POLLHUP) {
      printf("File %d (%s) hung up\n", i, argv[i + 1]);
    }
    if (fds[i].revents & POLLERR) {
      printf("File %d (%s) error condition\n", i, argv[i + 1]);
    }
    if (fds[i].revents & POLLNVAL) {
      printf("File %d (%s) invalid request\n", i, argv[i + 1]);
    }
  }

  // Close file descriptors
  for (i = 0; i < MAX_FDS; i++) {
    close(fds[i].fd);
  }

  return EXIT_SUCCESS;
}
