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

#define TRUE        1
#define FALSE       0

#define ARR_END     (char *) 0

#define READ_END    0
#define WRITE_END   1

typedef struct {
    pid_t pid;          /* Pid of process (0 = completed, else Pid) */
    int argc;           /* Number of arguments */
    char **argsv;       /* Arguments */
    int running;        /* Running status (0 = stopped, 1 = running) */
} Process;

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

void process_buf(char **bufargs) {
    Process proc;
    int status;

    proc.running = 0;

    /*  cd command*/
    if (strncmp(bufargs[0], "cd", 2) == 0) {
        set_cwd(bufargs[1]);
        return;
    }

    /* exit command */
    if (strncmp(bufargs[0], "exit", 4) == 0) {
        exit(ERR_EOF);
    }

    /* Split bufargs into process args */


    /* fork and run the command on path */
   if ((proc.pid = fork()) < 0)
       fprintf(stdout, "fork error");

   /* child */
   else if (proc.pid == 0) {
       execvp(bufargs[0], bufargs);
       fprintf(stdout, "couldn't execute: %s \r\n", bufargs[0]);
       exit (ERR_CHILD);
   }

   /* parent */
   if (waitpid(proc.pid, &status, 0) < 0)
       perror("waitpid err");


}

/* handler for SIGINT */
/*void sigint_recieved(int s) {

     If child exists, kill it.
    if (childPid && s == SIGINT) {
        kill(childPid, SIGINT);
    }

    fprintf(stdout, "\r\n");

    if (!childPid){
        print_prompt();
    }
}*/

/**
 * main execution loop
 */
int main(int argc, char **argv) {
    char *buf;                      /* Input buffer */
    int lineLen;                    /* Length of buffer */
    char **bufargs;                 /* malloc, may cause segfaults */

    struct sigaction sa;

    buf = (char *) malloc(sizeof(char));

    /* Handle SIGINT signals */
    /*sa.sa_handler = sigint_recieved;
    sa.sa_flags = 0x10000000;        SA_RESTART
    sigaction(SIGINT, &sa, 0);*/

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
            fprintf(stdout, "\r\n");
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

    }

    /* free buffer */
    free(buf);

    /* Print a newline to finish the prompt */
    fprintf(stdout, "\r\n");

    return (ERR_EOF);
}
