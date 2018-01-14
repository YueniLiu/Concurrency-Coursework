#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 100

sem_t sSync; // synchronises access to the counter
sem_t sDelayConsumer; // the consumer goes to sleep when there are no items available
sem_t sDelayProducer; // the producer goes to sleep when there no empty spaces available

int iIndex = 0;

char buffer[BUFFER_SIZE] = {0}; // initialize all elements to 0

void visualisation()
{
    printf("iIndex = %d\t\t", iIndex);
	int i;
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		if(buffer[i] == 0)
		{
			break;
		}
		printf("%c", buffer[i]);
	}
	printf("\n");
}

void printSemaphore()
{
	int syncValue, delayConsumerValue, delayProducerValue;
	sem_getvalue(&sSync, &syncValue);
	sem_getvalue(&sDelayConsumer, &delayConsumerValue);
	sem_getvalue(&sDelayProducer, &delayProducerValue);
	printf("sSync = %d, sDelayConsumer = %d, sDelayProducerValue = %d\n", syncValue, delayConsumerValue, delayProducerValue);
}

void addItem()
{
	buffer[iIndex] = '*'; // add to the end of the buffer
	iIndex++;
}

void removeItem()
{
	iIndex--;
	int j;
	for (j = 0; j < iIndex; j++)
	{
		buffer[j] = buffer[j + 1]; // shift the remaining elements forward
	}
	buffer[iIndex] = 0;
}

void * producer(void *p)
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		sem_wait(&sSync);
		addItem();
		int temp = iIndex;
		visualisation();
		if(iIndex == 1)
		{
			sem_post(&sDelayConsumer);
		}
		sem_post(&sSync);
		if(temp == BUFFER_SIZE) 
		{
			sem_wait(&sDelayProducer);
		}
	}
}

void * consumer(void *p)
{
	sem_wait(&sDelayConsumer);
    int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		sem_wait(&sSync);
		removeItem();
		int temp = iIndex;
		visualisation();
		if(iIndex == BUFFER_SIZE - 1)
		{
			sem_post(&sDelayProducer);
		}
		sem_post(&sSync);
		if(temp == 0 && i != NUMBER_OF_JOBS-1) // if i == NUM_OF_JOBS-1 -> already removed NUM_OF_JOBS items -> not go to sleep
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
	sem_init(&sDelayProducer, 0, 0); // initialised to 0

	// create threads
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

	// join with the main thread
	pthread_join(tid_producer, NULL);
	pthread_join(tid_consumer, NULL);

	printSemaphore();

	return 0;
}

