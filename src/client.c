#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../include/client.h"

#define MAX_SIZE 500
#define MFIFO "tmp/mfifo"

int main(int argc, char **argv){

    if (argc < 2) {
        printf("Usage: %s <command> [options]\n", argv[0]);
        return -1;
    }

    // vai servir como "uid do cliente"
    int mypid = getpid(); 
    
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
        
        printf("Opening FIFO for writing\n");
        int fd = open(MFIFO, O_WRONLY);
        if(fd == -1){
            perror("Error opening FIFO for writing");
            return -1;
        }

        // format
        // comando;exec_time;program_to_execute;modo;exec_args

        // enviar tambme o seu pid para abrir o fifo de retorno?
        snprintf(data_buffer, MAX_SIZE, "execute;%s;%s;%s;%s", exec_time, mode, program, exec_args);
        printf("Sending request: %s\n", data_buffer);
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
    return 0;
}