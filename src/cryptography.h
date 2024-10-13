#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <gpgme.h>

#include <stdbool.h>

void cryptography_init();

gpgme_key_t key_from_email(char *email);

char *encrypt_text(char *text, gpgme_key_t key);
char *decrypt_text(char *armor);

char *sign_text(char *text);

#endif                          // CRYPTOGRAPHY_H
