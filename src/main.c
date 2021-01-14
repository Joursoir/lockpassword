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

#include "easydir.h"
#include "handerror.h"
#include "implementation.h"

#define VERSION "1.0c"
#define DATE_RELEASE "14 January, 2021"
#define STANDARD_TEXTEDITOR "vim"
#define MAXLEN_TEXTEDITOR 16
#define MINLEN_PASSWORD 1
#define MAXLEN_PASSWORD 128
#define STANDARD_AMOUNT_GENERATE_SYMBOLS 14
#define LOCKPASS_DIR "/.lock-password/"
#define GPGKEY_FILE "/.gpg-key"

#define TREE_OUTPUT_FILE ".tree"
#define TEXTEDITOR_FILE ".text-editor"

#define HASH_INIT 6385337657
#define HASH_HELP 6385292014
#define HASH_VERSION 229486327000139
#define HASH_EDIT 6385183019
#define HASH_MV 5863624
#define HASH_MOVE 249844339311324255
#define HASH_GENERATE 7572409341523952
#define HASH_INSERT 6953633055386
#define HASH_RM 5863780
#define HASH_REMOVE 6953974396019
#define HASH_DELETE 6953426453624
#define WITHOUT_ARGUMENTS 1

#define STR_SHOWTREEUSE "Use: lpass [-c] [passname]\n"
#define STR_INITUSE "Use: lpass init gpg-key\n"
#define STR_INSERTUSE "Use: lpass insert [-ecf] passname\n"
#define STR_EDITUSE "Use: lpass edit [-t=text-editor] passname\n"
#define STR_GENERATEUSE "Use: lpass generate [-l=pass-length] [-f] passname\n"
#define STR_REMOVEUSE "Use: lpass remove/rm/delete passname\n"
#define STR_MOVEUSE "Use: lpass move/mv [-f] old-path new-path\n"

// == global var == 
char *gPath_rootdir = NULL; // /home/[username]/.lockpassword/
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

	#if defined(DEBUG)
		printf("g_suddir: %s\n", gPath_subdir);
		printf("g_pass: %s\n", gPath_pass);
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

static void cmd_edit(int argc, char *argv[])
{
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
				if(f_texteditor == NULL) callError(108);
				fputs(optarg, f_texteditor);
				fclose(f_texteditor);
				printf("You changed text editor to %s\n", optarg);
    			break;
    		}
    		default: printError(STR_EDITUSE);
    	}
    }

    if(optind < argc) optind++; // for skip "edit"
    #if defined(DEBUG)
    	for(int i=0; i < argc; i++) printf("arg: %s\n", argv[i]);
    	printf("passname: %s\n", argv[optind]);
    #endif
    
    char *path_to_password;
    if(argv[optind] == NULL) printError(STR_EDITUSE);
    else path_to_password = argv[optind];

	checkForbiddenPaths(path_to_password);
	globalSplitPath(path_to_password);

	if(checkFileExist(gPath_pass) != 1)
		printError("Error: No such file exists\n");

	// configure text editor file 
	char text_editor[MAXLEN_TEXTEDITOR];
	FILE *f_texteditor = fopen(TEXTEDITOR_FILE, "r");	
	if(f_texteditor == NULL) {
		f_texteditor = fopen(TEXTEDITOR_FILE, "w");	
		if(f_texteditor == NULL) callError(108);
		fputs(STANDARD_TEXTEDITOR, f_texteditor); // in file
		strcpy(text_editor, STANDARD_TEXTEDITOR); // in variable
	}
	else {
		if(!fgets(text_editor, sizeof(char)*MAXLEN_TEXTEDITOR, f_texteditor))
			callError(122);
	}
	fclose(f_texteditor);

	#if defined(DEBUG)
		printf("text editor: %s\n", text_editor);
	#endif
	// end configure

	// decryption
	int size_gpgkey = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *public_gpgkey = (char *) malloc(size_gpgkey);
	getGPGKey(public_gpgkey, size_gpgkey);

	char *decrypt_arg[] = {"gpg", "-d", "--quiet", "-r", public_gpgkey, "-o", path_to_password, gPath_pass, NULL};
	easyFork("gpg", decrypt_arg);

	// start vim/etc for edit passowrd
	char *editor_arg[] = {text_editor, path_to_password, NULL};
	easyFork(text_editor, editor_arg);

	// delete '\n' and paste good pass
	char password[MAXLEN_PASSWORD];
	fileCropLineFeed(path_to_password, password, MAXLEN_PASSWORD);

	FILE *file = fopen(path_to_password, "w");	
	if(file == NULL) callError(108);
	fputs(password, file);
	fclose(file);

	// encryption
	char *encrypt_arg[] = {"gpg", "--quiet", "--yes", "-r", public_gpgkey, "-e", path_to_password, NULL};
	easyFork("gpg", encrypt_arg);

	remove(path_to_password);
	free(public_gpgkey);
}

