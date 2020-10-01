#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <getopt.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include "easydir.h"
#include "handerror.h"

//#define DEBUG
#define MAXLEN_PASSWORD 128
#define STANDARD_AMOUNT_GENERATE_SYMBOLS 10
#define FULLNAMEFILE "/.lock-password/Password-Bank"
#define TREENAMEFILE "/.lock-password"
#define ADDNAMETOFULL "/Password-Bank"
#define NAMEFILE "Password-Bank"

static void clearStdinBuff()
{
	char garbage;

	while( (garbage = fgetc(stdin)) != '\n' && garbage != EOF )
		;
}

static char* getPassword(char *path, char *password)
{
	FILE *filePass;
	filePass = fopen(path, "r");
	if(filePass == NULL) {
		callError(110);
	}

	char sign[MAXLEN_PASSWORD];
	if(!fgets(sign, sizeof(sign), filePass)) {
		callError(111);
	}

	strcpy(password, sign);
	fclose(filePass);
	return password;
}

static void showTree(char *path)
{
	if(opendir(path) != NULL) { // if it's directory
		int pid;
		pid = fork();
		if(pid == -1) callError(101);
		if(pid == 0) { /* new process */
			execlp("tree", "tree", "-C", "--noreport", path, NULL);
			perror("tree");
			exit(4);
		}
		wait(&pid);
	}
	else {
		char password[MAXLEN_PASSWORD];
		getPassword(path, password);
		printf("%s\n", password);
	}
}

static void changePass(char *root_path, char *add_path, char *password)
{
	/* root_path = /home/[username]/
	add_path = banks/france/[number]
	main_path = banks/france
	file_path = [number] */

	char *main_path = malloc(sizeof(char) * strlen(add_path) + sizeof(char));
	char *file_path = malloc(sizeof(char) * strlen(add_path) + sizeof(char));

	if(splitPath(add_path, main_path, file_path) == NULL) {
		printError("lpass: The path you specified is incorrect\n");
	}

	int pass_buf = strlen(root_path) + strlen(add_path);
	char *final_path = (char*) malloc(sizeof(char) * pass_buf + 1);

	strcpy(final_path, root_path);
	strcat(final_path, "/");
	strcat(final_path, main_path);

	if(chdir(final_path) != 0) {
		callError(107);
	}

	FILE *filePass;
	filePass = fopen(file_path, "w");
	if(filePass == NULL) {
		if(errno == ENOENT) { // file doesn't exist
			printError("lpass: No such file exists\n");
		}
		callError(114);
	}
	fputs(password, filePass);

	free(main_path);
	free(file_path);
	fclose(filePass);
}

static void insertPass(char *root_path, char *add_path, char *password)
{
	/* root_path = /home/[username]/
	add_path = banks/france/[number]
	main_path = banks/france
	file_path = [number] */

	char *main_path = malloc(sizeof(char) * strlen(add_path) + 1);
	char *file_path = malloc(sizeof(char) * strlen(add_path) + 1);

	if(splitPath(add_path, main_path, file_path) == NULL) {
		printError("lpass: The path you specified is incorrect\n");
	}

	int pass_buf = strlen(root_path) + strlen(add_path);
	char *final_path = (char*) malloc(sizeof(char) * pass_buf + 1);

	strcpy(final_path, root_path);
	strcat(final_path, "/");
	strcat(final_path, main_path);

	int pid = fork();
	if(pid == -1) callError(103);
	if(pid == 0) { /* new process */
		execlp("mkdir", "mkdir", "-p", main_path, NULL);
		perror("mkdir");
		exit(4);
	}
	wait(&pid);

	if(chdir(final_path) != 0) {
		callError(107);
	}

	// create file, copy password there
	FILE *filePass;
	filePass = fopen(file_path, "w");
	if(filePass == NULL) {
		callError(108);
	}
	fputs(password, filePass);

	free(main_path);
	free(file_path);
	fclose(filePass);
}

