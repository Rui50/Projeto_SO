#ifndef REQUESTS_H
#define REQUESTS_H

#include "../include/orchestrator.h"

#define PROGNAME 100

typedef struct request {
    int pid_requester;
    int mode;
} REQUEST;

typedef struct log {
    int task_uid;
    double time_to_execute;
    char program_name[PROGNAME];
} LOG;



#endif