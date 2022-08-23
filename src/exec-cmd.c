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
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "exec-cmd.h"
#include "constants.h"
#include "easydir.h"
#include "routines.h"
#include "exec-cmd.h"
#include "tree.h"
#include "output.h"

#define print_options(msg)	print_usage("%s", msg)

int cmd_init(int argc, char *argv[])
{
	const char description[] = "init gpg-key\n";
	int retval = 0, result;
	char *gpg_key = argv[2];
	if(gpg_key == NULL) {
		print_options(description);
		return 1;
	}

	// create .gpg-key in storage
	FILE *filekey = fopen(GPGKEY_FILE, "w");	
	if(!filekey) {
		print_error("Error: %s\n", strerror(errno));
		return 1;
	}

	result = fputs(gpg_key, filekey);
	if(result == EOF) {
		print_error("Error: %s\n", strerror(errno));
		retval = 1;
	} else {
		printf("LockPassword initialized for %s\n", gpg_key);
	}

	fclose(filekey);
	return retval;
}

int cmd_insert(int argc, char *argv[])
{
	const char description[] = "insert [-ecf] passname\n";
	int retval = 0, result;
	int flag_echo = 0, flag_force = 0, flag_copy = 0;
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
			default:
				print_options(description);
				return 1;
		}
	}

	char *path = argv[optind];
	if(path == NULL) {
		print_options(description);
		return 1;
	}

	result = check_sneaky_paths(path);
	if(result) {
		print_error("Error: You have used forbidden paths\n");
		return 1;
	}

	if(file_exist(path) == F_ISFILE) {
		if(!flag_force) {
			if(overwrite_answer(path) != 'y')
				return 1;
		}
	}

	char *f_pass = NULL, *s_pass = NULL;
	if(!flag_echo) {
		visible_enter(0);

		printf("Type your password: ");
		f_pass = get_input(minlen_pass, maxlen_pass);
		printf("\n");
		if(f_pass == NULL) {
			print_error("Error: Incorrect password\n");
			retval = 1;
			goto out;
		}

		printf("Type your password again: ");
		s_pass = get_input(minlen_pass, maxlen_pass);
		printf("\n");
		if(s_pass == NULL) {
			print_error("Error: Incorrect password\n");
			retval = 1;
			goto out;
		}

		if(strcmp(f_pass, s_pass) != 0) {
			print_error("Error: Password do not match\n");
			retval = 1;
			goto out;
		}
	}
	else {
		printf("Type your password: ");
		f_pass = get_input(minlen_pass, maxlen_pass);
		if(f_pass == NULL) {
			print_error("Error: Incorrect password\n");
			return 1;
		}
	}

	result = insert_pass(path, f_pass);
	if(result) {
		print_error("Error: Can't add password to LockPassword\n");
		retval = 1;
		goto out;
	}
	if(flag_copy)
		copy_outside(f_pass);

	printf("Password added successfully for %s\n", path);

out:
	visible_enter(1);
	free(f_pass);
	free(s_pass);
	return retval;
}

int cmd_edit(int argc, char *argv[])
{
	const char description[] = "edit passname\n";
	int result, fd, pid, len_pass, save_errno;
	/* We expect tmpfs to be mounted at /dev/shm */
	char path_tmpfile[] = "/dev/shm/lpass.XXXXXX";
	char *editor, *password;
	char *path = argv[1];
	if(!path) {
		print_options(description);
		return 1;
	}

	result = check_sneaky_paths(path);
	if(result) {
		print_error("Error: You have used forbidden paths\n");
		return 1;
	}

	result = file_exist(path);
	if(result == F_NOEXIST) {
		print_error("Error: No such file exists\n");
		return 1;
	}
	if(result == F_ISDIR) {
		print_error("Error: It's a directory\n");
		return 1;
	}

	editor = getenv("EDITOR");
	if(!editor)
		editor = STD_TEXT_EDITOR;

	password = get_password(path);
	if(password == NULL) {
		print_error("Error: Decrypt password failed\n");
		return 1;
	}

	fd = mkstemp(path_tmpfile);
	if(fd == -1) {
		print_error("Error: Create temporary file failed: %s\n", strerror(errno));
		free(password);
		return 1;
	}
	dbgprint("tmp file: %s\n", path_tmpfile);

	len_pass = strlen(password);
	result = write(fd, password, len_pass);
	free(password);
	close(fd);
	if(result != len_pass) {
		print_error("Error: Write password to temporary file failed\n");
		unlink(path_tmpfile);
		return 1;
	}

	// fork for text editor
	char *editor_arg[] = {editor, path_tmpfile, NULL};
	pid = fork();
	if(pid == -1) {
		print_error("Error: Start %s failed: %s\n", editor, strerror(errno));
		unlink(path_tmpfile);
		return 1;
	}
	if(pid == 0) { /* new process */
		execvp(editor, editor_arg);
		perror(editor);
		exit(1);
	}
	wait(&pid);

	fd = open(path_tmpfile, O_RDONLY);
	if(fd == -1) {
		print_error("Error: Open temporary file failed: %s\n", strerror(errno));
		unlink(path_tmpfile);
		return 1;
	}

	password = malloc(sizeof(char) * (maxlen_pass + 1));
	len_pass = read(fd, password, maxlen_pass);
	save_errno = errno;
	close(fd);
	unlink(path_tmpfile);
	if(len_pass < minlen_pass) {
		if(len_pass == -1)
			print_error("Error: Read temporary file failed: %s\n", strerror(save_errno));
		else
			print_error("Error: Min. password length is %d\n", minlen_pass);
		free(password);
		return 1;
	}
	password[len_pass-1] = '\0';
	dbgprint("new pass: %s\n", password);

	// encrypt
	result = insert_pass(path, password);
	free(password);
	if(result) {
		print_error("Error: Can't add password to LockPassword\n");
		return 1;
	}

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
			default: 
				print_options(description);
				return 1;
		}
	}

	char *path = argv[optind];
	if(path == NULL) {
		print_options(description);
		return 1;
	}

	if(pass_length < minlen_pass || pass_length > maxlen_pass) {
		print_error("Error: You typed an incorrect length\n");
		return 1;
	}

	result = check_sneaky_paths(path);
	if(result) {
		print_error("Error: You have used forbidden paths\n");
		return 1;
	}

	result = file_exist(path);
	if(result == F_ISFILE) {
		if(!flag_force) {
			if(overwrite_answer(path) != 'y')
				return 1;
		}
	}
	else if(result == F_ISDIR) {
		print_error("Error: You can't generate password for directory\n");
		return 1;
	}

	// generate password 
	char *g_pass = gen_password(pass_length);

	result = insert_pass(path, g_pass);
	if(result) {
		print_error("Error: Can't add password to LockPassword\n");
		free(g_pass);
		return 1;
	}

	if(flag_copy)
		copy_outside(g_pass);
	else
		printf("Generated password: %s\n", g_pass);
	printf("Password added successfully for %s\n", path);
	free(g_pass);
	return 0;
}

