#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>

#include "easydir.h"
#include "handerror.h"
#include "implementation.h"

//#define DEBUG
#define MINLEN_PASSWORD 1
#define MAXLEN_PASSWORD 128
#define STANDARD_AMOUNT_GENERATE_SYMBOLS 10
#define LOCKPASS_DIR "/.lock-password/"
#define STORAGEPASS_DIR "Password-Storage/"
#define GPGKEY_FILE "/.gpg-key"

#define HASH_INIT 6385337657
#define HASH_HELP 6385292014
#define HASH_VERSION 229486327000139
#define HASH_COPY 6385123360
#define HASH_EDIT 6385183019
#define HASH_MOVE 249844339311324255
#define HASH_GENERATE 7572409341523952
#define HASH_INSERT 6953633055386
#define HASH_REMOVE 6953974396019
#define HASH_DELETE 6953426453624
#define WITHOUT_ARGUMENTS 1

// == global var == 
char *gPath_rootdir;
char *gSecret_gpgkey;

static void cmd_init(int argc, char *argv[])
{
	char *gpg_key = argv[2];

	if(gpg_key == NULL)
		printError("Use: lpass init [gpg key]\n");

	// create main directory:
	int len_init_storage = strlen(gPath_rootdir) + strlen(GPGKEY_FILE) + 1; // +1 for '\0'
	char *path_init_storage = (char *) malloc(sizeof(char) * len_init_storage);

	strcpy(path_init_storage, gPath_rootdir);
	
	int pid = fork();
	if(pid == -1) callError(100);
	if(pid == 0) { /* new process */
		execlp("mkdir", "mkdir", "-vp", path_init_storage, NULL);
		perror("mkdir");
		exit(4);
	}
	wait(&pid);

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

	if(checkFileExist(path_to_password) == 0) {
		printError("lpass: No such file exists\n");
	}

	/* ask user about change pass */
	int buffSize = (strlen("Do you want edit password in '' (Y/N)?") + strlen(path_to_password)) * sizeof(char) + sizeof(char);
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

static void cmd_generate(int argc, char *argv[])
{
	char *len_pass = argv[2];
	char *path_to_password = argv[argc-1];

	if(checkFileExist(path_to_password) == 1) {
		// ask user about change pass 
		int buffSize = (strlen("Do you want generate new password and paste it in '' (Y/N)?") + strlen(path_to_password)) * sizeof(char) + sizeof(char);
		char *str = malloc(buffSize);

		snprintf(str, buffSize, "Do you want generate new password and paste it in '%s' (Y/N)?", path_to_password);
		
		int answer = getAnswer(str);
		free(str);

		if(answer != 1)
			exit(EXIT_FAILURE);
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

	if(checkFileExist(path_to_password) == 1)
		printError("To change pass use: lpass change [path to password]\n");
			
	if(userEnterPassword(MINLEN_PASSWORD, MAXLEN_PASSWORD, path_to_password) == 1)
		printf("Password added successfully for %s\n", path_to_password);
	else
		printf("Passwords do not match\n");
}

static void cmd_remove(int argc, char *argv[])
{
	char *path_to_password = argv[2];

	char *path_with_gpg = malloc( sizeof(char) * (strlen(path_to_password)+1+4) );
	strcpy(path_with_gpg, path_to_password);
	strcat(path_with_gpg, ".gpg");

	if(checkFileExist(path_with_gpg) == 0) {
		printError("lpass: No such file exists\n");
	}

	// check working
	char *main_path, *file_path;
	main_path = (char *) malloc( sizeof(char) * (strlen(path_to_password)+1) );
	file_path = (char *) malloc( sizeof(char) * (strlen(path_to_password)+1) );

	if(splitPath(path_to_password, main_path, file_path) == NULL) { // check correct input
		printError("lpass: The path you specified is incorrect\n");
	}

	if(deleteFile(path_with_gpg)) {
		deleteEmptyDir(main_path);
	}

	free(path_with_gpg);
	free(main_path);
	free(file_path);
}

static void cmd_showtree(char *path)
{
	if(opendir(path) != NULL) { // if it's directory
		int pid;
		pid = fork();
		if(pid == -1) callError(101);
		if(pid == 0) { /* new process */
			execlp("tree", "tree", "-C", "--noreport", path, "-o", "tree", NULL);
			perror("tree");
			exit(4);
		}
		wait(&pid);

		pid = fork();
		if(pid == -1) callError(130);
		if(pid == 0) {
			execlp("sed", "sed", "-E", "s/\\.gpg(\\x1B\\[[0-9]+m)?( ->|$)/\\1\\2/g", "tree", NULL);
			perror("sed");
			exit(4);
		}
		wait(&pid);

		remove("tree");
	}
	else {
		char *path_with_gpg = (char *) malloc( sizeof(char) * (strlen(path)+1+4) ); // +4 for ".gpg"
		strcpy(path_with_gpg, path);
		strcat(path_with_gpg, ".gpg");

		char password[MAXLEN_PASSWORD];
		getPassword(path_with_gpg, password, MAXLEN_PASSWORD);

		free(path_with_gpg);

		printf("%s\n", password);
	}
}

static void cmd_help()
{

}

static void cmd_version()
{

}

int main(int argc, char *argv[])
{
	if(!isatty(0)) { // stdin
		printError("lpass: Please, use a terminal to run this program\n");
	}

	/* init global path to root directory */
	int len_rootdir = strlen(getenv("HOME")) + strlen(LOCKPASS_DIR) + strlen(STORAGEPASS_DIR) + 1; // +1 for '\0'

	gPath_rootdir = (char *) malloc(sizeof(char) * len_rootdir);
	strcpy(gPath_rootdir, getenv("HOME"));
	strcat(gPath_rootdir, LOCKPASS_DIR);
	strcat(gPath_rootdir, STORAGEPASS_DIR);
	/* end init */

	unsigned long ihash = WITHOUT_ARGUMENTS;
	if(argv[1] != NULL)
		ihash = hash(argv[1]);

	if(ihash != HASH_INIT)
	{
		if(chdir(gPath_rootdir) != 0)
			printError("Before starting work, you must initialize LockPassword\nUse: lpass init [gpg key]\n");
		else {
			/* init GPG key */
			FILE *fileGPG;
			fileGPG = fopen(".gpg-key", "r");
			if(fileGPG == NULL) {
				if(errno == ENOENT) printError("lpass: No GPG key exists\n");
				callError(121);
			}

			gSecret_gpgkey = (char *) malloc( sizeof(char) * (MAXLEN_PASSWORD+1) );

			char sign[MAXLEN_PASSWORD];
			if(!fgets(sign, sizeof(sign), fileGPG)) {
				callError(122);
			}
			strcpy(gSecret_gpgkey, sign);
			/* end init */
		}
	}

	if( (chdir(gPath_rootdir) != 0) && (ihash != HASH_INIT) ) {
		printError("Before starting work, you must initialize LockPassword\nUse: lpass init [gpg key]\n");
	}

	switch(ihash)
	{
		case HASH_INIT: {
			cmd_init(argc, argv);
			break;
		}
		case HASH_COPY: {
			cmd_copy(argc, argv);
			break;
		}
		case HASH_EDIT: {
			cmd_edit(argc, argv);
			break;
		}
		case HASH_MOVE:
			break;
		case HASH_GENERATE: {
			cmd_generate(argc, argv);
			break;
		}
		case HASH_INSERT: {
			cmd_insert(argc, argv);
			break;
		}
		case HASH_REMOVE:
		case HASH_DELETE: {
			cmd_remove(argc, argv);
			break;
		}
		case HASH_HELP: {
			cmd_help();
			break;
		}
		case HASH_VERSION: {
			cmd_version();
			break;
		}
		default:
		{
			// fix [./lpass ..]
			int newlen_rootdir = len_rootdir - strlen(STORAGEPASS_DIR) - 1;
			gPath_rootdir[newlen_rootdir-1] = '\0';

			if(chdir(gPath_rootdir) != 0)
				callError(123);

			char *ptr;
			if(ihash == WITHOUT_ARGUMENTS) {
				ptr = malloc(sizeof(char) * (strlen(STORAGEPASS_DIR)+1) );
				strcpy(ptr, STORAGEPASS_DIR);
			}
			else {
				int buff_tree = strlen(STORAGEPASS_DIR) + strlen(argv[argc-1]) + 1;
				ptr = malloc(sizeof(char) * buff_tree);
				strcpy(ptr, STORAGEPASS_DIR);
				strcat(ptr, argv[argc-1]);
			}
			cmd_showtree(ptr);

			free(ptr);
			break;
		}
	}

	free(gPath_rootdir);
	return EXIT_SUCCESS;
}