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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "tree.h"
#include "xstd.h"
#include "easydir.h"

#define ANSIC_RST  "\x1B[0m"
#define ANSIC_BBLU  "\x1B[34;1m"
#define ANSIC_BGRN  "\x1B[32;1m"

static void entries_sort(char **entries, const int size)
{
	int i, j;
	char *temp;
	for(i = 0; i < size; i++) {
		for(j = i + 1; j < size; j++) {
			if(strcmp(entries[i], entries[j]) > 0)
			{
				temp = entries[i];
				entries[i] = entries[j];
				entries[j] = temp;
			}
		}
	}
}

int tree(const char *path, const char *prefix, int use_color)
{
	DIR *main_dir;
	struct dirent *temp_dirent;
	char **entries;
	char *pointer, *prefix_depth;
	int cnt_ent, i;

	cnt_ent = count_dir_entries(path);
	if(cnt_ent == -1)
		return 1;
	entries = malloc(sizeof(char *) * cnt_ent);

	main_dir = opendir(path);
	if(main_dir == NULL) {
		perror("opendir");
		return 1;
	}

	i = 0;
	while((temp_dirent = readdir(main_dir))) {
		char *file_name = temp_dirent->d_name;
		if(file_name[0] == '.')
			continue;
		entries[i] = malloc(sizeof(char) * (strlen(file_name) + 1));
		strcpy(entries[i], file_name);
		i++;
	}
	closedir(main_dir);

	entries_sort(entries, cnt_ent);
	for(i = 0; i < cnt_ent; i++) {
		char *full_path;
		if(i == cnt_ent - 1) {
			pointer = "└── ";
			prefix_depth = "    ";
		}
		else {
			pointer = "├── ";
			prefix_depth = "│   ";
		}

		full_path = xstrcat(path, entries[i], "/");
		printf("%s%s", prefix, pointer);
		if(file_exist(full_path) == F_ISDIR) {
			printf("%s%s%s\n", 
				(use_color) ? ANSIC_BBLU : "",
				entries[i],
				(use_color) ? ANSIC_RST : "");

			prefix_depth = xstrcat(prefix, prefix_depth, NULL);
			tree(full_path, prefix_depth, use_color);
			free(prefix_depth);
		}
		else
			printf("%s\n", entries[i]);

		free(entries[i]);
		free(full_path);
	}

	free(entries);
	return 0;
}
