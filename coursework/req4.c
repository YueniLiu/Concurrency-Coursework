#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 100
#define NUMBER_OF_CONSUMERS 4

int iIndex = 0; // the number of jobs in the buffer
char buffer[BUFFER_SIZE] = { 0 }; // initialize all elements to 0
int number_of_removed = 0; // the number of jobs removed by consumers

sem_t sSync; // synchronises access to the buffer
sem_t sFull; // counting semaphore, the number of full buffers
sem_t sDelayProducer; // the producer goes to sleep when there no empty spaces available
sem_t sSyncNumOfRemoved; // synchronises access to "number_of_removed"

void visualisation()
{
	printf("iTID = %u, ", pthread_self()); 
    printf("iIndex = %d\t", iIndex);
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

void printSemaphores()
{
	int syncValue, fullValue, delayProducerValue, syncNumOfRemovedValue;
	sem_getvalue(&sSync, &syncValue);
	sem_getvalue(&sFull, &fullValue);
	sem_getvalue(&sDelayProducer, &delayProducerValue);
	sem_getvalue(&sSyncNumOfRemoved, &syncNumOfRemovedValue);
	printf("sSync = %d, sFull = %d, sDelayProducerValue = %d, sSyncNumOfRemoved = %d\n", syncValue, fullValue, delayProducerValue, syncNumOfRemovedValue);
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
		sem_post(&sFull);
		sem_post(&sSync);
		if(temp == BUFFER_SIZE) 
		{
			sem_wait(&sDelayProducer);
		}
	}
}

void * consumer(void *p)
{
	while(1) 
	{
		sem_wait(&sSyncNumOfRemoved);
		// check if have already removed all jobs
		if(number_of_removed == NUMBER_OF_JOBS)
		{
			sem_post(&sSyncNumOfRemoved);
			break;
		}

		sem_wait(&sFull);
		sem_wait(&sSync);
		number_of_removed++;
		sem_post(&sSyncNumOfRemoved);

		removeItem();
		visualisation();
		if(iIndex == BUFFER_SIZE - 1)
		{
			sem_post(&sDelayProducer);
		}
		sem_post(&sSync);
	}
}

int main()
{
	pthread_t tid_producer;
	pthread_t tid_consumer[NUMBER_OF_CONSUMERS];

	sem_init(&sSync, 0, 1); // initialised to 1
	sem_init(&sFull, 0, 0); // initialised to 0
	sem_init(&sDelayProducer, 0, 0); // initialised to 0
	sem_init(&sSyncNumOfRemoved, 0, 1); // initialised to 1

	// create threads
	if(pthread_create(&tid_producer, NULL, producer, NULL) == -1)
	{
		printf("unable to create the producer\n");
		exit(0);
	}
	int i;
	for(i = 0; i < NUMBER_OF_CONSUMERS; i++)
	{
		if(pthread_create(&tid_consumer[i], NULL, consumer, NULL) == -1)
		{
			printf("unable to create consumer %d\n", i);
			exit(0);
		}
	}

	// join with the main thread
	pthread_join(tid_producer, NULL);
    for(i = 0; i < NUMBER_OF_CONSUMERS; i++)
	{
		pthread_join(tid_consumer[i], NULL);
	}

	printSemaphores();

	return 0;
}

