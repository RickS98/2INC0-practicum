/*
 * Operating Systems  [2INCO]  Practical Assignment
 * Condition Variables Application
 *
 * Robert Jong-A-Lock 0726356
 * Tom Buskens 1378120
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
#include <semaphore.h>

#include "prodcons.h"


#define NROF_CONSUMERS          1

#define _NO_DEBUGOUT_COND_VARS

#ifdef _NO_DEBUGOUT_COND_VARS
#define fprintf(...)    // disable fprintf which is only used here to print to stderr (so as to check if their delay is not helping things work...)[ugly, should make this a proper macro]
#endif

#define MAX(x,y)    ((x<y)?y:x)

 typedef struct {
    pthread_t   thread_id;
    ITEM    item;
}thread_data_t;

typedef struct {
    ITEM items[BUFFER_SIZE];
    int     pos;
    int     next;
}buffer_s;

buffer_s buffer;

static pthread_mutex_t buffer_mutex              = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       buffer_cond_produced = PTHREAD_COND_INITIALIZER;
static pthread_cond_t       buffer_cond_consumed = PTHREAD_COND_INITIALIZER;

static void rsleep (int t);         // already implemented (see below)
static ITEM get_next_item (void);   // already implemented (see below)

#define PRTTIME     (clock())

#define ERR(err,s)  if (err < 0) { /* something bad happened */             \
                        perror(s); /* print how bad it was */               \
                        exit(-1);  /* then act imperial soldier like... */  \
                    }

/* producer thread */
// * put the item into buffer[]
static void * producer (void * arg)
{
    thread_data_t * data = (thread_data_t *)arg;

    fprintf(stderr, "prod%lu.%lu: thread %lu started\n", data->thread_id%10000, PRTTIME, data->thread_id);
    for(data->item = get_next_item();data->item < NROF_ITEMS; data->item = get_next_item()) {// * get the new item

        fprintf(stderr, "prod%lu.%lu: received item: %d\n", data->thread_id%10000, PRTTIME, data->item);

        rsleep (100);   // simulating all kind of activities...

        int err = pthread_mutex_lock( &buffer_mutex );//critical section start
        ERR(err,"mx_lock_prod");

        //      while not condition-for-this-producer
        //          wait-cv;
        while( !(buffer.pos<BUFFER_SIZE-1) || !(data->item==buffer.next) ) {//de morgan
            fprintf(stderr, "prod%lu.%lu: wait item %d, pos %d, bufval %d\n", data->thread_id%10000, PRTTIME, data->item, buffer.pos, buffer.items[MAX(buffer.pos,0)]);
            pthread_cond_wait(&buffer_cond_consumed, &buffer_mutex);
        }
        // enter critical-section;
        fprintf(stderr, "prod%lu.%lu: before incr buffer[pos]:%d\n", data->thread_id%10000, PRTTIME, buffer.pos);

        //go to the next buffer position we will write to.
        ++buffer.pos;

        //put item in the buffer
        buffer.items[buffer.pos] = data->item;
        fprintf(stderr, "prod%lu.%lu: put item %d at position %d, buffer[pos]:%d\n", data->thread_id%10000, PRTTIME, data->item, buffer.pos, buffer.items[buffer.pos]);

        ++buffer.next;
        fprintf(stderr, "prod%lu.%lu: next expected item:%d\n", data->thread_id%10000, PRTTIME, buffer.next);

        //tell consumers something was added to the buffer.
        pthread_cond_signal (&buffer_cond_produced);
        fprintf(stderr, "prod%lu.%lu: signal send\n",data->thread_id%10000,  PRTTIME);

        //but also inform other producers that something was added to the the buffer so they can check if they can proceed.
        pthread_cond_broadcast(&buffer_cond_consumed);

        //mutex-unlock
        err = pthread_mutex_unlock( &buffer_mutex );//critical section stop
        ERR(err,"mx_unlock_prod");
    }

    fprintf(stderr, "prod: received all items\n\n");

    pthread_exit(0);
}

