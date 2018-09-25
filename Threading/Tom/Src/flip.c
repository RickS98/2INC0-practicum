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

void createMutexes()
{

}

void closeMutexes()
{

}


void startThread(int numberToCheck)
{
	for(int i = numberToCheck; i<NROF_PIECES;i+=NumberToCheck )
	{
		toggleBit(i);
	}
}

void toggleBit(int index)
{
	int bufferIndex = index / 128;
	int indexInBuffer = index % 128;

	//check which buffer is needed and request its mutex
}


int main (void)
{
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
	createMutexes();

	for(int i=0;i<NROF_PIECES;i++)
	{
		startThread(i);
	}

	//join threads

	closeMutexes();

    return (0);
}

