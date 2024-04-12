#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../include/client.h"

#define MAX_SIZE 500
#define MFIFO "../tmp/mfifo"

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
        
        printf("Opening FIFO for the request\n");
        int fd = open(MFIFO, O_WRONLY);
        if(fd == -1){
            perror("Error opening FIFO for writing");
            return -1;
        }

        // Fazer já aqui o fifo de retorno por senão o server pode tentar abrir antes de ser criado

        // dados fifo retorno
        char fifoName[12];
        sprintf(fifoName, "fifo_%d", mypid);
        char uidTask[10];


        // criar fifo de return
        if (mkfifo(fifoName, 0660) == -1) {
            perror("Failed to create main FIFO: \n");
        }

        // format
        // comando;exec_time;program_to_execute;modo;exec_args

        // enviar tambme o seu pid para abrir o fifo de retorno?
        snprintf(data_buffer, MAX_SIZE, "%d;execute;%s;%s;%s;%s", mypid,exec_time, mode, program, exec_args);
        printf("Sending request: %s\n", data_buffer);
        if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
            perror("Error writing to FIFO");
        }
        // abaixo disto tudo dentro de else? prevenir caso de write falhar e abrir fifo de retorno
        close(fd);

        printf("FIFO RETORNO: %s\n", fifoName);
        
        // abrir fifo para receber o status
        int fd2 = open(fifoName, O_RDONLY);
        if(fd2 == -1){
            perror("Error opening FIFO for writing");
            return -1;
        }

        ssize_t bytes_read;
        if((bytes_read = read(fd2, uidTask, sizeof(uidTask))) > 0){
            printf("UID RECEIVED: %s\n", uidTask);
        }

        // apagar o fifo
        if (unlink(fifoName) == -1){
            perror("Erro a apagar o fifo");
        }

        free(exec_args);
    }

    else if (strcmp(argv[1], "status") == 0) {
        printf("Solicitando status das tarefas ao servidor...\n");

        // abrir o meu fifo para request do status
        int fd = open(MFIFO, O_WRONLY);
        if(fd == -1){
            perror("Error opening FIFO for writing");
            return -1;
        }
        
        // mensagem de status 

        snprintf(data_buffer, MAX_SIZE, "%d;status;", mypid); // leva o ; devido ao parsing feito no server

        // enviar status msg
        if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
            perror("Error writing to FIFO");
        }
        close(fd);


        // abrir fifo para receber o status
        char fifoName[12];
        sprintf(fifoName, "fifo_%d", mypid);
        
        /*
        int fd2 = open(fifoName, O_RDONLY);
        if(fd2 == -1){
            perror("Error opening FIFO for writing");
            return -1;
        }*/
    }
    
    else {
        printf("Comando desconhecido.\n");
        return -1;
    }
    return 0;
}