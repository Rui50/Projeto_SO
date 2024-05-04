#include "../include/execute.h"

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