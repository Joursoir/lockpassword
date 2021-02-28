#ifndef LPASS_CONSTANTS_H
#define LPASS_CONSTANTS_H

#define VERSION "1.0c"
#define DATE_RELEASE "14 January, 2021"

#define LOCKPASS_DIR ".lock-password/"
#define GPGKEY_FILE ".gpg-key"

#define errprint(RET, ...) \
	do { \
		fprintf(stderr, "Error: " __VA_ARGS__); \
		return RET; \
	} while(0)

#define usageprint(...) \
	do { \
		fprintf(stdout, "Usage: lpass " __VA_ARGS__); \
		return 1; \
	} while(0)

#ifdef DEBUG
	#define dbgprint(...) fprintf(stderr, "Debug: " __VA_ARGS__)
#else
	#define dbgprint(...)
#endif

enum {
	maxlen_texteditor = 16,
	minlen_pass = 1,
	maxlen_pass = 128,
	stdlen_pass = 14
};

#endif /* LPASS_CONSTANTS_H */
