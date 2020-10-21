#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "handerror.h"
#include "easydir.h"

#define GPG_OUTPUT_FILE ".gap"
#define GPG_PUBLICKEY_MAXLENGTH 1025 // +1 for '\0'

// == global var == 
extern char *gPath_rootdir; // /home/[username]/.lockpassword/
extern char *gPath_subdir; // example: programming/github.com
extern char *gPath_pass; // example: programming/github.com/joursoir.gpg

void checkForbiddenPaths(char *path) // check two dot in path
{
	int firstdot = 0;
	for(int i=0; i < strlen(path); i++)
	{
		if(path[i] == '.')
			firstdot ? printError("Error: please, don't use forbidden paths\n") : firstdot++;
		else firstdot = 0;
	}
}

static char *getGPGKey(char *dest, size_t size)
{
	FILE *fileGPG = fopen(".gpg-key", "r");
	if(fileGPG == NULL) {
		if(errno == ENOENT) printError("error: No GPG key exists\n");
		callError(121);
	}

	if(!fgets(dest, size, fileGPG)) {
		callError(122);
	}
	fclose(fileGPG);

	return dest;
}

char* getPassword(char *path_pass, char *password, size_t size) // path_pass is not used
{
	int size_gpgkey = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *secret_gpgkey = (char *) malloc(size_gpgkey);
	getGPGKey(secret_gpgkey, size_gpgkey);

	char *arguments[] = {"gpg", "-d", "--quiet", "-r", secret_gpgkey, "-o", GPG_OUTPUT_FILE, gPath_pass, NULL};
	easyFork("gpg", arguments);

	FILE *filePass = fopen(GPG_OUTPUT_FILE, "r");
	if(filePass == NULL) callError(127);

	if(!fgets(password, size, filePass)) {
		callError(111);
	}
	fclose(filePass);

	free(secret_gpgkey);
	remove(GPG_OUTPUT_FILE);
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
	/* gPath_rootdir = /home/[username]/.lock-password/
	add_path = banks/france/[number]
	gPath_pass = banks/france/[number].gpg
	gPath_subdir = banks/france */

	int size_gpgkey = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *secret_gpgkey = (char *) malloc(size_gpgkey);
	getGPGKey(secret_gpgkey, size_gpgkey);

	char *arguments1[] = {"mkdir", "-p", gPath_subdir, NULL};
	easyFork("mkdir", arguments1);

	// create file, copy password there
	FILE *filePass;
	filePass = fopen(add_path, "w");	
	if(filePass == NULL) {
		callError(108);
	}
	fputs(password, filePass);
	fclose(filePass);

	// encryption
	char *arguments2[] = {"gpg", "-r", secret_gpgkey, "-e", add_path, NULL};
	easyFork("gpg", arguments2);

	remove(add_path);
	free(secret_gpgkey);
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
