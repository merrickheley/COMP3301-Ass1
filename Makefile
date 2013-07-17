# qshell
# COMP3301 Assignment 1
# Copyright (C) 2013  Merrick Heley
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Program options
PROGRAM = qshell
AUTHOR = 42339915
LICENSE = gpl.txt

# Compiler options
CC = gcc
C_FLAGS = -Wall -ansi -pedantic

# Files to compile
C_FILES = main.c
OBJS := $(C_FILES:.c=.o)

# Distribution
DIST_NAME = $(AUTHOR).tar.gz
DIST_FILES = $(C_FILES) $(LICENSE) Makefile

all: install

install: $(OBJS)
	$(CC) $(C_FLAGS) $(OBJS) -o $(PROGRAM)

%.o: %.c
	$(CC) $(C_FLAGS) -c $<

clean:
	@rm -f *.o *.gch $(PROGRAM)
	
dist:
	tar -cvf $(DIST_NAME) $(DIST_FILES)

#Do not add OBJS, if the files exist do not rebuild them
.PHONY: all install clean dist