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
#include <sys/wait.h>
#include <getopt.h>
#include <dirent.h>
#include <libgen.h>

#include "easydir.h"
#include "handerror.h"
#include "implementation.h"

#define VERSION "1.0"
#define DATE_RELEASE "21 October, 2020"
//#define DEBUG
#define MINLEN_PASSWORD 1
#define MAXLEN_PASSWORD 128
#define STANDARD_AMOUNT_GENERATE_SYMBOLS 10
#define LOCKPASS_DIR "/.lock-password/"
#define GPGKEY_FILE "/.gpg-key"

#define TREE_OUTPUT_FILE ".tree"

#define HASH_INIT 6385337657
#define HASH_HELP 6385292014
#define HASH_VERSION 229486327000139
#define HASH_COPY 6385123360
#define HASH_EDIT 6385183019
#define HASH_MV 5863624
#define HASH_MOVE 249844339311324255
#define HASH_GENERATE 7572409341523952
#define HASH_INSERT 6953633055386
#define HASH_RM 5863780
#define HASH_REMOVE 6953974396019
#define HASH_DELETE 6953426453624
#define WITHOUT_ARGUMENTS 1

// == global var == 
char *gPath_rootdir; // /home/[username]/.lockpassword/
char *gPath_subdir; // example: programming/github.com
char *gPath_pass; // example: programming/github.com/joursoir.gpg

static void globalSplitPath(char *source)
{
	int len_path = strlen(source) + strlen(".gpg") + 1;	

	gPath_pass = malloc(sizeof(char) * len_path); // path without working dir
	strcpy(gPath_pass, source);
	strcat(gPath_pass, ".gpg");

	gPath_subdir = malloc(sizeof(char) * len_path); // path without working dir and pass file
	strcpy(gPath_subdir, source);
	gPath_subdir = dirname(gPath_subdir);

	#if defined(DEBUG)
		printf("dir: %s\n", gPath_subdir);
	#endif
}

static void cmd_init(int argc, char *argv[])
{
	char *gpg_key = argv[2];

	if(gpg_key == NULL)
		printError("Use: lpass init [gpg key]\n");

	// create main directory:
	int len_init_storage = strlen(gPath_rootdir) + strlen(GPGKEY_FILE) + 1; // +1 for '\0'
	char *path_init_storage = (char *) malloc(sizeof(char) * len_init_storage);
	strcpy(path_init_storage, gPath_rootdir);
	
	char *arguments[] = {"mkdir", "-vp", path_init_storage, NULL};
	easyFork("mkdir", arguments);

	strcat(path_init_storage, GPGKEY_FILE);

	// create .gpg-key in storage
	FILE *filekey;
	filekey = fopen(path_init_storage, "w");	
	if(filekey == NULL) {
		callError(122);
	}
	fputs(gpg_key, filekey);
	fclose(filekey);

	free(path_init_storage);
	printf("LockPassword initialized successfully\n");
}

static void cmd_copy(int argc, char *argv[])
{
	printf("Coming soon...\n");
}

static void cmd_edit(int argc, char *argv[])
{
	char *path_to_password = argv[2];

	if(path_to_password == NULL)
		printError("Use: lpass edit [path to password]\n");
	checkForbiddenPaths(path_to_password);

	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 0)
		printError("lpass: No such file exists\n");

	/* ask user about change pass */
	int buffSize = (strlen("Do you want edit password in '' (Y/N)?") + strlen(path_to_password) + 1) * sizeof(char);
	char *str = malloc(buffSize);
	snprintf(str, buffSize, "Do you want edit password in '%s' (Y/N)?", path_to_password);

	if(getAnswer(str) != 1) {
		free(str);
		exit(EXIT_SUCCESS);
	}
	free(str);

	if(userEnterPassword(MINLEN_PASSWORD, MAXLEN_PASSWORD, path_to_password) == 1)
		printf("Password updated successfully for %s\n", path_to_password);
	else
		printf("Passwords do not match\n");
}

static void cmd_move(int argc, char *argv[])
{
	char *old_path = argv[2];
	char *new_path = argv[argc-1];

	if(old_path == new_path)
		printError("Use: lpass move/mv [old path] [new path]");

	// soon ...
}

static void cmd_generate(int argc, char *argv[])
{
	char *len_pass = argv[2];
	char *path_to_password = argv[argc-1];

	if(path_to_password == NULL)
		printError("Use: lpass generate [len pass] [path to password]\n");
	checkForbiddenPaths(path_to_password);

	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 1) {
		// ask user about change pass 
		int buffSize = (strlen("Do you want generate new password and paste it in '' (Y/N)?") + \
			strlen(path_to_password) - 4 + 1) * sizeof(char);
		char *str = malloc(buffSize);

		snprintf(str, buffSize, "Do you want generate new password and paste it in '%s' (Y/N)?", path_to_password);
		
		int answer = getAnswer(str);
		free(str);

		if(answer != 1) exit(EXIT_SUCCESS);
	}

	int n_symbols = 0;
	if(strcmp(len_pass, path_to_password) == 0)
		n_symbols = STANDARD_AMOUNT_GENERATE_SYMBOLS;
	else n_symbols = atoi(len_pass);

	if(n_symbols < MINLEN_PASSWORD || n_symbols > MAXLEN_PASSWORD) {
		printError("lpass: you typed an incorrect number");
	}

	// generate password 
	char gpass[MAXLEN_PASSWORD];
	generatePassword(gpass, n_symbols, MAXLEN_PASSWORD);

	insertPass(path_to_password, gpass);
	printf("Generated password: %s\n", gpass);
	printf("Password added successfully for %s\n", path_to_password);
}

