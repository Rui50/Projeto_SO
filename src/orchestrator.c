#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../include/orchestrator.h"
#include "../include/requests.h"
#include "../include/execute.h"

#define MFIFO "../tmp/mfifo"
#define TASKS_FILE "tasks.txt"

void parseRequest(char *request, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, int *uid, char *output_folder){

    printf("\nreceived a request for parsing %s\n\n", request);

    char *pid = strsep(&request, ";");
    char *mode = strsep(&request, ";");

    if(strcmp(mode, "execute") == 0){
        TASK *newTask = createTask(pid, request, uid);
        printf("task time after creating: %f\n", newTask->time);
        int status = addTask(queue, newTask);
        if (status == 0){
            //printf("TASK INSERTED INTO THE QUEUE\n");
            returnIdToClient(pid, newTask->uid);
            printQueueTimes(queue);
        }
        else {
            printf("ERROR INSERTING TASK INTO THE QUEUE\n");
        }
    }
    else if (strcmp(mode, "status") == 0){
        printf("STATUS COMMAND EXECUTED\n");
        
        sendStatusToClient(pid, queue, running_tasks, parallel_tasks, output_folder);
    }

    else if (strcmp(mode, "terminated_task") == 0){

        int task_uid = atoi(strsep(&request, ";"));

        printf("terminated task had uid %d\n", task_uid);

        for (int i = 0; i < parallel_tasks; i++){
            if (running_tasks[i] != NULL) {
                if(running_tasks[i]->uid == task_uid){
                    printf("task uid %d removed from running tasks\n", running_tasks[i]->uid);

                    // METER AQUI O PID CERTO
                    printf("A recolher o pid %d\n", atoi(pid));
                    waitpid(atoi(pid), NULL, 0);

                    freeTask(&running_tasks[i]);
                    running_tasks[i] = NULL;
                }
            }
        }
    }
    else{
        printf("Unknown Type of Request\n");
    }
}


void returnIdToClient(char *pid, int uid){
    char fifoName[12];
    char data_buffer[10];
    sprintf(fifoName, "fifo_%s", pid);

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1){
        perror("Error opening return fifo: \n");
    }

    // mensagem com o uid
    snprintf(data_buffer, sizeof(data_buffer), "%d", uid);

    // enviar id
    if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
        perror("Error writing to FIFO");
    }
    close(fd);
}

void sendStatusToClient(char *pid, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder) {
    char executing[MAX_SIZE/3];
    char scheduled[MAX_SIZE/3];
    char completed[MAX_SIZE/3];
    char messageBUFFER[MAX_SIZE];

    strcpy(executing, "Executing\n");
    strcpy(scheduled, "Scheduled\n");
    strcpy(completed, "Completed\n");

    // tasks a ser executadas
    for (int i = 0; i < parallel_tasks; i++) {
        if (running_tasks[i] != NULL) {
            int task_uid = running_tasks[i]->uid;
            char *program = running_tasks[i]->program;
            char taskDetails[100];
            snprintf(taskDetails, sizeof(taskDetails), "%d %s\n", task_uid, program);
            strcat(executing, taskDetails);
        }
    }

    // na queue
    for (int i = 0; i < queue->size; i++) {
        if (queue->tasks[i] != NULL) {
            int task_uid = queue->tasks[i]->uid;
            char *program = queue->tasks[i]->program;
            char taskDetails[100];
            snprintf(taskDetails, sizeof(taskDetails), "%d %s\n", task_uid, program);
            strcat(scheduled, taskDetails);
        }
    }

    // tarefas acabadas
    char log_file_path[1024];
    snprintf(log_file_path, sizeof(log_file_path), "../%s/task_logs.txt", output_folder);
    int outputfd = open(log_file_path, O_RDONLY);

    LOG log;
    ssize_t bytesRead;

    while ((bytesRead = read(outputfd, &log, sizeof(LOG))) == sizeof(LOG)) {
        char taskDetails[512];
        snprintf(taskDetails, sizeof(taskDetails), "%d %f %s\n", log.task_uid, log.time_to_execute, log.program_name);
        strcat(completed, taskDetails);
    }

    snprintf(messageBUFFER, MAX_SIZE, "%s\n%s\n%s", executing, scheduled, completed);

    char fifoName[12];
    sprintf(fifoName, "fifo_%s", pid);

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1){
        perror("Error opening return fifo: \n");
    }

    if(write(fd, messageBUFFER, strlen(messageBUFFER)) == -1){
        perror("Error writing to FIFO");
    }

    close(fd);
    
    printf("%s", messageBUFFER);
}

