#ifndef EASYDIR_H
#define EASYDIR_H

int deleteFile(char *file_path);
int deleteEmptyDir(char *dir_path);
int checkFileExist(char *path_to_file);
char *fileCropLineFeed(char *path, char *text, int maxlen);

#endif