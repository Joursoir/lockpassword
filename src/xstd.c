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

