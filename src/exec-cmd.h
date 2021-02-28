#ifndef LPASS_EXECCMD_H
#define LPASS_EXECCMD_H

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

int cmd_showtree(int argc, char *argv[]);
int cmd_init(int argc, char *argv[]);
int cmd_insert(int argc, char *argv[]);
int cmd_edit(int argc, char *argv[]);
int cmd_generate(int argc, char *argv[]);
int cmd_remove(int argc, char *argv[]);
int cmd_move(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_version(int argc, char *argv[]);

#endif /* LPASS_EXECCMD_H */
