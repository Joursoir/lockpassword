/*
**	Code written by Joursoir
**	
**	This is free and unencumbered software released into the public domain.
**  (C) The Unlicense
*/

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

#include "easydir.h"
#include "handerror.h"
#include "implementation.h"
#include "exec-cmd.h"

enum constants {
	maxlen_texteditor = 16,
	minlen_pass = 1,
	maxlen_pass = 128,
	stdlen_pass = 14
};

#define VERSION "1.0c"
#define DATE_RELEASE "14 January, 2021"
#define STANDARD_TEXTEDITOR "vim"
#define MAXLEN_TEXTEDITOR 16
#define LOCKPASS_DIR ".lock-password/"
#define GPGKEY_FILE ".gpg-key"

#define TREE_OUTPUT_FILE ".tree"
#define TEXTEDITOR_FILE ".text-editor"

#define usageprint(...) \
	do { \
		fprintf(stdout, "Usage: lpass " __VA_ARGS__); \
		return 1; \
	} while(0)
#define errprint(...) \
	do { \
		fprintf(stderr, "Error: " __VA_ARGS__); \
		return 1; \
	} while(0)
#ifdef DEBUG
	#define dbgprint(...) fprintf(stderr, "Debug: " __VA_ARGS__)
#else
	#define dbgprint(...)
#endif

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

// == global var == 
char *gPath_subdir = NULL; // example: programming/github.com
char *gPath_pass = NULL; // example: programming/github.com/joursoir.gpg

static void globalSplitPath(char *source)
{
	int len_path = strlen(source) + strlen(".gpg") + 1;	

	gPath_pass = malloc(sizeof(char) * len_path); // path without working dir
	strcpy(gPath_pass, source);
	strcat(gPath_pass, ".gpg");

	gPath_subdir = malloc(sizeof(char) * len_path); // path without working dir and pass file
	strcpy(gPath_subdir, source);
	dirname(gPath_subdir);

	dbgprint("g_suddir: %s\n", gPath_subdir);
	dbgprint("g_pass: %s\n", gPath_pass);
}

int cmd_init(int argc, char *argv[])
{
	const char description[] = "init gpg-key\n";
	char *gpg_key = argv[2];
	if(gpg_key == NULL)
		usageprint("%s", description);

	// create .gpg-key in storage
	FILE *filekey = fopen(GPGKEY_FILE, "w");	
	if(!filekey)
		errprint("fopen() failed");

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

	result = checkForbiddenPaths(path);
	if(result)
		errprint("You have used forbidden paths\n");

	if(checkFileExist(path) == F_SUCCESS) {
		if(!flag_force) {
			if(getOverwriteAnswer(path) != OW_YES)
				return 1;
		}
	}

	char *f_pass;
	if(!flag_echo) {
		char *s_pass;
		nonvisibleEnter(1); // change terminal work

		fputs("Type your password: ", stdout);
		f_pass = getInput(minlen_pass, maxlen_pass);
		fputs("\n", stdout);
		if(f_pass == NULL) {
			nonvisibleEnter(0);
			errprint("Incorrect password");
		}

		fputs("Type your password again: ", stdout);
		s_pass = getInput(minlen_pass, maxlen_pass);
		fputs("\n", stdout);
		nonvisibleEnter(0);
		if(s_pass == NULL) {
			free(f_pass);
			errprint("Incorrect password");
		}

		if(strcmp(f_pass, s_pass) != 0) {
			free(f_pass);
			free(s_pass);
			errprint("Password do not match");
		}
		free(s_pass);
	}
	else {
		fputs("Type your password: ", stdout);
		f_pass = getInput(minlen_pass, maxlen_pass);
		if(f_pass == NULL)
			errprint("Incorrect password");
	}

	result = insertPass(path, f_pass, flag_copy);
	if(result)
		errprint("Can't add password to LockPassword");

	printf("Password added successfully for %s\n", path);
	return 0;
}

