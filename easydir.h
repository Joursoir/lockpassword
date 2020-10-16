#ifndef EASYDIR_H
#define EASYDIR_H

char* splitPath(char *source, char *path, char *file);
int deleteFile(char *file_path);
int deleteEmptyDir(char *dir_path);
int checkFileExist(char *path_to_file);

#endif