#ifndef LPASS_RGPGME_H
#define LPASS_RGPGME_H

int ecnrypt_data(const char *path, const char *data, const char *pubkey);
char *decrypt_data(const char *path);

#endif /* LPASS_RGPGME_H */
