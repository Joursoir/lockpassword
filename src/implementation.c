#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "implementation.h"
#include "constants.h"
#include "xstd.h"
#include "easydir.h"
#include "r-gpgme.h"

/* define in implementation.h */
// GPG_PUBLICKEY_MAXLENGTH NNNN


void copy_outside(char *password)
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
int check_sneaky_paths(const char *path)
{
	int length = strlen(path), i;
	for(i = 1; i < length; i++)
	{
		if(path[i-1] == '.' && path[i] == '.')
			return 1;
	}
	return 0;
}

char *get_pubkey()
{
	int size_key = sizeof(char) * GPG_PUBLICKEY_MAXLENGTH;
	char *pubkey = malloc(size_key + sizeof(char));

	FILE *fileGPG = fopen(".gpg-key", "r");
	if(fileGPG == NULL) {
		free(pubkey);
		if(errno == ENOENT)
			errprint(NULL, "No GPG key exists. Use \"lpass init\".");
		perror(".gpg-key");
		return NULL;
	}

	if(!fgets(pubkey, size_key, fileGPG)) {
		free(pubkey);
		pubkey = NULL;
	}
	fclose(fileGPG);

	return pubkey;
}

char *get_password(const char *path)
{
	return decrypt_data(path);
}

void visible_enter(int status)
{
	struct termios term_settings;
	tcgetattr(0, &term_settings); // get current settings
	if(status == 1)
		term_settings.c_lflag |= ECHO;
	else
		term_settings.c_lflag &= ~ECHO;
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

int insert_pass(const char *path, const char *password)
{
	char *public_gpgkey = get_pubkey();
	if(public_gpgkey == NULL)
		return 1;

	// create directories
	char *tmp, *dirs_path;
	tmp = strdup(path);
	dirs_path = dirname(tmp);
	if(dirs_path[0] != '.')
		mkdir_recursive(dirs_path);
	free(tmp);

	int ret = ecnrypt_data(path, password, public_gpgkey);
	free(public_gpgkey);
	return ret;
}

char *get_input(int minlen, int maxlen)
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

void gen_password(char *dest, int amount)
{
	int i, min = 33, max = 126;
	char password[amount];

	srand(time(NULL));
	for(i = 0; i < amount; i++) 
		password[i] = min + rand() % (max-min);

	strcpy(dest, password);
}

static void clearStdinBuff()
{
	int garbage;
	while( (garbage = fgetc(stdin)) != '\n' && garbage != EOF )
		;
}

int overwrite_answer(const char *path)
{
	int answer;
	printf("Password for \"%s\" exists. Overwrite? (Y/N)\n", path);
	while((answer = fgetc(stdin)))
	{
		clearStdinBuff();
		switch(answer)
		{
			case 'Y':
			case 'y': return 'y';
			case 'N':
			case 'n':
			case EOF: return 'n'; 
			default: {
				printf("Overwrite? (Y/N) ");
				break;
			}
		}
	}

	return 'n';
}
