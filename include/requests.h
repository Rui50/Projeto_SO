#ifndef REQUESTS_H
#define REQUESTS_H

#define PROGNAME 100
#define REQUEST_ARGS 518

typedef enum{
    EXECUTE,
    STATUS,
    TERMINATED_TASK,
    TERMINATED_STATUS
} TYPE;

typedef struct request {
    int pid_requester;
    TYPE type;
    char requestArgs[REQUEST_ARGS];
} REQUEST;

typedef struct log {
    int task_uid;
    double time_to_execute;
    char program_name[PROGNAME];
} LOG;


#endif