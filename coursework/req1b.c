#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#define NUMBER_OF_JOBS 1000
#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2 
#define PRIORITY 3 
#define TIME_SLICE 25

int aiJobs[NUMBER_OF_JOBS][4];

int sumResponseTime = 0;
int sumTurnaroundTime = 0;

struct timeval startTime;

void generateJobs()
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS; i++)
	{
		aiJobs[i][JOB_INDEX] = i;
		aiJobs[i][BURST_TIME] = rand() % 99 + 1; 
		aiJobs[i][REMAINING_TIME] = aiJobs[i][BURST_TIME];
		aiJobs[i][PRIORITY] = rand() % 10; 
	}
}

void printJob(int iId, int iBurstTime, int iRemainingTime, int iPriority)
{
	printf("Id = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n", iId, iBurstTime, iRemainingTime, iPriority);
}

void printJobs()
{
	int i;
	printf("JOBS: \n");
	for(i = 0; i < NUMBER_OF_JOBS; i++)
		printJob(aiJobs[i][JOB_INDEX], aiJobs[i][BURST_TIME], aiJobs[i][REMAINING_TIME], aiJobs[i][PRIORITY]);
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
	struct timeval startTime, currentTime;
	gettimeofday(&startTime, NULL);
	do
	{	
		gettimeofday(&currentTime, NULL);
		iDifference = getDifferenceInMilliSeconds(startTime, currentTime);
	} while(iDifference < iTime);
}

int cmpfunc(const void * a, const void * b) 
{
	const int* one = (const int*) a;
	const int* two = (const int*) b;
	return (one[PRIORITY] - two[PRIORITY]);
}

// calculate the duration between "time" and current time
long int getDuration(struct timeval time)
{
	struct timeval currentTime;
	long int duration;
	gettimeofday(&currentTime, NULL);
	duration = getDifferenceInMilliSeconds(time, currentTime);
	return duration;
}

// if there is only one job in the current priority level, run oneJob()
void oneJob(int i)
{
	long int start = getDuration(startTime); // job start time
	simulateJob(aiJobs[i][BURST_TIME]);
	long int end = getDuration(startTime); // job end time
	printf("JOB ID = %d, Start time = %d, End time = %d, Priority = %d\n", aiJobs[i][JOB_INDEX], start, end, aiJobs[i][PRIORITY]);
	sumResponseTime += start;
	sumTurnaroundTime += end;
}

// if there are multiple jobs in the current priority level, run multipleJobs()
void multipleJobs(int samePriorityFrom, int samePriorityTo)
{
	long int start; // job start time
	long int end; // job end time
	int j;
	for (j = samePriorityFrom; j < samePriorityTo + 1; j++)
	{
		if (aiJobs[j][REMAINING_TIME] > 0)
		{
			printf("JOB ID = %d, ", aiJobs[j][JOB_INDEX]);

			start = getDuration(startTime);
			if (aiJobs[j][BURST_TIME] == aiJobs[j][REMAINING_TIME]) // the first time to run this job
			{
				printf("Start time = %d, ", start);
				sumResponseTime += start;
			}
			else
			{
				printf("re-start time = %d, ", start);
			}

			int runtime = (aiJobs[j][REMAINING_TIME] > TIME_SLICE) ? TIME_SLICE : aiJobs[j][REMAINING_TIME];
			simulateJob(runtime);

			aiJobs[j][REMAINING_TIME] -= runtime;
			if (aiJobs[j][REMAINING_TIME] > 0)
			{
				printf("Remaining time = %d, ", aiJobs[j][REMAINING_TIME]);
			}
			else
			{
				end = getDuration(startTime);
				printf("End time = %d, ", end);
				sumTurnaroundTime += end;
			}

			printf("PRIORITY = %d\n", aiJobs[j][PRIORITY]);
		}
	}
}

// check if completed all the jobs in the current priority level
int checkCompletion(int samePriorityFrom, int samePriorityTo)
{
	int allEnd = 0;
	// calculate the number of completed jobs
	int numOfEnd = 0;
	int j;
	for (j = samePriorityFrom; j < samePriorityTo + 1; j++)
	{
		if (aiJobs[j][REMAINING_TIME] == 0)
		{
			numOfEnd++;
		}
	}
	if (numOfEnd == samePriorityTo - samePriorityFrom + 1)
	{
		allEnd = 1;
	}
	return allEnd;
}

void priorityQueue()
{  
    printf("\nSorting by priority\n");
    qsort(aiJobs, NUMBER_OF_JOBS, sizeof(int[4]), cmpfunc);
    printJobs();
    
	printf("\nROUND ROBIN; time Slice: 25,\n");
	gettimeofday(&startTime, NULL);
	int i = 0;
    while(i<NUMBER_OF_JOBS)
    {
    	if(i == NUMBER_OF_JOBS -1 || aiJobs[i][PRIORITY]<aiJobs[i+1][PRIORITY]) // this is the last job, or there is only one job in the current priority level
    	{
			oneJob(i);
    	}
    	else
    	{
    		int samePriorityFrom = i; // the start of this priority section
			while (aiJobs[i][PRIORITY] == aiJobs[i + 1][PRIORITY])
			{
				i++;
				if (i == NUMBER_OF_JOBS - 1)
				{
					break;
				}
			}
    		int samePriorityTo = i; // the end of this priority section

    		while(1) 
    		{
				multipleJobs(samePriorityFrom, samePriorityTo);
				int allEnd = checkCompletion(samePriorityFrom, samePriorityTo);
				if(allEnd)
				{
					break;
				}
    	    }
    	}
    	i++;
    }
	printf("Average Response Time: %f\n", (float)sumResponseTime/NUMBER_OF_JOBS);
	printf("Average Turnaround Time: %f\n", (float)sumTurnaroundTime/NUMBER_OF_JOBS);
}

int main()
{
	generateJobs();
	printJobs();
	priorityQueue();

	return 0;
}

