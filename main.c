/**
 *  main.c
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

#define _POSIX_SOURCE

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "errs.h"
#include "command.h"

#define TRUE            1
#define FALSE           0

#define ARR_END         (char *) 0

#define READ_END        0
#define WRITE_END       1

/* Must be globals for signal handler */
/* Global command struct */
struct Command *globalCmd[2];
/* Global boolean for input file */
int inputFile;

/**
 * Changes the current working directory
 * If no directory is specified, change to home
 */
void set_cwd(char *cwd) {
    /* if cwd not set, get it from the home variable */
    if (cwd == ARR_END)
        if ((cwd = getenv("HOME")) == NULL) {
            fprintf(stdout, "Failed to read HOME variable\r\n");
            exit(ERR_HOME);
        }

    /* cd to the new dir */
    if (chdir(cwd) < 0) {
        perror("chdir() error");
        /* exit(ERR_SET_CWD);   Do not crash on error */
    }
}

/**
 * prints the prompt to the user for a command
 */
void print_prompt() {

    char *cwd;

    /* Do nothing if input is from script */
    if (inputFile) {
        return;
    }

    /* change this to a CWD that is malloc'd */
    if ((cwd = getcwd(NULL, 0)) == NULL) {
        perror("getcwd() error");
        exit(ERR_GET_CWD);
    }

    fprintf(stdout, "%s? ", cwd);
    fflush(stdout);

    free(cwd);
}

/* Get an entire line of text */
int get_line(int input, char **buffer) {

    int count;
    int ret;
    char c[2];

    *buffer = (char *) malloc(sizeof(char));
    count = 0;

    /* Run while there are still characters to get */
    while ((ret=read(input, c, 1)) > 0) {

        /* If the string is done, set the buffer */
        if (c[0] == '\n' || c[0] == '\0') {
            return count + 1;
        }

        /* Reallocate the buffer and increase its size by 1 */
        *buffer = (char *) realloc(*buffer, (count+2) * sizeof(char) );
        /* Assign the character read to the buffer */
        c[1] = '\0';
        memcpy(*buffer+count, c, 2);

        count++;
    }

    return ret;
}

/**
 * splits a string into an array of its arguments
 * return: number of arguments
 */
char **split_string(char *buf) {
    int buflen = strlen(buf);
    int i;
    int j = 0;
    int lastspace = TRUE;
    char **args = (char **) malloc((j+1) * sizeof(char *));

    /** loop over each char */
    for (i = 0; i < buflen; i++) {
        /* Swap spaces to null chars */
        if (buf[i] == ' ') {
            buf[i] = '\0';
            lastspace = TRUE;
        }
        /* If the last char was a space, an argument starts here */
        else if (lastspace == TRUE) {
            lastspace = FALSE;
            args[j] = &buf[i];
            j++;

            args = (char **) realloc(args, (j+1) * sizeof(char *));
        }
    }

    args[j] = ARR_END;

    return args;
}

/**
 * Set background task with the given PID to stopped
 */
int stopBgProc(pid_t pid, int status) {
    struct Command *cmd = globalCmd[BG];
    struct Process *proc = NULL;

    /* Iterate over commands */
    while (cmd != NULL) {

        proc = cmd->procHead;

        /* Iterate over processes within command */
        while (proc != NULL) {
            /* if found, set to reaped */
            if (proc->pid == pid) {
                proc->status = status;
                proc->running = PROC_REAPED;
                return TRUE;
            }

            proc = proc->next;
        }
        cmd = cmd->next;
    }

    return FALSE;
}

/**
 * Allocate arguments to different processes
 * Perform piping if necessary
 * Mark processes as background or foreground
 * Return TRUE if error, FALSE if no error
 */
int allocate_args(int argc, char **bufargs, struct Command *cmd) {
    struct Process *proc = cmd->procHead;
    int i = 0;
    int j = 0;
    int fd[2];
    static char devnull[] = "/dev/null";

    /* Split bufargs into process args */
   while (bufargs[i] != ARR_END) {

       if (bufargs[i][0] == '|' && bufargs[i][1] == '\0') {
           /* pipe between processes */
           proc->argsv[j] = ARR_END;
           proc->next = init_process(argc * sizeof(char *));

           if (pipe(fd) < 0) {
               perror("pipe failed");
               exit(ERR_PIPE);
           }

           /* Update process and cmd */
           proc->fdout = fd[WRITE_END];
           proc->next->fdin = fd[READ_END];

           proc = proc->next;
           cmd->runningProcs++;
           j = 0;

       } else if (bufargs[i][0] == '<' && bufargs[i][1] == '\0') {
           /* Set file for input redirection */
           i++;
           proc->filein = bufargs[i];
       } else if (bufargs[i][0] == '>' && bufargs[i][1] == '\0') {
           /* Set file for input redirection */
           i++;
           proc->fileout = bufargs[i];
       } else if (bufargs[i][0] == '&' && bufargs[i][1] == '\0') {
           /* Set command for backgrounding */
           if (bufargs[i+1] == ARR_END) {
               cmd->type = BG;
               if (cmd->procHead->filein == NULL) {
                   cmd->procHead->filein = devnull;
               }
           } else {
               printf("invalid background command\r\n");
               return TRUE;
           }
       } else {
           /* copy buffer across to process */
           proc->argsv[j] = bufargs[i];
           proc->argsv[++j] = ARR_END;
       }

       i++;
   }
   proc->argsv[j] = ARR_END;

   return FALSE;
}