/* consumer thread */
static void * consumer (void * arg)
{
#ifndef _NO_DEBUGOUT_COND_VARS
    thread_data_t * data = (thread_data_t *)arg;
#endif

    fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: thread started\n",data->thread_id%10000,  PRTTIME);
    while (true)
    {
        //      mutex-lock;
        int err = pthread_mutex_lock( &buffer_mutex );//critical section start
        ERR(err,"mx_lock_cons");

        //      while not condition-for-this-consumer
        //          wait-cv;
        while( !(buffer.pos>=0) && (buffer.next < NROF_ITEMS) ) {
            fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: wait buffer.pos %d\n", data->thread_id%10000, PRTTIME, buffer.pos);
#if (NROF_CONSUMERS == 1)
            pthread_cond_wait(&buffer_cond_produced, &buffer_mutex);
#elif (NROF_CONSUMERS> 1)
            struct timespec tp;
            clock_gettime(CLOCK_REALTIME, &tp);
            tp.tv_sec += 1;
            pthread_cond_timedwait(&buffer_cond_produced, &buffer_mutex, &tp); //timed because the consumer threads do not know who took the last item, so check if condition still valid from time to time. They do not communicate with each other (yet...).
#else
    #error "NROF_CONSUMERS needs to be >= 1"
#endif
        }

        if ((buffer.next == NROF_ITEMS) && (buffer.pos<0)) { // buffer.next == NROF_ITEMS is a thread exit condition but only if all items have been printed.
            fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: printed all items\n\n", data->thread_id%10000, PRTTIME);
            pthread_cond_broadcast (&buffer_cond_produced); // make sure the other consumers are notified that this thread thinks we're done consuming.
            fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: cons done signal send\n", data->thread_id%10000, PRTTIME);
            //      mutex-unlock;
            err = pthread_mutex_unlock(&buffer_mutex );//critical section stop
            ERR(err,"mx_unlock_cons");
            break;
        }

        //      critical-section;
        //take one item from the buffer & print it
        fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: print buffer@pos: %d\n", data->thread_id%10000, PRTTIME, buffer.pos);
        fprintf(stderr, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
         printf("%d\n",buffer.items[0]); // print number to stdout....
        fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: buffer contents:", data->thread_id%10000, PRTTIME);
        //move all items down by one
        for(int i=0; (i<BUFFER_SIZE-1); ++i) {
            buffer.items[i] = buffer.items[i+1];
            fprintf(stderr, " %d", buffer.items[i]);
        }
        fprintf(stderr, "\n");
        //decrement the pos index since one spot freed-up in the buffer
        --buffer.pos;
        fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: decremended pos, curr pos is: %d\n", data->thread_id%10000, PRTTIME, buffer.pos);
        //      possible-cv-signals;
        fprintf(stderr, "\t\t\t\t\t\t\tcons%lu.%lu: signal send\n", data->thread_id%10000, PRTTIME);

        pthread_cond_broadcast(&buffer_cond_consumed); // broadcast to all sleeping consumers that something got consumed.

        //      mutex-unlock;
        err = pthread_mutex_unlock(&buffer_mutex );//critical section stop
        ERR(err,"mx_unlock_cons");
        rsleep (100);       // simulating all kind of activities...
    }
    pthread_exit(0);
}

int main (void)
{
    // TODO:
    // * startup the producer threads and the consumer thread
    // * wait until all threads are finished

    //init buffer and pos
    memset(&buffer,-1,sizeof(buffer_s)); // -1 signifies init state, yes memset works because -1 = 0xffffffff
    buffer.next = 0;

    thread_data_t* thread_data_prod = calloc(NROF_PRODUCERS,sizeof(thread_data_t));
    int tp=0;
    for(int err=0; (tp < NROF_PRODUCERS) && !(err < 0); ++tp) { // exit when NROF_PRODUCERS is reached, also terminate if the last create generated an error.

        thread_data_prod[tp].item = 0;

        err = pthread_create(&thread_data_prod[tp].thread_id, NULL, producer, (void *)&(thread_data_prod[tp]));
        ERR(err,"producer tread");
    }

    thread_data_t* thread_data_cons = calloc(NROF_CONSUMERS,sizeof(thread_data_t));
    int tc=0;
    for(int err=0; (tc < NROF_CONSUMERS) && !(err < 0); ++tc) { // exit when NROF_CONSUMERS is reached, also terminate if the last create generated an error.

        thread_data_cons[tc].item = 0;

        err = pthread_create(&thread_data_cons[tc].thread_id, NULL, consumer, (void *)&(thread_data_cons[tc]));
        ERR(err,"consumer thread");
    }

    for(;tp > 0; --tp) {// if we have at least one producer...
        int err = pthread_join(thread_data_prod[tp-1].thread_id, NULL);
        ERR(err,"prod join");
    }

    for(;tc > 0; --tc) {// if we have at least one consumer...
        int err = pthread_join(thread_data_cons[tc-1].thread_id, NULL);
        ERR(err,"cons join");
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
 *      thread-safe function to get a next job to be executed
 *      subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1
 *      in arbitrary order
 *      return value NROF_ITEMS indicates that all jobs have already been given
 *
 * parameters:
 *      none
 *
 * return value:
 *      0..NROF_ITEMS-1: job number to be executed
 *      NROF_ITEMS:      ready
 */
static ITEM
get_next_item(void)
{
    static pthread_mutex_t  job_mutex   = PTHREAD_MUTEX_INITIALIZER;
    static bool             jobs[NROF_ITEMS+1] = { false }; // keep track of issued jobs
    static int              counter = 0;    // seq.nr. of job to be handled
    ITEM                    found;          // item to be returned

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













