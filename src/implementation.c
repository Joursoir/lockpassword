#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "xstd.h"
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

/* check two dot in path */
int checkForbiddenPaths(char *path)
{
	int i, length;
	int firstdot = 0;
	for(i = 0, length = strlen(path); i < length; i++)
	{
		if(path[i] == '.') {
			if(firstdot)
				return 1;
			firstdot++;
		}
		else firstdot = 0;
	}
	return 0;
}

char *getGPGKey()
{
	int size_gpgkey = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *pub_gpgkey = malloc(size_gpgkey + sizeof(char));

	FILE *fileGPG = fopen(".gpg-key", "r");
	if(fileGPG == NULL) {
		if(errno == ENOENT)
			printError("error: No GPG key exists. Use \"lpass init\".");
		callError(121);
	}

	if(!fgets(pub_gpgkey, size_gpgkey, fileGPG)) {
		callError(122);
	}
	fclose(fileGPG);

	return pub_gpgkey;
}

char* getPassword(char *path_pass, char *password, size_t size, int flag_copy)
{
	char *public_gpgkey = getGPGKey();

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

static void mkdir_recursive(const char *path)
{
	char *subpath, *fullpath;

	fullpath = strdup(path);
	subpath = dirname(fullpath);
	if(subpath[0] != '.')
		mkdir_recursive(subpath);

	mkdir(path, S_IRWXU);
	free(fullpath);
}

int insertPass(char *path, char *password, int flag_copy)
{
	char *public_gpgkey = getGPGKey();

	// create directories
	char *tmp, *dirs_path;
	tmp = strdup(path);
	dirs_path = dirname(tmp);
	if(dirs_path[0] != '.')
		mkdir_recursive(dirs_path);
	free(tmp);

	// create file, copy password there
	FILE *file_pass;
	file_pass = fopen(path, "w");	
	if(file_pass == NULL) {
		free(public_gpgkey);
		return 1;
	}
	fputs(password, file_pass);
	fclose(file_pass);

	if(flag_copy)
		copyText(password);

	// encryption
	char *encrypt_arg[] = {"gpg", "--quiet", "--yes", "-r", public_gpgkey, "-e", path, NULL};
	easyFork("gpg", encrypt_arg);

	remove(path);
	free(public_gpgkey);
	return 0;
}

char *getInput(int minlen, int maxlen)
{
	size_t size = sizeof(char) * maxlen;
	char *pass = malloc(size + sizeof(char)); // +1 for '\0'
	int len;

	if(fgets(pass, size, stdin) == NULL) {
		free(pass);
		return NULL;
	}

	len = strlen(pass);
	if(len < minlen) {
		free(pass);
		return NULL;
	}

	if(pass[len-1] == '\n')
		pass[len-1] = '\0';

	return pass;
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
			case 'y': return OW_YES;
			case 'N':
			case 'n': return OW_NO;
			case EOF: printError("Error: Unexpected end of file\n");
			default: { printf("Overwrite? (Y/N) "); break; }
		}
	}

	return 2;
}
