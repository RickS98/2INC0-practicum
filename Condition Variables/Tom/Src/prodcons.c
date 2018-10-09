/* 
 * Operating Systems  [2INCO]  Practical Assignment
 * Condition Variables Application
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * "Extra" steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];

static void rsleep (int t);			// already implemented (see below)
static ITEM get_next_item (void);	// already implemented (see below)

int upperBoundUsed = 0;
int lowerBoundUsed = 0;
sem_t upperBoundSemaphore;
sem_t lowerBoundSemaphore;

pthread_mutex_t mutex;
pthread_cond_t orderCondition;

ITEM nextItemBuffer = 0;

int error;

void initialize()
{
	error = sem_init(&upperBoundSemaphore, 0, BUFFER_SIZE);
	if(error<0)
	{
		perror("upperBoundSemaphore init failed");
		exit(1);
	}

	error = sem_init(&lowerBoundSemaphore, 0, 0);
	if(error<0)
	{
		perror("lowerBoundSemaphore init failed");
		exit(1);
	}

	error = pthread_cond_init(&orderCondition,NULL);
	if(error<0)
	{
		perror("orderCondition init failed");
		exit(1);
	}
	error = pthread_mutex_init ( &mutex, NULL);
	if(error<0)
	{
		perror("Mutex init failed");
		exit(1);
	}

}

void destroy()
{
	error = sem_destroy(&upperBoundSemaphore);
	if(error<0)
	{
		perror("upperBoundSemaphore destroy failed");
		exit(1);
	}

	error = sem_destroy(&lowerBoundSemaphore);
	if(error<0)
	{
		perror("lowerBoundSemaphore destroy failed");
		exit(1);
	}

	error = pthread_cond_destroy(&orderCondition);
	if(error<0)
	{
		perror("orderCondition destroy failed");
		exit(1);
	}
	error = pthread_mutex_destroy(&mutex);
	if(error<0)
	{
		perror("Mutex destroy failed");
		exit(1);
	}
}

void pushToBuffer(ITEM number)
{
	error = sem_wait(&upperBoundSemaphore);
	if(error<0)
	{
		perror("Semaphore wait failed");
		exit(1);
	}

	buffer[upperBoundUsed] = number;

	upperBoundUsed = (upperBoundUsed+1) % BUFFER_SIZE;

	error = sem_post(&lowerBoundSemaphore);
	if(error<0)
	{
		perror("Semaphore post failed");
		exit(1);
	}
}

ITEM popFromBuffer()
{
	error = sem_wait(&lowerBoundSemaphore);
	if(error<0)
	{
		perror("Semaphore wait failed");
		exit(1);
	}

	ITEM temp = buffer[lowerBoundUsed];

	lowerBoundUsed = (lowerBoundUsed+1) % BUFFER_SIZE;

	error = sem_post(&upperBoundSemaphore);
	if(error<0)
	{
		perror("Semaphore post failed");
		exit(1);
	}

	return temp;
}

/* producer thread */
static void * 
producer (void * arg)
{
    while (nextItemBuffer<NROF_ITEMS)
    {
        // TODO: 
        // * get the new item
    	ITEM currentItem = get_next_item();
		
        rsleep (100);	// simulating all kind of activities...
		
		// TODO:
		// * put the item into buffer[]
		//
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
        pthread_mutex_lock(&mutex);
        //      while not condition-for-this-producer
        while (nextItemBuffer != currentItem)
        {
        //          wait-cv;
            pthread_cond_wait(&orderCondition, &mutex);
        }
        //      critical-section;

        pushToBuffer(currentItem);

        nextItemBuffer++;

        //      possible-cv-signals;
        pthread_cond_broadcast(&orderCondition);
        //      mutex-unlock;
        pthread_mutex_unlock(&mutex);



        //
        // (see condition_test() in condition_basics.c how to use condition variables)
    }
	return (NULL);
}

/* consumer thread */
static void * 
consumer (void * arg)
{
	int foundValue = 0;

	pthread_cond_broadcast(&orderCondition);

    while (foundValue<NROF_ITEMS)
    {
    	foundValue = popFromBuffer();

    	printf("%d\n",foundValue);

        // TODO: 
		// * get the next item from buffer[]
		// * print the number to stdout
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
        //      while not condition-for-this-consumer
        //          wait-cv;
        //      critical-section;
        //      possible-cv-signals;
        //      mutex-unlock;
		
        rsleep (100);		// simulating all kind of activities...
    }
	return (NULL);
}

int main (void)
{
	pthread_t threadId[NROF_PRODUCERS+1];

	initialize();

    // TODO: 
    // * startup the producer threads and the consumer thread
	for(int i = 1;i<NROF_PRODUCERS+1;i++)
	{
		error = pthread_create (&threadId[i], NULL, producer, NULL); //create the thread
		if(error<0)
		{
			perror("Creating thread failed");
			exit(1);
		}
	}

	error = pthread_create (&threadId[0], NULL, consumer, NULL); //create the thread
	if(error<0)
	{
		perror("Creating thread failed");
		exit(1);
	}

    // * wait until all threads are finished  

	for(int i = 0;i<NROF_PRODUCERS+1;i++)
	{
		error = pthread_join(threadId[i], NULL);
		if(error<0)
		{
			perror("Joining thread failed during run");
			exit(1);
		}
	}

	destroy();
    
    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void 
rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time(NULL));
        first_call = false;
    }
    usleep (random () % t);
}


/* 
 * get_next_item()
 *
 * description:
 *		thread-safe function to get a next job to be executed
 *		subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1 
 *		in arbitrary order 
 *		return value NROF_ITEMS indicates that all jobs have already been given
 * 
 * parameters:
 *		none
 *
 * return value:
 *		0..NROF_ITEMS-1: job number to be executed
 *		NROF_ITEMS:		 ready
 */
static ITEM
get_next_item(void)
{
    static pthread_mutex_t	job_mutex	= PTHREAD_MUTEX_INITIALIZER;
	static bool 			jobs[NROF_ITEMS+1] = { false };	// keep track of issued jobs
	static int              counter = 0;    // seq.nr. of job to be handled
    ITEM 					found;          // item to be returned
	
	/* avoid deadlock: when all producers are busy but none has the next expected item for the consumer 
	 * so requirement for get_next_item: when giving the (i+n)'th item, make sure that item (i) is going to be handled (with n=nrof-producers)
	 */
	pthread_mutex_lock (&job_mutex);

    counter++;
	if (counter > NROF_ITEMS)
	{
	    // we're ready
	    found = NROF_ITEMS;
	}
	else
	{
	    if (counter < NROF_PRODUCERS)
	    {
	        // for the first n-1 items: any job can be given
	        // e.g. "random() % NROF_ITEMS", but here we bias the lower items
	        found = (random() % (2*NROF_PRODUCERS)) % NROF_ITEMS;
	    }
	    else
	    {
	        // deadlock-avoidance: item 'counter - NROF_PRODUCERS' must be given now
	        found = counter - NROF_PRODUCERS;
	        if (jobs[found] == true)
	        {
	            // already handled, find a random one, with a bias for lower items
	            found = (counter + (random() % NROF_PRODUCERS)) % NROF_ITEMS;
	        }    
	    }
	    
	    // check if 'found' is really an unhandled item; 
	    // if not: find another one
	    if (jobs[found] == true)
	    {
	        // already handled, do linear search for the oldest
	        found = 0;
	        while (jobs[found] == true)
            {
                found++;
            }
	    }
	}
    jobs[found] = true;
			
	pthread_mutex_unlock (&job_mutex);
	return (found);
}


