#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

int main() {
  char buffer[BUF_LEN];

  // 初始化inotify
  int fd = inotify_init();
  if (fd < 0) {
    perror("inotify_init");
    exit(EXIT_FAILURE);
  }

  // 添加监控
  int wd = inotify_add_watch(fd, "/tmp", IN_MODIFY | IN_CREATE | IN_DELETE);
  if (wd == -1) {
    perror("inotify_add_watch");
    exit(EXIT_FAILURE);
  }

  // 读取事件
  while (1) {
    int length = read(fd, buffer, BUF_LEN);
    if (length < 0) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    // 处理事件
    int i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len) {
        if (event->mask & IN_CREATE) {
          printf("File %s created.\n", event->name);
        } else if (event->mask & IN_DELETE) {
          printf("File %s deleted.\n", event->name);
        } else if (event->mask & IN_MODIFY) {
          printf("File %s modified.\n", event->name);
        }
      }
      i += EVENT_SIZE + event->len;
    }
  }

  // 清理
  inotify_rm_watch(fd, wd);
  close(fd);

  return 0;
}
