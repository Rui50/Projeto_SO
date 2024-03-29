#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#include "../include/orchestrator.h"

#define MAX_SIZE 500
#define MFIFO "tmp/mfifo"

struct request{
    int uid; // identificador unico da task
    char *output;    
};

int parseRequest(){
    return 1;
}

int genTaskUID(){
    return 1;
}

// vai ter um fifo principal que lida com os requests
// outro fifo para cada cliente

int main(int argc, char **argv){
    /*if(argc != 4){
        printf("Usage:  ./orchestrator output_folder parallel-tasks sched-policy");
    }*/

    char request[MAX_SIZE];
    ssize_t bytesRead;

    // data parsing
    char *outputs_folder = argv[1];
    int parallel_taks = atoi(argv[2]);
    int sched_policy = atoi(argv[3]);

    if (mkfifo(MFIFO, 0660) == -1 && errno != EEXIST) {
        perror("Failed to create main FIFO\n");
        return -1;
    }

    // abrir o fifo para handle de requests
    int fd = open(MFIFO, O_RDONLY); // so vai ler os pedidos 
    if(fd == -1){
        perror("Error error opening FIFO!\n");
    }

    while((bytesRead = read(fd, request, MAX_SIZE - 1)) != -1){
        if(bytesRead > 0){
            printf("Received: %s\n", request);
        }
    }
    if (errno != EAGAIN) {
        perror("Error reading from FIFO");
    }  

    close(fd);
}