#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "tree.h"

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

static int is_dir(const char *path)
{
	struct stat buffer;
	if(stat(path, &buffer))
		return 0;
	return S_ISDIR(buffer.st_mode);
}

static int count_dir_entries(const char *path)
{
	int counter = 0;
	DIR *dir;
	struct dirent *dir_entry;

	dir = opendir(path);
	if(dir == NULL) {
		fprintf(stderr, "opendir() failed\n");
		return -1;
	}

	errno = 0;
	while((dir_entry = readdir(dir))) {
		if(dir_entry->d_name[0] == '.')
			continue;
		counter++;
	}
	if(errno) {
		fprintf(stderr, "readdir() failed\n");
		return -1;
	}
	closedir(dir);
	return counter;
}

static char *xstrcat(const char *first, const char *second,
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

int tree(const char *path, const char *prefix)
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
	if(errno) {
		perror("opendir");
		return 1;
	}

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
		if(is_dir(full_path)) {
			printf("%s%s%s%s%s\n", prefix, pointer, ANSIC_BBLU,
				entries[i], ANSIC_RST);

			prefix_depth = xstrcat(prefix, prefix_depth, NULL);
			tree(full_path, prefix_depth);
			free(prefix_depth);
		}
		else
			printf("%s%s%s\n", prefix, pointer, entries[i]);

		free(entries[i]);
		free(full_path);
	}

	free(entries);
	return 0;
}