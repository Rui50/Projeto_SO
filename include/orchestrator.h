#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

typedef struct task {
    int uid;
    double time; // time requested | will determine priority
    char *execution_mode;
    char *pid; // requester pid
    char *program;
    char *args; //opc
} TASK;

#endif