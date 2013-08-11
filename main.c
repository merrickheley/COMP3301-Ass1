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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include "errs.h"

#define TRUE            1
#define FALSE           0

#define ARR_END         (char *) 0

#define READ_END        0
#define WRITE_END       1

#define PROC_STOPPED    0
#define PROC_RUNNING    1

/**
 * Structure to hold a single process, can be linked (piped) to others
 */
struct Process {
    pid_t pid;              /* Pid of process */
    char **argsv;           /* Arguments */
    int running;            /* Running status (0 = stopped, 1 = running) */
    int status;             /* waitpid status */
    struct Process *next;   /* Next process to pipe data into */
    int fdin;               /* Stdin for process */
    int fdout;              /* Stdout for process */
};

/**
 * Structure to hold a command. Contains the head of the process
 */
struct Command {
    struct Process *procHead;
    struct Command *next;
};

/* Global foreground command */
struct Command *fgCmd;
/* Global background commands */
struct Command *bgCmds;

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
struct Command *init_command() {
    struct Command *cmd = (struct Command *) malloc(sizeof(struct Command));

    cmd->procHead = NULL;
    cmd->next = NULL;

    return cmd;
}
/**
 * Free a command and it's processes, and make the head
 */
void free_command(struct Command *head, struct Command *cmd) {
    struct Command *ptr = head;

    free_process(cmd->procHead);

    /* find the current commands pointer */
    while (ptr->next != cmd || ptr->next == NULL) {
        ptr = ptr->next;
    }

    /* if there is a pointer to this command, make it skip this command */
    if (ptr->next != NULL) {
        ptr->next = cmd->next;
    }

    free(cmd);
}

/**
 * changes the current working directory
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
        exit(ERR_SET_CWD);
    }
}

/**
 * prints the prompt to the user for a command
 * Check if any background tasks finished
 */
void print_prompt() {
    /* Handle any background tasks */

    char *cwd;

    /* change this to a CWD that is malloced */
    if ((cwd = getcwd(NULL, 0)) == NULL) {
        perror("getcwd() error");
        exit(ERR_GET_CWD);
    }

    fprintf(stdout, "%s? ", cwd);
    fflush(stdout);

    free(cwd);
}

