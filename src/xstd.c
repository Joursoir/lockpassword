/***
	This file is part of LockPassword
	Copyright (C) 2020-2025 Aleksandr D. Goncharov (Joursoir) <chat@joursoir.net>

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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xstd.h"

char *xstrcat(const char *first, ...)
{
    va_list args;
    const char *s;
    size_t total_len = 0;
    size_t len;

    // 1. Calculate the total length required
    if (first != NULL) {
        total_len += strlen(first);
    }

    va_start(args, first);
    while ((s = va_arg(args, const char *)) != NULL) {
        total_len += strlen(s);
    }
    va_end(args);

    // 2. Allocate memory for the concatenated string
    char *result = malloc(total_len + 1);
    if (result == NULL) {
        return NULL;
    }

    char *ptr = result;

    // 3. Copy the first string and then the rest of the strings
    if (first != NULL) {
        len = strlen(first);
        memcpy(ptr, first, len);
        ptr += len;
    }

    va_start(args, first);
    while ((s = va_arg(args, const char *)) != NULL) {
        len = strlen(s);
        memcpy(ptr, s, len);
        ptr += len;
    }
    va_end(args);

    *ptr = '\0';
    return result;
}
