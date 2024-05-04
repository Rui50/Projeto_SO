#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "../include/taskQueue.h"
#include "../include/task.h"

void parseRequest(char *request, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, int *uid, char *output_folder);
void returnIdToClient(char *pid, int uid);
void sendStatusToClient(char *pid, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder);
void sendTerminatedTask(TASK *terminatedTask, pid_t pid);
void checkTasks(TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder);

#endif