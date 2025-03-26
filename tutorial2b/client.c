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
    char str[100];
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in server;
    
    if (argc != 3) {
        printf("Args must be IP port\n");
        printf("Provided %d args\n", argc);
        return -1;
    }
    int port = atoi(argv[2]);
    char *ip = argv[1];

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    inet_pton(AF_INET,ip, &server.sin_addr);

    while(fgets(str, sizeof str, stdin) != NULL) {  // reads newline too
        str[strlen(str)] = '\0';                              // truncate the array
        
        if (sendto(sockfd, str, strlen(str), 0, (struct sockaddr *)&server, sizeof(server))<0){
            printf("Failed to send\n");
        }
    }

}
