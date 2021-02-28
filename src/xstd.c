/***
	This file is part of LockPassword
	Copyright (C) 2020-2021 Aleksandr D. Goncharov (Joursoir) <chat@joursoir.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
***/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void callError(int num)
{
	fprintf(stderr, "lpass: Sorry, there was an error in the program [#%d]\n", num);
	exit(3);
}

void printError(const char *text)
{
	fprintf(stderr, "%s", text);
	exit(4);
}

void easyFork(char *name, char *arguments[])
{
	int pid;
	pid = fork();
	if(pid == -1) callError(100);
	if(pid == 0) { /* new process */
		execvp(name, arguments);
		perror(name);
		exit(4);
	}
	wait(&pid);
}

char *xstrcat(const char *first, const char *second,
	const char *delimiter)
{
	size_t size = sizeof(char) * (strlen(first) + strlen(second) + 1);
	if(delimiter)
		size += sizeof(char) * strlen(delimiter);
	char *res = malloc(size);
	strcpy(res, first);
	if(delimiter)
		strcat(res, delimiter);
	strcat(res, second);
	return res;
}

