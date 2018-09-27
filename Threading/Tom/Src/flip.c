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

#include "uint128.h"
#include "flip.h"

#define NROF_BUFFERS (NROF_PIECES/128) + 1

pthread_mutex_t mutexLocks[NROF_BUFFERS];

void createMutexes()
{
	for(int i=0;i<NROF_BUFFERS;i++)
	{
		pthread_mutex_init ( &mutexLocks[i], NULL);
	}
}

void closeMutexes()
{
	for(int i=0;i<NROF_BUFFERS;i++)
	{
		pthread_mutex_destroy(&mutexLocks[i]);
	}
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
	for(int i=0;i<NROF_PIECES;i++)
	{
		int bufferIndex = i / 128;
		int indexInBuffer = i % 128;

		if((buffer[bufferIndex] >> indexInBuffer) & (uint128_t) 1)
		{
			printf("%d\n",i);
		}
	}

}

void toggleBit(int index)
{
	//printf("Toggling %d\n",index);

	int bufferIndex = index / 128;
	int indexInBuffer = index % 128;

	uint128_t bitMask = ((uint128_t) 1) << indexInBuffer;

	//check which buffer is needed and request its mutex

	pthread_mutex_lock(&mutexLocks[bufferIndex]);

	//printf("Buffer %d: 0x%llx\n",bufferIndex,buffer[bufferIndex]);
	//printf("Bitmask: 0x%llx\n", bitMask);

	buffer[bufferIndex] ^= bitMask;

	pthread_mutex_unlock(&mutexLocks[bufferIndex]);

	//printf("Result: 0x%llx\n", buffer[bufferIndex]);

}


void startThread(int numberToCheck)
{
	//printf("Started thread with number %d\n",numberToCheck);

	for(int i = numberToCheck; i<NROF_PIECES;i+=numberToCheck )
	{
		toggleBit(i);
	}
}

int main (void)
{
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
	setAllBitsOne();

	createMutexes();

	for(int i=2;i<NROF_PIECES;i++)
	{
		startThread(i);
	}

	//join threads

	closeMutexes();

	printBits();

    return (0);
}

