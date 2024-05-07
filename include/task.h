#ifndef TASK_H
#define TASK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct task {
    int uid;
    char *type; // tipo, status, execute
    double time; // time requested | will determine priority
    char *execution_mode;
    char *pid; // requester pid
    char *program;
    char *args; //opc
} TASK;

TASK *createTask(int pid, char *request, int *uid);
void freeTask(TASK **task);

#endif