#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h> // 包含 TCP_CORK 定義
#include <signal.h>
#include "system.h"

#define PORT 9999
#define BUFFER_SIZE 1024
#define LISTENQ 1024
#define PID_FILE "tmp/tcp_server.pid"

static void usage(const char *prog)
{
    printf(
        "Usage: %s [command]\n"
        "command options:\n"
        "  start : start the web server\n"
        "  stop  : stop accepting and wait for termination\n",
        prog);
}

static void send_signal(int signo)
{
    int pid = read_pidfile(PID_FILE);
    if (pid > 0) {
            // 發送SIGTERM信號以終止伺服器進程
            if (kill(pid, SIGTERM) == 0) {
                printf("Server stopped successfully.\n");
                // 刪除PID文件
                unlink(PID_FILE);
            } else {
                perror("Failed to stop the server");
            }
    } else {
        fprintf(stderr, "Could not read PID or server is not running.\n");
    }
}

int tcp_srv_init(int port) {
    int listenfd, optval = 1;
    struct sockaddr_in server_addr;

    /* 創建伺服器socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
        return -1;
    }

    /* 消除綁定過程中的“地址已在使用”錯誤 */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0) {
        perror("Set socket option SO_REUSEADDR failed");
        return -1;
    }

    /* 設置TCP_CORK選項 */
    if (setsockopt(listenfd, IPPROTO_TCP, TCP_CORK, (const void *)&optval, sizeof(int)) < 0) {
        perror("Set socket option TCP_CORK failed");
        return -1;
    }

    /* 將伺服器socket綁定到指定端口和地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    /* 使其成為一個監聽socket，準備接受連接請求 */
    if (listen(listenfd, LISTENQ) < 0) {
        perror("Listen failed");
        return -1;
    }

    printf("Server listening on port %d\n", port);
    return listenfd;
}

void handle_client(int connfd) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // 讀取客戶端的請求
    bytes_read = read(connfd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Failed to read from socket");
        close(connfd);
        return;
    }

    // 簡單地打印請求內容
    buffer[bytes_read] = '\0';
    printf("Received request:\n%s\n", buffer);

    // 構建HTTP響應
    char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, world!";

    // 發送HTTP響應給客戶端
    write(connfd, response, strlen(response));

    // 關閉客戶端連接
    close(connfd);
}

void worker_process_cycle(int listenfd) {
    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 處理客戶端連接
    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(connfd);
        close(connfd);
    }
}

int main(int argc, char** argv) {

    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {
        send_signal(SIGTERM);
        printf("cserv: stopped.\n");
        return 0;
    }
    /* unsupported command */
    if (strcmp(argv[1], "start")) {
        fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
        usage(argv[0]);
        return 1;
    }
    // 使用tcp_srv_init初始化伺服器
    int listenfd = tcp_srv_init(PORT);

    create_pidfile(PID_FILE);
    // 啟動工作進程循環
    worker_process_cycle(listenfd);

    // 關閉伺服器socket
    close(listenfd);


	return 0;
}