int cmd_remove(int argc, char *argv[])
{
	const char description[] = "rm passname\n";
	int result;
	char *path = argv[1];
	if(!path) {
		print_options(description);
		return 1;
	}

	result = check_sneaky_paths(path);
	if(result)
		errprint_r(1, "You have used forbidden paths\n");

	result = file_exist(path);
	if(result == F_NOEXIST)
		errprint_r(1, "No such file exists\n");
	if(result == F_ISDIR) {
		if(count_dir_entries(path) != 0)
			errprint_r(1, "Directory not empty\n");
	}

	result = remove(path);
	if(result)
		errprint_r(1, "%s\n", strerror(errno));
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
			default: 
				print_options(description);
				return 1;
		}
	}

	if(!argv[optind] || !argv[optind+1]) {
		print_options(description);
		return 1;
	}

	char *old_path = argv[optind];
	char *new_path = argv[optind+1];
	dbgprint("old-path = %s\n", old_path);
	dbgprint("new-path = %s\n", new_path);

	result = check_sneaky_paths(old_path);
	if(result)
		errprint_r(1, "You have used forbidden paths\n");
	result = file_exist(old_path);
	if(result == F_NOEXIST)
		errprint_r(1, "No such file exists\n");

	result = check_sneaky_paths(new_path);
	if(result)
		errprint_r(1, "You have used forbidden paths\n");
	result = file_exist(new_path);
	if(result != F_NOEXIST) {
		if(!flag_force) {
			if(overwrite_answer(new_path) != 'y')
				return 1;
		}
	}

	result = rename(old_path, new_path);
	if(result)
		errprint_r(1, "%s\n", strerror(errno));
	return 0;
}

int cmd_help(int argc, char *argv[])
{
	printf("Synopsis:\n"
		"\tlpass command [arguments] ...\n"

		"Commands:\n"
		"\tinit gpg-key\n"
		"\t\tInitialize the password manager using the passed gpg-key.\n"
		"\tinsert [-e, --echo] [-c, --copy] [-f, --force] passname\n"
		"\t\tAdd the specified passname to the password manager.\n"
		"\tshow [-c, --copy] [-C, --no-color] [passname]\n"
		"\t\tIf the specified passname is file, decrypt and print a password of passname. "
		"Otherwise list passnames inside the tree at passname.\n"
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
	const char description[] = "[-cC] [passname]\n";
	int flag_copy = 0, flag_color = 1;
	int retval = 0, result;
	char *path;
	const struct option long_options[] = {
		{"copy", no_argument, NULL, 'c'},
		{"no-color", no_argument, NULL, 'C'},
		{NULL, 0, NULL, 0}
	};

	while((result = getopt_long(argc, argv, "cC", long_options, NULL)) != -1) {
		switch(result) {
			case 'c': { flag_copy = 1; break; }
			case 'C': { flag_color = 0; break; }
			default:
				print_options(description);
				return 1;
		}
	}

	if(argv[optind]) {
		result = check_sneaky_paths(argv[optind]);
		if(result)
			errprint_r(1, "You have used forbidden paths\n");
		path = malloc(sizeof(char) * (strlen(argv[optind]) + 1));
		strcpy(path, argv[optind]);
	}
	else {
		path = malloc(sizeof(char) * 2);
		strcpy(path, ".");
	}

	do { // START_DO

		result = file_exist(path);
		if(result == F_ISDIR)
		{
			if(flag_copy) {
				errprint_ptr(&retval, 1,
					"You must type a passname, not a directory\n");
				break;
			}

			if(strcmp(path, ".") == 0)
				printf("Password Manager\n");
			else
				printf("Password Manager/%s\n", path);
			tree(path, "", flag_color);
		}
		else if(result == F_ISFILE)
		{
			char *pass = get_password(path);
			if(!pass) {
				errprint_ptr(&retval, 1, "Decrypt password failed\n");
				break;
			}

			if(flag_copy)
				copy_outside(pass);
			else
				printf("%s\n", pass);

			free(pass);
		}
		else
			errprint_ptr(&retval, 1, 
				"This path is not in the password storage\n");

	} while(0); // END_DO

	free(path);
	return retval;
}
