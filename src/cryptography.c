#include "cryptography.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "config.h"

#include <gpgme.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define HANDLE_ERROR(Return, Error, String) if (Error) \
    { \
        g_warning(C_("Error message constructor for failed GPGME operations", "Failed to %s: %s"), String, gpgme_strerror(Error)); \
        return Return; \
    }

/**
 * This function initializes GnuPG Made Easy for a GUI application.
 */
void cryptography_init()
{
    g_message("GnuPG Made Easy %s", gpgme_check_version(NULL));
}

/**
 * This function returns the secret key of an email.
 *
 * @param email Email of the owner of the key
 *
 * @return Key. Calling `gpgme_key_release()` is required
 */
gpgme_key_t key_from_email(char *email)
{
    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"));

    error = gpgme_op_keylist_start(context, NULL, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (strcmp(key->uids->email, email) == 0)
            break;
    }
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "find key matching email"));

    return key;
}

/**
 * This function encrypts a text using a GPG key.
 *
 * @param text Text to encrypt
 * @param key Key to encrypt for
 *
 * @return Encrypted text as an OpenPGP ASCII armor. Calling `free()` is required
 */
char *encrypt_text(char *text, gpgme_key_t key)
{
    gpgme_ctx_t context;
    gpgme_data_t decrypted;
    gpgme_data_t encrypted;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"));

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&decrypted, text, strlen(text), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "create new decrypted GPGME data from string"));

    error = gpgme_data_new(&encrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new encrypted GPGME data"));

    error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                             key, NULL}, 0, decrypted, encrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "encrypt GPGME data from memory"));

    gpgme_release(context);
    gpgme_data_release(decrypted);
    gpgme_key_release(key);

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(encrypted, &length);

    char *armor = malloc(length);
    memcpy(armor, buffer, length);
    gpgme_free(buffer);

    return armor;
}

/**
 * This function decrypts an OpenPGP ASCII armored text.
 *
 * @param armor Text to decrypt
 *
 * @return Decrypted text. Calling `free()` is required
 */
char *decrypt_text(char *armor)
{
    gpgme_ctx_t context;
    gpgme_data_t encrypted;
    gpgme_data_t decrypted;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"));

    error = gpgme_data_new_from_mem(&encrypted, armor, strlen(armor), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "create new encrypted GPGME data from string"));

    error = gpgme_data_new(&decrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new decrypted GPGME data"));

    error = gpgme_op_decrypt(context, encrypted, decrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "decrypt GPGME data from memory"));

    gpgme_release(context);
    gpgme_data_release(encrypted);

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(decrypted, &length);

    char *text = malloc(length);
    memcpy(text, buffer, length);
    gpgme_free(buffer);

    return text;
}

/**
 * This function signs a text using a GPG key.
 *
 * @param text Text to sign
 *
 * @return Signed text as an OpenPGP ASCII armor. Calling `free()` is required
 */
char *sign_text(char *text)
{
    gpgme_ctx_t context;
    gpgme_data_t plain;
    gpgme_data_t sign;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"));

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&plain, text, strlen(text), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "create new unsigned GPGME data from string"));

    error = gpgme_data_new(&sign);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new signed GPGME data"));

    error = gpgme_op_sign(context, plain, sign, GPGME_SIG_MODE_NORMAL);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "sign GPGME data from memory"));

    gpgme_release(context);
    gpgme_data_release(plain);

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(sign, &length);

    char *armor = malloc(length);
    memcpy(armor, buffer, length);
    gpgme_free(buffer);

    return armor;
}
