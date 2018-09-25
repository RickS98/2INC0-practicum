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
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>         // for execlp
#include <mqueue.h>         // for mq

#include <pthread.h>

#include "settings.h"
#include "common.h"


static char                 mq_name1[80];
static char                 mq_name2[80];

/*-------------------------------------------------------------------------*/

static void getattr (mqd_t mq_fd)
{
    struct mq_attr      attr;
    int                 rtnval;

    rtnval = mq_getattr (mq_fd, &attr);
    if (rtnval == -1)
    {
        perror ("mq_getattr() failed");
        exit (1);
    }
    fprintf (stderr, "%d: mqdes=%d max=%ld size=%ld nrof=%ld\n",
                getpid(),
                mq_fd, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}

void calc_preamble (char preamble[], uint8_t alphabet_size, uint64_t c)
{
    preamble[0] = 'a' + ((char)(c%alphabet_size));
    preamble[1] = ' ';
    preamble[2] = ' ';
    preamble[3] = ' ';
    preamble[4] = ' ';



}



pthread_mutex_t nexthash_mutex = PTHREAD_MUTEX_INITIALIZER;
int goto_next_hash = 1;


void * fill_req_queue(void * mq_fd_request)
{

    //uint64_t c = 0;
    MQ_REQUEST_MESSAGE req;
    int hash_idx = -1;
    //short b = 0;
	char c = -1;

    while(1) {
        // fill request message
        req.ctrl = RUN;


        pthread_mutex_lock( &nexthash_mutex );//critical section start
        if (goto_next_hash == 1) {
            ++hash_idx;
            goto_next_hash = 0;
            req.ctrl = NEWHASH;
        }
        pthread_mutex_unlock( &nexthash_mutex );//critical section stop

        req.hash = md5_list[hash_idx];

        req.alphabet_nrof_char = ALPHABET_NROF_CHAR;
        req.preamble_len = 1;
        //calc_preamble(req.word, ALPHABET_NROF_CHAR, c++);

        //sprintf(req.word,"%c",'a' + ((++c)%26) ); // seed with preamble
		++c;
		c %= ALPHABET_NROF_CHAR;
        req.word[0] = 'a' + c; // seed with preamble

        // send the request
        fprintf (stderr, "parent: pushing '%s' to queue...\n", req.word);
        int err = mq_send (*(mqd_t *)mq_fd_request, (char *) &req, sizeof (req), 0);
        if (err < 0) perror("parent");

        //sleep (1);
    }
}


void * parse_rsp_queue(void* mq_fd_response)
{
    // read the result and store it in the response message
    MQ_RESPONSE_MESSAGE rsp;

    while(1) {
        printf ("parent: waiting for response...\n");
        int err = mq_receive (*(mqd_t *)mq_fd_response, (char *) &rsp, sizeof (rsp), NULL);
        if (err < 0) perror("parent");

        printf ("parent: received from %d: %s", rsp.pid, rsp.str);

        switch(rsp.ctrl)
        {
        case MATCH:
            pthread_mutex_lock( &nexthash_mutex );//critical section start
            goto_next_hash = 1; //go to next hash index
            pthread_mutex_unlock( &nexthash_mutex );//critical section stop
			printf("\n\n      '%s'    \n\n",rsp.str);
            break;

        default:
        //should not reach here...
            break;

        }

    }
}

int main (int argc, char * argv[])
{
    if (argc != 1)
    {
        fprintf (stderr, "%s: invalid arguments\n", argv[0]);
    }

    // TODO:
    //  * create the message queues (see message_queue_test() in interprocess_basic.c)
    //  * create the child processes (see process_test() and message_queue_test())
    //  * do the farming
    //  * wait until the children have been stopped (see process_test())
    //  * clean up the message queues (see message_queue_test())

    // Important notice: make sure that the names of the message queues contain your
    // student name and the process id (to ensure uniqueness during testing)

    pid_t               processID[NROF_WORKERS];      /* Process ID from fork() */
    mqd_t               mq_fd_request;
    mqd_t               mq_fd_response;
    struct mq_attr      attr;

    sprintf (mq_name1, "/mq_request_%s_%d", STUDENT_NAME, getpid());
    sprintf (mq_name2, "/mq_response_%s_%d", STUDENT_NAME, getpid());

    attr.mq_maxmsg  = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof (MQ_REQUEST_MESSAGE);
    mq_fd_request = mq_open (mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

    attr.mq_maxmsg  = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof (MQ_RESPONSE_MESSAGE);
    mq_fd_response = mq_open (mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

    getattr(mq_fd_request);
    getattr(mq_fd_response);

    //create workers
    for( int i = 0; i < NROF_WORKERS; ++i)
    {
        // create_child();
        processID[i] = fork();
        if (processID[i] < 0)
        {
            perror("fork() failed");
            exit (1);
        }
        else
        {
            if (processID[i] == 0)
            {//child
                printf ("parent created child with pid:%d\n", getpid());

                // printf("mq_name1: %s\n",mq_name1);
                // printf("mq_name2: %s\n",mq_name2);

                //sleep(3);

                //spawn worker i
                execlp ("./worker","-mq_name1", mq_name1, "-mq_name2", mq_name2, " ",(char *) NULL);//figure out that space thingy at the end later...

                exit (0);
            }
        }
    }

    pthread_t thread_id_req;
    pthread_create(&thread_id_req, NULL, fill_req_queue, (void *)&mq_fd_request);

    pthread_t thread_id_rst;
    pthread_create(&thread_id_rst, NULL, parse_rsp_queue, (void *)&mq_fd_response);

    for( int i = 0; i < NROF_WORKERS; ++i) {
        waitpid (processID[i], NULL, 0);   // wait for the children
    }

    printf("exiting...\n");

    mq_close (mq_fd_response);
    mq_close (mq_fd_request);
    mq_unlink (mq_name1);
    mq_unlink (mq_name2);


    return (0);
}

