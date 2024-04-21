#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../include/orchestrator.h"
#include "../include/taskQueue.h"

#define MFIFO "../tmp/mfifo"
#define TASKS_FILE "tasks.txt"

int uid = 1;

TASK *createTask(char *pid, char *request){
    TASK *task = (TASK*)malloc(sizeof(TASK));

    char *time = strsep(&request, ";");
    char *mode = strsep(&request, ";");
    char *program = strsep(&request, ";");
    char *args = request;

    task->uid = uid;
    uid++;
    task->pid = strdup(pid);    
    task->time = strtod(time, NULL);
    task->program = strdup(program);
    task->args = args ? strdup(args) : strdup(""); // caso de nao ter argumentos

    printf("Task request PID: %s\n", task->pid);
    printf("Time: %f\n", task->time);
    printf("Program: %s\n", task->program);
    printf("Args: %s\n", task->args);

    return task;
}

void executeSingleTask(char *requester_pid, TASK *task, char *outputs_folder){
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    pid_t pid = fork();

    if(pid == 0){

        // ouput path com base no uid da task

        char output_path[MAX_SIZE];
        snprintf(output_path, sizeof(output_path), "../%s/task_%d.txt", outputs_folder, task->uid);

        int outputFD = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0660);
        if(outputFD == -1){
            perror("Erro a criar ficheiro de output: ");
        }

        // redirect stdout para o output_path
        int out = dup2(outputFD, STDOUT_FILENO);
        if (out == -1){
            perror("Erro dup stdout: ");
            close(outputFD);
        }

        // redirect stderr para o output_path
        int err = dup2(outputFD, STDERR_FILENO);
        if (err == -1){
            perror("Erro dup stderr: ");
            close(outputFD);
        }


        execlp(task->program, task->program, task->args, NULL);
        _exit(0);
    }
    else{
        int status;
        waitpid(pid, &status, 0);

        gettimeofday(&end_time, NULL);

        long seconds = end_time.tv_sec - start_time.tv_sec;
        long useconds = end_time.tv_usec - start_time.tv_usec;
        double elapsed = seconds + useconds*1e-6;

        printf("TIME TO EXECUTE: %f\n", elapsed);

        // Abrir/criar os logs das tasks executadas no output_folder
        char log_file_path[1024];
        snprintf(log_file_path, sizeof(log_file_path), "../%s/task_logs.txt", outputs_folder);
        printf("LOG FILES PATH %s\n", log_file_path);
        int fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd == -1) {
            perror("Failed to open log file: ");
            return;
        }

        // String log
        char log_entry[256];
        int log_entry_size = snprintf(log_entry, sizeof(log_entry), "Task ID: %d, Time: %f seconds\n", task->uid, elapsed);

        if (write(fd, log_entry, log_entry_size) != log_entry_size) {
            perror("Failed to write log");
        }

        close(fd);
    }
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


void executeTask(TASK *task, char *outputs_folder) {
    pid_t pid = fork();
    if (pid == 0) { // Child process
        executeSingleTask(task->pid, task, outputs_folder);
        exit(0);
    }
    // Pai nao espera | evitar espera no ciclo | temos de dar fix
}

// vai ter um fifo principal que lida com os requests
// outro fifo para cada cliente


int main(int argc, char **argv){
    if(argc != 4){
        printf("Usage:  ./orchestrator output_folder parallel-tasks sched-policy\n");
        return 1;
    }

    // Initiate vars
    TaskPriorityQueue queue;
    char request[MAX_SIZE];
    ssize_t bytesReadPIPE;

    //iniciar a queue
    initQueue(&queue);

    // data parsing
    char *outputs_folder = argv[1];
    int parallel_taks = atoi(argv[2]);
    //int sched_policy = atoi(argv[3]);

    if (mkfifo(MFIFO, 0660) == -1) {
        perror("Failed to create main FIFO: \n");
    }

    int fd = open(MFIFO, O_RDONLY);
    if (fd == -1) {
        perror("Error opening FIFO: \n");
        return 1;
    }

    int fdpipe[2];
    if (pipe(fdpipe) == -1) {
        perror("Pipe failed: \n");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed: \n");
        return 1;
    }

    if (pid == 0) { // Child process
        close(fdpipe[0]); // Close read end in child
    
        char request[MAX_SIZE];
        while (1) {
            ssize_t bytesRead = read(fd, request, MAX_SIZE - 1);
            if (bytesRead > 0) {
                request[bytesRead] = '\0';
                write(fdpipe[1], request, bytesRead + 1);
            } else {
                // No data read, send a heartbeat
                const char* heartbeat = "HEARTBEAT";
                write(fdpipe[1], heartbeat, strlen(heartbeat) + 1);
                sleep(1); // Sleep for a bit to prevent tight looping
            }
        }
        close(fd);
        close(fdpipe[1]);
        exit(0);
    } else { 
        // Processo Pai
        // Vai receber os requests que o filho leu e verificar se existem elementos na queue

        close(fdpipe[1]); // Fechar escrita do pipe

        char buffer[MAX_SIZE];
        while (1) {
            // como pai desbloquear do read
            // so vai executar quando ler alguma coisa, senao fica la preso
            ssize_t bytesRead = read(fdpipe[0], buffer, MAX_SIZE);
            if (bytesRead > 0) {
                if (strcmp(buffer, "HEARTBEAT") == 0) {
                    // Just a heartbeat, nothing to do
                    continue;
                }
                printf("Received request from child: %s\n", buffer);
                parseRequest(buffer, &queue);
            }

            // Check and process tasks from the queue
            if (!isQueueEmpty(&queue)) {
                TASK *task = getNextTask(&queue);
                printf("EXECUTING PROGRAM %s\n", task->program);
                executeTask(task, outputs_folder);
            }
        }

        close(fd);
        close(fdpipe[0]);
    }

    return 0;
}


/*int main(int argc, char **argv){
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
}*/