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

#include <stdlib.h>
#include <unistd.h>

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

    return cmd;
}

/**
 * Free a command and it's processes, and make the head skip this command
 */
void free_command(struct Command *head, struct Command *cmd) {
    struct Command *ptr = head;

    free_process(cmd->procHead);

    if (ptr != NULL) {

        /* find the current commands pointer */
        while (ptr->next != cmd && ptr->next != NULL) {
            ptr = ptr->next;
        }

        /* if there is a pointer to this command, make it skip this command */
        if (ptr->next != NULL) {
            ptr->next = cmd->next;
        }
    }

    free(cmd);
}