void sendTerminatedTask(TASK *terminatedTask, pid_t pid){
    int fd =  open(MFIFO, O_WRONLY);
    if (fd == -1){
        perror("Erro a abrir fifo principal");
    }
    
    char data_buffer[MAX_SIZE];
    snprintf(data_buffer, MAX_SIZE, "%d;terminated_task;%d", pid, terminatedTask->uid);

    if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
            perror("Error writing to FIFO");
    }

    close(fd);
}


void checkTasks(TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder) {
    for (int i = 0; i < parallel_tasks; i++) {
        //verificar se a posicao ta vazia
        if (running_tasks[i] == NULL && !isQueueEmpty(queue)) {
            running_tasks[i] = getNextTask(queue);
            printf("\n\nAvaliable index: %d\n", i);
            printf("New task inserted  into the running_tasks with uid %d\n\n", running_tasks[i]->uid);

            if(strcmp(running_tasks[i]->execution_mode, "-u") == 0){
                pid_t pid = fork();

                if (pid == 0){
                    executeSingleTask(running_tasks[i]->pid, running_tasks[i], output_folder, getpid());
                    _exit(0);
                }
                else {
                    // arranjar solucão para os pids
                    printf("Started executing program %s with uid %d\n", running_tasks[i]->program, running_tasks[i]->uid);
                }
            }
            else if(strcmp(running_tasks[i]->execution_mode, "-l") == 0){
                printf("PIPELINE %d PROGRAM %s\n", running_tasks[i]->uid, running_tasks[i]->program);
            }
        }
    }
}

int main(int argc, char **argv){
    if(argc != 4){
        printf("Usage:  ./orchestrator output_folder parallel-tasks sched-policy\n");
        return 1;
    }

    TaskPriorityQueue queue;
    initQueue(&queue);

    int uid = 0;

    // data parsing
    char *outputs_folder = argv[1];
    int parallel_tasks = atoi(argv[2]);
    printf("Parellel_tasks: %d\n", parallel_tasks);

    // Array que vai conter as tasks em execução
    TASK **running_tasks = malloc(parallel_tasks * sizeof(TASK*));
    for (int i = 0; i < parallel_tasks; i++) {
        running_tasks[i] = NULL;
    }

    if (mkfifo(MFIFO, 0660) == -1) {
        perror("Failed to create main FIFO: \n");
    }

    int fd = open(MFIFO, O_RDONLY);
    if (fd == -1) {
        perror("Error opening FIFO: \n");
        return 1;
    }


    // para evitar a espera ativa
    pid_t pid_block = fork();

    if (pid_block == 0){
        int fd_block = open(MFIFO, O_WRONLY);
        if(fd_block == -1){
            perror("Erro ao abrir o FAKE_FIFO");
            exit(1);
        }

        while(1){
            sleep(10);
        }
        _exit(0);
    }

    ssize_t bytesread;
    char request[MAX_SIZE];

    while(1){
        // espera ativa
        bytesread = read(fd, &request, sizeof(request));
        request[bytesread] = '\0';
        
        if (strlen(request) > 0){
            //printf("request received:  %s\n", request);
            parseRequest(request, &queue, running_tasks, parallel_tasks, &uid, outputs_folder);
        }

        checkTasks(&queue, running_tasks, parallel_tasks, outputs_folder);
    }
    return 0;
}