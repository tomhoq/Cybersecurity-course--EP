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
    char str[100], buffer[100];
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in server;
    socklen_t len =sizeof(server);
    int t;
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
        memset(buffer,0,sizeof(buffer)); 
        if (sendto(sockfd, str, strlen(str), 0, (struct sockaddr *)&server, sizeof(server))<0 ){
            printf("Failed to send\n");
        }
        if ((t=recvfrom(sockfd, buffer, 99, 0,(struct sockaddr *)&server, &len))<0 ) {
            printf("Error receiving\n");   
        }
        buffer[t] = '\0';
        
        printf("%s\n", buffer);   
    }

}
