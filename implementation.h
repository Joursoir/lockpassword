#ifndef IMPLEMENTATION_H
#define IMPLEMENTATION_H

char* getPassword(char *path, char *password, int maxlen);
void nonvisibleEnter(int status);
void insertPass(char *add_path, char *password);
char *typePass(char *text, char *dest, int minlen, int maxlen);
int userEnterPassword(int minlen, int maxlen, char *path_insert);
char *generatePassword(char *dest, int amount, int max_len);
unsigned long hash(char *str);
int getAnswer(char *text);

#endif