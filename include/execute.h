#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../include/orchestrator.h"
#include "../include/task.h"
#include "../include/taskQueue.h"
#include "../include/requests.h"

void executePipelineTask(char *requester_pid, TASK *task, char *outputs_folder, pid_t pid_to_collect);
void executeSingleTask(char *requester_pid, TASK *task, char *outputs_folder, pid_t pid_to_collect);

#endif