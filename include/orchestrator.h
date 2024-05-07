#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "../include/taskQueue.h"
#include "../include/task.h"
#include "../include/requests.h"

void parseRequest(REQUEST *request, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, int *uid, char *output_folder);
void returnIdToClient(int pid, int uid);
void sendStatusToClient(int pid, TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder, pid_t pid_to_collect);
void sendTerminatedTask(TASK *terminatedTask, pid_t pid);
void checkTasks(TaskPriorityQueue *queue, TASK **running_tasks, int parallel_tasks, char *output_folder);

#endif