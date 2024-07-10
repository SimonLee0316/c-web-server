#define _GNU_SOURCE
#include "coro.h"
#include "system.h"
#include "task.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 9999
#define BUFFER_SIZE 1024
#define LISTENQ 1024
#define PID_FILE "tmp/tcp_server.pid"
#define MAX_WORKERS 1024

int pids[MAX_WORKERS];

static void usage(const char *prog) {
  printf("Usage: %s [command]\n"
         "command options:\n"
         "  start : start the web server\n"
         "  stop  : stop accepting and wait for termination\n",
         prog);
}

static void send_signal(int signo) {
  int num_pids = 0;
  read_pidfile(PID_FILE, pids, &num_pids);
  for (int i = 0; i < num_pids; i++) {
    if (kill(pids[i], signo) == 0) {
      printf("Stopped process %d successfully.\n", pids[i]);
    } else {
      perror("Failed to stop the process");
    }
  }
  unlink(PID_FILE);
}

int tcp_srv_init(int port) {
  int listenfd, optval = 1;
  struct sockaddr_in server_addr;

  /* Create server socket */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Could not create socket");
    return -1;
  }

  /* Eliminate 'Address already in use' error during the binding process */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int)) < 0) {
    perror("Set socket option SO_REUSEADDR failed");
    return -1;
  }

  /* Setting TCP_CORK option */
  if (setsockopt(listenfd, IPPROTO_TCP, TCP_CORK, (const void *)&optval,
                 sizeof(int)) < 0) {
    perror("Set socket option TCP_CORK failed");
    return -1;
  }

  /* Bind the server socket to the specified port and address */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    return -1;
  }

  /* Make it a listening socket, ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) {
    perror("Listen failed");
    return -1;
  }

  printf("Server listening on port %d\n", port);
  return listenfd;
}

void handle_client(void *udata) {
  int connfd = *(int *)udata;
  char buffer[BUFFER_SIZE];
  int bytes_read;

  printf("accept request, fd is %d, pid is %d\n", connfd, getpid());
  // read client request
  bytes_read = read(connfd, buffer, BUFFER_SIZE - 1);
  if (bytes_read < 0) {
    perror("Failed to read from socket");
    close(connfd);
    return;
  }

  //簡單地打印請求內容
  // buffer[bytes_read] = '\0';
  // printf("Received request:\n%s\n", buffer);

  // build http response
  char *response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: 13\r\n"
                   "\r\n"
                   "Hello, world!";

  // send http response to client
  write(connfd, response, strlen(response));

  // close client connect
  close(connfd);
}

void worker_process_cycle(void *udata) {
  int listenfd = *(int *)udata;
  int connfd;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  // accept cilent request
  while (1) {
    connfd =
        accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (connfd < 0) {
      perror("Accept failed");
      continue;
    }
    quick_start(handle_client, co_cleanup, &(int){connfd});
  }
}

void create_workers(int listenfd, int num_workers) {

  for (int i = 0; i < num_workers; i++) {
    pid_t pid = fork();
    if (pid == 0) { // Child
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(i % num_workers, &cpuset);
      if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) < 0) {
        perror("Could not set CPU affinity");
        exit(1);
      }
      quick_start(worker_process_cycle, co_cleanup, &(int){listenfd});

    } else if (pid > 0) { // Parent
      pids[i] = pid;
      printf("Child process %d created and assigned to CPU %d\n", pid,
             i % num_workers);
    } else {
      perror("Fork failed");
    }
  }
  create_pidfile(PID_FILE, pids, num_workers);
  // Parent process waits for children to exit
  while (wait(NULL) > 0)
    ;
}

int main(int argc, char **argv) {

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
  // initial server
  int listenfd = tcp_srv_init(PORT);

  int num_workers = sysconf(_SC_NPROCESSORS_ONLN);
  create_workers(listenfd, num_workers);

  // close server socket
  close(listenfd);

  return 0;
}