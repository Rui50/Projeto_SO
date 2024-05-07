#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../include/orchestrator.h"
#include "../include/execute.h"

#define MFIFO "../tmp/mfifo"
#define TASKS_FILE "tasks.txt"

void parseRequest(REQUEST *request, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, int *uid, char *output_folder){

    switch (request->type){

        case EXECUTE:
            TASK *newTask = createTask(request->pid_requester, request->requestArgs, uid);
            int status = addTask(queue, newTask);
            if (status == 0){
                returnIdToClient(request->pid_requester, newTask->uid);
                printQueueTimes(queue);
            }
            else{
                printf("Error inserting task into the queue\n");
            }
            break;

        case STATUS:

            pid_t pid = fork();

            if (pid == 0){
                sendStatusToClient(request->pid_requester, queue, running_tasks, parallel_tasks, output_folder, pid);
                _exit(0);
            }
            break;

        case TERMINATED_TASK:
            int task_uid = atoi(request->requestArgs);

            for (int i = 0; i < parallel_tasks; i++){
                if (running_tasks[i] != NULL) {
                    if(running_tasks[i]->uid == task_uid){
                        printf("Task with uid %d was removed from running tasks\n", running_tasks[i]->uid);

                        waitpid(request->pid_requester, NULL, 0);

                        freeTask(&running_tasks[i]);
                        running_tasks[i] = NULL;
                    }
                }
            }
            break;

        case TERMINATED_STATUS:
            waitpid(request->pid_requester, NULL, 0);

        default:
            break;
    }
}

/**
 * Função responsavel por enviar o uid ta task ao cliente que fez o request
*/
void returnIdToClient(int pid, int uid){
    char fifoName[12];
    char data_buffer[10];
    sprintf(fifoName, "fifo_%d", pid);

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1){
        perror("Error opening return fifo: \n");
    }

    snprintf(data_buffer, sizeof(data_buffer), "%d", uid);

    // enviar id
    if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
        perror("Error writing to FIFO");
    }
    close(fd);
}

void sendTerminatedStatus(pid_t pid){
    int fd =  open(MFIFO, O_WRONLY);
    if (fd == -1){
        perror("Erro a abrir fifo principal");
    }

    REQUEST *request = malloc(sizeof(REQUEST));
    
    request->type = TERMINATED_STATUS;
    request->pid_requester = pid;

    if(write(fd, request, sizeof(REQUEST)) == -1){
        perror("Error writing terminated task: \n");
        close(fd);
    }
    close(fd);
}


/**
 * Função faz a construção da mensagem com o status do programa
 * Para as tasks que estão a executar usa o array de running_tasks que contem as tasks que estão a decorrer
 * Para as tasks que estão scheduled consulta a queue
 * Para as tasks concluidas lê do ficheiro task_logs que contem as logs serializadas das tarefas
 * 
*/
void sendStatusToClient(int pid, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder, pid_t pid_to_collect) {
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

    // mensagem final
    snprintf(messageBUFFER, MAX_SIZE, "%s\n%s\n%s", executing, scheduled, completed);

    char fifoName[12];
    sprintf(fifoName, "fifo_%d", pid);

    int fd = open(fifoName, O_WRONLY);
    if (fd == -1){
        perror("Error opening return fifo: \n");
    }

    if(write(fd, messageBUFFER, strlen(messageBUFFER)) == -1){
        perror("Error writing to FIFO");
    }

    close(fd);
    sendTerminatedStatus(pid_to_collect);
    printf("%s", messageBUFFER);
}

/**
 * Envia uma mensagem através do fifo principal a avisar que uma task terminou
 * Isto "acorda" o read para para voltar a executar a prox task
*/
void sendTerminatedTask(TASK *terminatedTask, pid_t pid){
    int fd =  open(MFIFO, O_WRONLY);
    if (fd == -1){
        perror("Erro a abrir fifo principal");
    }

    REQUEST *request = malloc(sizeof(REQUEST));
    
    request->type = TERMINATED_TASK;
    request->pid_requester = pid;
    snprintf(request->requestArgs, REQUEST_ARGS, "%d", terminatedTask->uid);

    if(write(fd, request, sizeof(REQUEST)) == -1){
        perror("Error writing terminated task: \n");
        close(fd);
    }
    close(fd);
}

void checkTasks(TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder) {
    for (int i = 0; i < parallel_tasks; i++) {
        //verificar se a posicao ta vazia
        if (running_tasks[i] == NULL && !isQueueEmpty(queue)) {
            running_tasks[i] = getNextTask(queue);
            printf("New task inserted into running_tasks, task uid: %d\n", running_tasks[i]->uid);

            if(strcmp(running_tasks[i]->execution_mode, "-u") == 0){
                pid_t pid = fork();

                if (pid == 0){
                    executeSingleTask(running_tasks[i]->pid, running_tasks[i], output_folder, getpid());
                    _exit(0);
                }
                else {
                    printf("Started executing program %s with uid %d\n", running_tasks[i]->program, running_tasks[i]->uid);
                }
            }
            else if(strcmp(running_tasks[i]->execution_mode, "-p") == 0){

                pid_t pid = fork();
                
                if (pid == 0){
                    executePipelineTask(running_tasks[i]->pid, running_tasks[i], output_folder, getpid());
                    _exit(0);
                }
                else{
                    printf("Started executing tasks %s with uid %d\n", running_tasks[i]->program, running_tasks[i]->uid);
                }
            }
        }
    }
}

int main(int argc, char **argv){
    if(argc != 3){
        /*sched-policy nao foi implementado*/
        printf("Usage:  ./orchestrator output_folder parallel-tasks\n");
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
            perror("Error opening fd: \n");
            exit(1);
        }

        while(1){
            sleep(10);
        }
        _exit(0);
    }

    ssize_t bytesread;
    REQUEST *request = malloc(sizeof(REQUEST));

    while((bytesread = read(fd, request, sizeof(REQUEST))) > 0){
        /*if (bytesread < 0){
            perror("Erro na leitura de requests: \n");
        }*/
        
        parseRequest(request, &queue, running_tasks, parallel_tasks, &uid, outputs_folder);

        checkTasks(&queue, running_tasks, parallel_tasks, outputs_folder);
    }
    return 0;
}