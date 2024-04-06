#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#define MAX_SIZE 1000

typedef struct task {
    int uid;
    double time; // time requested | will determine priority
    char *pid; // requester pid
    char *program;
    char *args;
    char *output_path;
} TASK;

#endif