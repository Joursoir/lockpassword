#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>

#include "easydir.h"
#include "xstd.h"

int file_exist(const char *path)
{
	FILE *file = fopen(path, "r+"); // r+ so that errno can equal EISDIR
	if(!file)
		return errno == EISDIR ? F_ISDIR : F_NOEXIST;
	fclose(file);
	return F_ISFILE;
}

int count_dir_entries(const char *path)
{
	int counter = 0;
	DIR *dir;
	struct dirent *dir_entry;

	dir = opendir(path);
	if(dir == NULL)
		return 0;

	while((dir_entry = readdir(dir))) {
		if(dir_entry->d_name[0] == '.')
			continue;
		counter++;
	}
	closedir(dir);
	return counter;
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
