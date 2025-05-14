#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

const int MAX_EVENT_NUMBER = 10000;  // 最大事件数

// 设置句柄非阻塞
int set_nonblocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

int main() {
  // 创建套接字
  int nRet = 0;
  int m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
  if (m_listenfd < 0) {
    printf("fail to socket!");
    return -1;
  }
  //
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(6666);

  int flag = 1;
  // 设置ip可重用
  setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
  // 绑定端口号
  int ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
  if (ret < 0) {
    printf("fail to bind!,errno :%d", errno);
    return ret;
  }

  // 监听连接fd
  ret = listen(m_listenfd, 200);
  if (ret < 0) {
    printf("fail to listen!,errno :%d", errno);
    return ret;
  }

  // 初始化红黑树和事件链表结构rdlist结构
  epoll_event events[MAX_EVENT_NUMBER];
  // 创建epoll实例
  int m_epollfd = epoll_create(5);
  if (m_epollfd == -1) {
    printf("fail to epoll create!");
    return m_epollfd;
  }

  // 创建节点结构体将监听连接句柄
  epoll_event event;
  event.data.fd = m_listenfd;
  // 设置该句柄为边缘触发（数据没处理完后续不会再触发事件，水平触发是不管数据有没有触发都返回事件），
  event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  // 添加监听连接句柄作为初始节点进入红黑树结构中，该节点后续处理连接的句柄
  epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &event);

  // 进入服务器循环
  while (1) {
    int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
    if (number < 0 && errno != EINTR) {
      printf("epoll failure");
      break;
    }
    for (int i = 0; i < number; i++) {
      int sockfd = events[i].data.fd;
      // 属于处理新到的客户连接
      if (sockfd == m_listenfd) {
        struct sockaddr_in client_address;
        socklen_t client_addrlength = sizeof(client_address);
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address,
                            &client_addrlength);
        if (connfd < 0) {
          printf("errno is:%d accept error", errno);
          return 1;
        }
        epoll_event event;
        event.data.fd = connfd;
        // 设置该句柄为边缘触发（数据没处理完后续不会再触发事件，水平触发是不管数据有没有触发都返回事件），
        event.events = EPOLLIN | EPOLLRDHUP;
        // 添加监听连接句柄作为初始节点进入红黑树结构中，该节点后续处理连接的句柄
        epoll_ctl(m_epollfd, EPOLL_CTL_ADD, connfd, &event);
        set_nonblocking(connfd);
      } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        // 服务器端关闭连接，
        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, 0);
        close(sockfd);
      }
      // 处理客户连接上接收到的数据
      else if (events[i].events & EPOLLIN) {
        char buf[1024] = {0};
        read(sockfd, buf, 1024);
        printf("from client: %s", buf);

        // 将事件设置为写事件返回数据给客户端
        events[i].data.fd = sockfd;
        events[i].events = EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        epoll_ctl(m_epollfd, EPOLL_CTL_MOD, sockfd, &events[i]);
      } else if (events[i].events & EPOLLOUT) {
        std::string response = "server response \n";
        write(sockfd, response.c_str(), response.length());

        // 将事件设置为读事件，继续监听客户端
        events[i].data.fd = sockfd;
        events[i].events = EPOLLIN | EPOLLRDHUP;
        epoll_ctl(m_epollfd, EPOLL_CTL_MOD, sockfd, &events[i]);
      }
      // else if 可以加管道，unix套接字等等数据
    }
  }
}
