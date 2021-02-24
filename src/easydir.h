#ifndef EASYDIR_H
#define EASYDIR_H

enum status_file {
	F_SUCCESS,
	F_NOEXIST,
	F_ISDIR
};

int checkFileExist(char *path_to_file);
char *fileCropLineFeed(char *path, char *text, int maxlen);

#endif