#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 100
#define NUMBER_OF_CONSUMERS 4

#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2 
#define PRIORITY 3 

int iIndex = 0; // the number of jobs in the buffer
int buffer[BUFFER_SIZE][4];
int number_of_removed = 0; // the number of jobs removed by consumers

sem_t sSync; // synchronises access to the buffer
sem_t sFull; // counting semaphore, the number of full buffers
sem_t sDelayProducer; // the producer goes to sleep when there no empty spaces available
sem_t sSyncNumOfRemoved; // synchronises access to "number_of_removed"

int sumOfStartTime;

int currentJob[4];

void printSemaphores()
{
	int syncValue, fullValue, delayProducerValue, syncNumOfRemovedValue;
	sem_getvalue(&sSync, &syncValue);
	sem_getvalue(&sFull, &fullValue);
	sem_getvalue(&sDelayProducer, &delayProducerValue);
	sem_getvalue(&sSyncNumOfRemoved, &syncNumOfRemovedValue);
	printf("sSync = %d, sFull = %d, sDelayProducerValue = %d, sSyncNumOfRemoved = %d\n", syncValue, fullValue, delayProducerValue, syncNumOfRemovedValue);
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end)
{
	int iSeconds = end.tv_sec - start.tv_sec;
	int iUSeconds = end.tv_usec - start.tv_usec;
	long int mtime = (iSeconds * 1000 + iUSeconds / 1000.0);
	return mtime;
}

void simulateJob(int iTime)
{
	long int iDifference = 0;
	struct timeval starttime, currenttime;
	gettimeofday(&starttime, NULL);
	do
	{
		gettimeofday(&currenttime, NULL);
		iDifference = getDifferenceInMilliSeconds(starttime, currenttime);
	} while(iDifference < iTime);
}

void addItem(int id)
{
	// generate and add to the end of the buffer
	buffer[iIndex][JOB_INDEX] = id;
	buffer[iIndex][BURST_TIME] = rand() % 99 + 1;
	buffer[iIndex][REMAINING_TIME] = buffer[iIndex][BURST_TIME];
	buffer[iIndex][PRIORITY] = rand() % 10;
	printf("JOB ID = %d, Burst Time = %d, Remaining Time = %d ", buffer[iIndex][JOB_INDEX], buffer[iIndex][BURST_TIME], buffer[iIndex][REMAINING_TIME]);
	iIndex++;
}

void removeItem()
{
	iIndex--;
	memcpy(&currentJob, &buffer[0], sizeof(int[4]));

	int j;
	for(j = 0; j < iIndex; j++)
	{
		memcpy(&buffer[j], &buffer[j + 1], sizeof(int[4])); // shift the remaining elements forward
	}
}

void * producer(void *p)
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		sem_wait(&sSync);
		printf("Producing: ");
		addItem(i);
		int temp = iIndex;
		printf("(jobs produced = %d, jobs in buffer = %d)\n", i + 1, iIndex);
		sem_post(&sSync);
		sem_post(&sFull);
		if(temp == BUFFER_SIZE) 
		{
			sem_wait(&sDelayProducer);
		}
	}
}

void * consumer(void *id)
{
	int threadIndex = *((int*)id);
	struct timeval startTime;
	gettimeofday(&startTime, NULL);

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
		int temp_num_of_removed = number_of_removed; // use temp_num_of_removed to make the critical section of "sSyncNumOfRemoved" smaller
		sem_post(&sSyncNumOfRemoved);

		removeItem();
		int runTime = currentJob[BURST_TIME];

		if (iIndex == BUFFER_SIZE - 1)
		{
			sem_post(&sDelayProducer);
		}
		
		// calculate the job start time
		struct timeval currentTime;
		gettimeofday(&currentTime, NULL);
		int start = getDifferenceInMilliSeconds(startTime, currentTime);
		sumOfStartTime += start;

		printf("Thread %d removes: ", threadIndex);
		printf("JOB ID = %d, Burst Time = %d, Start Time = %d ", currentJob[JOB_INDEX], currentJob[BURST_TIME], start);
		printf("(jobs removed = %d, jobs in buffer = %d)\n", temp_num_of_removed, iIndex);

		sem_post(&sSync);
		simulateJob(runTime); // simulateJob is not in the critical section
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
	int id[NUMBER_OF_CONSUMERS];
	for(i = 0; i < NUMBER_OF_CONSUMERS; i++)
	{
		id[i] = i + 1;
		if(pthread_create(&tid_consumer[i], NULL, consumer, (void *)(id + i)) == -1)
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

	printf("Average start time = %d\n", sumOfStartTime/NUMBER_OF_JOBS);

	printSemaphores();

	return 0;
}

