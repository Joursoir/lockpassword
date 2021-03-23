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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <gpgme.h>

#include "constants.h"
#include "r-gpgme.h"

#ifdef DEBUG
	#define ret_if_err(ret, err) \
		do { \
			if(err != GPG_ERR_NO_ERROR) { \
				fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, \
					__LINE__, gpgme_strsource(err), \
					gpgme_strerror(err)); \
				return ret; \
			} \
		} while(0)
#else
	#define ret_if_err(ret, err) \
		do { \
			if(err != GPG_ERR_NO_ERROR) { \
				fprintf(stderr, "%s: %s\n", \
					gpgme_strsource(err), gpgme_strerror(err)); \
				return ret; \
			} \
		} while(0)
#endif

static void init_gpgme()
{
	/* The GPGME library communicates with child processes (the 
	crypto engines). If a child process dies unexpectedly, for
	example due to a bug, or system problem, a SIGPIPE signal
	will be delivered to the application. The default action is
	to abort the program. To protect against this,
	gpgme_check_version sets the SIGPIPE signal action to SIG_IGN
	which means that the signal will be ignored. */
	setlocale(LC_ALL, "");
	gpgme_check_version(NULL);
	gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
	gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif
}

static int init_ctx(gpgme_ctx_t ctx, gpgme_protocol_t protocol)
{
	gpgme_error_t err;
	err = gpgme_engine_check_version(protocol);
	ret_if_err(1, err);
	err = gpgme_set_protocol(ctx, protocol);
	ret_if_err(1, err);
	/* output be ASCII armored */
	gpgme_set_armor(ctx, 1);
	return 0;
}

static int loop_read(const char *path, gpgme_data_t dh)
{
	char buf[maxlen_pass];
	int ret;

	FILE *f = fopen(path, "w");	
	if(f == NULL)
		return 1;

	ret = gpgme_data_seek(dh, 0, SEEK_SET);
	if(ret) {
		fclose(f);
		ret_if_err(1, gpgme_err_code_from_errno(errno));
	}
	while((ret = gpgme_data_read(dh, buf, maxlen_pass)) > 0)
		fwrite(buf, ret, 1, f);
	if(ret < 0) {
		fclose(f);
		ret_if_err(1, gpgme_err_code_from_errno(errno));
	}

	fclose(f);
	return 0;
}

int ecnrypt_data(const char *path, const char *data, const char *pubkey)
{
	gpgme_error_t err;	
	gpgme_ctx_t ctx;
	gpgme_data_t plain, cipher;
	gpgme_key_t key[] = { NULL, NULL };
	init_gpgme();

	err = gpgme_new(&ctx);
	ret_if_err(1, err);
	err = init_ctx(ctx, GPGME_PROTOCOL_OPENPGP);
	ret_if_err(1, err);

	/* start encrypt */

	err = gpgme_data_new_from_mem(&plain, data,
		sizeof(char) * (strlen(data) + 1), 0);
	ret_if_err(1, err);
	err = gpgme_data_new(&cipher);
	ret_if_err(1, err);

	err = gpgme_get_key(ctx, pubkey, &key[0], 0);
	ret_if_err(1, err);

	err = gpgme_op_encrypt(ctx, key, GPGME_ENCRYPT_ALWAYS_TRUST,
		plain, cipher);
	ret_if_err(1, err);

	if(loop_read(path, cipher))
		return 1;
	
	gpgme_data_release(plain);
	gpgme_data_release(cipher);
	gpgme_release(ctx);
	return 0;
}

char *decrypt_data(const char *path)
{
	gpgme_error_t err;	
	gpgme_ctx_t ctx;
	gpgme_data_t cipher, plain;
	init_gpgme();

	err = gpgme_new(&ctx);
	ret_if_err(NULL, err);
	err = init_ctx(ctx, GPGME_PROTOCOL_OPENPGP);
	ret_if_err(NULL, err);

	/* start decrypt */

	err = gpgme_data_new_from_file(&cipher, path, 1);
	ret_if_err(NULL, err);
	err = gpgme_data_new(&plain);
	ret_if_err(NULL, err);

	err = gpgme_op_decrypt(ctx, cipher, plain);
	ret_if_err(NULL, err);

	int ret = gpgme_data_seek(plain, 0, SEEK_SET);
	if(ret)
		ret_if_err(NULL, gpgme_err_code_from_errno(errno));

	char *data = malloc(sizeof(char) * (maxlen_pass + 1));
	gpgme_data_read(plain, data, maxlen_pass);
	
	gpgme_data_release(plain);
	gpgme_data_release(cipher);
	gpgme_release(ctx);
	return data;
}
