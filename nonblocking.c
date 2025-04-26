#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9000

int main() {
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    
    // 创建 socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    // 设置 socket 为非阻塞模式
    int flags = fcntl(listenfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL error");
        exit(EXIT_FAILURE);
    }
    if (fcntl(listenfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL error");
        exit(EXIT_FAILURE);
    }
    
    // 初始化地址结构，并绑定端口
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    
    if (listen(listenfd, 10) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    
    printf("Non-blocking server listening on port %d\n", PORT);
    
    // 循环尝试接受连接
    while (1) {
        connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("No incoming connection, non-blocking mode, continue...\n");
                sleep(1);
                continue;
            } else {
                perror("accept error");
                break;
            }
        }
        printf("Accepted a connection!\n");
        close(connfd);
    }
    
    close(listenfd);
    return 0;
}
