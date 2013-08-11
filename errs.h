/**
 *  errs.h
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

#ifndef ERRS_H_
#define ERRS_H_

/* system errors */
#define ERR_EOF                             0
#define ERR_GET_CWD                         1
#define ERR_SET_CWD                         2
#define ERR_HOME                            3
#define ERR_FORK                            4
#define ERR_PIPE                            5
#define ERR_WAIT                            6
#define ERR_DUP2                            7
#define ERR_CLOSE                           8
#define ERR_READ                            9

/* Debug errrors */
#define ERR_FREE_RUNNING_PROC               100

/* Child errors */
#define ERR_CHILD                           200

#endif /* ERRS_H_ */
