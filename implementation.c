#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>

#include "handerror.h"
#include "easydir.h"

// == global var == 
extern char *gPath_rootdir;
extern char *gSecret_gpgkey;

char* getPassword(char *path, char *password, int maxlen)
{
	// gpg 
	int pid = fork();
	if(pid == 1) callError(126);
	if(pid == 0) {
		execlp("gpg", "-d", "--quiet", "-r", gSecret_gpgkey, "-o", "gap", path, NULL);
		perror("gpg");
		exit(4);
	}
	wait(&pid);

	FILE *filePass = fopen("gap", "r");
	if(filePass == NULL) callError(127);

	char sign[maxlen];
	if(!fgets(sign, sizeof(sign), filePass)) {
		callError(111);
	}

	strcpy(password, sign);
	fclose(filePass);

	remove("gap");
	return password;
}

void nonvisibleEnter(int status)
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

void insertPass(char *add_path, char *password)
{
	/* gPath_rootdir = /home/[username]/.lock-password/Password-Storage/
	add_path = banks/france/[number]
	main_path = banks/france
	file_path = [number] */

	char *main_path = malloc( sizeof(char) * (strlen(add_path)+1) );
	char *file_path = malloc( sizeof(char) * (strlen(add_path)+1) );

	if(splitPath(add_path, main_path, file_path) == NULL) {
		printError("lpass: The path you specified is incorrect\n");
	}

	int pass_buf = strlen(gPath_rootdir) + strlen(add_path) + 1;
	char *final_path = (char*) malloc(sizeof(char) * pass_buf);

	strcpy(final_path, gPath_rootdir);
	strcat(final_path, main_path);

	if(strcmp(main_path, "") != 0) {
		int pid = fork();
		if(pid == -1) callError(103);
		if(pid == 0) { /* new process */
			execlp("mkdir", "mkdir", "-p", main_path, NULL);
			perror("mkdir");
			exit(4);
		}
		wait(&pid);
	}
	
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
	fclose(filePass);

	// gpg 
	int pid = fork();
	if(pid == 1) callError(225);
	if(pid == 0) {
		execlp("gpg", "-a", "-r", gSecret_gpgkey, "-e", file_path, NULL);
		perror("gpg");
		exit(4);
	}
	wait(&pid);
	remove(file_path);

	free(main_path);
	free(file_path);
}

char *typePass(char *text, char *dest, int minlen, int maxlen)
{
	printf("%s", text);
	if(fgets(dest, sizeof(char)*maxlen, stdin) == NULL) {
		nonvisibleEnter(0);
		printError("lpass: Unexpected end of file\n");
	}

	int len = strlen(dest);
	if(len < minlen || len > maxlen) {
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

int userEnterPassword(int minlen, int maxlen, char *path_insert)
{
	char *pass_one = (char *) malloc(sizeof(char) * maxlen);
	char *pass_two = (char *) malloc(sizeof(char) * maxlen);

	nonvisibleEnter(1); // change terminal work
	typePass("Please type your new password: ", pass_one, minlen, maxlen);
	typePass("Please type your new password again: ", pass_two, minlen, maxlen);
	nonvisibleEnter(0);

	int rvalue = 0;
	if(strcmp(pass_one, pass_two) == 0) {
		insertPass(path_insert, pass_one);
		rvalue = 1;
	}

	free(pass_one);
	free(pass_two);
	return rvalue;
}

char *generatePassword(char *dest, int amount, int max_len)
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

	char password[max_len];
	for(int i=0; i < amount; i++)
	{
		char c = allowed_symbols[rand() % max];

		password[i] = c;
		password[i+1] = '\0';
	}

	strcpy(dest, password);
	return dest;
}

unsigned long hash(char *str)
{
    unsigned long hash = 5381;
    char c;

    while( (c = *str++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static void clearStdinBuff()
{
	char garbage;

	while( (garbage = fgetc(stdin)) != '\n' && garbage != EOF )
		;
}


int getAnswer(char *text)
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
				return 1;
			case 'N':
			case 'n':
				return 0;
			case EOF:
				printError("lpass: Unexpected end of file\n");
			default: {
				printf("%s\n", text);
				break;
			}
		}
	}

	return -1;
}
