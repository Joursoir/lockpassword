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

#include <stdlib.h>
#include <string.h>

#include "xstd.h"

char *xstrcat(const char *first, const char *second,
	const char *delimiter)
{
	size_t size = strlen(first) + strlen(second) + 1;
	if(delimiter)
		size += strlen(delimiter);
	char *res = malloc(size * sizeof(char));
	strcpy(res, first);
	if(delimiter)
		strcat(res, delimiter);
	strcat(res, second);
	return res;
}

