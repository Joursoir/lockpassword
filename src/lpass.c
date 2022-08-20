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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "constants.h"
#include "exec-cmd.h"
#include "xstd.h"
#include "output.h"

struct cmd_struct {
	const char *cmd;
	int (*func)(int, char **);
};

static struct cmd_struct commands[] = {
	{ "init", cmd_init },
	{ "insert", cmd_insert },
	{ "show", cmd_showtree },
	{ "edit", cmd_edit },
	{ "generate", cmd_generate },
	{ "rm", cmd_remove },
	{ "mv", cmd_move },
	{ "help", cmd_help },
	{ "version", cmd_version },
	{ NULL, NULL }
};

static struct cmd_struct *get_cmd(const char *name)
{
	if(!name)
		return NULL;

	struct cmd_struct *ptr;
	for(ptr = commands; ptr->cmd; ptr++) {
		if(strcmp(name, ptr->cmd) == 0)
			return ptr;
	}
	return NULL;
}

static int goto_maindir()
{
	char *rootdir = xstrcat(getenv("HOME"), LOCKPASS_DIR, "/");
	int retval = chdir(rootdir);
	if(retval) // failed
	{
		if(errno == ENOENT) {
			// create main directory:
			retval = mkdir(rootdir, S_IRWXU);
			if(retval && errno != EEXIST) {
				print_error("Error: %s\n", strerror(errno));
			} else { // try again:
				retval = chdir(rootdir);
			}
		} else {
			print_error("Error: %s\n", strerror(errno));
		}
	}

	free(rootdir);
	return retval;
}

int main(int argc, char *argv[])
{
	if(!isatty(STDIN_FILENO)) {
		print_error("Please, use a terminal to run this application\n");
		return 1;
	}

	if(goto_maindir())
		return 1;

	struct cmd_struct *ptr = get_cmd(argv[1]);
	if(ptr)
		return ptr->func(--argc, ++argv);
	
	cmd_help(argc, argv);
	return 1;
}