#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_ITEMS 20000    // Max items to produce/consume
#define MESSAGE_SIZE 100

int buffer_size;  // Number of buffer slots

struct buffer_item {
    char message[MESSAGE_SIZE];
    struct sockaddr_in client_addr; // Store client address for correct responses
};

struct buffer_item buffer[100];
int in = 0, out = 0;
int produced_count = 0, consumed_count = 0;

pthread_mutex_t mutex[100];
pthread_cond_t cond[100];

struct arg_struct {
    int fd;
    int id;
};

int delay;

void *producer(void *arg) {
    struct arg_struct *args = (struct arg_struct *)arg;
    int sockfd = args->fd;
    
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    char message[MESSAGE_SIZE];

    while (produced_count < MAX_ITEMS) {

        int bytes_received = recvfrom(sockfd, message, MESSAGE_SIZE - 1, 0, 
                                      (struct sockaddr *)&client_addr, &addr_size);
        if (bytes_received > 0) {
            pthread_mutex_lock(&mutex[in]);

            message[bytes_received] = '\0';  // Null terminate received data
            strcpy(buffer[in].message, message);     // Copy to buffer
            buffer[in].client_addr = client_addr;    // Store client address
            printf("%s", message);
            //printf("Produced: %s from %s:%d\n", buffer[in].message, 
                 //  inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            
            produced_count++;
            //printf("Signaling consumer %d\n", in);
            pthread_cond_signal(&cond[in]);  // Notify consumer
            pthread_mutex_unlock(&mutex[in]);

        } else {
            perror("recvfrom error");
        }

        in = (in + 1) % buffer_size;  // Move buffer index
    }
    return NULL;
}

void *consumer(void *arg) {
    struct arg_struct *args = (struct arg_struct *)arg;
    int sockfd = args->fd;
    int index = args->id;

    char message[MESSAGE_SIZE];
    
    //printf("Consumer thread %ld started, index %d\n", pthread_self(), index);

    while (consumed_count < MAX_ITEMS) {
        pthread_mutex_lock(&mutex[index]);

        while (buffer[index].message[0] == '\0') {  // Wait for data
            //printf("Consumer %d waiting\n", index);
            pthread_cond_wait(&cond[index], &mutex[index]);
            //printf("Consumer %d woke up\n", index);
        }

        usleep(delay);
        strcpy(message, buffer[index].message);
        struct sockaddr_in client_addr = buffer[index].client_addr; // Get stored client info
        
        //printf("Consumer %d sending: %s to %s:%d\n", index, message, 
             //  inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (sendto(sockfd, message, strlen(message), 0, 
                   (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
            perror("Failed to send");
        }

        buffer[index].message[0] = '\0';  // Mark buffer slot as empty
        consumed_count++;
        pthread_mutex_unlock(&mutex[index]);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int port;
    buffer_size = 5;  // Default buffer size

    if (argc < 2) {
        fprintf(stderr, "Missing argument. Syntax: %s <PORT> [N_REQUESTS_MAX] [DELAY_MS].\n", argv[0]);
        return 1;
    }
    port = atoi(argv[1]);
    if (argc >= 3) buffer_size = atoi(argv[2]);
    if (argc >= 4) delay = atoi(argv[3]) * 1000;

    printf("Running server on port %d with %d max parallel requests and %d us as delay.\n", port, buffer_size, delay);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(sockfd);
        return 1;
    }

    pthread_t producerThread, consumerThread[buffer_size];
    struct arg_struct args[buffer_size];

    for (int i = 0; i < buffer_size; i++) {
        buffer[i].message[0] = '\0';
        if (pthread_mutex_init(&mutex[i], NULL) != 0) {
            perror("Mutex init failed");
            return 1;
        }
        if (pthread_cond_init(&cond[i], NULL) != 0) {
            perror("Cond init failed");
            return 1;
        }
    }
    sleep(0.1);

    struct arg_struct producer_args = {sockfd, 0};
    for (int i = 0; i < buffer_size; i++) {
        args[i].fd = sockfd;
        args[i].id = i;
        pthread_create(&consumerThread[i], NULL, consumer, &args[i]);
        sleep(0.1);
    }

    pthread_create(&producerThread, NULL, producer, &producer_args);

    pthread_join(producerThread, NULL);
    for (int i = 0; i < buffer_size; i++) {
        pthread_join(consumerThread[i], NULL);
    }

    for (int i = 0; i < buffer_size; i++) {
        pthread_mutex_destroy(&mutex[i]);
        pthread_cond_destroy(&cond[i]);
    }

    close(sockfd);
    return 0;
}