static void cmd_move(int argc, char *argv[])
{
	/* we have a two situation:
	1) mv file file
	2) mv file directory */

	const struct option long_options[] = {
        {"force", no_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}
    };

    int result, flag_force = 0;
    while((result = getopt_long(argc, argv, "f", long_options, NULL)) != -1) {
    	switch(result) {
    		case 'f': { flag_force = 1; break; }
    		default: printError(STR_MOVEUSE);
    	}
    }

    if(optind < argc) optind++; // for skip "move"
    #if defined(DEBUG)
    	for(int i=0; i < argc; i++) printf("arg: %s\n", argv[i]);
    	printf("old-path: %s\n", argv[optind]);
    	if(argv[optind] != NULL) printf("new-path: %s\n", argv[optind+1]);
    #endif
    
    if(argv[optind] == NULL) printError(STR_MOVEUSE);
    if(argv[optind+1] == NULL) printError(STR_MOVEUSE);

	char *old_path = argv[optind];
	checkForbiddenPaths(old_path); globalSplitPath(old_path);
	if(checkFileExist(gPath_pass) != 1) printError("Error: No such old-path exists\n");

	char *old_path_gpg = gPath_pass;
	char *old_path_subdir = gPath_subdir;

	char *new_path = argv[optind+1];
	checkForbiddenPaths(new_path); globalSplitPath(new_path);

	if(checkFileExist(new_path) == 2) // if new-path = dir
		;
	else if(checkFileExist(gPath_pass) == 1) { // if new-path = file
		if(!flag_force) {
			if(getOverwriteAnswer(new_path) != 1)
				return;
		}
		new_path = gPath_pass;
	}
	else printError("Error: No such new-path exists\n");

	char *arguments[] = {"mv", "-f", old_path_gpg, new_path, NULL};
	easyFork("mv", arguments);

	deleteEmptyDir(old_path_subdir);
	free(old_path_subdir); free(old_path_gpg);
}

static void cmd_generate(int argc, char *argv[])
{
	int pass_length = STANDARD_AMOUNT_GENERATE_SYMBOLS, flag_force = 0, flag_copy = 0, result;
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
				return;
		}
	}

	// generate password 
	char gpass[pass_length];
	generatePassword(gpass, pass_length);

	insertPass(path_to_password, gpass, flag_copy);
	if(!flag_copy) printf("Generated password: %s\n", gpass);
	printf("Password added successfully for %s\n", path_to_password);
}

static void cmd_insert(int argc, char *argv[])
{
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
				return;
		}
	}
			
	if(userEnterPassword(MINLEN_PASSWORD, MAXLEN_PASSWORD, path_to_password, flag_echo, flag_copy) == 1) {
		printf("Password added successfully for %s\n", path_to_password);
	}
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

	if(checkFileExist(gPath_pass) != 1)
		printError("Error: No such file exists\n");

	if(deleteFile(gPath_pass))
		deleteEmptyDir(gPath_subdir);
}

static void cmd_showtree(int argc, char *argv[])
{
	int flag_copy = 0, result;
	char *path;
	const struct option long_options[] = {
        {"copy", no_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
    };

    while((result = getopt_long(argc, argv, "c", long_options, NULL)) != -1) {
    	switch(result) {
    		case 'c': { flag_copy = 1; break; }
    		default: printError(STR_SHOWTREEUSE);
    	}
    }

    #if defined(DEBUG)
    	for(int i=0; i < argc; i++) printf("arg: %s\n", argv[i]);
    	printf("passname: %s\n", argv[optind]);
    #endif

    if(argv[optind] == NULL) {
    	if(flag_copy) printError(STR_SHOWTREEUSE);
    	else {
    		path = (char *) malloc(sizeof(char) * 2);
    		strcpy(path, ".");
    	}
    }
    else path = argv[optind];
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
			getPassword(path, password, sizeof(char)*MAXLEN_PASSWORD, flag_copy);
			if(!flag_copy) printf("%s\n", password);
		}
		else printf("Error: %s is not in the password storage\n", path);
	}

	if(argv[1] == NULL) free(path);
}

static void cmd_help()
{
	printf("Synopsis:\n\tlpass [command] [arguments] ...\n");

	printf("Commands:\n\tinit gpg-key\n");
	printf("\t\tInitialize the password manager using the passed gpg-key.\n");

	printf("\tinsert [-e, --echo] [-c, --copy] [-f, --force] passname\n");
	printf("\t\tAdd the specified passname to the password manager.\n");

	printf("\tedit [-t, --text-editor=text-editor] passname\n");
	printf("\t\tOpen the specified passname in a text editor, waiting for changes.\n");

	printf("\tgenerate [-l, --length=pass-length] [-c, --copy] [-f, --force] passname\n");
	printf("\t\tGenerate a random password and write it in passname.\n");

	printf("\tmv/move [-f, --force] old-path new-path\n");
	printf("\t\tMove/rename old-path to new-path.\n");

	printf("\trm/remove/delete passname\n");
	printf("\t\tRemove the passname you specified from the password manager.\n");

	printf("\thelp\n");
	printf("\t\tPrint help information about commands and the application itself.\n");

	printf("\tversion\n");
	printf("\t\tPrint version information.\n");

	printf("\nMore information may be found in the lpass(1) man page.\n");
}

static void cmd_version()
{
	printf("LockPassword v%s\n", VERSION);
	printf("Release date: %s\n\n", DATE_RELEASE);
	printf("Code was written by Joursoir\n");
	printf("This is free and unencumbered software released into the public domain.\n\n");
}

int main(int argc, char *argv[])
{
	if(!isatty(0)) { // stdin
		printError("lpass: Please, use a terminal to run this application\n");
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
	
	return 0;
}