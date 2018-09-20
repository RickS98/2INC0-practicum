/* 
 * Operating Systems  [2INCO]  Practical Assignment
 * Interprocess Communication
 *
 * Contains definitions which are commonly used by the farmer and the workers
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include "uint128.h"

// maximum size for any message in the tests
#define MAX_MESSAGE_LENGTH	6
#define VARIABLE_MESSAGE_LENGTH 3
#define CONSTANT_MESSAGE_LENGTH MAX_MESSAGE_LENGTH-VARIABLE_MESSAGE_LENGTH

// TODO: put your definitions of the datastructures here

struct JOBMESSAGE {

	int killYourself;

    int hashIndex;
    char startLetters[CONSTANT_MESSAGE_LENGTH+1];
    uint128_t hash;

};


struct RESULTMESSAGE{

	int suicideNote;

    int hashIndex;
    char correctString[MAX_MESSAGE_LENGTH+1] ;

};


#endif

