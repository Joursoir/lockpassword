#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "easydir.h"
#include "handerror.h"

int checkFileExist(char *source)
{
	FILE *file = fopen(source, "r+"); // r+ so that errno can equal EISDIR
	if(!file)
		return errno == EISDIR ? F_ISDIR : F_NOEXIST;
	fclose(file);
	return F_NOEXIST;
}

char *fileCropLineFeed(char *path, char *text, int maxlen)
{
	FILE *file = fopen(path, "r+");	
	if(file == NULL) callError(130);
	
	int symbol;
	int pos = 0;
	char *str = (char *) malloc(sizeof(char) * maxlen);
	while((symbol = fgetc(file)))
	{
		switch(symbol)
		{
			case '\n':
			case EOF: {
				str[pos] = '\0';
				pos = -1; // for break while
				break;
			}
			default: {
				str[pos] = symbol;
				pos++;
				break;
			}
		}
		if(pos == -1) break;
		if(pos > maxlen-1) { str[pos-1] = '\0'; break; }
	}
	fclose(file);

	strcpy(text, str);
	free(str);
	return text;
}
