#ifndef LPASS_CONSTANTS_H
#define LPASS_CONSTANTS_H

/***
	This file is part of LockPassword
	Copyright (C) 2020-2021 Aleksandr D. Goncharov (Joursoir) <chat@joursoir.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
***/

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