static char *generatePassword(char *dest, int amount)
{
	char allowed_symbols[] = {
		'A','E','I','J','O','U','B','C','D','F','G','H',
		'K','L','M','N','P','Q','R','S','T','V','W','X',
		'Y','Z','a','e','i','j','o','u','b','c','d','f',
		'g','h','k','l','m','n','p','q','r','s','t','v',
		'w','x','y','z','1','2','3','4','5','6','7','8',
		'9','0','!','#','$',';','%','^',':','&','?','*',
		'(',')','-','_','+','=','<', '>'
	};
	int max = sizeof(allowed_symbols);
	srand(time(NULL));

	char password[MAXLEN_PASSWORD];
	for(int i=0; i < amount; i++)
	{
		char c = allowed_symbols[rand() % max];

		password[i] = c;
		password[i+1] = '\0';
	}

	strcpy(dest, password);
	return dest;
}

static void nonvisibleEnter(int status)
{
	struct termios term_settings;
	tcgetattr(0, &term_settings); // get current settings
	if(status == 1) {
		term_settings.c_lflag &= ~ECHO; // flag reset
	}
	else {
		term_settings.c_lflag |= ECHO;
	}
	tcsetattr(0, TCSANOW, &term_settings);
}

static int getAnswer(char *text)
{
	char answer;
	printf("%s\n", text);
	while( (answer = fgetc(stdin)) )
	{
		clearStdinBuff();
		switch(answer)
		{
			case 'Y':
			case 'y':
			{
				return 1;
			}
			case 'N':
			case 'n':
			{
				return 0;
			}
			case EOF:
			{
				printError("lpass: Unexpected end of file\n");
			}
			default:
			{
				printf("%s\n", text);
				break;
			}
		}
	}

	return -1;
}

static char *typePass(char *text, char *dest)
{
	printf("%s", text);
	if(fgets(dest, (sizeof(char)*MAXLEN_PASSWORD + 1), stdin) == NULL) {
		nonvisibleEnter(0);
		printError("lpass: Unexpected end of file\n");
	}

	int len = strlen(dest);
	if(len < 2) {
		nonvisibleEnter(0);
		printError("lpass: incorrect password\n");
	}
	
	if(dest[len-1] == '\n') {
		dest[len-1] = '\0';
	}

	#if defined(DEBUG)
		printf("%s", dest);
	#endif

	printf("\n"); // for new line

	return dest;
}