/**
 * Wait on runnings tasks in the command. If a child is return that's a
 * background task, mark it as reaped.
 */
void wait_running(struct Command *cmd) {
    struct Process *proc;
    int status;
    pid_t pid;

    while (cmd->runningProcs > 0) {
        /* parent, wait on children */
        if ((pid = waitpid(-1, &status, 0)) < 0) {
            perror("waitpid err");
        }

        /* find the returned process and set it to stopped */
        proc = globalCmd[FG]->procHead;
        while (proc != NULL) {
            if (proc->pid  == pid) {
                proc->running = PROC_STOPPED;
                cmd->runningProcs--;
                break;
            }

            proc = proc->next;
        }

        /* no children not found and pid does not belong to a bg process */
        if (proc == NULL && stopBgProc(pid, status) == FALSE) {
            printf("Error: Child not found\r\n");
            exit(ERR_CHILD_NOT_FOUND);
        }
    }
}

/**
 * Add to the global command list
 */
void add_global(struct Command *cmd) {
    struct Command *prev;

    if (globalCmd[cmd->type] == NULL) {
        globalCmd[cmd->type] = cmd;
    } else {
        prev = globalCmd[cmd->type];
        while (prev->next != NULL) prev = prev->next;
        prev->next = cmd;
    }
}

/**
 * Process the buffer
 * Return TRUE to exit program
 */
int process_buf(char **bufargs) {
    struct Command *cmd;
    int argc = 0;

    /*  cd command*/
    if (!strcmp(bufargs[0], "cd") && bufargs[0][2] == '\0') {
        set_cwd(bufargs[1]);
        return FALSE;
    }

    /* exit command */
    if (!strcmp(bufargs[0], "exit") && bufargs[0][4] == '\0') {
        return TRUE;
    }

    /* Get argc */
    while (bufargs[argc++] != ARR_END);

    /* Set up command and first process */
    cmd = init_command();
    cmd->procHead = init_process(argc * sizeof(char *));
    cmd->runningProcs++;

    /* Allocate arguments to their processes within the command */
    if (allocate_args(argc, bufargs, cmd) == TRUE) {
        /* Invalid command */
        free_command(NULL, cmd);
        return FALSE;
    }

    /* Add the command to the global commands */
    add_global(cmd);

    /* For each process in the command, fork it and set it up as running */
    fork_command(cmd);

    /* If foreground command, then wait on processes */
    if (cmd->type == FG) {
        wait_running(cmd);

        /* set the foreground command to null and clean it */
        globalCmd[FG] = NULL;
        free_command(NULL, cmd);
    }

    return FALSE;
}

/**
 *  handler for SIGINT
 */
void sigint_recieved(int s) {
    struct Process *proc;

    fprintf(stdout, "\r\n");

    /* If foreground command exists, kill it, otherwise reprint the prompt */
    if (globalCmd[FG] != NULL && globalCmd[FG]->runningProcs > 0) {
        proc = globalCmd[FG]->procHead;
        /* Kill all running processes within the command */
        while (proc != NULL) {
            if (proc->running == PROC_RUNNING) {
                kill(proc->pid, SIGINT);
            }
            proc = proc->next;
        }
    } else {
        reap_command(&globalCmd[BG]);
        print_prompt();
    }
}

/**
 * Handle a script argument to qshell
 */
void handle_args(int argc, char **argv) {
    int inputFd = 0;

    inputFile = FALSE;
    if (argc > 1) {
        /* Check if necessary to do output redirection*/
        if ((inputFd = open(argv[1], O_RDONLY)) < 0) {
            perror("write open failed");
            exit(ERR_OPEN);
        }

        /* dupe and close old file if different to standard */
        dupefd(inputFd, STDIN_FILENO);
        inputFile = TRUE;
    }
}

/**
 * main execution loop
 */
int main(int argc, char **argv) {
    char *buf = NULL;               /* Input buffer */
    int lineLen;                    /* Length of buffer */
    char **bufargs;                 /* malloc, may cause segfaults */
    struct sigaction sa;
    int quit = FALSE;

    /* set up fg and bg cmds */
    globalCmd[FG] = NULL;
    globalCmd[BG] = NULL;

    /* Handle SIGINT signals */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_recieved;
    sa.sa_flags = (int) 0x10000000;    /* SA_RESTART, not available in ANSI */
    sigaction(SIGINT, &sa, 0);

    /* Handle script argument */
    handle_args(argc, argv);

    while (!quit) {

        /* print prompt */
        reap_command(&globalCmd[BG]);
        print_prompt();

        /* get line */
        lineLen = get_line(STDIN_FILENO, &buf);

        /* Check if error in reading line */
        if (lineLen < 0) {
            printf("error reading line\r\n");
            exit(ERR_READ);
        }

        /* Check if EOF (no chars in buffer) */
        if (lineLen == 0) {
            free(buf);
            break;
        }

        /* if nothing as read or it's a comment, do nothing */
        if (lineLen == 1 || buf[0] == '#') {
            free(buf);
            continue;
        }

        /* Split the string into args */
        bufargs = split_string(buf);

        /* Process the args */
        quit = process_buf(bufargs);

        /* free memory */
        free(bufargs);
        free(buf);
    }

    return (ERR_EOF);
}
