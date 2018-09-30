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
#include <string.h>
#include <unistd.h>         // for getpid()
#include <math.h>

//#include "uint128.h"
#include "flip.h"

typedef struct {
    uint32_t    stepsz;
    uint128_t * buf;
}toggle_data;

#define SZ(b) (sizeof(b)/sizeof(b[0]))

pthread_mutex_t toggle_mutex = PTHREAD_MUTEX_INITIALIZER;

void showbits(int w)
{
    int l = 0;
    for(int j = 0, n=0; j < SZ(buffer); ++j) {
        uint128_t c = buffer[j];
        for(int i = (j==0)?1:0; (i < 128) && (n<NROF_PIECES); ++i, ++n) {
            uint32_t d = (c>>i) & 0x1;
            printf("%u ", d);
            if (((j*128+i)%w)==0) {
                fprintf(stderr, "%u\t\t\t", 1+w*l++);
                double dummy;
                if(modf(sqrt(w*l),&dummy)==0)fprintf(stderr, "--> ");
                fprintf(stdout, "\n");
            }
        }
    }
    printf("\n");
}

void toggle(uint128_t* segment, uint8_t bit)
{
    uint128_t mask = ((uint128_t) 0x1 << bit); // make the mask by setting the appropriate bit to one

   // int err = pthread_mutex_lock( &toggle_mutex );//critical section start
   // if (err < 0) perror("mx_lock");

    uint128_t tmp = *segment & mask; // get the bit value
    *segment &= ~mask; //clear the bit
    *segment |= ~tmp & mask; //put bit back;

   // err = pthread_mutex_unlock( &toggle_mutex );//critical section stop
   // if (err < 0) perror("mx_unlock");
}

void * toggle_thread(void *ptr)
{
    toggle_data data = *(toggle_data *)ptr; // make local copy

    for(uint32_t m = 0; m <= NROF_PIECES; m+= data.stepsz) { // loop over all with step size of multiples
        toggle( &(data.buf[m/128]), (m%128) );
    }
return 0;
//    pthread_exit(0);
}

int main (void)
{
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)

    memset(buffer, 0xff, sizeof(buffer));

    pthread_t thread_id[NROF_THREADS];
    toggle_data thread_data[NROF_THREADS] = {0};

    for(int n = 1,t = 0; n <= NROF_PIECES;) { // loop for each value in NROF_PIECES

         for(; (t < NROF_THREADS) && (n <= NROF_PIECES); ++t, ++n) {
            thread_data[t].buf = buffer;
            thread_data[t].stepsz = n;
			
			toggle_thread((void *)&(thread_data[t]));

            // int err = pthread_create(&thread_id[t], NULL, toggle_thread, (void *)&(thread_data[t]));
            // if (err < 0) perror("create");
         }

         //for(--t; t >= 0; --t) {
         for(; t > 0; --t) {
            // int err = pthread_join(thread_id[t], NULL);
            // if (err < 0) perror("join");
         }

        fprintf(stderr,".");
    }

    fprintf(stderr,"\n");

    showbits(1);
    fprintf(stderr, "done! \n");
    return (0);
}

