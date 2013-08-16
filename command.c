/**
 *  command.c
 *  Merrick Heley (merrick.heley@uqconnect.edu.au)
 *  Simple shell replacement
 *
 *  qshell is a simple shell written in C.
 *
 *  COMP3301 Assignment 1
 *  Copyright (C) 2013  Merrick Heley
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http: *www.gnu.org/licenses/>.
 */

#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define FALSE   0
#define TRUE    1

/**
 * Initialise a process with default values
 */
struct Process *init_process(size_t argSize) {
    struct Process * proc = (struct Process *) malloc(sizeof(struct Process));

    proc->pid = -1;
    proc->argsv = (char **) malloc(argSize);
    proc->running = 0;
    proc->next = NULL;
    proc->fdin = STDIN_FILENO;
    proc->fdout = STDOUT_FILENO;
    proc->filein = NULL;
    proc->fileout = NULL;

    return proc;
}

/**
 * Free a process and linked processes in the list
 */
void free_process(struct Process *proc) {

    if (proc->next != NULL) {
        free_process(proc->next);
    }

    if (proc->running) {
        exit(ERR_FREE_RUNNING_PROC);
    }

    free(proc->argsv);
    free(proc);
}

/**
 * Initialise a command with default values
 */
struct Command *init_command(void) {
    struct Command *cmd = (struct Command *) malloc(sizeof(struct Command));

    cmd->procHead = NULL;
    cmd->next = NULL;
    cmd->runningProcs = 0;
    cmd->group = 0;

    return cmd;
}

/**
 * Free a command and it's processes, and make the head skip this command
 */
void free_command(struct Command *head, struct Command *cmd) {
    struct Command *ptr = head;

    /* Link pointer to cmd to cmd->next */
    while (ptr != NULL) {
        if (ptr->next == cmd) {
            ptr->next = ptr->next->next;
            break;
        }

        ptr = ptr->next;
    }

    free_process(cmd->procHead);
    free(cmd);
    cmd = NULL;
}

/**
 * Reap any commands that have completed
 */
void reapCommand(struct Command **head) {
    struct Command *cmd = *head;
    struct Process *proc = NULL;
    struct Command *nextCmd = NULL;
    int isHead = FALSE;

    /* Iterate over commands */
    while (cmd != NULL) {

        proc = cmd->procHead;

        /* Iterate over processes within command */
        while (proc != NULL) {

            /* print the exit status if already reaped, or requires reaping */
            if (proc->running == PROC_REAPED
                    || (proc->running == PROC_RUNNING
                    && waitpid(proc->pid, &proc->status, WNOHANG) > 0)) {

                proc->running = PROC_STOPPED;
                cmd->runningProcs--;
                printf("%d\t%d\r\n", proc->pid, WEXITSTATUS(proc->status));
            }

            proc = proc->next;
        }

        /* free cmd if no more running procs
         * move to the next command */
        if (cmd->runningProcs == 0) {
            isHead = cmd == *head;

            nextCmd = cmd->next;
            free_command(*head, cmd);
            cmd = nextCmd;

            if (isHead) {
                *head = nextCmd;
            }

        } else {
            cmd = cmd->next;
        }
    }
}


