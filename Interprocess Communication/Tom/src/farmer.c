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

#include "settings.h"
#include "common.h"

#define STUDENT_NAME "RobertEnTom"

mqd_t queJobDescriptor; //descriptor from the job queue
mqd_t queResultDescriptor; //descriptor from the result queue
mqd_t queResultDescriptorBlocking; //descriptor from the result queue that will block

char queNameJob[80]; //name of job queue
char queNameResult[80]; //name of result queue

struct RESULTMESSAGE currentMessage; //memory location for processing result messages

int foundHashes [MD5_LIST_NROF] = {0}; //array for keeping track of already found hashes
char hashesResult [MD5_LIST_NROF][MAX_MESSAGE_LENGTH+1]; //array for keeping track of all results

//function that opens all message queues needed for the communication
void createMessageQueues()
{
    struct mq_attr attribute;

    attribute.mq_maxmsg = MQ_MAX_MESSAGES;
    attribute.mq_msgsize = sizeof (struct JOBMESSAGE);

    queJobDescriptor = mq_open (queNameJob, O_WRONLY | O_CREAT | O_EXCL, 0600, &attribute);

    attribute.mq_maxmsg = MQ_MAX_MESSAGES;
    attribute.mq_msgsize = sizeof (struct RESULTMESSAGE);
    
    queResultDescriptor = mq_open (queNameResult, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attribute);
    queResultDescriptorBlocking = mq_open (queNameResult, O_RDONLY, 0600, &attribute);

    if(queJobDescriptor < 0 || queResultDescriptor < 0)
    {
        perror("Creating Message Queues failed");
        exit(1);
    } 

    //printf("Message Queue successfully created\n");
}

//function that cleans up all message queues needed for the communication
void cleanMessageQueues()
{
	int returnJobClose = mq_close(queJobDescriptor);
	int returnResultClose = mq_close(queResultDescriptor);

	if(returnJobClose < 0 || returnResultClose < 0)
	{
		perror("Cleaning message queues failed");
		exit(1);
	}

	int returnJobUnlink = mq_unlink (queNameJob);
	int returnResultUnlink = mq_unlink (queNameResult);

	if(returnJobUnlink < 0 || returnResultUnlink < 0)
	{
		perror("unlinking message queues failed");
		exit(1);
	}
	//printf("Message Queue successfully cleaned\n");
}

/*function that creates all child processes and start the worker program
 * The names of the message queues are given as arguments to let them start
 * the communication on their side.
 *
 * But also the start and end letter are supplied because they do not change
 * after this point
 */
void createChildProcesses(int amount)
{
	char startChar[2] = {ALPHABET_START_CHAR};
	char endChar[2] = {ALPHABET_END_CHAR};

	pid_t processID;

	for(int i = 0; i < amount; i++) {
        
		processID = fork();
        
		if(processID < 0)
        {
			perror("Forking of parent process failed");
			exit(1);
        }
		else if (processID == 0)
		{
			//printf("Worker created: %d\n",i);

			execlp ("./worker", queNameJob,queNameResult,startChar,endChar,NULL);

			perror("Exec for child process failed");
			exit(1);
		}

    }

	//printf("Workers successfully created\n");
}

//Function that reads contents of currentMessage and places found values in the correct location
void processResult()
{
	if(currentMessage.suicideNote)
	{
		perror("Child killed itself too soon");
		exit(1);
	}


	foundHashes[currentMessage.hashIndex]=1;
	strcpy(hashesResult[currentMessage.hashIndex],currentMessage.correctString);
	//printf("Result processed: HashIndex %d, CorrectString %s\n",currentMessage.hashIndex, currentMessage.correctString);


}
/*Function that will queue a job dependent of the constant letters to all hashes
 * that are not yet found Whenever something is added to the job que the entire
 * result queue is emptied to prevent deadlock.
 */

