#include <stdio.h>
#include <stdlib.h>

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

