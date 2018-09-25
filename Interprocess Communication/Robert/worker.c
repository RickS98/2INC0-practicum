/*
 * Operating Systems  [2INCO]  Practical Assignment
 * Interprocess Communication
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
#include <string.h>
#include <errno.h>          // for perror()
#include <unistd.h>         // for getpid()
#include <mqueue.h>         // for mq-stuff
#include <time.h>           // for time()
#include <complex.h>

#include <pthread.h>

#include "common.h"
#include "md5s.h"

static void rsleep (int t);

static char                 mq_name1[80];
static char                 mq_name2[80];

// max word size wlen=6 and alphasize=20 needs 52bits. log2(6^20)=51.7
void cnttowrd(uint64_t c, char word[], uint8_t alphabet_size, int preamble_len)
{
    #if (MAX_MESSAGE_LENGTH > 6)
        #warning("message length more than 6, risk of overflow in word generation");
    #endif

    int i = preamble_len;

    for(; i < MAX_MESSAGE_LENGTH, ( (c > 0) || (i == preamble_len) ) ; ++i) {

        word[i] = 'a' + ((char)(c%alphabet_size));
        c /= alphabet_size;
    }

    // for(; i >= 0 ; --i) {
        // word[i] = ' ';
    // }
    // word[MAX_MESSAGE_LENGTH] = 0;

}

uint128_t hash = 0;
pthread_mutex_t testhash_mutex = PTHREAD_MUTEX_INITIALIZER;

// void * read_req_queue(void* mq_fd_request)
// {
    // MQ_REQUEST_MESSAGE  req;

    // pid_t pid = getpid();

    // while(1) {
        // // read the message queue and store it in the request message
        // printf ("                                                                           child %d: waiting for request...\n",pid);
        // mq_receive (*(mqd_t *)mq_fd_request, (char *) &req, sizeof (req), NULL);
// //        printf ("                                                                           child %d: received request...\n",pid);

        // printf ("                                                                           child %d: received: %s\n",pid, req.word);

        // if(req.ctrl == NEWHASH) {
            // pthread_mutex_lock( &testhash_mutex );//critical section start
            // hash = req.hash;
            // pthread_mutex_unlock( &testhash_mutex );//critical section stop
        // }
    // }

// }

// void * send_rsp_msg(void* mq_fd_response)
// {
    // MQ_RESPONSE_MESSAGE rsp;

    // pid_t pid = getpid();

    // while(1) {
        // // send the response
        // printf ("                                                                           child %d: sending...\n", pid);
        // int err = mq_send (*(mqd_t *)mq_fd_response, (char *) &rsp, sizeof (rsp), 0);
        // if (err < 0) perror("child");
    // }
// }

void * rval = NULL;

void * do_md5(void* word)
{
    uint128_t tmp = md5s((const char * )word, MAX_MESSAGE_LENGTH);


    pthread_mutex_lock( &testhash_mutex );//critical section start
    if(hash == tmp) { // we have a hit, return 'word' to farmer.
        rval = word;
    }
	
	// //hack
	// // if(((const char * )word)[0] == 'z')
		// // rval = word;
	// time_t t = time(NULL);
	// char r = (t % 256);
	// //sprintf((char *)word),"%d", r);
	// ((char *)word)[0] = r;
	// ((char *)word)[1] = (t%32);
	// rval = word;
	// printf("doing something %ld\n",t);
	
    pthread_mutex_unlock( &testhash_mutex );//critical section stop
	pthread_exit(0);
}

int main (int argc, char * argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the arguments)
    //  * repeatingly:
    //      - read from a message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do that job
    //      - write the results to a message queue
    //    until there are no more tasks to do
    //  * close the message queues

    // mqd_t               mq_fd_request;
    // mqd_t               mq_fd_response;


    if (1 == argc) {
        fprintf(stderr,"invalid number of arguments, expecting at least 2, mq_name1 and mq_name2\n");
        return -1;
    }

    // printf("argc: %d\n",argc);

    for (int i  = 0; i < argc; i += 2) {
        if (0 == strcmp(argv[i], "-mq_name1")) {
            strcpy(mq_name1,argv[i+1]);
            argc -= 2;
        }
        if (0 == strcmp(argv[i], "-mq_name2")) {
            stpcpy(mq_name2,argv[i+1]);
            argc -= 2;
        }
        // printf("argv: %s\n",argv[i]);
    }

    mqd_t mq_fd_request = mq_open (mq_name1, O_RDONLY);
    if(mq_fd_request < 0) perror("child");
    mqd_t mq_fd_response = mq_open (mq_name2, O_WRONLY);
    if(mq_fd_response < 0) perror("child");

    // printf("mq_name1: %s\n",mq_name1);
    // printf("mq_name2: %s\n",mq_name2);

    if ((mq_fd_request < 0) || ( mq_fd_response < 0)) {
        fprintf(stderr,"error opening either mq_name1 or mq_name2 queues, terminating");
        return -1;
    }

    pid_t pid = getpid();

	//pthread_t thread_id_rsp;
    //pthread_create(&thread_id_req, NULL, send_rsp_msg, (void *)&mq_fd_response);

	MQ_REQUEST_MESSAGE  req;

    while(1) {
		pthread_t thread_id_md5[NUM_MD5s];
		char buf[NUM_MD5s][MAX_MESSAGE_LENGTH] = {0};

		for(int m = 0; m < NUM_MD5s; ++m) {
			// read the message queue and store it in the request message
			printf ("                                                                           child %d: waiting for request...\n",pid);
			mq_receive (mq_fd_request, (char *) &req, sizeof (req), NULL);
			printf ("                                                                           child %d: received: %s\n",pid, req.word);

			rsleep(1000);

			switch(req.ctrl)
			{
			case NEWHASH:
				pthread_mutex_lock( &testhash_mutex );//critical section start
				hash = req.hash;
				pthread_mutex_unlock( &testhash_mutex );//critical section stop
				break;
			case TERM:
				break;
			default:
				break;
			}

			strcpy(buf[m], req.word);

			//exapnd the preamble in the buf before starting thread
				// cnttowrd(cnt,req.word, req.alphabet_nrof_char, req.preamble_len);
				 // printf("%ld   %s\n",cnt, req.word);

				 //d[m] = md5s(req.word, MAX_MESSAGE_LENGTH);
			
		


			pthread_create(&thread_id_md5[m], NULL, do_md5, (void *)buf[m]);
		}

		sleep(3);

		for(int m = 0; m < NUM_MD5s; ++m) {
			pthread_join(thread_id_md5[m], NULL);
		}

		if(rval != NULL) {
			MQ_RESPONSE_MESSAGE rsp;
			strcpy(rsp.str, rval);
			rsp.ctrl = MATCH;
			// send the response
			printf ("                                                                           child %d: sending...\n", pid);
			int err = mq_send (mq_fd_response, (char *) &rsp, sizeof (rsp), 0);
			if (err < 0) perror("child");
			rval = NULL;
		}
	}

    mq_close (mq_fd_response);
    mq_close (mq_fd_request);

    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;

    if (first_call == true) {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}


