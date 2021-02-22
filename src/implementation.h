#ifndef IMPLEMENTATION_H
#define IMPLEMENTATION_H

#define GPG_PUBLICKEY_MAXLENGTH 1025 // +1 for '\0'

void checkForbiddenPaths(char *path);
char *getGPGKey(char *dest, size_t size);
char* getPassword(char *path_pass, char *password, size_t size, int flag_copy);
void nonvisibleEnter(int status);
void insertPass(char *add_path, char *password, int flag_copy);
char *typePass(char *text, char *dest, int minlen, int maxlen);
int userEnterPassword(int minlen, int maxlen, char *path_insert, int flag_echo, int flag_copy);
char *generatePassword(char *dest, int amount);
int getOverwriteAnswer(char *path);

#endif