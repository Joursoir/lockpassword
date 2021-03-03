#ifndef IMPLEMENTATION_H
#define IMPLEMENTATION_H

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

#define GPG_PUBLICKEY_MAXLENGTH 1024

int copy_outside(char *password);
int check_sneaky_paths(const char *path);
char *get_pubkey();
char *get_password(const char *path);
void visible_enter(int status);
int insert_pass(const char *path, const char *password);
char *get_input(int minlen, int maxlen);
char *gen_password(int length);
int overwrite_answer(const char *path);

#endif