#ifndef IMPLEMENTATION_H
#define IMPLEMENTATION_H

#define GPG_PUBLICKEY_MAXLENGTH 1024

enum asnwers {
	OW_YES = 0,
	OW_NO = 1,
};

int check_sneaky_paths(const char *path);
char *getGPGKey();
char* getPassword(char *path_pass, char *password, size_t size, int flag_copy);
void nonvisibleEnter(int status);
int insertPass(char *path, char *password, int flag_copy);
char *getInput(int minlen, int maxlen);
char *generatePassword(char *dest, int amount);
int getOverwriteAnswer(char *path);

#endif