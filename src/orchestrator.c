#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "../include/orchestrator.h"
#include "../include/taskQueue.h"

#define MAX_SIZE 500
#define MFIFO "../tmp/mfifo"

int uid = 1;

TASK *createTask(char *pid, char *request){
    TASK *task = (TASK*)malloc(sizeof(TASK));

    char *time = strsep(&request, ";");
    char *mode = strsep(&request, ";");
    char *program = strsep(&request, ";");
    char *args = strsep(&request, ";");

    task->pid = strdup(pid);    
    task->time = strtod(time, NULL);
    task->program = strdup(program);
    task->args = strdup(args);

    printf("Task request PID: %s\n", task->pid);
    printf("Time: %f\n", task->time);
    printf("Program: %s\n", task->program);
    printf("Args: %s\n", task->args);

    return task;
}

void execute(char *pid, char *request){

}

void status(char *pid, char *request){
    char fifoName[12];
    sprintf(fifoName, "fifo_%d", pid);

    // função que pega nas tarefas da queue etc 
}

void parseRequest(char *request, TaskPriorityQueue *queue){
    char *pid = strsep(&request, ";");
    char *mode = strsep(&request, ";");

    if(strcmp(mode, "execute") == 0){
        TASK *newTask = createTask(pid, request);
        int status = addTask(queue, newTask);
        if (status == 0){
            printf("TASK INSERTED INTO THE QUEUE\n");
            printQueueTimes(queue);
        }
        else {
            printf("ERROR INSERTING TASK INTO THE QUEUE\n");
        }
    }
    else if (strcmp(mode, "status") == 0){
        status(pid, request);
    }
    else{
        printf("Unknown Type of Request\n");
    }
}



// vai ter um fifo principal que lida com os requests
// outro fifo para cada cliente
int main(int argc, char **argv){
    //if(argc != 4){
    //    printf("Usage:  ./orchestrator output_folder parallel-tasks sched-policy");
    //}

    // Initiate vars
    TaskPriorityQueue queue;
    char request[MAX_SIZE];
    ssize_t bytesRead;

    //iniciar a queue
    initQueue(&queue);

    // data parsing
    char *outputs_folder = argv[1];
    int parallel_taks = atoi(argv[2]);
    int sched_policy = atoi(argv[3]);

    if (mkfifo(MFIFO, 0660) == -1) {
        perror("Failed to create main FIFO: \n");
    }

    // abrir o fifo para handle de requests
    int fd = open(MFIFO, O_RDONLY); // so vai ler os pedidos 
    if(fd == -1){
        perror("Error error opening FIFO!\n");
    }

    while((bytesRead = read(fd, request, MAX_SIZE - 1)) != -1){
        if(bytesRead > 0){
            printf("Received: %s\n", request);
            parseRequest(request, &queue);
        }
    }

    close(fd);
}