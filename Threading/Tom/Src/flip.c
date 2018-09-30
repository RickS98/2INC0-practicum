/* 
 * Operating Systems  [2INCO]  Practical Assignment
 * Threaded Application
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>          // for perror()
#include <pthread.h>
#include <semaphore.h>

#include "uint128.h"
#include "flip.h"

#define NROF_BUFFERS (NROF_PIECES/128) + 1
#define THREAD_UNINITIALIZED 0
#define THREAD_BUSY 1
#define THREAD_FINISHED 2


pthread_mutex_t mutexLocks[NROF_BUFFERS];
sem_t threadCounter;

int threadStatus[NROF_THREADS] = {THREAD_UNINITIALIZED};
pthread_t threadId[NROF_THREADS];

struct THREADCOMMAND{
	int numberToCheck;
	int threadNumber;
};

void initialize()
{
	for(int i=0;i<NROF_BUFFERS;i++)
	{
		pthread_mutex_init ( &mutexLocks[i], NULL);
	}
	sem_init(&threadCounter, 0, NROF_THREADS);
}

void destroy()
{
	for(int i=0;i<NROF_BUFFERS;i++)
	{
		pthread_mutex_destroy(&mutexLocks[i]);
	}
	sem_destroy(&threadCounter);
}

void setAllBitsOne()
{
	uint128_t onlyOnes = ~0;

	for(int i=0;i<NROF_BUFFERS;i++)
	{
		buffer[i] = onlyOnes;
	}
}

void printBits()
{
	//print all numbers that end up true, one number per line
	int k = 1;

	for(int i=0;i<NROF_BUFFERS;i++)
	{
		uint128_t temp = buffer[i];

		for(int j=0;j<128 && k<=NROF_PIECES; j++)
		{
			temp = temp >> 1;


			if(temp & (uint128_t) 1)
			{
				printf("%d\n",k);
			}

			k++;

		}
	}

}


void *thread(void *arg)
{
	//printf("Started thread with number %d\n",numberToCheck);

	struct THREADCOMMAND command = *(struct THREADCOMMAND*)arg;
	free(arg);

	uint128_t bitMask[NROF_BUFFERS];

	for(int i=command.numberToCheck;i<NROF_PIECES;i+=command.numberToCheck)
	{
		int maskIndex = i / 128;
		int indexInMask = i % 128;

		bitMask[maskIndex] |= (((uint128_t) 1) << indexInMask);
	}


	for(int i = 0;i<NROF_BUFFERS;i++)
	{

		//check which buffer is needed and request its mutex

		int returnMutexLock = pthread_mutex_lock(&mutexLocks[i]);
		if(returnMutexLock<0)
		{
			perror("Mutex lock failed");
		}

		//printf("Buffer %d: 0x%llx\n",i,buffer[i]);
		//printf("BufferNumber: %d\nNumber: %d\nBitmask: 0x%016lx%016lx\n",i,numberToCheck,HI(bitMask), LO(bitMask));

		buffer[i] ^= bitMask[i];

		int returnMutexUnlock = pthread_mutex_unlock(&mutexLocks[i]);
		if(returnMutexUnlock<0)
		{
			perror("Mutex unlock failed");
		}

		//printf("Result: 0x%llx\n", buffer[bufferIndex])

	}

	threadStatus[command.threadNumber] = THREAD_FINISHED;

	int returnSemPost = sem_post(&threadCounter);

	if(returnSemPost<0)
	{
		perror("Semaphore post failed");
		exit(1);
	}

	pthread_exit(NULL);

	//return NULL;
}

void createThread(int currentNumber,int selectedThread)
{
	struct THREADCOMMAND *newCommand;
	newCommand = malloc(sizeof (struct THREADCOMMAND));
	newCommand->numberToCheck = currentNumber;
	newCommand->threadNumber=selectedThread;

	threadStatus[selectedThread] = THREAD_BUSY;

	pthread_create (&threadId[selectedThread], NULL, thread, newCommand);
}


void runThreads()
{
	for(int currentNumber = 2;currentNumber<=NROF_PIECES;)
	{
		int returnSemWait = sem_wait(&threadCounter);

		if(returnSemWait<0)
		{
			perror("Semaphore post failed");
			exit(1);
		}

		for(int selectedThread = 0 ;selectedThread<NROF_THREADS&&currentNumber<=NROF_PIECES;selectedThread++)
		{
			if(threadStatus[selectedThread]!=THREAD_BUSY)
			{

				if(threadStatus[selectedThread]==THREAD_FINISHED)
				{
					pthread_join(threadId[selectedThread], NULL);
				}

				createThread(currentNumber, selectedThread);

				currentNumber++;
			}
		}
	}

	for(int i = 0; i<NROF_THREADS;i++)
	{
		pthread_join(threadId[i], NULL);
	}
}

int main (void)
{
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
	setAllBitsOne();

	initialize();

	runThreads();

	destroy();

	printBits();

    return (0);
}

