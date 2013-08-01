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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define TRUE        1
#define FALSE       0

#define MAXCHARS    150

#define ERR_SUCCESS 0
#define ERR_GET_CWD 1
#define ERR_SET_CWD 2

#define ERR_CHILD   127

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
 */
void set_cwd(char *cwd) {
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

    printf("%s? ", cwd);
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

    return j;
}

/**
 * main execution loop
 */
int main(int argc, char **argv) {
    char buf[MAXCHARS];
    char cwd[MAXCHARS];
    char *args[22];
    pid_t pid;
    int status;

    /* print initial prompt */
    print_prompt(cwd);

    while (fgets(buf, MAXCHARS, stdin) != NULL) {

        /* replace newline with null */
        buf[strlen(buf) - 1] = 0;

        split_string(buf, args);

        /* this needs to be done more robustly */
        if (strncmp(buf, "cd", 2) == 0) {
            set_cwd(buf+3);

            print_prompt(cwd);
            continue;
        }

        if (strncmp(buf, "exit", 4) == 0) {
            exit(0);
        }


        if ((pid = fork()) < 0)
            printf("fork error");

        /* child */
        else if (pid == 0) {
            execlp(buf, buf, (char *) 0);
            printf("couldn't execute: %s \r\n", buf);
            return (ERR_CHILD);
        }

        /* parent */
        if ((pid = waitpid(pid, &status, 0)) < 0)
            printf("waitpid error");

        /* print prompt */
        print_prompt(cwd);
    }

    return (ERR_SUCCESS);
}
