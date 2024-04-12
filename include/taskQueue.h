#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#define MAX_SIZE 1000

#include "../include/orchestrator.h"

typedef struct taskQueue {
  TASK *tasks[MAX_SIZE];
  int size;
} TaskPriorityQueue;

void initQueue(TaskPriorityQueue *queue);
int addTask(TaskPriorityQueue *queue, TASK *task);
TASK *getNextTask(TaskPriorityQueue *queue);
void printQueueTimes(TaskPriorityQueue *queue);
int isQueueEmpty(TaskPriorityQueue *queue);

#endif