#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <gpgme.h>

void cryptography_init();

gpgme_key_t key_from_email(char *email);

char *encrypt_text(char *text, gpgme_key_t key);

#endif                          // CRYPTOGRAPHY_H
