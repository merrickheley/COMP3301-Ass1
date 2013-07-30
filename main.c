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

#define MAXCHARS 150

#define ERR_SUCCESS 0
#define ERR_CWD 1

/**
 * prints the prompt to the user for a command
 */
void print_prompt(char *cwd) {
    if (getcwd(cwd, MAXCHARS) == NULL) {
        perror("getcwd() error");
        exit(ERR_CWD);
    }

    printf("%s? ", cwd);
}

/**
 * main execution loop
 */
int main(int argc, char **argv) {
    char buf[MAXCHARS];
    char cwd[MAXCHARS];
    pid_t pid;
    int status;

    /* print initial prompt */
    print_prompt(cwd);

    while (fgets(buf, MAXCHARS, stdin) != NULL) {

        /* replace newline with null */
        buf[strlen(buf) - 1] = 0;

        pid = fork();
        if (pid < 0) {
            printf("fork error");
        }
        /* child */
        else if (pid == 0) {
            execlp(buf, buf, (char *) 0);
            printf("couldn't execute: %s \r\n", buf);
            return (127);
        }
        /* parent */
        if ((pid = waitpid(pid, &status, 0)) < 0) {
            printf("waitpid error");
        }

        /* print prompt */
        print_prompt(cwd);
    }

    return (ERR_SUCCESS);
}
