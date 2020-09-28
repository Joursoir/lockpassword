#include <stdio.h>
#include <stdlib.h>

void call_error(int num)
{
	fprintf(stderr, "Sorry, there was an error in the program [#%d]\n", num);
	exit(3);
}

void print_error(char *text)
{
	fprintf(stderr, "%s", text);
	exit(4);
}