int main(int argc, char *argv[])
{
	if(!isatty(0)) { // stdin
		printError("lpass: Please, use a terminal to run this program\n");
	}

	#if defined(DEBUG)
		char* short_options = "c:e:g:i:R:t";
	#else
		char* short_options = "c:e:g:i:R:";
	#endif

	struct option long_options[] = {
		{"init", no_argument, NULL, 'I'},
		{NULL, 0, NULL, 0}
	};

	int rootBuf = strlen(getenv("HOME")) + strlen(FULLNAMEFILE);
	char *rootPath = (char *) malloc(sizeof(char) * rootBuf + 1); // +1 for '\0'

	int path_init = 0;
	strcpy(rootPath, getenv("HOME"));
	strcat(rootPath, TREENAMEFILE);

	if(chdir(rootPath) == 0) path_init = 1;

	int option_index = -1, result = -1;
	result = getopt_long(argc, argv, short_options, long_options, &option_index);
	if(!path_init) {
		if(result != 'I') // if doesn't init
		{ 
			if(errno == ENOENT) { // file doesn't exist
				printError("lpass: Before starting work, you must initialize LockPassword\n\
					Use: lpass --init\n");
			}
			callError(102);
		}
	}
	else {
		if(result == 'I') {
			printError("lpass: You already initialized LockPassword\n");
		}
		else if(result != -1)
		{
			strcat(rootPath, ADDNAMETOFULL); // complements to full path
			if(chdir(rootPath) != 0) {
				callError(112);
			}
		}
	}

	if(result != -1)
	{
		switch(result) {
		case 'c': {
			printf("Coming soon...\n");
			break;
		}
		case 'e': {
			char *passPath = optarg;

			if(checkFileExist(passPath) == 0) {
				printError("lpass: No such file exists\n");
			}

			/* ask user about change pass */
			int buffSize = (strlen("Do you want edit password in '' (Y/N)?") + strlen(passPath)) * sizeof(char) + sizeof(char);
			char *str = malloc(buffSize);
			snprintf(str, buffSize, "Do you want edit password in '%s' (Y/N)?", passPath);

			if(getAnswer(str) != 1) {
				free(str);
				return 0;
			}
			free(str);

			/* enter password */
			nonvisibleEnter(1); // change terminal work
			char pass_one[MAXLEN_PASSWORD], pass_two[MAXLEN_PASSWORD];
			typePass("Please type your new password: ", pass_one);
			typePass("Please type your new password again: ", pass_two);
			nonvisibleEnter(0);

			if(strcmp(pass_one, pass_two) == 0) {
				printf("Password correct\n");

				changePass(rootPath, passPath, pass_one);
				printf("Password updated successfully for %s\n", passPath);
			}
			else printf("Passwords do not match\n");

			break;
		}
		case 'g': {
			char *passPath = argv[argc-1];
			char *lenPass = optarg;

			if(checkFileExist(passPath) == 1) {
				/* ask user about change pass */
				int buffSize = (strlen("Do you want generate new password and paste it in '' (Y/N)?") + strlen(passPath)) * sizeof(char) + 1;
				char *str = malloc(buffSize);

				snprintf(str, buffSize, "Do you want generate new password and paste it in '%s' (Y/N)?", passPath);
				if(getAnswer(str) != 1) {
					// free(str);
					return 0;
				}
				free(str);
			}

			int n_symbols = 0;
			if(strcmp(lenPass, passPath) == 0)
				n_symbols = STANDARD_AMOUNT_GENERATE_SYMBOLS;
			else n_symbols = atoi(lenPass);


			if(n_symbols < 1 || n_symbols > 127) {
				printError("lpass: you typed an incorrect number");
			}

			/* generate password */
			char gpass[MAXLEN_PASSWORD];
			generatePassword(gpass, n_symbols);

			insertPass(rootPath, passPath, gpass);
			printf("Generated password: %s\n", gpass);
			printf("Password added successfully for %s\n", passPath);

			break;
		}
		case 'i': {
			if(checkFileExist(optarg) == 1) {
				printf("lpass: To change pass use argument '-e'\n");
				return 0;
			}
			
			/* enter password */
			nonvisibleEnter(1); // change terminal work
			char pass_one[MAXLEN_PASSWORD], pass_two[MAXLEN_PASSWORD];
			typePass("Please type your password: ", pass_one);
			typePass("Please type your password again: ", pass_two);
			nonvisibleEnter(0);

			if(strcmp(pass_one, pass_two) == 0) {
				printf("Password correct\n");

				insertPass(rootPath, optarg, pass_one);
				printf("Password added successfully for %s\n", optarg);
			}
			else printf("lpass: Passwords do not match\n");

			break;
		}
		case 'R': {
			if(checkFileExist(optarg) == 0) {
				printError("lpass: No such file exists\n");
			}

			char *main_path = malloc(sizeof(char) * strlen(optarg) + 1);
			char *file_path = malloc(sizeof(char) * strlen(optarg) + 1);

			if(splitPath(optarg, main_path, file_path) == NULL) { // check correct input
				printError("lpass: The path you specified is incorrect\n");
			}

			if(deleteFile(optarg)) {
				deleteEmptyDir(main_path);
			}

			free(main_path);
			free(file_path);
			break;
		}
		case 'I': {
			// create direction:
			int pid = fork();
			if(pid == -1) callError(100);
			if(pid == 0) { /* new process */
				execlp("mkdir", "mkdir", "-vp", rootPath, NULL);
				perror("mkdir");
				exit(4);
			}
			wait(&pid);
			printf("LockPassword initialized successfully\n");
			break;
		}

		#if defined(DEBUG)
		case 't':
		{
			// for check new features:
			break;
		}
		#endif

		default: break;

		}
	}
	else
	{
		char *ptr;
		if(strcmp(argv[argc-1], argv[0]) == 0) {
			ptr = malloc(sizeof(char) * strlen(NAMEFILE) + sizeof(char));
			strcpy(ptr, NAMEFILE);
		}
		else {
			int buff_tree = strlen(NAMEFILE) + strlen(argv[argc-1]);
			ptr = malloc(sizeof(char) * buff_tree + sizeof(char) + sizeof(char));
			strcpy(ptr, NAMEFILE);
			strcat(ptr, "/");
			strcat(ptr, argv[argc-1]);
		}
		showTree(ptr);

		free(ptr);
	}

	return 0;
}