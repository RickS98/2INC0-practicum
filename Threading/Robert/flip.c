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

//#include "uint128.h"
#include "flip.h"

void showbits(uint128_t* v, int w)
{
    int l = 0;
    for(int j = 0; j < BUFSZ; ++j) {
        uint128_t c = v[j];
        for(int i = (j==0)?1:0; i < 128; ++i){
            uint32_t d = (c>>i) & 0x1;
            printf("%u ", d);
            //c >>= 1;
            if (((j*128+i)%w)==0) printf("     %u0\n", l++);
        }
    }
    printf("\n");
}


void toggle(uint128_t* segment, uint8_t bit)
{
    uint128_t mask = ((uint128_t) 0x1 << bit); // make the mask by setting the appropriate bit to one

    uint128_t tmp = *segment & mask; // get the bit value
    *segment &= ~mask; //clear the bit
    *segment |= ~tmp & mask; //put bit back;
}

int main (void)
{
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)

    memset(buffer, 0xff, sizeof(uint128_t)*(BUFSZ));

	//pthread_t thread_id[NROF_THREADS];

    for(uint32_t n = 1; n < NROF_PIECES; ++n) { // loop for each value in NROF_PIECES
        for(uint32_t m = 0; m < NROF_PIECES; m+= n) { // loop for multiples
            
			//pthread_create(&thread_id_md5[m], NULL, do_md5, (void *)buf[m]);
			
			toggle( &(buffer[m/128]), (m%128) );
        }
    }

    showbits(buffer,10);
    printf("done! \n");
    return (0);
}

