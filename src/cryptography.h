#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <gpgme.h>

#include <stdbool.h>

typedef enum {
    ENCRYPT = 1 << 0,
    DECRYPT = 1 << 1,
    SIGN = 1 << 2,
    VERIFY = 1 << 3
} cryptography_flags;

void cryptography_init();

// Keys
gpgme_key_t key_search(const char *uid);
bool key_import(const char *path);
bool key_generate(const char *userid, const char *sign_algorithm,
                  const char *encrypt_algorithm, unsigned long expiry);
bool key_export(const char *uid, const char *path);
bool key_remove(gpgme_key_t key);

/* Operations */
char *process_text(const char *text, cryptography_flags flags, gpgme_key_t key);

// Encrypt
bool encrypt_file(const char *input_path, const char *output_path,
                  gpgme_key_t key);

// Decrypt
bool decrypt_file(const char *input_path, const char *output_path);

// Sign
bool sign_file(const char *input_path, const char *output_path);

// Verify
bool verify_file(const char *input_path, const char *output_path);

#endif                          // CRYPTOGRAPHY_H
