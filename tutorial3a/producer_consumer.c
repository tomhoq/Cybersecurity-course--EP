#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define MAX_ITEMS 20

int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;
int produced_count = 0;
int consumed_count = 0;

pthread_mutex_t mutex[BUFFER_SIZE];
pthread_cond_t cond[BUFFER_SIZE];

void* producer(void* arg) {
   int item = 1;

   while (produced_count < MAX_ITEMS) {
      // Produce item
      pthread_mutex_lock(&mutex[in]);
      buffer[in] = item;  // Add item to the buffer
      //printf("Produced: %d at index %d\n", item, in);

      item++;
      produced_count++;
      pthread_cond_signal(&cond[in]);  // Signal the corresponding consumer
      //printf("Signaled consumer at index %d\n", in);
      pthread_mutex_unlock(&mutex[in]);  // Unlock mutex after signaling the consumer

      in = (in + 1) % BUFFER_SIZE;  // Update the producer's index

   }

   //finish
   sleep(1);
   printf("Producer thread %ld finished\n\n\n", pthread_self());
   for (int i = 0; i < BUFFER_SIZE; i++) {
      pthread_mutex_lock(&mutex[i]);
   }
   for (int i = 0; i < BUFFER_SIZE; i++) {
      buffer[i] = 1;
   }
   for (int i = 0; i < BUFFER_SIZE; i++) {

      pthread_cond_signal(&cond[i]);  // Signal all consumers to finish
   }
   for (int i = 0; i < BUFFER_SIZE; i++) {
      pthread_mutex_unlock(&mutex[i]);
   }

   return NULL;

}

void* consumer(void* arg) {
   int index = *(int*)arg;
   printf("Consumer thread %ld created with index %d\n", pthread_self(), index);

   while (consumed_count < MAX_ITEMS) {
      pthread_mutex_lock(&mutex[index]);

      // Wait until the slot is filled with a valid item
      while (buffer[index] == -1) {
         //printf("Consumer %d blocked, waiting for item\n", index);
         pthread_cond_wait(&cond[index], &mutex[index]);  // Wait for signal from producer
         //printf("Consumer %d woke up\n", index);
      }

      // Consume the item
      int item = buffer[index];
      printf("Consumer %d consumed: %d\n", index, item);
      buffer[index] = -1;  // Mark the slot as empty

      consumed_count++;
      pthread_mutex_unlock(&mutex[index]);

   }

   return NULL;

}

int main() {
   pthread_t producerThread, consumerThread[BUFFER_SIZE];
   int array[BUFFER_SIZE];

   // Initialize buffer slots to -1 (empty)
   for (int i = 0; i < BUFFER_SIZE; i++) {
      buffer[i] = -1;
      pthread_cond_init(&cond[i], NULL);
      pthread_mutex_init(&mutex[i], NULL);
   }

   // Create consumer threads
   for (int i = 0; i < BUFFER_SIZE; i++) {
      array[i] = i;
      pthread_create(&consumerThread[i], NULL, consumer, &array[i]);
      sleep(0.1);  // Adding slight delay to stagger consumer thread creation
   }

   // Create producer thread
   pthread_create(&producerThread, NULL, producer, NULL);

   // Wait for the producer and consumer threads to finish
   pthread_join(producerThread, NULL);
   for (int i = 0; i < BUFFER_SIZE; i++) {
      pthread_join(consumerThread[i], NULL);
   }

   // Clean up
   for (int i = 0; i < BUFFER_SIZE; i++) {
      pthread_cond_destroy(&cond[i]);
      pthread_mutex_destroy(&mutex[i]);
   }

   return 0;
}
