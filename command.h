/**
 *  command.h
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

#ifndef COMMAND_H_
#define COMMAND_H_

#include <sys/types.h>

#include "errs.h"

#define PROC_STOPPED    0
#define PROC_RUNNING    1
#define PROC_REAPED     2

/***************************************************************************
 *
 * Public Structures
 *
 ***************************************************************************/

/**
 * Structure to hold a single process, can be linked (piped) to others
 */
struct Process {
    pid_t pid;                  /* Pid of process */
    char **argsv;               /* Arguments */
    int running;                /* Running status (0 = stopped, 1 = running) */
    int status;                 /* waitpid status */
    struct Process *next;       /* Next process to pipe data into */
    int fdin;                   /* Stdin for process */
    int fdout;                  /* Stdout for process */
    char *filein;               /* File name for input redirection */
    char *fileout;              /* File name for output redirection */
};

/**
 * Structure to hold a command. Contains the head of the process
 */
struct Command {
    struct Process *procHead;   /* Head for process linked list */
    struct Command *next;       /* Next command */
    int runningProcs;           /* Number of processes in command */
    pid_t group;                /* pid group for command */
};

/***************************************************************************
 *
 * Public Functions
 *
 ***************************************************************************/

/**
 * Initialise a process with default values
 */
struct Process *init_process(size_t argSize);

/**
 * Free a process and linked processes in the list
 */
void free_process(struct Process *proc);

/**
 * Initialise a command with default values
 */
struct Command *init_command(void);

/**
 * Free a command and it's processes, and make the head skip this command
 */
void free_command(struct Command *head, struct Command *cmd);

/**
 * Reap any commands that have completed
 */
void reapCommand(struct Command **head);

#endif /* COMMAND_H_ */
