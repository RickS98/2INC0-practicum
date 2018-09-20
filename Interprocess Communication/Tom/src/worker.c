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

#include "common.h"
#include "md5s.h"

static void rsleep (int t);

mqd_t queJobDescriptor;
mqd_t queResultDescriptor;

struct JOBMESSAGE currentJob;

char startLetter;
char endLetter;

int found = 0;

//open message queues needed for communication
void openMessageQueues(char queNameJob[],char queNameResult[])
{
	queJobDescriptor = mq_open (queNameJob, O_RDONLY);
	queResultDescriptor = mq_open (queNameResult, O_WRONLY);

	if(queJobDescriptor < 0 || queResultDescriptor < 0)
	{
	    perror("Failed to open message que");
	    exit(1);
	}
}

//function that checks a generated hash to check hash.
void checkHash(char string[], int currentLength)
{
	uint128_t generatedHash = md5s(string, currentLength);

	if(currentJob.hash == generatedHash)
	{
		struct RESULTMESSAGE result;

		result.suicideNote = 0;
		result.hashIndex = currentJob.hashIndex;
		strcpy(result.correctString,string);

		mq_send(queResultDescriptor,(char *) &result, sizeof (result), 0);

		//printf("Found Hash: hashIndex %d, correctString %s\n",result.hashIndex,result.correctString);
	}
}

//recursive function that generates last part of string needed to check
void generateVariableString(char destination[],int currentLength)
{
	if(currentLength > MAX_MESSAGE_LENGTH)
	{
		perror("Message is too long");
		exit(1);
	}
	else
	{
		//Wheneever hash is found job is imidiately and a new job is requested

		if(!found)
		{
			//current string is also checked but first a zero is concatenated
			destination[currentLength]='\0';
			checkHash(destination,currentLength);
		}

		if(currentLength < MAX_MESSAGE_LENGTH)
		{
			//a for loop that goes through all requested letters

			for( char i = startLetter; i<=endLetter&&!found;i++) {

				//adds letter to string
				destination[currentLength] = i;

				generateVariableString(destination,currentLength+1);

				//removes earlier added letter from string
				destination[currentLength] = '\0';
			}
		}

	}
}

//main function that waits on job que and processes found jobs
void processMessages()
{
	//worker keeps checking que untill it gets a killyourself command
	int deadDesire = 0;

	while(!deadDesire)
	{
		mq_receive (queJobDescriptor, (char *) &currentJob, sizeof (currentJob), NULL);

		if(currentJob.killYourself)
		{
			deadDesire = 1;

			struct RESULTMESSAGE result;

			result.suicideNote = 1;

			mq_send(queResultDescriptor,(char *) &result, sizeof (result), 0);

		}
		else
		{
			//printf("Job Received:HashIndex %d, startLetters %s, Hash 0x%llx\n",currentJob.hashIndex,currentJob.startLetters,currentJob.hash);

			found = 0;

			char testedString[MAX_MESSAGE_LENGTH+1];

			strcpy(testedString,currentJob.startLetters);

			//the mandetory random sleep
			rsleep(10000);

			/*it is checked if there is a '0' in supplied string, this means it is
			 * not nessesary to generate more letters
			 */

			int nullIndex = 0;

			for(int i=0;i<CONSTANT_MESSAGE_LENGTH && !nullIndex;i++)
			{
				if(testedString[i] == '\0')
				{
					nullIndex = i;
				}
			}

			if(nullIndex)
			{
				checkHash(testedString,nullIndex);
			}
			else
			{
				//if no 0 is found extra letters are generated
				generateVariableString(testedString,CONSTANT_MESSAGE_LENGTH);
			}
		}

	}
	//printf("Kill command received and confirmed\n");
}


//function that closes message queues
void closeMessageQueues()
{
	int returnJobClose = mq_close(queJobDescriptor);
	int returnResultClose = mq_close(queResultDescriptor);

	if(returnJobClose < 0 || returnResultClose < 0)
	{
		perror("Closing message queues failed");
		exit(1);
	}
}

int main (int argc,char *argv[])
{
	if (argc != 4)
	{
	    perror("Four arguments are needed to start worker");
	    exit(1);
	}

	//printf("Worker started: %s\n %s\n %s\n %s\n",argv[0],argv[1],argv[2],argv[3]);


	startLetter = *argv[2];
	endLetter = *argv[3];
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the arguments)

	openMessageQueues(argv[0],argv[1]);
    //  * repeatingly:
    //      - read from a message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do that job 
    //      - write the results to a message queue
    //    until there are no more tasks to do

	processMessages();

    //  * close the message queues

	closeMessageQueues();

    
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
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}

