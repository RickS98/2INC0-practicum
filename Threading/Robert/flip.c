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
#include <string.h>         //for memset()
#include <unistd.h>         // for getpid()
#include <math.h>

//#include "uint128.h" //already included in flip.h.
#include "flip.h"

typedef struct {
    pthread_t   thread_id;
    uint32_t    stepsz;
    uint128_t * buf;
}toggle_data;

#define SZ(b)   (sizeof(b)/sizeof(b[0]))
#define NRBITS  (sizeof(uint128_t)*8) // #chars in a uint128_t times #bits in a char. safest option, fails if buffer[] array has a type change.

pthread_mutex_t toggle_mutex = PTHREAD_MUTEX_INITIALIZER;

void showbits(int w)
{
    int err = 0;
    int l = 0;
    for(int j = 0, n=0; j < SZ(buffer); ++j) {  // for all elements in buffer
        uint128_t c = buffer[j];
        for(int i = (j==0)?1:0; (i < 128) && (n<NROF_PIECES); ++i, ++n) { // go through buf bit by bit, we don't want to print bit0@buffer[0] so init to 1 when j==0. if n reaches NROF_PIECES stop printing.
            uint32_t d = (c>>i) & 0x1; // get the i-th bit, implicit cast to uint32. we don't care, only interested in lsb.
            fprintf(stdout, "%u ", d); // print 1 or 0 to stdout.
            if (((j*128+i)%w)==0) { // stdout: print <newline>. stderr: print line number and --> when there's supposed to be a zero.
                fprintf(stderr, "%u\t\t\t", 1+w*l++);
                double dummy;
                if(modf(sqrt(w*l),&dummy)==0) {
                    fprintf(stderr, "--> ");
                    err += d; // if d is correct, thus a zero, this should not increase err
                } else {
                    if(d==0) --err; // if d was zero when it shouldn't be; decrease err to signify an "off-squares" zero
                }
                fprintf(stdout, "\n");
            }
        }
    }
    fprintf(stderr,"#of incorrect zeros: %d ",err);
    fprintf(stdout, "\n");
}

void toggle(uint128_t* segment, uint8_t bit)
{
    uint128_t mask = ((uint128_t) 0x1 << bit); // make the mask by setting the appropriate bit to one

   int err = pthread_mutex_lock( &toggle_mutex );//critical section start
   if (err < 0) perror("mx_lock");

    uint128_t tmp = *segment & mask; // get the bit value
    *segment &= ~mask; //clear the bit
    *segment |= ~tmp & mask; //put bit back;

   err = pthread_mutex_unlock( &toggle_mutex );//critical section stop
   if (err < 0) perror("mx_unlock");
}

void * toggle_thread(void *ptr)
{
    toggle_data data = *(toggle_data *)ptr; // make local copy

    for(uint32_t m = 0; m <= NROF_PIECES; m+= data.stepsz) { // loop over all with step size of multiples
        toggle( &(data.buf[m/NRBITS]), (m%NRBITS) );
    }

    pthread_exit(0);
}

int main (void)
{
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)

    memset(buffer, 0xff, sizeof(buffer));

    toggle_data* thread_data = calloc(NROF_THREADS,sizeof(toggle_data));

    for(int n = 1,t = 0; n <= NROF_PIECES ;) { // n:loop for each value in NROF_PIECES,t:thread count, seed with zero at main loop entry.

        for(int err=0; (t < NROF_THREADS) && (n <= NROF_PIECES) && !(err < 0); ++t, ++n) { // exit when either NROF_THREADS or NROF_PIECES is reached, also terminate if the last create generated an error.

            thread_data[t].buf = buffer;
            thread_data[t].stepsz = n;

            err = pthread_create(&thread_data[t].thread_id, NULL, toggle_thread, (void *)&(thread_data[t]));
            if (err < 0) perror("create");
        }
        /* t now contains the number of threads that have been started by the create loop.
           This is the number of threads we need to join again afterwards. */

#if(NROF_THREADS > 30) // guesstimate, ifdef because NROF_THREADS is compile time constant.
        /* Start with the oldest one since this is the one with the highest likelihood of being done
           if there are many threads. */
        for(int tmax = t; t > 0; --t) {//seed tmax with the #threads we actually started as t decreased tmax-t increases.
            int err = pthread_join(thread_data[tmax-t].thread_id, NULL);
            if (err < 0) perror("join");
        }//subsequent joins will not terminate on errors since there could be other threads that are ok.
#else
        /* when we have a small number of threads the first one may still be busy but the last one
           may already be done (especially in the beginning because the multiples are small)
           so start with the newest one first.*/
        for(;t > 0; --t) {
            int err = pthread_join(thread_data[t-1].thread_id, NULL);
            if (err < 0) perror("join");
        }
#endif
        fprintf(stderr,"."); // print a little progress indicator
    }

    free(thread_data);

    fprintf(stderr,"\n");

    showbits(1);
    fprintf(stderr, "done! \n"); //since I'm allowed to print anything I want to stderr :-)
    return (0);
}

