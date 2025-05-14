#include <poll.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    struct pollfd fds[1];
    int ret;
    
    // 监视标准输入
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    printf("Waiting for input...\n");
    
    // 等待5秒
    ret = poll(fds, 1, 5000);
    
    if (ret == -1) {
        perror("poll");
    } else if (ret) {
        printf("Data is available now.\n");
        if (fds[0].revents & POLLIN) {
            char buf[100];
            read(STDIN_FILENO, buf, sizeof(buf));
            printf("Read: %s", buf);
        }
    } else {
        printf("No data within 5 seconds.\n");
    }
    
    return 0;
}
