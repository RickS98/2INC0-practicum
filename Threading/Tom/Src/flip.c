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


pthread_mutex_t mutexLocks[NROF_BUFFERS]; //mutex lock to eliminate race conditions
sem_t threadCounter; //semaphore to lock main thread when max threads is reached

int threadStatus[NROF_THREADS] = {THREAD_UNINITIALIZED}; //array that keeps track of status of threads
pthread_t threadId[NROF_THREADS]; //allocation of threads

struct THREADCOMMAND{
	int numberToCheck;//this is neccesary to know which bits to toggle
	int threadNumber; //this is neccesary to update the status of the thread when finished
};

//This function initializes all mutexes and the semaphore
void initialize()
{
	for(int i=0;i<NROF_BUFFERS;i++)
	{
		pthread_mutex_init ( &mutexLocks[i], NULL);//initialize all mutexes
	}
	sem_init(&threadCounter, 0, NROF_THREADS); //initialize semaphore its initial numbers is the amount of threads that can be used
}

//This function destroys all mutexes and the semaphore
void destroy()
{
	for(int i=0;i<NROF_BUFFERS;i++)
	{
		pthread_mutex_destroy(&mutexLocks[i]);//cleunup mutexes
	}
	sem_destroy(&threadCounter);//cleunup semaphore
}

//This funtions sets all bits to one
void setAllBitsOne()
{
	uint128_t onlyOnes = ~0;

	for(int i=0;i<NROF_BUFFERS;i++)
	{
		buffer[i] = onlyOnes;//sets all bits to one in the entire buffer
	}
}

//This function prints the location of all found values
void printBits()
{
	//print all numbers that end up true, one number per line

	for(int k = 1, i=0;i<NROF_BUFFERS;i++)
	{
		uint128_t temp = buffer[i];

		for(int j=0;j<128 && k<=NROF_PIECES; j++,k++)
		{
			temp = temp >> 1;

			if(temp & (uint128_t) 1)
			{
				printf("%d\n",k);
			}
		}
	}

}

//This is the thread that will be run to toggle a certain multiple of bits
void *thread(void *arg)
{
	struct THREADCOMMAND *command = (struct THREADCOMMAND*)arg; //cast arg to correct struct

	uint128_t bitMask[NROF_BUFFERS] = {(uint128_t) 0}; //create array and sets all values to 0 to use for the masks later

	//goes to all values and updates the masks untill the masks contains each value that have to be toggled
	for(int i=command->numberToCheck;i<=NROF_PIECES;i+=command->numberToCheck)
	{
		int maskIndex = i / 128;
		int indexInMask = i % 128;

		bitMask[maskIndex] |= (((uint128_t) 1) << indexInMask);
	}

	//Now loop through the buffers to apply the masks
	for(int i = 0;i<NROF_BUFFERS;i++)
	{

		//check which buffer is needed and request its mutex
		int returnMutexLock = pthread_mutex_lock(&mutexLocks[i]);
		if(returnMutexLock<0)
		{
			perror("Mutex lock failed");
			exit(1);
		}

		buffer[i] ^= bitMask[i]; //toggle bits of mask

		//release mutex so other thread can access it again
		int returnMutexUnlock = pthread_mutex_unlock(&mutexLocks[i]);
		if(returnMutexUnlock<0)
		{
			perror("Mutex unlock failed");
			exit(1);
		}

	}

	threadStatus[command->threadNumber] = THREAD_FINISHED; //set status of current thread to finished

	free(arg); //clear allocated memory that was made for this thread

	int returnSemPost = sem_post(&threadCounter); //increase semaphore so that main thread can clean up current thread.

	if(returnSemPost<0)
	{
		perror("Semaphore post failed");
		exit(1);
	}

	pthread_exit(NULL); //kill thread

	//return NULL;
}

//function that is responsible for allocating memory of thread and starting it
void createThread(int currentNumber,int selectedThread)
{
	struct THREADCOMMAND *newCommand; //create pointer to struct
	newCommand = malloc(sizeof (struct THREADCOMMAND)); //allocate its memory
	newCommand->numberToCheck = currentNumber; //and fill it with the needed values
	newCommand->threadNumber=selectedThread;

	threadStatus[selectedThread] = THREAD_BUSY; //sets state of thread to busy so it will be left alone

	pthread_create (&threadId[selectedThread], NULL, thread, newCommand); //create the thread
}

//function that is responsible for going through all numbers and creating a tag for it
void runThreads()
{
	//loop that goes through all numbers that have to be done
	for(int currentNumber = 2;currentNumber<=NROF_PIECES;)
	{
		//wait for semaphore as a non busy wait for a thread to become finished.
		//just goes though it when uninitialized threads are left.
		int returnSemWait = sem_wait(&threadCounter);

		if(returnSemWait<0)
		{
			perror("Semaphore post failed");
			exit(1);
		}
		//Goes through all threads to see which one can be used for current number
		for(int selectedThread = 0 ;selectedThread<NROF_THREADS&&currentNumber<=NROF_PIECES;selectedThread++)
		{
			//if thread is not busy it can be used
			if(threadStatus[selectedThread]!=THREAD_BUSY)
			{
				//if thread is finished it is nessesary to first join it before creating a new one.
				if(threadStatus[selectedThread]==THREAD_FINISHED)
				{
					pthread_join(threadId[selectedThread], NULL);
				}

				createThread(currentNumber, selectedThread);
				//increate currentNumber so next cycle the next number will be inserted in a thread
				currentNumber++;
			}
		}
	}
	//wait for all thread to finish
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

