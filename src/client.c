#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_SIZE 500

int main(int argc, char **argv){

    if (argc < 2) {
        printf("Usage: %s <command> [options]\n", argv[0]);
        return -1;
    }


    // vai servir como "uid do cliente"
    int mypid = getpid(); 

    char *com_fifo = malloc(MAX_SIZE);
    snprintf(com_fifo, MAX_SIZE, "tmp/com_fifo_%d", mypid);

    printf("%s\n", com_fifo);

    int fifo = mkfifo(com_fifo, 0664);
    if(fifo == -1){
        perror("Error creating communication fifo");
    }

    char data_buffer[MAX_SIZE];

    //execute time -u "prog-a [args]"
    if (strcmp(argv[1], "execute") == 0){
        char *exec_time = argv [2];
        // -u -> um programa | -p -> pipeline de programas
        char *mode = argv [3];
        char *program = argv[4];

        // mudar se depois for preciso incluir o nome do programa para os comandos exec
        char *exec_args =  malloc(MAX_SIZE);
        exec_args[0] = '\0'; 

        for(int i = 5; i < argc; i++){
            // espaço para separar argumentos
            if(i != 5) strcat(exec_args, " ");
            strcat(exec_args, argv[i]);
        }

        int fd = open(com_fifo, O_WRONLY);
        if(fd == -1){
            perror("Error opening FIFO for writing");
            free(com_fifo);
            return -1;
        }

        // format
        // comando;exec_time;program_to_execute;modo;exec_args
        snprintf(data_buffer, MAX_SIZE, "execute;%s;%s;%s", exec_time, program, exec_args);

        if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
            perror("Error writing to FIFO");
        }
        close(fd);

        free(exec_args);
    }

    else if (strcmp(argv[1], "status") == 0) {
        printf("Solicitando status das tarefas ao servidor...\n");


    }
    
    else {
        printf("Comando desconhecido.\n");
        return -1;
    }

    free(com_fifo);
    return 0;
}