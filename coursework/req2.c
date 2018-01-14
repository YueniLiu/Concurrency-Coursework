#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define NUMBER_OF_JOBS 1000

sem_t sSync; // synchronises access to the counter
sem_t sDelayConsumer; // the consumer goes to sleep when there are no items available

int iIndex = 0;

void visualisation()
{
    printf("iIndex = %d\n", iIndex);
}

void * producer(void *p)
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		sem_wait(&sSync);
		iIndex++;
		visualisation();
		if(iIndex == 1)
		{
			sem_post(&sDelayConsumer);
		}
		sem_post(&sSync);
	}
}

void * consumer(void *p)
{
	sem_wait(&sDelayConsumer);
    int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		sem_wait(&sSync);
		iIndex--;
		int temp = iIndex;
		visualisation();
		sem_post(&sSync);
		if (temp == 0 && i != NUMBER_OF_JOBS-1) // if i == NUM_OF_JOBS-1 -> already removed NUMBER_OF_JOBS items -> not go to sleep
		{
			sem_wait(&sDelayConsumer);
		}
	}
}

int main()
{
	pthread_t tid_producer, tid_consumer;

	sem_init(&sSync, 0, 1); // initialised to 1
	sem_init(&sDelayConsumer, 0, 0); // initialised to 0

	if(pthread_create(&tid_producer, NULL, producer, NULL) == -1)
	{
		printf("unable to create the producer");
		exit(0);
	}
	if(pthread_create(&tid_consumer, NULL, consumer, NULL) == -1)
	{
		printf("unable to create the consumer");
		exit(0);
	} 

	pthread_join(tid_producer, NULL);
	pthread_join(tid_consumer, NULL);

    int syncValue, delayConsumerValue;
	sem_getvalue(&sSync, &syncValue);
	sem_getvalue(&sDelayConsumer, &delayConsumerValue);
	printf("sSync = %d, sDelayConsumer = %d\n", syncValue, delayConsumerValue);

	return 0;
}

