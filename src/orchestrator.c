#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

// vai ter um fifo principal que lida com os requests
// outro fifo para cada cliente

int main(int argc, char **argv){
    if(argc != 4){
        printf("Usage:  ./orchestrator output_folder parallel-tasks sched-policy");
    }

    // data parsing
    char *outputs_folder = argv[1];
    int parallel_taks = atoi(argv[2]);
    int sched_policy = atoi(argv[3]);

    // main comunication FIFO
    int mfifo = mkfifo("tmp/mfifo", 0660);
    if (mfifo == -1){
        perror("Error creating FIFO.\n");
        return -1;
    }

    // abrir o fifo para handle de requests
    int fd = open("tmp/mfifo", O_RDONLY); // so vai ler os pedidos 
    if(fd == -1){
        perror("Error error opening FIFO!");
    }




}