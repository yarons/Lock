#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <gpgme.h>

#include <stdbool.h>

void cryptography_init();

// Keys
gpgme_key_t key_search(const char *uid);
bool key_import(const char *path);
bool key_export(const char *uid, const char *path);

// Encrypt
char *encrypt_text(const char *text, gpgme_key_t key);
bool encrypt_file(const char *input_path, const char *output_path,
                  gpgme_key_t key);

// Decrypt
char *decrypt_text(const char *armor);
bool decrypt_file(const char *input_path, const char *output_path);

// Sign
char *sign_text(const char *text);
bool sign_file(const char *input_path, const char *output_path);

// Verify
char *verify_text(const char *armor);
bool verify_file(const char *input_path, const char *output_path);

#endif                          // CRYPTOGRAPHY_H
