#include "cryptography.h"

#include <adwaita.h>

#include <gpgme.h>

#include <stdio.h>
#include <string.h>

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
    if (error)
        return NULL;

    error = gpgme_op_keylist_start(context, NULL, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (strcmp(key->uids->email, email) == 0)
            break;
    }

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
    if (error)
        return NULL;
    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&decrypted, text, strlen(text), 0);
    if (error)
        return NULL;

    error = gpgme_data_new(&encrypted);
    if (error)
        return NULL;

    error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                             key, NULL}
                             , 0, decrypted, encrypted);
    if (error)
        return NULL;

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
