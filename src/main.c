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
#include <getopt.h>
#include <dirent.h>
#include <libgen.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "constants.h"
#include "easydir.h"
#include "xstd.h"
#include "implementation.h"
#include "exec-cmd.h"
#include "tree.h"

struct cmd_struct {
	const char *cmd;
	int (*func)(int, char **);
};

static struct cmd_struct commands[] = {
	{ "init", cmd_init },
	{ "insert", cmd_insert },
	{ "edit", cmd_edit },
	{ "generate", cmd_generate },
	{ "rm", cmd_remove },
	{ "mv", cmd_move },
	{ "help", cmd_help },
	{ "version", cmd_version },
	{ NULL, NULL }
};

int cmd_init(int argc, char *argv[])
{
	const char description[] = "init gpg-key\n";
	char *gpg_key = argv[2];
	if(gpg_key == NULL)
		usageprint("%s", description);

	// create .gpg-key in storage
	FILE *filekey = fopen(GPGKEY_FILE, "w");	
	if(!filekey)
		errprint(1, "fopen() failed");

	fputs(gpg_key, filekey);
	fclose(filekey);

	printf("LockPassword initialized for %s\n", gpg_key);
	return 0;
}

int cmd_insert(int argc, char *argv[])
{
	const char description[] = "insert [-ecf] passname\n";
	int flag_echo = 0, flag_force = 0, flag_copy = 0, result;
	const struct option long_options[] = {
		{"echo", no_argument, NULL, 'e'},
		{"force", no_argument, NULL, 'f'},
		{"copy", no_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};

	while((result = getopt_long(argc, argv, "efc", long_options, NULL)) != -1) {
		switch(result) {
			case 'e': { flag_echo = 1; break; }
			case 'f': { flag_force = 1; break; }
			case 'c': { flag_copy = 1; break; }
			default: usageprint("%s", description);
		}
	}

	if(optind < argc) optind++; // for skip "insert"
	dbgprint("passname: %s\n", argv[optind]);

	char *path = argv[optind];
	if(path == NULL)
		usageprint("%s", description);

	result = check_sneaky_paths(path);
	if(result)
		errprint(1, "You have used forbidden paths\n");

	if(file_exist(path) == F_ISFILE) {
		if(!flag_force) {
			if(overwrite_answer(path) != 'y')
				return 1;
		}
	}

	char *f_pass;
	if(!flag_echo) {
		char *s_pass;
		visible_enter(0);

		fputs("Type your password: ", stdout);
		f_pass = get_input(minlen_pass, maxlen_pass);
		fputs("\n", stdout);
		if(f_pass == NULL) {
			visible_enter(1);
			errprint(1, "Incorrect password");
		}

		fputs("Type your password again: ", stdout);
		s_pass = get_input(minlen_pass, maxlen_pass);
		fputs("\n", stdout);
		visible_enter(1);
		if(s_pass == NULL) {
			free(f_pass);
			errprint(1, "Incorrect password");
		}

		if(strcmp(f_pass, s_pass) != 0) {
			free(f_pass);
			free(s_pass);
			errprint(1, "Password do not match");
		}
		free(s_pass);
	}
	else {
		fputs("Type your password: ", stdout);
		f_pass = get_input(minlen_pass, maxlen_pass);
		if(f_pass == NULL)
			errprint(1, "Incorrect password");
	}

	result = insert_pass(path, f_pass);
	if(result) {
		free(f_pass);
		errprint(1, "Can't add password to LockPassword");
	}
	if(flag_copy)
		copy_outside(f_pass);

	printf("Password added successfully for %s\n", path);
	free(f_pass);
	return 0;
}

int cmd_edit(int argc, char *argv[])
{
	const char description[] = "edit passname\n";
	int result, fd, pid, len_pass, save_errno;
	/* We expect tmpfs to be mounted at /dev/shm */
	char path_tmpfile[] = "/dev/shm/lpass.XXXXXX";
	char *editor, *password;
	char *path = argv[2];
	if(!path)
		usageprint("%s", description);

	result = check_sneaky_paths(path);
	if(result)
		errprint(1, "You have used forbidden paths\n");

	editor = getenv("EDITOR");
	if(!editor)
		editor = STD_TEXT_EDITOR;

	password = get_password(path);
	if(password == NULL)
		errprint(1, "Decrypt password failed\n");

	fd = mkstemp(path_tmpfile);
	if(fd == -1) {
		free(password);
		errprint(1, "mkstemp() failed\n");
	}
	dbgprint("tmp file: %s\n", path_tmpfile);

	len_pass = strlen(password);
	result = write(fd, password, len_pass);
	free(password);
	close(fd);
	if(result != len_pass) {
		unlink(path_tmpfile);
		errprint(1, "Write password to temporary file failed\n");
	}

	// fork for text editor
	char *editor_arg[] = {editor, path_tmpfile, NULL};
	pid = fork();
	if(pid == -1) {
		unlink(path_tmpfile);
		errprint(1, "%s fork() failed\n", editor);
	}
	if(pid == 0) { /* new process */
		execvp(editor, editor_arg);
		perror(editor);
		exit(1);
	}
	wait(&pid);

	fd = open(path_tmpfile, O_RDONLY);
	if(fd == -1) {
		unlink(path_tmpfile);
		perror("open");
		return 1;
	}

	password = malloc(sizeof(char) * (maxlen_pass + 1));
	len_pass = read(fd, password, maxlen_pass);
	save_errno = errno;
	close(fd);
	unlink(path_tmpfile);
	if(len_pass < minlen_pass) {
		free(password);
		if(len_pass == -1)
			errprint(1, "Read temporary file: %s\n", strerror(save_errno));
		else
			errprint(1, "Min. password length is %d\n", minlen_pass);
	}
	password[len_pass-1] = '\0';
	dbgprint("new pass: %s\n", password);

	// encrypt
	result = insert_pass(path, password);
	free(password);
	if(result)
		errprint(1, "Can't add password to LockPassword\n");

	return 0;
}

int cmd_generate(int argc, char *argv[])
{
	const char description[] = "generate [-l=pass-length] [-f] passname\n";
	int pass_length = stdlen_pass;
	int flag_force = 0, flag_copy = 0, result;
	const struct option long_options[] = {
		{"length", required_argument, NULL, 'l'},
		{"force", no_argument, NULL, 'f'},
		{"copy", no_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};

	while((result = getopt_long(argc, argv, "l:fc", long_options, NULL)) != -1) {
		switch(result) {
			// if optarg - incorrect number, atoi return 0
			case 'l': { pass_length = atoi(optarg); break; }
			case 'f': { flag_force = 1; break; }
			case 'c': { flag_copy = 1; break; }
			default: usageprint("%s", description);
		}
	}

	if(optind < argc) optind++; // for skip "generate"
	dbgprint("passname: %s\n", argv[optind]);

	char *path = argv[optind];
	if(path == NULL)
		usageprint("%s", description);

	if(pass_length < minlen_pass || pass_length > maxlen_pass)
		errprint(1, "You typed an incorrect length\n");

	result = check_sneaky_paths(path);
	if(result)
		errprint(1, "You have used forbidden paths\n");

	result = file_exist(path);
	if(result == F_ISFILE) {
		if(!flag_force) {
			if(overwrite_answer(path) != 'y')
				return 1;
		}
	}
	else if(result == F_ISDIR)
		errprint(1, "You can't generate password for directory\n");

	// generate password 
	char g_pass[pass_length];
	gen_password(g_pass, pass_length);

	result = insert_pass(path, g_pass);
	if(result)
		errprint(1, "Can't add password to LockPassword");

	if(flag_copy)
		copy_outside(g_pass);
	else
		printf("Generated password: %s\n", g_pass);
	printf("Password added successfully for %s\n", path);
	return 0;
}

int cmd_remove(int argc, char *argv[])
{
	const char description[] = "rm passname\n";
	int result;
	char *path = argv[2];
	if(!path)
		usageprint("%s", description);

	result = check_sneaky_paths(path);
	if(result)
		errprint(1, "You have used forbidden paths\n");

	result = file_exist(path);
	if(result == F_NOEXIST)
		errprint(1, "No such file exists\n");
	if(result == F_ISDIR) {
		if(count_dir_entries(path) != 0)
			errprint(1, "Directory not empty\n");
	}

	remove(path);
	return 0;
}

int cmd_move(int argc, char *argv[])
{
	/* we have a two situation:
	1) mv file file
	2) mv file directory */
	
	const char description[] = "mv [-f] old-path new-path\n";
	const struct option long_options[] = {
		{"force", no_argument, NULL, 'f'},
		{NULL, 0, NULL, 0}
	};

	int result, flag_force = 0;
	while((result = getopt_long(argc, argv, "f", long_options, NULL)) != -1) {
		switch(result) {
			case 'f': { flag_force = 1; break; }
			default: usageprint("%s", description);
		}
	}

	if(optind < argc) optind++; // for skip "move"
	if(!argv[optind] || !argv[optind+1])
		usageprint("%s", description);

	char *old_path = argv[optind];
	char *new_path = argv[optind+1];
	dbgprint("old-path = %s\n", old_path);
	dbgprint("new-path = %s\n", new_path);

	result = check_sneaky_paths(old_path);
	if(result)
		errprint(1, "You have used forbidden paths\n");
	result = file_exist(old_path);
	if(result == F_NOEXIST)
		errprint(1, "No such file exists\n");

	result = check_sneaky_paths(new_path);
	if(result)
		errprint(1, "You have used forbidden paths\n");
	result = file_exist(new_path);
	if(result != F_NOEXIST) {
		if(!flag_force) {
			if(overwrite_answer(new_path) != 'y')
				return 1;
		}
	}

	if(rename(old_path, new_path))
		perror("rename");
	return 0;
}

int cmd_help(int argc, char *argv[])
{
	printf("Synopsis:\n"
		"\tlpass [command] [arguments] ...\n"

		"Commands:\n"
		"\tinit gpg-key\n"
		"\t\tInitialize the password manager using the passed gpg-key.\n"
		"\tinsert [-e, --echo] [-c, --copy] [-f, --force] passname\n"
		"\t\tAdd the specified passname to the password manager.\n"
		"\tedit [-t, --text-editor=text-editor] passname\n"
		"\t\tOpen the specified passname in a text editor, waiting for changes.\n"
		"\tgenerate [-l, --length=pass-length] [-c, --copy] [-f, --force] passname\n"
		"\t\tGenerate a random password and write it in passname.\n"
		"\tmv [-f, --force] old-path new-path\n"
		"\t\tMove/rename old-path to new-path.\n"
		"\trm passname\n"
		"\t\tRemove the passname you specified from the password manager.\n"
		"\thelp\n"
		"\t\tPrint help information about commands and the application itself.\n"
		"\tversion\n"
		"\t\tPrint version information.\n"

		"\nMore information may be found in the lpass(1) man page.\n");
	return 0;
}

int cmd_version(int argc, char *argv[])
{
	printf("LockPassword v%s\n"
		"Release date: %s\n\n"
		"Copyright (C) 2020-2021 Aleksandr D. Goncharov (Joursoir)\n"
		"License: GNU GPL version 3\n"
		"This is free software: you are free to change and redistribute it.\n"
		"This program comes with ABSOLUTELY NO WARRANTY.\n",
		VERSION, DATE_RELEASE);
	return 0;
}

int cmd_showtree(int argc, char *argv[])
{
	const char description[] = "[-c] [passname]\n";
	int flag_copy = 0, result;
	char *path;
	const struct option long_options[] = {
		{"copy", no_argument, NULL, 'c'},
		{NULL, 0, NULL, 0}
	};

	while((result = getopt_long(argc, argv, "c", long_options, NULL)) != -1) {
		switch(result) {
			case 'c': { flag_copy = 1; break; }
			default: usageprint("%s", description);
		}
	}

	if(argv[optind]) {
		result = check_sneaky_paths(argv[optind]);
		if(result)
			errprint(1, "You have used forbidden paths\n");
		path = malloc(sizeof(char) * (strlen(argv[optind]) + 1));
		strcpy(path, argv[optind]);
	}
	else {
		path = malloc(sizeof(char) * 2);
		strcpy(path, ".");
	}

	result = file_exist(path);
	if(result == F_ISDIR)
	{
		if(flag_copy)
			errprint(1, "You must type a passname, not a directory\n");

		if(strcmp(path, ".") == 0) printf("Password Manager\n");
		else printf("Password Manager/%s\n", path);
		tree(path, "");
	}
	else if(result == F_ISFILE)
	{
		char *pass = get_password(path);
		if(pass == NULL) {
			free(path);
			errprint(1, "Decrypt password failed\n");
		}
		if(flag_copy)
			copy_outside(pass);
		else
			printf("%s\n", pass);

		free(pass);
	}
	else {
		free(path);
		errprint(1, "This path is not in the password storage\n");
	}

	free(path);
	return 0;
}

static struct cmd_struct *get_cmd(const char *name)
{
	struct cmd_struct *ptr;
	for(ptr = commands; ptr->cmd; ptr++) {
		if(strcmp(name, ptr->cmd) == 0)
			return ptr;
	}
	return NULL;
}

static int goto_maindir()
{
	int ret = 0;
	char *rootdir = xstrcat(getenv("HOME"), LOCKPASS_DIR, "/");
	if(chdir(rootdir)) // failed
	{
		// create main directory:
		int res = mkdir(rootdir, S_IRWXU);
		if(res) {
			if(errno != EEXIST) {
				perror("mkdir");
				ret = 1;
			}
		}
		else // try again:
			ret = chdir(rootdir);
	}

	free(rootdir);
	return ret;
}

int main(int argc, char *argv[])
{
	if(!isatty(STDIN_FILENO))
		errprint(1, "Please, use a terminal to run this application\n");

	if(goto_maindir())
		perror("chdir");

	int ret = 0;
	char *cmd = (argv[1] != NULL) ? argv[1] : "";
	struct cmd_struct *ptr;
	if((ptr = get_cmd(cmd)))
		ret = ptr->func(argc, argv);
	else
		ret = cmd_showtree(argc, argv);
	
	return ret;
}