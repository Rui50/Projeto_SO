#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../include/orchestrator.h"
#include "../include/task.h"

#define MFIFO "../tmp/mfifo"
#define TASKS_FILE "tasks.txt"

//int uid = 1;
//TASK **running_tasks;

void freeTask(TASK **task) {
    if (task == NULL || *task == NULL) return;

    free((*task)->pid);
    free((*task)->program);
    free((*task)->execution_mode);
    free((*task)->args);

    free(*task);
}
// INITIAL INTERACTION FUNCTIONS
/**
 * 
 * 
*/
TASK *createTask(char *pid, char *request, int *uid){
    TASK *task = (TASK*)malloc(sizeof(TASK));

    char *time = strsep(&request, ";");
    char *mode = strsep(&request, ";");
    char *program = strsep(&request, ";");
    char *args = request;

    task->uid = ++(*uid);
    printf("New Task UID: %d",task->uid);
    task->pid = strdup(pid);    
    task->time = strtod(time, NULL);
    task->program = strdup(program);
    task->execution_mode = strdup(mode);
    task->args = args ? strdup(args) : strdup(""); // caso de nao ter argumentos

    printf("Task created for program: %s\n", task->program);

    return task;
}

void parseRequest(char *request, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, int *uid, char *output_folder){

    printf("\nreceived a request for parsing %s\n\n", request);

    char *pid = strsep(&request, ";");
    char *mode = strsep(&request, ";");

    if(strcmp(mode, "execute") == 0){
        TASK *newTask = createTask(pid, request, uid);
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
    //printf("returning uid should be: %d\n", &uid)
    sprintf(fifoName, "fifo_%s", pid);
    //printf("RETURN FIFO = %s\n", fifoName);


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

    // taefas acabadas
    printf("output_folder %s\n", output_folder);
    char log_file_path[1024];
    snprintf(log_file_path, sizeof(log_file_path), "../%s/task_logs.txt", output_folder);
    printf("path: %s\n", log_file_path);
    int outputfd = open(log_file_path, O_RDONLY);

    char buffer[1024];

    int bytesRead = read(output_folder, buffer, sizeof(buffer));

    printf("%d bytes read!\n", bytesRead);
    printf("File Contents: %s\n", buffer);

    close(outputfd);

    snprintf(messageBUFFER, MAX_SIZE, "%s\n%s\n%s", executing, scheduled, completed);

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

void executePipelineTask(char *requester_pid, TASK *task, char *outputs_folder){
    
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
    
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    //preciso de função para obter numero de tasks a executar
    int num_tasks = 0;

    int pipes[num_tasks-1][2];

    pid_t pid;

    for(int i = 0; i < num_tasks; i++){
        
        //primeira task
        if(i == 0){
            pipe(pipes[i]);

            pid = fork();

            if(pid == 0){
                //fechar leitura
                close(pipes[i][0]);
                dup2(pipes[i][1], 1); //para stdout

                //dup2(err, 2);
                //close(err);
                // executar, temos que fazer o parsing da linha antes por causa dos parametros
            }
            close(pipes[i][1]);
            //fechar sderr    
        }

        // tasks intermediarias
        else if(i != num_tasks - 1) {
            pipe(pipes[i]);

            pid = fork();

            if(pid == 0){
                //fechar leitura
                close(pipes[i][0]);
                dup2(pipes[i-1][0], 0);
                close(pipes[i-1][0]);

                dup2(pipes[i][1],1);
                close(pipes[i][1]);

                //dup2(err,2);
                //close(err);

                //executar
            }

            close(pipes[i-1][0]);
            close(pipes[i][1]);
            //close(err);
        }

        if(i == num_tasks - 1){
            dup2(pipes[i-1][0],0);
            close(pipes[i-1][0]);

            //dup2(out,1);
            //close(out);

            //dup2(err,2);
            //close(err);

            // executar
        }

        close(pipes[i-1][0]);
        //close(out)
        //close(err)
    }

    //falta waits etc

}

void executeSingleTask(char *requester_pid, TASK *task, char *outputs_folder, pid_t pid_to_collect){
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    printf("pid para recollha %d\n", pid_to_collect);
    
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

        close(out);
        close(err);
        _exit(0);
    }
    else{
        int status;
        waitpid(pid, &status, 0);

        sendTerminatedTask(task, pid_to_collect);

        gettimeofday(&end_time, NULL);

        long seconds = end_time.tv_sec - start_time.tv_sec;
        long useconds = end_time.tv_usec - start_time.tv_usec;
        double elapsed = seconds + useconds*1e-6;

        printf("TIME TO EXECUTE: %f\n", elapsed);

        // Abrir/criar os logs das tasks executadas no output_folder
        char log_file_path[1024];
        snprintf(log_file_path, sizeof(log_file_path), "../%s/task_logs.txt", outputs_folder);
        //printf("LOG FILES PATH %s\n", log_file_path);
        int fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd == -1) {
            perror("Failed to open log file: ");
            return;
        }

        // String log
        char log_entry[256];
        int log_entry_size = snprintf(log_entry, sizeof(log_entry), "Task ID: %d, Time: %f seconds, Program: %s\n", task->uid, elapsed, task->program);

        if (write(fd, log_entry, log_entry_size) != log_entry_size) {
            perror("Failed to write log");
        }

        close(fd);
    }
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

    ssize_t bytesread;
    char request[MAX_SIZE];

    while(1){
        // espera ativa
        bytesread = read(fd, &request, sizeof(request));
        request[bytesread] = '\0';
        
        if (strlen(request) > 0){
            printf("request received:  %s\n", request);
            parseRequest(request, &queue, running_tasks, parallel_tasks, &uid, outputs_folder);
        }

        checkTasks(&queue, running_tasks, parallel_tasks, outputs_folder);
    }
    return 0;
}