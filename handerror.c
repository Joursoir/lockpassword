#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void callError(int num)
{
	fprintf(stderr, "lpass: Sorry, there was an error in the program [#%d]\n", num);
	exit(3);
}

void printError(char *text)
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

