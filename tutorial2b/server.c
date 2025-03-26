#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>   


int main(int argc, char *argv[]){
    char buffer[100];
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in server;
    socklen_t server_len = sizeof(server);

    if (argc != 2) {
        printf("Args must be port\n");
        printf("Provided %d args\n", argc);
        return -1;
    }
    int port = atoi(argv[1]);
    
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server))<0) {
        printf("failed to bind\n");
        return 1;
    }

    while(recvfrom(sockfd, buffer, 99, 0,(struct sockaddr *)&server, &server_len)>0) {  // reads newline too
       
        buffer[strlen(buffer)] = '\0';                              // truncate the array
        printf("%s\n", buffer);        
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server, sizeof(server))<0 ){
            printf("Failed to send\n");
        }
        memset(buffer, 0, sizeof(buffer));
    }

}
