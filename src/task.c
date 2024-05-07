#include "../include/task.h"
 
 /**
  * Libertar memória alocada para a task
 */
void freeTask(TASK **task) {
    free((*task)->pid);
    free((*task)->program);
    free((*task)->execution_mode);
    free((*task)->args);

    free(*task);
}

/**
 * Cria uma task 
*/
TASK *createTask(int pid, char *request, int *uid) {
    TASK *task = (TASK*)malloc(sizeof(TASK));

    printf("request: %s\n", request);
    
    char *time = strsep(&request, ";");
    char *mode = strsep(&request, ";");
    
    char programs[1024] = {0}; 
    char arguments[1024] = {0};

    // Modo de execução -u | -p
    task->execution_mode = strdup(mode);

    char *token;
    while ((token = strsep(&request, "|"))) {
        char *program = strtok(token, " ");
        char *args = strtok(NULL, "");

        if (programs[0] != '\0') {
            strcat(programs, " | ");
            strcat(arguments, " | ");
        }
        
        strcat(programs, program ? program : "");
        strcat(arguments, args ? args : "");
    }

    task->program = strdup(programs);
    task->args = strdup(arguments);

    char pid_str[12];
    sprintf(pid_str, "%d", pid);

    task->uid = ++(*uid);
    task->pid = strdup(pid_str);
    task->time = strtod(time, NULL);

    printf("Task sucessfully created: uid %d, pid %s, Time %.1f, Execution Mode: %s, Programs: %s, Args: %s\n",
           task->uid, task->pid, task->time, task->execution_mode, task->program, task->args);

    return task;
}