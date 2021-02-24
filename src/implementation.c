#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "handerror.h"
#include "easydir.h"
#include "implementation.h"

/* define in implementation.h */
// GPG_PUBLICKEY_MAXLENGTH NNNN

// == global var == 
extern char *gPath_subdir; // example: programming/github.com
extern char *gPath_pass; // example: programming/github.com/joursoir.gpg

static void copyText(char *password)
{
	char *simple_path = malloc(sizeof(char) * (5 + 1));
	strcpy(simple_path, ".pass");

	if(getenv("DISPLAY") != NULL)
	{
		FILE *f_pass;
		f_pass = fopen(simple_path, "w");
		if(f_pass == NULL) {
			callError(130);
		}
		fputs(password, f_pass);
		fclose(f_pass);

		char *xclip[] = {"xclip", "-selection", "clipboard", "-i", simple_path, NULL};
		easyFork("xclip", xclip);

		remove(simple_path);
		free(simple_path);
	}
	else if(getenv("WAYLAND_DISPLAY") != NULL)
	{
		char *wl_copy[] = {"wl-copy", password, NULL};
		easyFork("wl-copy", wl_copy);
	}
	else printError("Error: No X11 or Wayland");

	#if defined(DEBUG)
	  	printf("Password copied to clipboard\n");
	#endif
}

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

char *getGPGKey(char *dest, size_t size)
{
	FILE *fileGPG = fopen(".gpg-key", "r");
	if(fileGPG == NULL) {
		if(errno == ENOENT) printError("error: No GPG key exists. Use \"lpass init\".");
		callError(121);
	}

	if(!fgets(dest, size, fileGPG)) {
		callError(122);
	}
	fclose(fileGPG);

	return dest;
}

char* getPassword(char *path_pass, char *password, size_t size, int flag_copy)
{
	int size_gpgkey = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *public_gpgkey = (char *) malloc(size_gpgkey);
	getGPGKey(public_gpgkey, size_gpgkey);

	char *arguments[] = {"gpg", "-d", "--quiet", "-r", public_gpgkey, "-o", path_pass, gPath_pass, NULL};
	easyFork("gpg", arguments);

	FILE *filePass = fopen(path_pass, "r");
	if(filePass == NULL) callError(127);

	if(!fgets(password, size, filePass)) {
		callError(111);
	}
	fclose(filePass);

	if(flag_copy) copyText(password);

	remove(path_pass);
	free(public_gpgkey);
	return password;
}

void nonvisibleEnter(int status)
{
	struct termios term_settings;
	tcgetattr(0, &term_settings); // get current settings
	if(status == 1)
		term_settings.c_lflag &= ~ECHO; // flag reset
	else
		term_settings.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &term_settings);
}

void insertPass(char *add_path, char *password, int flag_copy)
{
	/* add_path = banks/france/[number]
	gPath_pass = banks/france/[number].gpg
	gPath_subdir = banks/france */

	int size_gpgkey = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *public_gpgkey = (char *) malloc(size_gpgkey);
	getGPGKey(public_gpgkey, size_gpgkey);

	char *mkdir_arg[] = {"mkdir", "-p", gPath_subdir, NULL};
	easyFork("mkdir", mkdir_arg);

	// create file, copy password there
	FILE *filePass;
	filePass = fopen(add_path, "w");	
	if(filePass == NULL) {
		callError(108);
	}
	fputs(password, filePass);
	fclose(filePass);

	if(flag_copy) copyText(password);

	// encryption
	char *encrypt_arg[] = {"gpg", "--quiet", "--yes", "-r", public_gpgkey, "-e", add_path, NULL};
	easyFork("gpg", encrypt_arg);

	remove(add_path);
	free(public_gpgkey);
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

int userEnterPassword(int minlen, int maxlen, char *path_insert, int flag_echo, int flag_copy)
{
	char *pass_one = malloc(sizeof(char) * maxlen);
	int rvalue = 0;
	if(!flag_echo) {
		char *pass_two = malloc(sizeof(char) * maxlen);

		nonvisibleEnter(1); // change terminal work
		typePass("Type your password: ", pass_one, minlen, maxlen);
		typePass("Type your password again: ", pass_two, minlen, maxlen);
		nonvisibleEnter(0);

		if(strcmp(pass_one, pass_two) == 0) {
			insertPass(path_insert, pass_one, flag_copy);
			rvalue = 1;
		}
		free(pass_two);
	}
	else {
		typePass("Type your password: ", pass_one, minlen, maxlen);
		insertPass(path_insert, pass_one, flag_copy);
		rvalue = 1;
	}

	free(pass_one);
	return rvalue;
}

char *generatePassword(char *dest, int amount)
{
	int i, min = 33, max = 126;
	char password[amount];

	srand(time(NULL));
	for(i = 0; i < amount; i++) 
		password[i] = min + rand() % (max-min);

	strcpy(dest, password);
	return dest;
}

static void clearStdinBuff()
{
	char garbage;

	while( (garbage = fgetc(stdin)) != '\n' && garbage != EOF )
		;
}


int getOverwriteAnswer(char *path)
{
	int answer;
	printf("Password for \"%s\" exists. Overwrite? (Y/N)\n", path);
	while((answer = fgetc(stdin)))
	{
		clearStdinBuff();
		switch(answer)
		{
			case 'Y':
			case 'y': return 1;
			case 'N':
			case 'n': return 0;
			case EOF: printError("Error: Unexpected end of file\n");
			default: { printf("Overwrite? (Y/N) "); break; }
		}
	}

	return -1;
}
