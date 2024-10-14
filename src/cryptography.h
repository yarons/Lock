#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <gpgme.h>

#include <stdbool.h>

void cryptography_init();

gpgme_key_t key_from_email(char *email);

char *encrypt_text(char *text, gpgme_key_t key);
bool encrypt_file(char *input_path, char *output_path, gpgme_key_t key);

char *decrypt_text(char *armor);
bool decrypt_file(char *input_path, char *output_path);

char *sign_text(char *text);
char *verify_text(char *armor);

#endif                          // CRYPTOGRAPHY_H
