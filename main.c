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

static char* get_password(char *path, char *password)
{
	FILE *filePass;
	filePass = fopen(path, "r");
	if(filePass == NULL) {
		call_error(110);
	}

	char sign[MAXLEN_PASSWORD];
	if(!fgets(sign, sizeof(sign), filePass)) {
		call_error(111);
	}

	strcpy(password, sign);
	fclose(filePass);
	return password;
}

static void show_tree(char *path)
{
	if(opendir(path) != NULL) { // if it's directory
		int pid;
		pid = fork();
		if(pid == -1) call_error(101);
		if(pid == 0) { /* new process */
			execlp("tree", "tree", "-C", "--noreport", path, NULL);
			perror("tree");
			exit(4);
		}
		wait(&pid);
	}
	else {
		char password[MAXLEN_PASSWORD];
		get_password(path, password);
		printf("%s\n", password);
	}
}

static void changePass(char *root_path, char *add_path, char *password)
{
	/* root_path = /home/[username]/
	add_path = banks/france/[number]
	main_path = banks/france
	file_path = [number] */

	char *main_path = malloc(sizeof(char) * strlen(add_path) + 1);
	char *file_path = malloc(sizeof(char) * strlen(add_path) + 1);

	if(splitPath(add_path, main_path, file_path) == NULL) {
		print_error("[Error] The path you specified is incorrect\n");
	}

	int pass_buf = strlen(root_path) + strlen(add_path);
	char *final_path = (char*) malloc(sizeof(char) * pass_buf + 1);

	strcpy(final_path, root_path);
	strcat(final_path, "/");
	strcat(final_path, main_path);

	if(chdir(final_path) != 0) {
		call_error(107);
	}

	FILE *filePass;
	filePass = fopen(file_path, "w");
	if(filePass == NULL) {
		if(errno == ENOENT) { // file doesn't exist
			print_error("Error: No such file exists\n");
		}
		call_error(114);
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
		print_error("[Error] The path you specified is incorrect\n");
	}

	int pass_buf = strlen(root_path) + strlen(add_path);
	char *final_path = (char*) malloc(sizeof(char) * pass_buf + 1);

	strcpy(final_path, root_path);
	strcat(final_path, "/");
	strcat(final_path, main_path);

	int pid = fork();
	if(pid == -1) call_error(103);
	if(pid == 0) { /* new process */
		execlp("mkdir", "mkdir", "-p", main_path, NULL);
		perror("mkdir");
		exit(4);
	}
	wait(&pid);

	if(chdir(final_path) != 0) {
		call_error(107);
	}

	// create file, copy password there
	FILE *filePass;
	filePass = fopen(file_path, "w");
	if(filePass == NULL) {
		call_error(108);
	}
	fputs(password, filePass);

	free(main_path);
	free(file_path);
	fclose(filePass);
}

static char *generate_password(char *dest, int amount)
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

static void nonvisible_enter(int status)
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

static int get_answer(char *text)
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
				print_error("Unexpected end of file\n");
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
		nonvisible_enter(0);
		print_error("Unexpected end of file\n");
	}

	int len = strlen(dest);
	if(len < 2) {
		nonvisible_enter(0);
		print_error("Error: uncorrect password\n");
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
		print_error("Please, use a terminal to run this program\n");
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
				print_error("Before starting work, you must initialize LockPassword\n\
					Use: lpass --init\n");
			}
			call_error(102);
		}
	}
	else {
		if(result == 'I') {
			print_error("You already initialized LockPassword\n");
		}
		else if(result != -1)
		{
			strcat(rootPath, ADDNAMETOFULL); // complements to full path
			if(chdir(rootPath) != 0) {
				call_error(112);
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
				print_error("Error: No such file exists\n");
			}

			/* ask user about change pass */
			int buffSize = (strlen("Do you want edit password in '' (Y/N)?") + strlen(passPath)) * sizeof(char) + 1;
			char *str = malloc(buffSize);

			snprintf(str, buffSize, "Do you want edit password in '%s' (Y/N)?", passPath);
			if(get_answer(str) != 1) {
				// free(str);
				return 0;
			}
			free(str);

			/* enter password */
			nonvisible_enter(1); // change terminal work
			char pass_one[MAXLEN_PASSWORD], pass_two[MAXLEN_PASSWORD];
			typePass("Please type your new password: ", pass_one);
			typePass("Please type your new password again: ", pass_two);
			nonvisible_enter(0);

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

			if(checkFileExist(passPath) == 0) {
				/* ask user about change pass */
				int buffSize = (strlen("Do you want generate new password and paste it in '' (Y/N)?") + strlen(passPath)) * sizeof(char) + 1;
				char *str = malloc(buffSize);

				snprintf(str, buffSize, "Do you want generate new password and paste it in '%s' (Y/N)?", passPath);
				if(get_answer(str) != 1) {
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
				print_error("Error: you typed an incorrect number");
			}

			/* generate password */
			char gpass[MAXLEN_PASSWORD];
			generate_password(gpass, n_symbols);

			insertPass(rootPath, passPath, gpass);
			printf("Generated password: %s\n", gpass);
			printf("Password added successfully for %s\n", passPath);

			break;
		}
		case 'i': {
			if(checkFileExist(optarg) == 1) {
				printf("To change pass use lpass -e\n");
				return 0;
			}
			
			/* enter password */
			nonvisible_enter(1); // change terminal work
			char pass_one[MAXLEN_PASSWORD], pass_two[MAXLEN_PASSWORD];
			typePass("Please type your password: ", pass_one);
			typePass("Please type your password again: ", pass_two);
			nonvisible_enter(0);

			if(strcmp(pass_one, pass_two) == 0) {
				printf("Password correct\n");

				insertPass(rootPath, optarg, pass_one);
				printf("Password added successfully for %s\n", optarg);
			}
			else printf("Passwords do not match\n");

			break;
		}
		case 'R': {
			if(checkFileExist(optarg) == 0) {
				print_error("Error: No such file exists\n");
			}

			char *main_path = malloc(sizeof(char) * strlen(optarg) + 1);
			char *file_path = malloc(sizeof(char) * strlen(optarg) + 1);

			if(splitPath(optarg, main_path, file_path) == NULL) { // check correct input
				print_error("[Error] The path you specified is incorrect\n");
			}

			if(delete_file(optarg)) {
				delete_emptydir(main_path);
			}

			free(main_path);
			free(file_path);
			break;
		}
		case 'I': {
			int pid;

			// create direction:
			pid = fork();
			if(pid == -1) call_error(100);
			if(pid == 0) { /* new process */
				execlp("mkdir", "mkdir", "-vp", rootPath, NULL);
				perror("mkdir");
				exit(4);
			}
			wait(&pid);
			printf("LockPassword initialized\n");
			break;
		}

		#if defined(DEBUG)
		case 't':
		{
			char *main_path = malloc(sizeof(char) * strlen(optarg) + 1);
			char *file_path = malloc(sizeof(char) * strlen(optarg) + 1);

			if(splitPath(optarg, main_path, file_path) == NULL)
				printf("NULL\n");
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
			ptr = malloc(sizeof(char) * strlen(NAMEFILE) + 1);
			strcpy(ptr, NAMEFILE);
		}
		else {
			int buff_tree = strlen(NAMEFILE) + strlen(argv[argc-1]);
			ptr = malloc(sizeof(char) * buff_tree + 1 + 1);
			strcpy(ptr, NAMEFILE);
			strcat(ptr, "/");
			strcat(ptr, argv[argc-1]);
		}
		show_tree(ptr);

		free(ptr);
	}

	return 0;
}