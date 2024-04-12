#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "../include/orchestrator.h"
#include "../include/taskQueue.h"

#define MAX_SIZE 1000
#define MFIFO "../tmp/mfifo"
#define TASKS_FILE "tasks.txt"

int uid = 1;

TASK *createTask(char *pid, char *request){
    TASK *task = (TASK*)malloc(sizeof(TASK));

    char *time = strsep(&request, ";");
    char *mode = strsep(&request, ";");
    char *program = strsep(&request, ";");
    char *args = strsep(&request, ";");

    task->uid = uid;
    uid++;
    task->pid = strdup(pid);    
    task->time = strtod(time, NULL);
    task->program = strdup(program);
    task->args = strdup(args);

    // output path
    /*char *file_extension = ".txt";
    char *output_path = strdup(program);
    strcat(output_path, file_extension);
    task->output_path = program;*/

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
    sprintf(fifoName, "fifo_%s", pid);

    // função que pega nas tarefas da queue etc 
}

void returnIdToClient(char *pid, int uid){
    char fifoName[12];
    char data_buffer[10];
    sprintf(fifoName, "fifo_%s", pid);
    printf("RETURN FIFO = %s\n", fifoName);


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

void parseRequest(char *request, TaskPriorityQueue *queue){
    char *pid = strsep(&request, ";");
    char *mode = strsep(&request, ";");

    if(strcmp(mode, "execute") == 0){
        TASK *newTask = createTask(pid, request);
        int status = addTask(queue, newTask);
        if (status == 0){
            printf("TASK INSERTED INTO THE QUEUE\n");
            returnIdToClient(pid, newTask->uid);
            printQueueTimes(queue);
        }
        else {
            printf("ERROR INSERTING TASK INTO THE QUEUE\n");
        }
    }
    else if (strcmp(mode, "status") == 0){
        printf("STATUS COMMAND EXECUTED\n");
        if (queue->size > 0) {
            TASK *test = getNextTask(queue);
            double timer = test->time;
            printf("TIMER REMOVER = %f\n", timer);
        } else {
            printf("The queue is empty.\n");
        }
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
    // filho vai tratar das executions, pai dos requests

    pid_t pid = fork();

    if (pid == 0){
        while (1){
            printf("SIZE %d\n", queue.size);
            if (!isQueueEmpty(&queue)) {
                TASK *t = getNextTask(&queue);
                if(t != NULL){ // erro
                    printf("t-PROGRAM = %s\n", t->program);
                }
                else{
                    printf("null task\n");
                }
            }
            sleep(1);
        }
    }
    else{
        while((bytesRead = read(fd, request, MAX_SIZE - 1)) != -1){
            if(bytesRead > 0){
                printf("Received: %s\n", request);
                parseRequest(request, &queue);
            }
        }
        close(fd);
    }

    return 0;
}