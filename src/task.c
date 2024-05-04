#include "../include/task.h"
 
void freeTask(TASK **task) {
    if (task == NULL || *task == NULL) return;

    free((*task)->pid);
    free((*task)->program);
    free((*task)->execution_mode);
    free((*task)->args);

    free(*task);
}

TASK *createTask(char *pid, char *request, int *uid){
    TASK *task = (TASK*)malloc(sizeof(TASK));

    char *time = strsep(&request, ";");
    char *mode = strsep(&request, ";");
    char *program = strsep(&request, ";");
    char *args = request;

    task->uid = ++(*uid);
    printf("New Task UID: %d\n",task->uid);
    task->pid = strdup(pid);    
    task->time = strtod(time, NULL);
    printf("TIME OF THE TASK: %f", task->time);
    task->program = strdup(program);
    task->execution_mode = strdup(mode);
    task->args = args ? strdup(args) : strdup(""); // caso de nao ter argumentos

    printf("Task created for program: %s\n", task->program);

    return task;
}