/* Get an entire line of text */
int get_line(int input, char *buffer) {

    int count;
    int ret;
    char c[2];

    buffer = (char *) realloc(buffer, sizeof(char));
    count = 0;

    /* Run while there are still characters to get */
    while ((ret=read(input, c, 1)) > 0) {

        /* If the string is done, set the buffer */
        if (c[0] == '\n' || c[0] == '\0') {
            return count + 1;
        } else {
            /* Reallocate the buffer and increase its size by 1 */
            buffer = (char*) realloc(buffer,
                    (size_t) sizeof(char) *(count+2) );

            /* Assign the character read to the buffer */
            c[1] = '\0';
            memcpy(buffer+count, c, 2);

        }
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
 * Process the buffer arguments
 */
void process_buf(char **bufargs) {
    struct Command *cmd;
    struct Process *proc;
    int i;
    int j;
    int argc = 0;
    int fd[2];

    printf("gotcha %s\r\n", bufargs[1]);

    /*  cd command*/
    if (strcmp(bufargs[0], "cd") == 0 && bufargs[0][2] == '\0') {
        set_cwd(bufargs[1]);
        return;
    }

    /* exit command */
    if (strcmp(bufargs[0], "exit") == 0 && bufargs[0][4] == '\0') {
        exit(ERR_EOF);
    }

    /* Get argc */
    while (bufargs[argc++] != ARR_END);

    /* Set up command and first process */
    cmd = init_command();
    proc = init_process(argc * sizeof(char *));

    /* Set the head of the current cmd to the process */
    cmd->procHead = proc;

    /* Split bufargs into process args */
    i = 0;
    j = 0;
    while (bufargs[i] != ARR_END) {

        if (bufargs[i][0] == '|' && bufargs[i][1] == '\0') {
            /* pipe between processes */

            proc->argsv[j+1] = ARR_END;
            proc->next = init_process(argc * sizeof(char *));

            if (pipe(fd) < 0) {
                perror("pipe failed");
                exit(ERR_PIPE);
            }

            proc->fdout = fd[WRITE_END];
            proc->next->fdin = fd[READ_END];

            proc = proc->next;
            j = 0;
        } else {
            /* copy buffer across to process */

            proc->argsv[j] = bufargs[i];
            j++;
        }

        i++;
    }
    proc->argsv[j] = ARR_END;

    /* reset proc back to start */
    proc = cmd->procHead;

    /* set the foreground command to the current command */
    fgCmd = cmd;

    while (TRUE) {

        /* Process is now running */
        proc->running = PROC_RUNNING;

        /* fork and run the command on path */
       if ((proc->pid = fork()) < 0) {
           fprintf(stdout, "fork error");
       } else if (proc->pid == 0) {
           /* child */
           if (proc->fdin != STDIN_FILENO) {
               if (dup2(proc->fdin, STDIN_FILENO) < 0) {
                   perror("dup2");
                   exit(ERR_DUP2);
               }

               if (close(proc->fdin) < 0) {
                   perror("close");
                   exit(ERR_CLOSE);
               }
           }

           if (proc->fdout != STDOUT_FILENO) {
               if (dup2(proc->fdout, STDOUT_FILENO) < 0) {
                   perror("dup2");
                   exit(ERR_DUP2);
               }

               if (close(proc->fdout) < 0) {
                   perror("close");
                   exit(ERR_CLOSE);
               }
           }

           execvp(proc->argsv[0], proc->argsv);
           fprintf(stdout, "couldn't execute: %s \r\n", proc->argsv[0]);
           exit (ERR_CHILD);
       } else {
           /* parent */

           /* close created pipes */
           if (proc->fdin != STDIN_FILENO) {
               if (close(proc->fdin) < 0) {
                   perror("close");
                   exit(ERR_CLOSE);
               }
           }

           if (proc->fdout != STDOUT_FILENO) {
               if (close(proc->fdout) < 0) {
                   perror("close");
                   exit(ERR_CLOSE);
               }
           }
       }

       if (proc->next == NULL) {
           break;
       } else {
           proc = proc->next;
       }
    }

    /* fix this to wait on all children */

    /* parent, wait on child */
    if (waitpid(proc->pid, &proc->pid, 0) < 0) {
        perror("waitpid err");
    }

    /* Process has been stopped */
    proc->running = PROC_STOPPED;

   fgCmd = NULL;
}

/* handler for SIGINT */
void sigint_recieved(int s) {

    fprintf(stdout, "\r\n");

    /* If child exists, kill it, otherwise reprint the prompt */
    if (fgCmd != NULL && fgCmd->procHead->running == PROC_RUNNING) {
        kill(fgCmd->procHead->pid, SIGINT);
    } else {
        print_prompt();
    }
}

/**
 * main execution loop
 */
int main(int argc, char **argv) {
    char *buf;                      /* Input buffer */
    int lineLen;                    /* Length of buffer */
    char **bufargs;                 /* malloc, may cause segfaults */
    struct sigaction sa;

    /* set up buffer */
    buf = (char *) malloc(sizeof(char));

    /* set up fg and bg cmds */
    fgCmd = NULL;
    bgCmds = NULL;

    /* Handle SIGINT signals */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_recieved;
    sa.sa_flags = (int) 0x10000000;        /* SA_RESTART, not available in ANSI */
    sigaction(SIGINT, &sa, 0);

    while (TRUE) {

        /* print prompt */
        print_prompt();

        /* get line */
        lineLen = get_line(STDIN_FILENO, buf);

        /* Check if EOF (no chars in buffer) */
        if (lineLen == 0) {
            break;
        }

        /* Check if error in reading line */
        if (lineLen < 0) {
            printf("error reading line\r\n");
            exit(ERR_READ);
        }

        /* only a newline char was read */
        if (lineLen == 1) {
            continue;
        }

        /* command is a comment, do not process */
        if (buf[0] == '#') {
            continue;
        }

        printf("liner %s\r\n", buf);

        /* Split the string into args */
        bufargs = split_string(buf);

        /* Process the args */
        process_buf(bufargs);

        /* free bufargs */
        free(bufargs);

    }

    /* free buffer */
    free(buf);

    /* Print a newline to finish the prompt */
    fprintf(stdout, "\r\n");

    return (ERR_EOF);
}
