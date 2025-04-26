#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 9100
#define MAX_CLIENTS FD_SETSIZE

int main() {
    int listenfd, connfd, maxfd, nready, i;
    int client[MAX_CLIENTS]; // 保存所有客户端描述符
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len;
    fd_set rset, allset;
    char buffer[1024];
    
    // 创建监听 socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    // 绑定地址
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
    
    printf("Multiplexing server (select) listening on port %d\n", PORT);
    
    maxfd = listenfd;
    for (i = 0; i < MAX_CLIENTS; i++)
        client[i] = -1;  // 初始化数组
    
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    
    while (1) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }
        
        // 检查是否有新连接
        if (FD_ISSET(listenfd, &rset)) {
            cliaddr_len = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddr_len)) < 0) {
                perror("accept error");
                continue;
            }
            // 找一个空位置保存新连接
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client[i] < 0) {
                    client[i] = connfd;
                    break;
                }
            }
            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Too many clients\n");
                close(connfd);
                continue;
            }
            FD_SET(connfd, &allset);
            if (connfd > maxfd)
                maxfd = connfd;
            
            if (--nready <= 0)
                continue;
        }
        
        // 检查所有客户端是否有数据可读
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client[i] < 0)
                continue;
            if (FD_ISSET(client[i], &rset)) {
                int n = read(client[i], buffer, sizeof(buffer)-1);
                if (n <= 0) {
                    // 客户端关闭
                    close(client[i]);
                    FD_CLR(client[i], &allset);
                    client[i] = -1;
                } else {
                    buffer[n] = '\0';
                    printf("Received from client[%d]: %s", i, buffer);
                    // 将接收到的数据回显到客户端
                    write(client[i], buffer, n);
                }
                if (--nready <= 0)
                    break;
            }
        }
    }
    
    return 0;
}
