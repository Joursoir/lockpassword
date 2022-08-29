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
#include "output.h"

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
	if(err != GPG_ERR_NO_ERROR)
		goto error;

	err = gpgme_set_protocol(ctx, protocol);
	if(err != GPG_ERR_NO_ERROR)
		goto error;

	/* output be ASCII armored */
	gpgme_set_armor(ctx, 1);

	return 0;
error:
	print_error("%s: %s\n", gpgme_strsource(err), gpgme_strerror(err));
	return 1; 
}

static int loop_read(const char *path, gpgme_data_t dh)
{
	char buf[maxlen_pass];
	int ret;

	FILE *f = fopen(path, "w");	
	if(f == NULL)
		return 1;

	ret = gpgme_data_seek(dh, 0, SEEK_SET);
	if (ret)
		goto out;

	while((ret = gpgme_data_read(dh, buf, maxlen_pass)) > 0) {
		fwrite(buf, ret, 1, f);
	}

out:
	if (ret != 0) {
		gpgme_error_t err = gpgme_err_code_from_errno(errno);
		print_error("%s: %s\n", gpgme_strsource(err), gpgme_strerror(err));
	}
	fclose(f);
	return ret;
}

int ecnrypt_data(const char *path, const char *data, const char *pubkey)
{
	gpgme_error_t err;	
	gpgme_ctx_t ctx;
	gpgme_data_t plain, cipher;
	gpgme_key_t key[] = { NULL, NULL };
	int ret = 1;

	init_gpgme();
	err = gpgme_new(&ctx);
	if (err != GPG_ERR_NO_ERROR)
		goto out;
	err = init_ctx(ctx, GPGME_PROTOCOL_OPENPGP);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_context;

	/* start encrypt */
	err = gpgme_data_new_from_mem(&plain, data,
		sizeof(char) * (strlen(data) + 1), 0);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_context;
	err = gpgme_data_new(&cipher);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_plain;

	err = gpgme_get_key(ctx, pubkey, &key[0], 0);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_cipher;

	err = gpgme_op_encrypt(ctx, key, GPGME_ENCRYPT_ALWAYS_TRUST,
		plain, cipher);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_cipher;

	ret = loop_read(path, cipher);

out_release_cipher:
	gpgme_data_release(cipher);
out_release_plain:
	gpgme_data_release(plain);
out_release_context:
	gpgme_release(ctx);
out:
	if (err != GPG_ERR_NO_ERROR) {
		print_error("%s: %s\n", gpgme_strsource(err), gpgme_strerror(err));
	}
	return ret;
}

char *decrypt_data(const char *path)
{
	gpgme_error_t err;	
	gpgme_ctx_t ctx;
	gpgme_data_t cipher, plain;
	char *data = NULL;

	init_gpgme();
	err = gpgme_new(&ctx);
	if (err != GPG_ERR_NO_ERROR)
		goto out;
	err = init_ctx(ctx, GPGME_PROTOCOL_OPENPGP);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_context;

	/* start decrypt */
	err = gpgme_data_new_from_file(&cipher, path, 1);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_context;
	err = gpgme_data_new(&plain);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_cipher;

	err = gpgme_op_decrypt(ctx, cipher, plain);
	if (err != GPG_ERR_NO_ERROR)
		goto out_release_plain;

	int ret = gpgme_data_seek(plain, 0, SEEK_SET);
	if (ret) {
		err = gpgme_err_code_from_errno(errno);
		goto out_release_plain;
	}

	data = malloc(sizeof(char) * (maxlen_pass + 1));
	gpgme_data_read(plain, data, maxlen_pass);
	
out_release_plain:
	gpgme_data_release(plain);
out_release_cipher:
	gpgme_data_release(cipher);
out_release_context:
	gpgme_release(ctx);
out:
	if (err != GPG_ERR_NO_ERROR) {
		print_error("%s: %s\n", gpgme_strsource(err), gpgme_strerror(err));
	}
	return data;
}
