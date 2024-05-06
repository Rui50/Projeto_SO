#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../include/client.h"
#include "../include/task.h"

#define MAX_SIZE 500
#define MFIFO "../tmp/mfifo"

int main(int argc, char **argv){

    if (argc < 2) {
        printf("Usage: execute <time> <-u|-p> \"<command>\"\n");
        return -1;
    }

    int mypid = getpid();
    
    char data_buffer[MAX_SIZE];

    //execute time -u "prog-a [args]"
    if (strcmp(argv[1], "execute") == 0){
        char *exec_time = argv [2];
        // -u -> um programa | -p -> pipeline de programas
        char *mode = argv [3];
        char *commands = argv[4];
        
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
        char *pid_str = malloc(15 * sizeof(char));
        snprintf(pid_str, 25, "%d", mypid);

        // criar fifo de return
        if (mkfifo(fifoName, 0660) == -1) {
            perror("Error creating return FIFO \n");
        }

        // enviar tambme o seu pid para abrir o fifo de retorno

        snprintf(data_buffer, MAX_SIZE, "%d;execute;%s;%s;%s", mypid,exec_time, mode, commands);
        printf("Sending request: %s\n", data_buffer);
        if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
            perror("Error writing to FIFO");
        }
        // abaixo disto tudo dentro de else? prevenir caso de write falhar e abrir fifo de retorno
        close(fd);
        
        // abrir fifo para receber o status
        int fd2 = open(fifoName, O_RDONLY);
        if(fd2 == -1){
            perror("Error opening FIFO");
            return -1;
        }

        ssize_t bytes_read;
        if((bytes_read = read(fd2, uidTask, sizeof(uidTask) - 1)) > 0){
            uidTask[bytes_read] = '\0';
            printf("UID RECEIVED: %s\n", uidTask);
        }


        // apagar o fifo
        if (unlink(fifoName) == -1){
            perror("Error deleting FIFO");
        }
    }

    else if (strcmp(argv[1], "status") == 0) {

        // abrir o meu fifo para request do status
        int fd = open(MFIFO, O_WRONLY);
        if(fd == -1){
            perror("Error opening FIFO");
            return -1;
        }

        // abrir fifo para receber o status
        char fifoName[12];
        sprintf(fifoName, "fifo_%d", mypid);

        // criar fifo de return
        if (mkfifo(fifoName, 0660) == -1) {
            perror("Error creating main FIFO: \n");
        }
        
        // mensagem de status 

        snprintf(data_buffer, MAX_SIZE, "%d;status;", mypid); // leva o ; devido ao parsing feito no server

        // enviar status msg
        if (write(fd, data_buffer, strlen(data_buffer)) == -1) {
            perror("Error writing to FIFO");
        }
        close(fd);
        
        
        int fd2 = open(fifoName, O_RDONLY);
        if(fd2 == -1){
            perror("Error opening FIFO for writing");
            return -1;
        }

        ssize_t bytes_read;
        char databuffer[MAX_SIZE];

        if((bytes_read = read(fd2, databuffer, sizeof(data_buffer))) > 0){
            printf("%s", data_buffer);
        }

        close(fd2);
    }
    
    else {
        printf("Comando desconhecido.\n");
        return -1;
    }
    return 0;
}