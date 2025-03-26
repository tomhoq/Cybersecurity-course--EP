/*
 * mutex.c
 *
 *  Created on: Mar 19, 2016
 *      Author: jiaziyi
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define NTHREADS 50
void *increase_counter(void *);

pthread_mutex_t lock;
int  counter = 0;

int main()
{
    if (pthread_mutex_init(&lock, NULL) != 0){
        printf("\n mutex init failed\n");
        return 1;
    }
    pthread_t thread_a[50];
	//create 50 threads of increase_counter, each thread adding 1 to the counter
    for (int i = 0; i<50; i++) {
        if(pthread_create(&thread_a[i], NULL, increase_counter, NULL)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }
    for (int i = 0; i < 50; i++) {
        if (pthread_join(thread_a[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");    
        }  // Wait for each thread to finish
    }
	printf("\nFinal counter value: %d\n", counter);
    if (pthread_mutex_destroy(&lock) != 0) {                                     
        perror("pthread_mutex_destroy() error");                                    
        return 1;                                                                    
    }   
}

void *increase_counter(void *arg)
{
    pthread_mutex_lock(&lock);
	printf("Thread number %ld, working on counter. The current value is %d\n", (long)pthread_self(), counter);
	int i = counter;
	usleep (10000); //simulate the data processing
	counter = i+1;
    pthread_mutex_unlock(&lock);
    return NULL;
}
