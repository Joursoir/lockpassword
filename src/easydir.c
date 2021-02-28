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
