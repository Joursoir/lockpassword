#ifndef IMPLEMENTATION_H
#define IMPLEMENTATION_H

#define GPG_PUBLICKEY_MAXLENGTH 1024

#define errprint(RET, ...) \
	do { \
		fprintf(stderr, "Error: " __VA_ARGS__); \
		return RET; \
	} while(0)

void copy_outside(char *password);
int check_sneaky_paths(const char *path);
char *get_pubkey();
char *get_password(const char *path);
void visible_enter(int status);
int insert_pass(const char *path, const char *password);
char *get_input(int minlen, int maxlen);
void gen_password(char *dest, int amount);
int overwrite_answer(const char *path);

#endif