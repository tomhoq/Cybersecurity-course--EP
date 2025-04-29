#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#define DEBUG
#ifdef DEBUG
#define DEBUG_OUTPUT printf
#else
#define DEBUG_OUTPUT(...) do{} while(0)
#endif
int parse(int sockfd)
{
  sleep(20);
  char greeting_text[128];
  char buf[256] = {0};
  // Redirect stdout and stdin to the socket
  if (dup2(sockfd, STDOUT_FILENO) < 0) {
    perror("dup2");
  }
  if (dup2(sockfd, STDIN_FILENO) < 0) {
    perror("dup2");
  }
  DEBUG_OUTPUT("%p %p\n", buf, greeting_text);
  printf("What is your name?\n");
  fflush(stdout);
  fgets(buf, sizeof(buf), stdin);
  strcpy(greeting_text, "Hello, dear ");
  strcat(greeting_text, buf);
  printf("%s\n", greeting_text);
  return 0;
}

int main(int argc, char** argv)
{
  int listenfd = 0,connfd = 0;
  struct sockaddr_in serv_addr;
  char sendBuff[1024];
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("listen");
    return -1;
  }
  memset(&serv_addr, 0, sizeof(serv_addr));
  memset(sendBuff, 0, sizeof(sendBuff));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));
  if (bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0) {
    perror("bind");
    return -1;
  }
  if (listen(listenfd, 10) < 0) {
    perror("listen");
    return -1;
  }
  signal(SIGCHLD, SIG_IGN);
  while(1) {
    if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) < 0) {
      perror("accept");
    }
    if (fork() == 0) {
        parse(connfd);
	close(connfd);
        return 0;
    } else {
        close(connfd);
    }
  }
  return 0;
}
