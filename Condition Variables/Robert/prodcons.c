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
#include <time.h>

#include "prodcons.h"

#define	MAX(x,y)	((x<y)?y:x)
 
 typedef struct {
    pthread_t   thread_id;
    ITEM    item;
}thread_data_t;

typedef struct {
	ITEM items[BUFFER_SIZE];
	int		pos;
	int		next;
}buffer_s;

buffer_s buffer;

static pthread_mutex_t buffer_mutex 		= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       buffer_cond_produced = PTHREAD_COND_INITIALIZER;
static pthread_cond_t       buffer_cond_consumed = PTHREAD_COND_INITIALIZER;
//static pthread_cond_t       buffer_cond = PTHREAD_COND_INITIALIZER;

static void rsleep (int t);			// already implemented (see below)
static ITEM get_next_item (void);	// already implemented (see below)

#define PRTTIME		(clock())

/* producer thread */
static void * producer (void * arg)
{
	thread_data_t * data = (thread_data_t *)arg;
	
	fprintf(stderr, "prod%lu.%lu: thread %lu started\n", data->thread_id%10000, PRTTIME, data->thread_id);
    while (true /* TODO: not all items produced */)
    {
        // TODO: 
        // * get the new item
		data->item = get_next_item();
		fprintf(stderr, "prod%lu.%lu: received item: %d\n", data->thread_id%10000, PRTTIME, data->item);

			
        rsleep (100);	// simulating all kind of activities...
		//sleep(1); //debug
		// TODO:
		// * put the item into buffer[]
		//
        // follow this pseudocode (according to the ConditionSynchronization lecture):
		int err = pthread_mutex_lock( &buffer_mutex );//critical section start
		if (err < 0) perror("mx_lock_prod");
        //      while not condition-for-this-producer
        //          wait-cv;
		if (data->item == NROF_ITEMS) {
			buffer.next = NROF_ITEMS;
			pthread_cond_signal (&buffer_cond_produced);
			fprintf(stderr, "prod%lu.%lu: signal send\n",data->thread_id%10000,  PRTTIME);
			fprintf(stderr, "prod: received all items\n\n");
			err = pthread_mutex_unlock( &buffer_mutex );//critical section stop
			if (err < 0) perror("mx_unlock_prod");
			break;
		}

		while( !(buffer.pos<BUFFER_SIZE) || !(data->item==buffer.next+1) ) {//de morgan
			fprintf(stderr, "prod%lu.%lu: wait item %d, pos %d, bufval %d\n", data->thread_id%10000, PRTTIME, data->item, buffer.pos, buffer.items[MAX(buffer.pos,0)]);
			pthread_cond_wait(&buffer_cond_consumed, &buffer_mutex);
		}
        //      critical-section;
		++buffer.pos;
		buffer.items[buffer.pos] = data->item;
		fprintf(stderr, "prod%lu.%lu: put item %d at position %d, buffer[pos]:%d\n", data->thread_id%10000, PRTTIME, data->item, buffer.pos, buffer.items[buffer.pos]);

		++buffer.next;
		fprintf(stderr, "prod%lu.%lu: next expected item:%d\n", data->thread_id%10000, PRTTIME, buffer.next);

        //      possible-cv-signals;
		pthread_cond_signal (&buffer_cond_produced);
		fprintf(stderr, "prod%lu.%lu: signal send\n",data->thread_id%10000,  PRTTIME);
        //      mutex-unlock;
		err = pthread_mutex_unlock( &buffer_mutex );//critical section stop
		if (err < 0) perror("mx_unlock_prod");
        // (see condition_test() in condition_basics.c how to use condition variables)
    }
	pthread_exit(0);
}

