#include <stdlib.h>
#include <stdio.h>

#include "../include/taskQueue.h"
#include "../include/task.h"

int left(int i) { return 2 * i + 1; }
int right(int i) { return 2 * i + 2; }
int parent(int i) { return (i-1)/2; }

void swap(TaskPriorityQueue *queue, int a, int b){
    TASK *temp = queue->tasks[a];
    queue->tasks[a] = queue->tasks[b];
    queue->tasks[b] = temp;
}

void initQueue(TaskPriorityQueue *queue) {
    queue->size = 0;
}

int isQueueEmpty(TaskPriorityQueue *queue) {
    return queue->size == 0;
}

void bubbleUp (int i, TaskPriorityQueue *queue){
    while(i > 0 && queue->tasks[i]->time < queue->tasks[parent(i)]->time){
        swap(queue, i, parent(i));
        i = parent(i);
    }
}

void bubbleDown (int i, TaskPriorityQueue *queue){
    int temp;

    while (left(i) < queue->size){
        if(right(i) < queue->size && queue->tasks[right(i)]->time < queue->tasks[left(i)]->time){
            temp = right(i);
        }
        else{
            temp = left(i);
        }
    

        if(queue->tasks[temp]->time > queue->tasks[i]->time){
            break;
        }

        swap(queue, i, temp);
        i = temp;
    }
}

int addTask(TaskPriorityQueue *queue, TASK *task){
    if (queue->size == MAX_SIZE) return -1;
    
    queue->tasks[queue->size] = task;
    bubbleUp(queue->size, queue);
    queue->size++;

    return 0;
}

TASK *getNextTask(TaskPriorityQueue *queue){
    TASK *task = queue->tasks[0];
    queue->tasks[0] = queue->tasks[queue->size - 1];
    queue->size--;
    bubbleDown(0, queue);

    return task;
} 

void printQueueTimes(TaskPriorityQueue *queue) {
    printf("Queue (times): [");
    for (int i = 0; i < queue->size; i++) {
        printf("%.1f", queue->tasks[i]->time);
        if (i < queue->size - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}