#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "handerror.h"

/* Buff size: source == path = file
splitPath thinks, that in end path always stay FILE, not directory */
char* splitPath(char *source, char *path, char *file)
{
	int fSymbol = 0, f = 0;

	char *main_path = malloc(sizeof(char) * strlen(source) + 1);
	char *file_path = malloc(sizeof(char) * strlen(source) + 1);

	for(int i=0; i < strlen(source); i++)
	{
		if(fSymbol == 1)
		{
			switch(source[i])
			{
			case '/':
			{
				fSymbol = 0;
				f = 0;

				strcat(main_path, file_path);
				strcat(main_path, "/");
				file_path[0] = '\0';
				break;
			}
			default:
			{
				file_path[f] = source[i];
				file_path[f+1] = '\0';
				f++;

				break;
			}

			}
		}
		else // if it's beginning of string
		{
			// handling first symbol
			switch(source[i])
			{
			case '.':
			case '\\':
			case '/':
			{
				print_error("[Error] You can't use these symbol at the beginning: '.', '/', '\\' \n");
				break;
			}
			default:
				fSymbol = 1;

				// enter first symbol
				file_path[0] = source[i];
				file_path[1] = '\0';
				f++;

				break;
			}
		}
	}

	strcpy(path, main_path);
	strcpy(file, file_path);
	free(main_path);
	free(file_path);

	if(*file) return file;
	return NULL;
}

int delete_file(char *file_path)
{
	int pid;
	pid = fork();
	if(pid == -1) call_error(112);
	if(pid == 0) { /* new process */
		execlp("rm", "rm", file_path, NULL);
		perror("rm");
		exit(4);
	}
	wait(&pid);

	return 1;
}

int delete_emptydir(char *dir_path)
{
	int pid;
	pid = fork();
	if(pid == -1) call_error(113);
	if(pid == 0) { /* new process */
		#if defined(DEBUG)
			execlp("rmdir", "rmdir", "-p", dir_path, NULL);
		#else
			execlp("rmdir", "rmdir", "-p", "--ignore-fail-on-non-empty", dir_path, NULL);
		#endif
		perror("rmdir");
		exit(4);
	}
	wait(&pid);

	return 1;
}