static void cmd_insert(int argc, char *argv[])
{
	char *path_to_password = argv[2];
	if(path_to_password == NULL)
		printError("Use: lpass insert [path to password]\n");
	checkForbiddenPaths(path_to_password);

	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 1)
		printError("To change pass use: lpass change [path to password]\n");
			
	if(userEnterPassword(MINLEN_PASSWORD, MAXLEN_PASSWORD, path_to_password) == 1)
		printf("Password added successfully for %s\n", path_to_password);
	else
		printf("Passwords do not match\n");
}

static void cmd_remove(int argc, char *argv[])
{
	char *path_to_password = argv[2];
	if(path_to_password == NULL)
		printError("Use: lpass remove/rm/delete [path to password]\n");
	checkForbiddenPaths(path_to_password);

	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 0) {
		printError("lpass: No such file exists\n");
	}

	if(deleteFile(gPath_pass)) {
		deleteEmptyDir(gPath_subdir);
	}
}

static void cmd_showtree(char *path)
{
	if(opendir(path) != NULL ) // if it's directory
	{
		char *arg1[] = {"tree", "-C", "--noreport", path, "-o", TREE_OUTPUT_FILE, NULL};
		easyFork("tree", arg1);

		char *arg2[] = {"sed", "-i", "-E", "s/\\.gpg(\\x1B\\[[0-9]+m)?( ->|$)/\\1\\2/g", TREE_OUTPUT_FILE, NULL};
		easyFork("sed", arg2); // remove .gpg at the pass name

		if(strcmp(path, ".") == 0) printf("Password Storage\n");
		else printf("Password Storage/%s\n", path);

		char *arg3[] = {"tail", "-n", "+2", TREE_OUTPUT_FILE, NULL};
		easyFork("tail", arg3); // remove working directory

		remove(TREE_OUTPUT_FILE);
	}
	else
	{
		globalSplitPath(path);

		if(checkFileExist(gPath_pass) == 1) // exist
		{
			char password[MAXLEN_PASSWORD];
			getPassword(path, password, sizeof(char)*MAXLEN_PASSWORD);
			printf("%s\n", password);
		}
		else printf("Error: %s is not in the password storage\n", path);
	}
}

static void cmd_help()
{

}

int main(int argc, char *argv[])
{
	if(!isatty(0)) { // stdin
		printError("lpass: Please, use a terminal to run this program\n");
	}

	/* init global path to root directory */
	int len_rootdir = strlen(getenv("HOME")) + strlen(LOCKPASS_DIR) + 1; // +1 for '\0'

	gPath_rootdir = (char *) malloc(sizeof(char) * len_rootdir);
	strcpy(gPath_rootdir, getenv("HOME"));
	strcat(gPath_rootdir, LOCKPASS_DIR);
	/* end init */

	unsigned long ihash = WITHOUT_ARGUMENTS;
	if(argv[1] != NULL)
		ihash = hash(argv[1]);

	if(chdir(gPath_rootdir) != 0 && ihash != HASH_INIT)
		printError("Before starting work, you must initialize LockPassword\nUse: lpass init [gpg key]\n");

	switch(ihash)
	{
		case HASH_INIT: { cmd_init(argc, argv); break; }
		case HASH_COPY: { cmd_copy(argc, argv); break; }
		case HASH_EDIT: { cmd_edit(argc, argv); break; }
		case HASH_MV:
		case HASH_MOVE: { cmd_move(argc, argv); break; }
		case HASH_GENERATE: { cmd_generate(argc, argv); break; }
		case HASH_INSERT: { cmd_insert(argc, argv); break; }
		case HASH_RM:
		case HASH_REMOVE:
		case HASH_DELETE: { cmd_remove(argc, argv); break; }
		case HASH_HELP: { cmd_help(); break; }
		case HASH_VERSION: {
			printf("LockPassword v%s\n", VERSION);
			printf("Release date: %s\n\n", DATE_RELEASE);
			printf("Code written by Joursoir\n");
			printf("This is free and unencumbered software released into the public domain.\n\n");
			break;
		}
		default:
		{
			if(ihash == WITHOUT_ARGUMENTS) strcpy(argv[argc-1], ".");
			else checkForbiddenPaths(argv[argc-1]);
			
			cmd_showtree(argv[argc-1]);
			break;
		}
	}

	if(gPath_subdir != NULL) {
		free(gPath_subdir);
		free(gPath_pass);
	}
	free(gPath_rootdir);
	return EXIT_SUCCESS;
}