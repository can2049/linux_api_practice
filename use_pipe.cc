#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  int fd[2];
  pipe(fd); // 创建管道

  if (fork() != 0) { // 主进程
    int pid = getpid();
    close(fd[0]); // 关闭读端
    std::string msg = "Hello from: " + std::to_string(pid);
    write(fd[1], msg.c_str(), msg.size() + 1); // 发送消息

    close(fd[1]);
  } else {        // 子进程
    close(fd[1]); // 关闭写端
    char buf[20];
    read(fd[0], buf, 20);
    // printf("Received: %s\n", buf);
    std::cout << "Received: " << buf << std::endl;
    close(fd[0]);
    wait(NULL);
  }
  return 0;
}
