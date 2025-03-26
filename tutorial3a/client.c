#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>   
#include <pthread.h>


struct arg_struct {
    int fd;
    struct sockaddr_in *server_addr;
};

void *listen_udp(void *arg) {
    struct arg_struct *stru= (struct arg_struct *)arg;
    int sockfd = stru->fd;
    struct sockaddr_in *server = stru->server_addr;
    socklen_t size = sizeof(server);
    char buffer[100];
    int t;
    while(1) {
        //printf("Listening\n");
        if ((t=recvfrom(sockfd, buffer, 99, 0,(struct sockaddr *)server, &size))<0 ) {
            perror("Error receiving\n");   
        }
        buffer[strlen(buffer)] = '\0';   //replace \n with \0

        buffer[t] = '\0';
        printf("%s", buffer);   
    }
    return NULL;
}

void *read_and_send(void *args) {
    struct arg_struct *stru= (struct arg_struct *)args;
    int sockfd = stru->fd;
    struct sockaddr_in *server = stru->server_addr;
    //printf("Socket %d\n", sockfd);
    //printf("Server IP: %s, Port: %d\n", inet_ntoa(server->sin_addr), ntohs(server->sin_port));
    //printf("%p\n", args);

    char str[100], buffer[100];
    while(fgets(str, sizeof str, stdin) != NULL) {  // reads newline too
        str[strlen(str)] = '\0';                              // truncate the array
        memset(buffer,0,sizeof(buffer)); 
        if ((sendto(sockfd, str, strlen(str), 0, (struct sockaddr *)server, sizeof(*server)))<0 ){
            perror("Failed to send\n");
        }
    }
    return NULL;
}

int main(int argc, char *argv[]){
    pthread_t thread_listen;
    pthread_t thread_read_and_send;


    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in server;

    struct arg_struct args;
    args.fd = sockfd;
    args.server_addr = &server;

    if (argc < 3) {
        fprintf(stderr, "Missing argument. Syntax: %s <SERVER_ADDRESS> <PORT>\n", argv[0]);
	    return 1;
    }
    
    int port = atoi(argv[2]);
    char *ip = argv[1];
    printf("Running client for server at address %s and port %d\n", ip, port);


    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    inet_pton(AF_INET,ip, &server.sin_addr);

    //printf("Socket %d\n", sockfd);
    //printf("Server IP: %s, Port: %d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    //printf("%p\n", &args);
    if (pthread_create(&thread_listen, NULL, listen_udp, &args)) {
        perror("Error creating thread\n");
        return 1;
    }

    if (pthread_create(&thread_read_and_send, NULL, read_and_send, &args)) {
        perror("Error creating thread\n");
        return 1;
    }

    pthread_join(thread_listen, NULL);
    pthread_join(thread_read_and_send, NULL);
    close(sockfd);

    return 0;
}