/* consumer thread */
static void * consumer (void * arg)
{
	thread_data_t * data = (thread_data_t *)arg;
	
	fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: thread started\n",data->thread_id%10000,  PRTTIME);
    while (true /* TODO: not all items retrieved from buffer[] */)
    {
        // TODO: 
		// * get the next item from buffer[]
		// * print the number to stdout
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
		int err = pthread_mutex_lock( &buffer_mutex );//critical section start
		if (err < 0) perror("mx_lock_cons");

        //      while not condition-for-this-consumer
        //          wait-cv;
		//while( (buffer.next < NROF_ITEMS) && !(buffer.pos>=0) ) {
		while( !(buffer.pos>=0) ) {
			fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: wait buffer.pos %d\n", data->thread_id%10000, PRTTIME, buffer.pos);
			pthread_cond_wait(&buffer_cond_produced, &buffer_mutex);
		}
		
		if (buffer.next == NROF_ITEMS) {
			fprintf(stderr, "cons: printed all items\n\n");
			break;
		}
		//      critical-section;
		//take one item from the buffer & print it
		fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: print buffer@pos: %d\n", data->thread_id%10000, PRTTIME, buffer.pos);
		fprintf(stderr, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
		fprintf(stdout,"%d\n",buffer.items[0]);
		fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: buffer contents:", data->thread_id%10000, PRTTIME);
		//move all items down by one
		for(int i=0; (i<BUFFER_SIZE-1); ++i) {
			buffer.items[i] = buffer.items[i+1];
			fprintf(stderr, " %d", buffer.items[i]);
		}
		fprintf(stderr, "\n");
		//decrement the pos since one spot freed-up in the buffer
		--buffer.pos;
		fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: pos is now: %d\n", data->thread_id%10000, PRTTIME, buffer.pos);
        //      possible-cv-signals;
		fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: signal send\n", data->thread_id%10000, PRTTIME);
		pthread_cond_signal (&buffer_cond_consumed);

        //      mutex-unlock;
		err = pthread_mutex_unlock(&buffer_mutex );//critical section stop
		if (err < 0) perror("mx_unlock_cons");
        rsleep (100);		// simulating all kind of activities...
    }
	pthread_exit(0);
}

int main (void)
{
    // TODO: 
    // * startup the producer threads and the consumer thread
    // * wait until all threads are finished
	
    thread_data_t* thread_data_prod = calloc(NROF_PRODUCERS,sizeof(thread_data_t));
	int tp=0;
	for(int err=0; (tp < NROF_PRODUCERS) && !(err < 0); ++tp) { // exit when NROF_PRODUCERS is reached, also terminate if the last create generated an error.

		thread_data_prod[tp].item = 0;

		err = pthread_create(&thread_data_prod[tp].thread_id, NULL, producer, (void *)&(thread_data_prod[tp]));
		if (err < 0) perror("producer");
	}

	//sleep(1); //debug
    thread_data_t* thread_data_cons = calloc(NROF_CONSUMERS,sizeof(thread_data_t));
	int tc=0;
	for(int err=0; (tc < NROF_CONSUMERS) && !(err < 0); ++tc) { // exit when NROF_CONSUMERS is reached, also terminate if the last create generated an error.

		thread_data_cons[tc].item = 0;

		err = pthread_create(&thread_data_cons[tc].thread_id, NULL, consumer, (void *)&(thread_data_cons[tc]));
		if (err < 0) perror("consumer");
	}
	
	//clear the bufer and its cnt
	memset(&buffer,-1,sizeof(buffer_s)); // -1 signify's init state, yes memset works because -1 = 0xff

	//start everything by generating a signal;
	//pthread_cond_signal (&buffer_cond);
	
	for(;tp > 0; --tp) {// if we have at least one producer...
		int err = pthread_join(thread_data_prod[tp-1].thread_id, NULL);
		if (err < 0) perror("prod join");
	}
	
	for(;tc > 0; --tc) {// if we have at least one consumer...
		int err = pthread_join(thread_data_cons[tc-1].thread_id, NULL);
		if (err < 0) perror("cons join");
	}
	
	pthread_cond_destroy(&buffer_cond_produced);
	pthread_cond_destroy(&buffer_cond_consumed);
	pthread_mutex_destroy(&buffer_mutex);
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


/*
proc producer =
[ while true do 
	

*/