int cmd_edit(int argc, char *argv[])
{
	const char description[] = "edit [-t=text-editor] passname\n";
	const struct option long_options[] = {
		{"text-editor", required_argument, NULL, 't'},
		{NULL, 0, NULL, 0}
	};

	int result;
	while((result = getopt_long(argc, argv, "t:", long_options, NULL)) != -1) {
		switch(result) {
			case 't':
			{
				// create file, copy name text editor there
				FILE *f_texteditor = fopen(TEXTEDITOR_FILE, "w");	
				if(!f_texteditor)
					errprint("fopen() failed");
				fputs(optarg, f_texteditor);
				fclose(f_texteditor);
				printf("You changed text editor to %s\n", optarg);
				break;
			}
			default: usageprint("%s", description);
		}
	}

	if(optind < argc) optind++; // for skip "edit"
	char *path_to_password = argv[optind];
	if(argv[optind] == NULL)
		usageprint("%s", description);
	dbgprint("passname: %s\n", argv[optind]);

	result = checkForbiddenPaths(path_to_password);
	if(result)
		errprint("You have used forbidden paths\n");
	globalSplitPath(path_to_password);

	result = checkFileExist(gPath_pass);
	if(result != F_SUCCESS) {
		if(result == F_ISDIR) errprint("It is a directory\n");
		errprint("No such file exists\n");
	}

	// configure text editor file 
	char text_editor[MAXLEN_TEXTEDITOR];
	FILE *f_texteditor = fopen(TEXTEDITOR_FILE, "r");	
	if(f_texteditor == NULL) {
		f_texteditor = fopen(TEXTEDITOR_FILE, "w");	
		if(f_texteditor == NULL)
			errprint("fopen() failed");
		fputs(STANDARD_TEXTEDITOR, f_texteditor); // in file
		strcpy(text_editor, STANDARD_TEXTEDITOR); // in variable
	}
	else {
		if(!fgets(text_editor, sizeof(char)*MAXLEN_TEXTEDITOR, f_texteditor))
			errprint("fgets() failed");
	}
	fclose(f_texteditor);

	dbgprint("text editor: %s\n", text_editor);
	// end configure

	// decryption
	char *public_gpgkey = getGPGKey();

	char *decrypt_arg[] = {"gpg", "-d", "--quiet", "-r", public_gpgkey, "-o", path_to_password, gPath_pass, NULL};
	easyFork("gpg", decrypt_arg);

	// start vim/etc for edit passowrd
	char *editor_arg[] = {text_editor, path_to_password, NULL};
	easyFork(text_editor, editor_arg);

	// delete '\n' and paste good pass
	char password[maxlen_pass];
	fileCropLineFeed(path_to_password, password, maxlen_pass);

	FILE *file = fopen(path_to_password, "w");	
	if(file == NULL) callError(108);
	fputs(password, file);
	fclose(file);

	// encryption
	char *encrypt_arg[] = {"gpg", "--quiet", "--yes", "-r", public_gpgkey, "-e", path_to_password, NULL};
	easyFork("gpg", encrypt_arg);

	remove(path_to_password);
	free(public_gpgkey);
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

	if(argv[optind] == NULL)
		usageprint("%s", description);
	char *path_to_password = argv[optind];

	if(pass_length < minlen_pass || pass_length > maxlen_pass)
		errprint("You typed an incorrect number\n");

	result = checkForbiddenPaths(path_to_password);
	if(result)
		errprint("You have used forbidden paths\n");
	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == F_SUCCESS) {
		if(!flag_force) {
			if(getOverwriteAnswer(path_to_password) != OW_YES)
				return 1;
		}
	}

	// generate password 
	char gpass[pass_length];
	generatePassword(gpass, pass_length);

	result = insertPass(path_to_password, gpass, flag_copy);
	if(result)
		errprint("Can't add password to LockPassword");

	if(!flag_copy) printf("Generated password: %s\n", gpass);
	printf("Password added successfully for %s\n", path_to_password);
	return 0;
}

