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
#include <libgen.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "routines.h"
#include "constants.h"
#include "easydir.h"
#include "r-gpgme.h"
#if defined(DISPLAY)
	#include "r-x11.h"
#endif
#include "output.h"

#define NUMBER_ALLOWED_CHARS (10+26+26+8)

int copy_outside(char *password)
{
	#if defined(DISPLAY)
		int pid;
		pid = fork();
		if(pid == -1)
			errprint_r(1, "X11 fork() failed\n");
		if(pid == 0) /* new process */
			exit(run_clipboard(password));
		return 0;
	#else
		if(getenv("WAYLAND_DISPLAY") != NULL) {
			char * const wl_copy[] = {"wl-copy", password, NULL};
			int pid;
			pid = fork();
			if(pid == -1)
				errprint_r(1, "Wayland fork() failed\n");
			if(pid == 0) { /* new process */
				execvp("wl-copy", wl_copy);
				perror("wl-copy");
				exit(1);
			}

			return 0;
		}

		errprint_r(1, "You didn't have x11 or wayland when app builded\n");
	#endif
}

int check_sneaky_paths(const char *path)
{
	int length = strlen(path), i;
	for(i = 1; i < length; i++)
	{
		if(path[i-1] == '.' && path[i] == '.') {
			print_error("Error: A double dot is found in the path.\n");
			return 1;
		}
	}
	return 0;
}

char *get_pubkey()
{
	char *pubkey;

	FILE *fileGPG = fopen(".gpg-key", "r");
	if(fileGPG == NULL) {
		if(errno == ENOENT)
			errprint_r(NULL, "No GPG key exists. Use \"lpass init\"\n");
		errprint_r(NULL, "%s\n", strerror(errno));
	}

	pubkey = malloc(sizeof(char) * (maxlen_fingerprint + 1));
	if(fgets(pubkey, maxlen_fingerprint + 1, fileGPG) == NULL) {
		free(pubkey);
		errprint_ptr(&pubkey, NULL, "%s\n", strerror(errno));
	}

	fclose(fileGPG);
	return pubkey;
}

char *get_password(const char *path)
{
	return decrypt_data(path);
}

void visible_enter(int status)
{
	struct termios term_settings;
	tcgetattr(0, &term_settings); // get current settings
	if(status == 1)
		term_settings.c_lflag |= ECHO;
	else
		term_settings.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &term_settings);
}

static void mkdir_recursive(const char *path)
{
	char *subpath, *fullpath;

	fullpath = strdup(path);
	subpath = dirname(fullpath);
	if(subpath[0] != '.')
		mkdir_recursive(subpath);

	mkdir(path, S_IRWXU);
	free(fullpath);
}

int insert_pass(const char *path, const char *password)
{
	char *public_gpgkey = get_pubkey();
	if(public_gpgkey == NULL)
		return 1;

	// create directories
	char *tmp, *dirs_path;
	tmp = strdup(path);
	dirs_path = dirname(tmp);
	if(dirs_path[0] != '.')
		mkdir_recursive(dirs_path);
	free(tmp);

	int ret = ecnrypt_data(path, password, public_gpgkey);
	free(public_gpgkey);
	return ret;
}

char *get_input(int minlen, int maxlen)
{
	char *pass = malloc(sizeof(char) * (maxlen + 1));
	int len;

	if(fgets(pass, maxlen + 1, stdin) == NULL) {
		free(pass);
		errprint_ptr(&pass, NULL, "%s\n", strerror(errno));
	}

	len = strlen(pass);
	if(len < minlen) {
		free(pass);
		return NULL;
	}

	if(pass[len-1] == '\n')
		pass[len-1] = '\0';

	return pass;
}

char *gen_password(int length)
{
	int i;
	char allowed_chars[NUMBER_ALLOWED_CHARS] = 
		"abcdefghijklmnoqprstuvwyzx"
		"ABCDEFGHIJKLMNOQPRSTUYWVZX"
		"0123456789"
		"!@#$^&*?";
	char *password = malloc(sizeof(char) * (length + 1));

	srand(time(NULL));
	for(i = 0; i < length; i++) 
		password[i] = allowed_chars[rand() % NUMBER_ALLOWED_CHARS];

	password[i] = '\0';
	return password;
}

static void clear_stdin()
{
	int garbage;
	while( (garbage = fgetc(stdin)) != '\n' && garbage != EOF )
		;
}

int overwrite_answer(const char *path)
{
	int answer;
	printf("Password for \"%s\" exists. Overwrite? (Y/N)\n", path);
	while((answer = fgetc(stdin)))
	{
		clear_stdin();
		switch(answer)
		{
			case 'Y':
			case 'y': return 'y';
			case 'N':
			case 'n':
			case EOF: return 'n'; 
			default: {
				printf("Overwrite? (Y/N) ");
				break;
			}
		}
	}

	return 'n';
}
