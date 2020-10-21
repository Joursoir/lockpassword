#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "handerror.h"

int deleteFile(char *file_path)
{
	char *arguments[] = {"rm", file_path, NULL};
	easyFork("rm", arguments);

	return 1;
}

int deleteEmptyDir(char *dir_path)
{
	#if defined(DEBUG)
		char *arguments[] = {"rmdir", "-p", dir_path, NULL};
	#else
		char *arguments[] = {"rmdir", "-p", "--ignore-fail-on-non-empty", dir_path, NULL};
	#endif
	easyFork("rmdir", arguments);

	return 1;
}

int checkFileExist(char *path_to_file)
{
	FILE *pFile;

	pFile = fopen(path_to_file, "r");
	if(pFile == NULL) {
		if(errno == ENOENT) { // file doesn't exist
			return 0;
		}
		else callError(120);
	}
	fclose(pFile);

	return 1;
}