int cmd_remove(int argc, char *argv[])
{
	const char description[] = "rm passname\n";
	int result;
	char *path = argv[2];
	if(!path)
		usageprint("%s", description);

	result = checkForbiddenPaths(path);
	if(result)
		errprint("You have used forbidden paths\n");
	globalSplitPath(path);

	result = checkFileExist(gPath_pass);
	if(result != F_SUCCESS) {
		if(result == F_ISDIR) errprint("It is a directory\n");
		errprint("No such file exists\n");
	}

	if(unlink(gPath_pass) == 0)
		rmdir(gPath_subdir);
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
	dbgprint("old-path: %s\n", argv[optind]);
	dbgprint("new-path: %s\n", argv[optind+1]);

	char *old_path = argv[optind];
	result = checkForbiddenPaths(old_path);
	if(result)
		errprint("You have used forbidden paths\n");
	globalSplitPath(old_path);
	result = checkFileExist(gPath_pass);
	if(result != F_SUCCESS) {
		if(result == F_ISDIR) errprint("It is a directory\n");
		errprint("No such file exists\n");
	}

	char *old_path_gpg = gPath_pass;
	char *old_path_subdir = gPath_subdir;

	char *new_path = argv[optind+1];
	result = checkForbiddenPaths(new_path);
	if(result)
		errprint("You have used forbidden paths\n");
	globalSplitPath(new_path);

	if(checkFileExist(new_path) == F_ISDIR)
		;
	else if(checkFileExist(gPath_pass) == F_SUCCESS) {
		if(!flag_force) {
			if(getOverwriteAnswer(new_path) != OW_YES)
				return 1;
		}
		new_path = gPath_pass;
	}
	else errprint("No such new-path exists\n");

	char *arguments[] = {"mv", "-f", old_path_gpg, new_path, NULL};
	easyFork("mv", arguments);

	rmdir(old_path_subdir);
	free(old_path_subdir);
	free(old_path_gpg);
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
		"Code was written by Joursoir\n"
		"This is free and unencumbered software released into the public domain.\n\n",
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
		result = checkForbiddenPaths(argv[optind]);
		if(result)
			errprint("You have used forbidden paths\n");
		path = malloc(sizeof(char) * (strlen(argv[optind]) + 1));
		strcpy(path, argv[optind]);
	}
	else {
		path = malloc(sizeof(char) * 2);
		strcpy(path, ".");
	}

	if(opendir(path)) // if it's directory
	{
		if(flag_copy)
			errprint("You must type a passname, not a directory\n");

		char *arg1[] = {"tree", "-C", "--noreport", path, "-o", TREE_OUTPUT_FILE, NULL};
		easyFork("tree", arg1);

		char *arg2[] = {"sed", "-i", "-E", "s/\\.gpg(\\x1B\\[[0-9]+m)?( ->|$)/\\1\\2/g", TREE_OUTPUT_FILE, NULL};
		easyFork("sed", arg2); // remove .gpg at the pass name

		if(strcmp(path, ".") == 0) printf("Password Manager\n");
		else printf("Password Manager/%s\n", path);

		char *arg3[] = {"tail", "-n", "+2", TREE_OUTPUT_FILE, NULL};
		easyFork("tail", arg3); // remove working directory from output

		remove(TREE_OUTPUT_FILE);
	}
	else
	{
		globalSplitPath(path);

		if(checkFileExist(gPath_pass) == F_SUCCESS)
		{
			char password[maxlen_pass];
			getPassword(path, password, sizeof(char)*maxlen_pass, flag_copy);
			if(!flag_copy) printf("%s\n", password);
		}
		else errprint("%s is not in the password storage\n", path);
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

static int gotoMainDir()
{
	char *env_home = getenv("HOME");
	int len_rootdir = strlen(env_home) + 1 + strlen(LOCKPASS_DIR) + 1;
	char *rootdir = malloc(sizeof(char) * len_rootdir);
	sprintf(rootdir, "%s/%s", env_home, LOCKPASS_DIR);

	if(chdir(rootdir)) // failed
	{
		// create main directory:
		if(mkdir(rootdir, S_IRWXU)) {
			if(errno != EEXIST)
				errprint("mkdir() failed\n");
		}

		// try again:
		return chdir(rootdir);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if(!isatty(STDIN_FILENO))
		errprint("Please, use a terminal to run this application\n");

	if(gotoMainDir())
		errprint("chdir() failed\n");

	int ret = 0;
	char *cmd = (argv[1] != NULL) ? argv[1] : "";
	struct cmd_struct *ptr;
	if((ptr = get_cmd(cmd)))
		ret = ptr->func(argc, argv);
	else
		ret = cmd_showtree(argc, argv);

	if(gPath_subdir != NULL) {
		free(gPath_subdir);
		free(gPath_pass);
	}
	
	return ret;
}