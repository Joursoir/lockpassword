#ifndef EASYDIR_H
#define EASYDIR_H

enum status_file {
	F_ISFILE,
	F_NOEXIST,
	F_ISDIR
};

int file_exist(const char *path);
int count_dir_entries(const char *path);
char *fileCropLineFeed(char *path, char *text, int maxlen);

#endif