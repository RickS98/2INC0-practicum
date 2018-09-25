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
#define MAX_MESSAGE_LENGTH  (6)
 
//#define ALPHABET_SIZE     (6)
// #define NROF_WORKERS     (7)
// #define MQ_MAX_MESSAGES      (10)
#define NUM_MD5s            (6)

//#define PREAMBLE          ((uint8_t)((log(NROF_WORKERS)/log(MAX_MESSAGE_LENGTH))+0.5))  // smart compilers resolve this compile time

#define MAX_WRDS            (pow(MAX_MESSAGE_LENGTH,ALPHABET_NROF_CHAR)) //total number of words possible for given msg len and alpha size.

// TODO: put your definitions of the datastructures here
typedef struct
{
    // a data structure with 3 members
    uint128_t               hash;
    char                    word[MAX_MESSAGE_LENGTH];
    uint8_t                 preamble_len;
    char                    ctrl;
    uint8_t                 alphabet_nrof_char;
} MQ_REQUEST_MESSAGE;

typedef struct
{
    // a data structure with 3 members
    int                     ctrl;
    char                    str[64+5];//

    pid_t                   pid; // for degugging
} MQ_RESPONSE_MESSAGE;

enum {
	RUN = 0,
	MATCH = 1,
	NEWHASH = 2,
	TERM = 13
};


#define STUDENT_NAME        "Tom_Buskens_Robert_Jongalock"

#endif

