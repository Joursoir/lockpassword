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
#define DEBUG
#define MINLEN_PASSWORD 1
#define MAXLEN_PASSWORD 128
#define STANDARD_AMOUNT_GENERATE_SYMBOLS 14
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

#define STR_SHOWTREEUSE "Use: lpass [-c=passname] [passname]"
#define STR_INITUSE "Use: lpass init gpg-key\n"
#define STR_INSERTUSE "Use: lpass insert [-ef] passname\n"
//#define STR_EDITUSE "Use: lpass edit [-e] passname\n"
#define STR_GENERATEUSE "Use: lpass generate [-l=pass-length] [-f] passname\n"
#define STR_REMOVEUSE "Use: lpass remove/rm/delete passname\n"

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
		printf("pass: %s\n", gPath_pass);
	#endif
}

static void cmd_init(int argc, char *argv[])
{
	char *gpg_key = argv[2];
	if(gpg_key == NULL) printError(STR_INITUSE);

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
	printf("LockPassword initialized for %s\n", gpg_key);
}

static void cmd_copy(int argc, char *argv[])
{
	printf("Coming soon...\n");
}

static void cmd_edit(int argc, char *argv[])
{
	printf("Coming soon...\n");
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
	int pass_length = STANDARD_AMOUNT_GENERATE_SYMBOLS, flag_force = 0, result;
	const struct option long_options[] = {
        {"length", required_argument, NULL, 'l'},
        {"force", no_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}
    };

    while((result = getopt_long(argc, argv, "l:f", long_options, NULL)) != -1) {
    	switch(result) {
    		// if optarg - incorrect number, atoi return 0
    		case 'l': { pass_length = atoi(optarg); break; }
    		case 'f': { flag_force = 1; break; }
    		default: printError(STR_GENERATEUSE);
    	}
    }

    if(optind < argc) optind++; // for skip "generate"
    #if defined(DEBUG)
    	for(int i=0; i < argc; i++) printf("arg: %s\n", argv[i]);
    	printf("passname: %s\n", argv[optind]);
    #endif
    
    char *path_to_password;
    if(argv[optind] == NULL) printError(STR_GENERATEUSE);
    else path_to_password = argv[optind];

    if(pass_length < MINLEN_PASSWORD || pass_length > MAXLEN_PASSWORD)
		printError("Error: you typed an incorrect number\n");

	checkForbiddenPaths(path_to_password);
	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 1) {
		if(!flag_force) {
			if(getOverwriteAnswer(path_to_password) != 1)
				exit(EXIT_SUCCESS);
		}
	}

	// generate password 
	char gpass[MAXLEN_PASSWORD];
	generatePassword(gpass, pass_length, MAXLEN_PASSWORD);

	insertPass(path_to_password, gpass);
	printf("Generated password: %s\n", gpass);
	printf("Password added successfully for %s\n", path_to_password);
}

static void cmd_insert(int argc, char *argv[])
{
	int flag_echo = 0, flag_force = 0, result;
	const struct option long_options[] = {
        {"echo", no_argument, NULL, 'e'},
        {"force", no_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}
    };

    while((result = getopt_long(argc, argv, "ef", long_options, NULL)) != -1) {
    	switch(result) {
    		case 'e': { flag_echo = 1; break; }
    		case 'f': { flag_force = 1; break; }
    		default: printError(STR_INSERTUSE);
    	}
    }

    if(optind < argc) optind++; // for skip "insert"
    #if defined(DEBUG)
    	for(int i=0; i < argc; i++) printf("arg: %s\n", argv[i]);
    	printf("passname: %s\n", argv[optind]);
    #endif
    
    char *path_to_password;
    if(argv[optind] == NULL) printError(STR_INSERTUSE);
    else path_to_password = argv[optind];

	checkForbiddenPaths(path_to_password);
	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 1) {
		if(!flag_force) {
			if(getOverwriteAnswer(path_to_password) != 1)
				exit(EXIT_SUCCESS);
		}
	}
			
	if(userEnterPassword(MINLEN_PASSWORD, MAXLEN_PASSWORD, path_to_password, flag_echo) == 1)
		printf("Password added successfully for %s\n", path_to_password);
	else
		printf("Passwords do not match\n");
}

static void cmd_remove(int argc, char *argv[])
{
	char *path_to_password = argv[2];
	if(path_to_password == NULL)
		printError(STR_REMOVEUSE);

	checkForbiddenPaths(path_to_password);
	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) == 0)
		printError("Error: No such file exists\n");

	if(deleteFile(gPath_pass))
		deleteEmptyDir(gPath_subdir);
}

static void cmd_showtree(int argc, char *argv[])
{
	int flag_copy = 0, result;
	char *path;
	const struct option long_options[] = {
        {"copy", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
    };

    while((result = getopt_long(argc, argv, "c:", long_options, NULL)) != -1) {
    	switch(result) {
    		case 'c': { flag_copy = 1; path = optarg; break; }
    		default: printError(STR_SHOWTREEUSE);
    	}
    }

    #if defined(DEBUG)
    	for(int i=0; i < argc; i++) printf("arg: %s\n", argv[i]);
    #endif
    
    if(!flag_copy) {
    	if(argv[1] == NULL) {
    		path = (char *) malloc(sizeof(char) * 2);
    		strcpy(path, ".");
    	}
    	else path = argv[1];
    }
    checkForbiddenPaths(path);

	if(opendir(path) != NULL) // if it's directory
	{
		if(flag_copy) printError("Error: you must type a passname, not a directory\n");

		char *arg1[] = {"tree", "-C", "--noreport", path, "-o", TREE_OUTPUT_FILE, NULL};
		easyFork("tree", arg1);

		char *arg2[] = {"sed", "-i", "-E", "s/\\.gpg(\\x1B\\[[0-9]+m)?( ->|$)/\\1\\2/g", TREE_OUTPUT_FILE, NULL};
		easyFork("sed", arg2); // remove .gpg at the pass name

		if(strcmp(path, ".") == 0) printf("Password Manager\n");
		else printf("Password Manager/%s\n", path);

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

	if(argv[1] == NULL) free(path);
}

static void cmd_help()
{

}

static void cmd_version()
{
	printf("LockPassword v%s\n", VERSION);
	printf("Release date: %s\n\n", DATE_RELEASE);
	printf("Code written by Joursoir\n");
	printf("This is free and unencumbered software released into the public domain.\n\n");
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
	if(argv[1] != NULL) {
		ihash = hash(argv[1]);
	}

	if(chdir(gPath_rootdir) != 0 && ihash != HASH_INIT) {
		printf("Before starting work, you must initialize LockPassword\n");
		printError(STR_INITUSE);
	}

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
		case HASH_VERSION: { cmd_version(); break; }
		default: { cmd_showtree(argc, argv); break; }
	}

	if(gPath_subdir != NULL) {
		free(gPath_subdir);
		free(gPath_pass);
	}
	free(gPath_rootdir);
	return EXIT_SUCCESS;
}