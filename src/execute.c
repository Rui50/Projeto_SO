#include "../include/execute.h"

#include <ctype.h>

char *fix_string(char *str) {
    char *end;

    while(isspace((unsigned char)*str)) str++;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

void executePipelineTask(char *requester_pid, TASK *task, char *outputs_folder, pid_t pid_to_collect) {
    char output_path[MAX_SIZE];
    snprintf(output_path, sizeof(output_path), "../%s/task_%d.txt", outputs_folder, task->uid);

    int outputFD = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0660);
    if (outputFD == -1) {
        perror("Error creating output file");
        return;
    }

    char *programs = strdup(task->program);
    char *args = strdup(task->args);
    char *programList[128];
    char *argsList[128];
    char *token;
    int num_tasks = 0;
    while ((token = strsep(&programs, "|")) != NULL) {
        token =  fix_string(token);
        programList[num_tasks++] = strdup(token);
    }
    int num_args = 0;
    while ((token = strsep(&args, "|")) != NULL) {
        token =  fix_string(token);
        argsList[num_args++] = strdup(token);
    }


    // para debug pipeline
    printf("Program list:\n");
    for (int i = 0; i < num_tasks; i++) {
        printf("%d: %s\n", i, programList[i]);
    }
    printf("Arguments list:\n");
    for (int i = 0; i < num_args; i++) {
        printf("%d: %s\n", i, argsList[i]);
    }
    int pipes[num_tasks - 1][2];
    pid_t pids[num_tasks];

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < num_tasks; i++) {
        printf("Executing: %s, with args: %s\n", programList[i], argsList[i]);

        if (i < num_tasks - 1) {
            if (pipe(pipes[i]) == -1) {
                perror("Pipe failed");
                return;
            }
        }

        pids[i] = fork();

        if (pids[i] == 0) { 
            if (i == 0) {
                // Primeiro programa, stdout para a extremidade de escrita do pipe
                close(pipes[i][0]);
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][1]);

            } else if (i == num_tasks - 1) {
                
                // ultima "task", le do ultimo pipe
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][1]);
                dup2(outputFD, STDOUT_FILENO);
                dup2(outputFD, STDERR_FILENO);

            } else {

                // Intermediários, leem da extremidade de leitura do pipe anterior e escrevem no prox
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][1]);
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
            }

            execlp(programList[i], programList[i], argsList[i], NULL);
            perror("execlp failed");  
            exit(EXIT_FAILURE);
        } else {
            if (i != 0) {
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
        }
    }

    close(outputFD); 

    for (int i = 0; i < num_tasks; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }

    sendTerminatedTask(task, pid_to_collect);

    gettimeofday(&end_time, NULL);

    long seconds = end_time.tv_sec - start_time.tv_sec;
    long useconds = end_time.tv_usec - start_time.tv_usec;
    double elapsed = seconds + useconds*1e-6;

    printf("TIME TO EXECUTE: %f\n", elapsed);

    // Abrir/criar os logs das tasks executadas no output_folder
    char log_file_path[1024];
    snprintf(log_file_path, sizeof(log_file_path), "../%s/task_logs.txt", outputs_folder);
    int fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("Failed to open log file: ");
        return;
    }

    LOG log;

    log.task_uid = task->uid;
    log.time_to_execute = elapsed;
    snprintf(log.program_name, sizeof(log.program_name), "%s", task->program);

    if (write(fd, &log, sizeof(LOG)) != sizeof(LOG)) {
        perror("Failed to write log to file");
    }

    close(fd);

    // Free resources
    free(programs);
    free(args);
    for (int i = 0; i < num_tasks; i++) {
        free(programList[i]);
        free(argsList[i]);
    }
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
            perror("Error creating/opening output_file: ");
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

        LOG log;

        log.task_uid = task->uid;
        log.time_to_execute = elapsed;
        snprintf(log.program_name, sizeof(log.program_name), "%s", task->program);

        printf("WRITE UID: %d | TIME: %f | PROGRAM: %s\n", log.task_uid, log.time_to_execute, log.program_name);

        if (write(fd, &log, sizeof(LOG)) != sizeof(LOG)) {
            perror("Failed to write log to file");
        }

        close(fd);
    }
}