void queJob(char startLetters[])
{
	for(int hashIndex=0;hashIndex<MD5_LIST_NROF;hashIndex++)
	{
		if(!foundHashes[hashIndex])
		{
			struct JOBMESSAGE job;

			job.killYourself = 0;
			job.hash = md5_list[hashIndex];
			job.hashIndex = hashIndex;
			strcpy(job.startLetters,startLetters);

			//printf("New job queuing\n");

			mq_send(queJobDescriptor,(char *) &job, sizeof (job), 0);

			//printf("Job Queued: HashIndex %d, startLetters %s, Hash 0x%llx\n",job.hashIndex,job.startLetters,job.hash);


			while(mq_receive (queResultDescriptor, (char *) &currentMessage, sizeof (currentMessage), NULL)>0)
			{
				processResult();
			}
		}
	}
}
/*Recursive funcion to generate letters in constant part of the job
 *If desired length is not yet reached a new letter is generated and
 *inserted as job.
 */

void generateStartLetters(char startLetters[],int currentLength)
{
	if(currentLength > CONSTANT_MESSAGE_LENGTH)
	{
		perror("Message is too long");
		exit(1);
	}
	else
	{

		startLetters[currentLength]='\0';
		queJob(startLetters);


		if(currentLength < CONSTANT_MESSAGE_LENGTH)
		{

			for( char i = ALPHABET_START_CHAR; i<=ALPHABET_END_CHAR;i++)
			{

				startLetters[currentLength] = i;

				generateStartLetters(startLetters,currentLength+1);

				startLetters[currentLength] = '\0';
			}
		}

	}
}

//Main function that fills queue and processses results
void fillQueue()
{
	char startLetters[CONSTANT_MESSAGE_LENGTH+1];

	for(int i=0;i<MD5_LIST_NROF;i++)
	{
		strcpy(hashesResult[i],"");//fill array with '\0' charaters so it can easily checked it is changed
	}


	generateStartLetters(startLetters,0);//All jobs are queued

	//printf("All jobs are queued\n");

	int foundDeathChilds = 0;

	struct JOBMESSAGE job;
	job.killYourself = 1;
	for(int i=0;i<NROF_WORKERS;i++)
	{
		//printf("New kill yourself queuing\n");

		mq_send(queJobDescriptor,(char *) &job, sizeof (job), 0);

		//printf("kill yourself queued\n");

		/*Dead children are tracked to make sure all of them are finished.
		 * Because they only die when they processed a kill yourself job
		 * which is always later then the relevant jobs.
		 */
		int foundDeathChild = 0;

		while(!foundDeathChild)
		{
			/*Because a dying child does not take a new job it is nessesary to wait untill
			 * the suicide note of the child to prevent blocking of the job queue and
			 * therefore a deadlock.
			 */

			mq_receive (queResultDescriptorBlocking, (char *) &currentMessage, sizeof (currentMessage), NULL);

			if(currentMessage.suicideNote)
			{
				foundDeathChilds++;
				foundDeathChild = 1;
			}
			else
			{
				//it's still possible to receive legit results
				processResult();
			}
		}
	}

	//printf("Suicide message is generated for all childs\n");

	/* Now all kill yourself jobs are send it's only neccesary untill
	 * all children died. To be sure all jobs are done
	 */
	while(foundDeathChilds<NROF_WORKERS)
	{
		mq_receive (queResultDescriptorBlocking, (char *) &currentMessage, sizeof (currentMessage), NULL);

		if(currentMessage.suicideNote)
		{
			foundDeathChilds++;
		}
		else
		{
			processResult();
		}
	}

	while ((wait(NULL)) > 0); //wait till all childs are finished with cleaning up


	for(int i=0;i<MD5_LIST_NROF;i++)
	{
		if(*hashesResult[i] == '\0')
		{
			/*When an element has not been update during the process
			 * it has not been found.
			 */
			printf("Not Found\n");
		}
		else
		{
			printf("%s\n", hashesResult[i]);
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
    sprintf (queNameJob, "/JobQue_%s_%d", STUDENT_NAME, getpid());
    sprintf (queNameResult, "/ResultQue_%s_%d", STUDENT_NAME, getpid());
    
    createMessageQueues();

    //  * create the child processes (see process_test() and message_queue_test())
    createChildProcesses(NROF_WORKERS);
    //  * do the farming

    fillQueue();
    //  * wait until the chilren have been stopped (see process_test())
    //  * clean up the message queues (see message_queue_test())

    cleanMessageQueues();

    // Important notice: make sure that the names of the message queues contain your
    // student name and the process id (to ensure uniqueness during testing)
    
    return (0);
}



