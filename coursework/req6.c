#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 100
#define NUMBER_OF_CONSUMERS 4

#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2 
#define PRIORITY 3 
#define TIME_SLICE 25

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

void bubbleSort()
{
	int i, j;
	int temp[4];
	for(i = BUFFER_SIZE - 1; i > 0; i--)
	{
		for(j = 0; j < i; j++)
		{
			if(buffer[j][PRIORITY] > buffer[j + 1][PRIORITY])
			{
				memcpy(&temp, &buffer[j], sizeof(int[4]));
				memcpy(&buffer[j], &buffer[j + 1], sizeof(int[4]));
				memcpy(&buffer[j + 1], &temp, sizeof(int[4]));
			}
		}
	}
}

void addItem(int id)
{
	// generate and add to the end of the buffer
	buffer[iIndex][JOB_INDEX] = id;
	buffer[iIndex][BURST_TIME] = rand() % 99 + 1;
	buffer[iIndex][REMAINING_TIME] = buffer[iIndex][BURST_TIME];
	buffer[iIndex][PRIORITY] = rand() % 10;
	printf("JOB ID = %d, Burst Time = %d, Remaining Time = %d, Priority = %d ", 
		buffer[iIndex][JOB_INDEX], buffer[iIndex][BURST_TIME], buffer[iIndex][REMAINING_TIME], buffer[iIndex][PRIORITY]);
	iIndex++;
}

int removeItem()
{
	bubbleSort(); // bubbleSort is stable

	/*
	printf("\n\n");
	int hh;
	for(hh = 0; hh < BUFFER_SIZE; hh++)
	{
		printf("??%d, %d, %d, %d??\n", buffer[hh][0], buffer[hh][1], buffer[hh][2], buffer[hh][3]);
	}
	printf("\n\n");
	*/

	// get run time
	int runTime;
	if(iIndex != 1 && buffer[0][PRIORITY] == buffer[1][PRIORITY])
	{
		runTime = (buffer[0][REMAINING_TIME] > TIME_SLICE) ? TIME_SLICE : buffer[0][REMAINING_TIME];
	}
	else
	{
		runTime = buffer[0][REMAINING_TIME];
	}

	buffer[0][REMAINING_TIME] -= runTime;

	memcpy(&currentJob, &buffer[0], sizeof(int[4]));

	iIndex--;
	int j;
	for(j = 0; j < iIndex; j++)
	{
		memcpy(&buffer[j], &buffer[j + 1], sizeof(int[4])); // shift the remaining elements forward
	}
	
	if(currentJob[REMAINING_TIME] != 0) // not fully completed
	{
		memcpy(&buffer[iIndex], &currentJob, sizeof(int[4])); // add to the end again
		iIndex++;
	}

	return runTime;
}

void * producer(void *p)
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		sem_wait(&sSync);
		printf("Producing: ");
		addItem(i); // addItem(i) adds jobs to the end of the buffer
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
		int temp = iIndex;
		int runTime = removeItem(); // removeItem() sorts the buffer, removes job, shifts the remaining elements forward and adds unfinished jobs back

		if(temp != iIndex) 
		{
			number_of_removed++; // iIndex changes -> the job is completed -> the number of jobs removed should be increased
		}
		else
		{
			sem_post(&sFull); // iIndex doesn't change -> the job isn't completed and was added back to the buffer -> the value of sFull should be increased
		}

		int temp_num_of_removed = number_of_removed; // use temp_num_of_removed to make the critical section of "sSyncNumOfRemoved" smaller
		sem_post(&sSyncNumOfRemoved);

		if(temp != iIndex && iIndex == BUFFER_SIZE - 1) // if iIndex changes from BUFFER_SIZE to BUFFER_SIZE - 1
		{
			sem_post(&sDelayProducer);
		}

		// calculate the job start time
		struct timeval currentTime;
		gettimeofday(&currentTime, NULL);
		int start = getDifferenceInMilliSeconds(startTime, currentTime);
		sumOfStartTime += start;

		printf("Thread %d consuming: ", threadIndex);
		printf("JOB ID = %d, Start Time = %d, Remaining time = %d, Priority = %d ", currentJob[JOB_INDEX], start, currentJob[REMAINING_TIME], currentJob[PRIORITY]);
		printf("(jobs removed = %d, jobs in buffer = %d)\n", temp_num_of_removed, iIndex);

		sem_post(&sSync);
		simulateJob(runTime); // simulateJob is not in the critical section
	}
}

int main()
{
	// srand(time(NULL)); // randomize seed

	pthread_t tid_producer;
	pthread_t tid_consumer[NUMBER_OF_CONSUMERS];

	sem_init(&sSync, 0, 1); // initialised to 1
	sem_init(&sFull, 0, 0); // initialised to 0
	sem_init(&sDelayProducer, 0, 0); // initialised to 0
	sem_init(&sSyncNumOfRemoved, 0, 1); // initialised to 1

	int i;
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		buffer[i][PRIORITY] = 10; // initialize the priority to 10, which is invalid
	}

	// create threads
	if(pthread_create(&tid_producer, NULL, producer, NULL) == -1)
	{
		printf("unable to create the producer\n");
		exit(0);
	}
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

