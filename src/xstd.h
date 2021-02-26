#ifndef LPASS_XSTD_H
#define LPASS_XSTD_H

void easyFork(char *name, char *arguments[]);
void callError(int num);
void printError(const char *text);
char *xstrcat(const char *first, const char *second,
	const char *delimiter);

#endif