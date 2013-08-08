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

#define TRUE        1
#define FALSE       0

#define MAXCHARS    128
#define ARR_END     (char *) 0

#define ERR_EOF     0
#define ERR_GET_CWD 1
#define ERR_SET_CWD 2
#define ERR_HOME    3

#define ERR_CHILD   127

pid_t childPid;
char cwd[MAXCHARS];

/**
 * gets the current working directory
 */
void read_cwd(char *cwd) {
    if (getcwd(cwd, MAXCHARS) == NULL) {
        perror("getcwd() error");
        exit(ERR_GET_CWD);
    }
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
 */
void print_prompt(char *cwd) {
    read_cwd(cwd);

    fprintf(stdout, "%s? ", cwd);
    fflush(stdout);
}

/**
 * splits a string into an array of its arguments
 * return: number of arguments
 */
int split_string(char *buf, char **args) {
    int buflen = strlen(buf);
    int i, j;
    int lastspace = TRUE;

    j = 0;
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
        }
    }

    args[j] = ARR_END;

    return j;
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

/* handler for SIGINT */
void sigint_recieved(int s) {

    /* If child exists, kill it. */
    if (childPid && s == SIGINT) {
        kill(childPid, SIGINT);
    }

    fprintf(stdout, "\r\n");

    if (!childPid){
        print_prompt(cwd);
    }
}

/**
 * main execution loop
 */
int main(int argc, char **argv) {
    char *buf;
    int lineLen;
    char *bufargs[25];
    int status;
    struct sigaction sa;

    buf = malloc(sizeof(char));

    /* Handle SIGINT signals */
    sa.sa_handler = sigint_recieved;
    sa.sa_flags = 0x10000000;       /* SA_RESTART */
    sigaction(SIGINT, &sa, 0);

    while (TRUE) {
        /* Clear the child Pid */
        childPid = 0;

        /* print prompt */
        print_prompt(cwd);

        /* get line */
        lineLen = get_line(STDIN_FILENO, buf);

        /* Check if EOF (no chars in buffer) */
        if (lineLen == 0) {
            exit(ERR_EOF);
        }

        /* Check if error in reading line */
        if (lineLen < 0) {
            printf("\r\n");
            continue;
        }

        /* Split the string into args */
        split_string(buf, bufargs);

        /*  cd command*/
        if (strncmp(bufargs[0], "cd", 2) == 0) {
            set_cwd(bufargs[1]);
            continue;
        }

        /* exit command */
        if (strncmp(bufargs[0], "exit", 4) == 0) {
            exit(ERR_EOF);
        }

        /* fork and run the command on path */
        if ((childPid = fork()) < 0)
            fprintf(stdout, "fork error");

        /* child */
        else if (childPid == 0) {
            execvp(bufargs[0], bufargs);
            fprintf(stdout, "couldn't execute: %s \r\n", buf);
            return (ERR_CHILD);
        }

        /* parent */
        if (waitpid(childPid, &status, 0) < 0)
            perror("waitpid err");
    }

    return (ERR_EOF);
}
