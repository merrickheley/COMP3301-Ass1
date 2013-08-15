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

#define BG              0
#define FG              1

/* Global command struct */
struct Command *globalCmd[2];

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
 * Duplicate oldfd over newfd if different
 */
void dupeIfDif(int oldfd, int newfd) {
    if (oldfd != newfd && dup2(oldfd, newfd) < 0) {
        perror("dup2");
        exit(ERR_DUP2);
    }
}

/**
 * Close oldfd if different to newfd
 */
void closeIfDif(int oldfd, int newfd) {
    if (oldfd != newfd && close(oldfd) < 0) {
        perror("close");
        exit(ERR_CLOSE);
    }
}

/**
 * Dup2 new over old file descriptor if different, close the old file after
 * if it's different
 */
void dupefd(int oldfd, int newfd) {
    dupeIfDif(oldfd, newfd);
    closeIfDif(oldfd, newfd);
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
    int status;
    pid_t pid;
    int cmdType = FG;

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
    cmd->runningProcs++;

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
            cmdType = BG;
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

    /* set the command type */
    globalCmd[cmdType] = cmd;

    while (TRUE) {

        /* Process is now running */
        proc->running = PROC_RUNNING;

        /* fork and run the command on path */
       if ((proc->pid = fork()) < 0) {
           fprintf(stdout, "fork error");
       } else if (proc->pid == 0) {
           /* child */

           /* Check if necessary to open files */
           if (proc->filein != NULL
                   && (proc->fdin = open(proc->filein, O_RDONLY)) < 0) {
               perror("read open failed");
               exit(ERR_OPEN);
           }

           if (proc->fileout != NULL
                   && (proc->fdout = open(proc->fileout,
                           O_CREAT|O_WRONLY|O_TRUNC, 0777)) < 0) {
               perror("write open failed");
               exit(ERR_OPEN);
           }

           /* dupe and close old file if different to standard */
           dupefd(proc->fdin, STDIN_FILENO);
           dupefd(proc->fdout, STDOUT_FILENO);

           /* exec, print error if failed */
           execvp(proc->argsv[0], proc->argsv);
           fprintf(stdout, "couldn't execute: %s \r\n", proc->argsv[0]);
           exit (ERR_CHILD);
       } else {
           /* parent */

           /* close created pipes */
           closeIfDif(proc->fdin, STDIN_FILENO);
           closeIfDif(proc->fdout, STDOUT_FILENO);
       }

       /* Move to the next process in the queue */
       if (proc->next == NULL) {
           break;
       } else {
           proc = proc->next;
       }
    }

    /* If background command, then don't wait on processes */
    if (cmdType == BG) {
        return;
    }

    /* Wait on running processes */
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

        if (proc == NULL) {
            printf("Error: Child not found\r\n");
            exit(ERR_CHILD_NOT_FOUND);
        }
    }

    /* set the foreground command to null and clean it */
    globalCmd[FG] = NULL;
    free_command(NULL, cmd);
}

/* handler for SIGINT */
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
        print_prompt();
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

    /* set up fg and bg cmds */
    globalCmd[FG] = NULL;
    globalCmd[BG] = NULL;

    /* Handle SIGINT signals */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_recieved;
    sa.sa_flags = (int) 0x10000000;        /* SA_RESTART, not available in ANSI */
    sigaction(SIGINT, &sa, 0);

    while (TRUE) {

        /* print prompt */
        print_prompt();

        /* get line */
        lineLen = get_line(STDIN_FILENO, &buf);

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

        /* Split the string into args */
        bufargs = split_string(buf);

        /* Process the args */
        process_buf(bufargs);

        /* free bufargs */
        free(bufargs);

        /* free buffer */
        free(buf);
    }

    /* Print a newline to finish the prompt */
    fprintf(stdout, "\r\n");

    return (ERR_EOF);
}
