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

#define HANDLE_ERROR(Return, Error, String, Context, FreeCode) if (Error) \
    { \
        g_warning(C_("Error message constructor for failed GPGME operations", "Failed to %s: %s"), String, gpgme_strerror(Error)); \
        \
        gpgme_release(Context); \
        \
        FreeCode \
        \
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
gpgme_key_t key_from_email(const char *email)
{
    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_op_keylist_start(context, NULL, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (strcmp(key->uids->email, email) == 0)
            break;
    }
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "find key matching email"),
                 context, gpgme_key_release(key););

    /* Cleanup */
    gpgme_release(context);

    return key;
}

/**
 * This function imports a key from a file.
 *
 * @param path Path of the file to import as a key
 *
 * @return Success
 */
bool key_import_from_file(const char *path)
{
    gpgme_ctx_t context;
    gpgme_data_t keydata;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_data_new_from_file(&keydata, path, 1);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "load GPGME key data from file"), context,
                 gpgme_data_release(keydata);
        );

    error = gpgme_op_import(context, keydata);
    HANDLE_ERROR(false, error, C_("GPGME Error", "import GPG key from file"),
                 context, gpgme_data_release(keydata);
        );

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(keydata);

    return true;
}

/**
 * This function encrypts a text using a GPG key.
 *
 * @param text Text to encrypt
 * @param key Key to encrypt for
 *
 * @return Encrypted text as an OpenPGP ASCII armor. Calling `free()` is required
 */
char *encrypt_text(const char *text, gpgme_key_t key)
{
    gpgme_ctx_t context;
    gpgme_data_t decrypted;
    gpgme_data_t encrypted;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&decrypted, text, strlen(text), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "create new decrypted GPGME data from string"), context,
                 gpgme_data_release(decrypted););

    error = gpgme_data_new(&encrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new encrypted GPGME data"), context,
                 gpgme_data_release(decrypted); gpgme_data_release(encrypted););

    error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                             key, NULL}, 0, decrypted, encrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "encrypt GPGME data from memory"), context,
                 gpgme_data_release(decrypted); gpgme_data_release(encrypted););

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(encrypted, &length);

    char *armor = malloc(length + 1);
    memcpy(armor, buffer, length);
    armor[length] = '\0';

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(decrypted);

    gpgme_free(buffer);
    buffer = NULL;

    return armor;
}

/**
 * This function decrypts an OpenPGP ASCII armored text.
 *
 * @param armor Text to decrypt
 *
 * @return Decrypted text. Calling `free()` is required
 */
char *decrypt_text(const char *armor)
{
    gpgme_ctx_t context;
    gpgme_data_t encrypted;
    gpgme_data_t decrypted;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_set_pinentry_mode(context, GPGME_PINENTRY_MODE_ASK);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "set pinentry mode of GPGME context to ask"), context,);

    error = gpgme_data_new_from_mem(&encrypted, armor, strlen(armor), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "create new encrypted GPGME data from string"), context,
                 gpgme_data_release(encrypted););

    error = gpgme_data_new(&decrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new decrypted GPGME data"), context,
                 gpgme_data_release(encrypted); gpgme_data_release(decrypted););

    error = gpgme_op_decrypt(context, encrypted, decrypted);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "decrypt GPGME data from memory"), context,
                 gpgme_data_release(encrypted); gpgme_data_release(decrypted););

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(decrypted, &length);

    char *text = malloc(length + 1);
    memcpy(text, buffer, length);
    text[length] = '\0';

    /* Cleanup */
    gpgme_free(buffer);
    buffer = NULL;

    gpgme_release(context);
    gpgme_data_release(encrypted);

    return text;
}

/**
 * This function signs a text using a GPG key.
 *
 * @param text Text to sign
 *
 * @return Signed text as an OpenPGP ASCII armor. Calling `free()` is required
 */
char *sign_text(const char *text)
{
    gpgme_ctx_t context;
    gpgme_data_t plain;
    gpgme_data_t sign;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_set_pinentry_mode(context, GPGME_PINENTRY_MODE_ASK);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "set pinentry mode of GPGME context to ask"), context,);

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&plain, text, strlen(text), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "create new unsigned GPGME data from string"), context,
                 gpgme_data_release(plain););

    error = gpgme_data_new(&sign);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new signed GPGME data"), context,
                 gpgme_data_release(plain); gpgme_data_release(sign););

    error = gpgme_op_sign(context, plain, sign, GPGME_SIG_MODE_NORMAL);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "sign GPGME data from memory"),
                 context, gpgme_data_release(plain); gpgme_data_release(sign););

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(sign, &length);

    char *armor = malloc(length + 1);
    memcpy(armor, buffer, length);
    armor[length] = '\0';

    /* Cleanup */
    gpgme_free(buffer);
    buffer = NULL;

    gpgme_release(context);
    gpgme_data_release(plain);

    return armor;
}

/**
 * This function verifies a text using a GPG key.
 *
 * @param armor Text to verify
 *
 * @return Verified text. Calling `free()` is required
 */
char *verify_text(const char *armor)
{
    gpgme_ctx_t context;
    gpgme_data_t sign;
    gpgme_data_t plain;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&sign, armor, strlen(armor), 1);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new signed GPGME data from string"),
                 context, gpgme_data_release(sign););

    error = gpgme_data_new(&plain);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "create new verified GPGME data"), context,
                 gpgme_data_release(sign); gpgme_data_release(plain););

    error = gpgme_op_verify(context, sign, NULL, plain);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "verify GPGME data from memory"), context,
                 gpgme_data_release(sign); gpgme_data_release(plain););

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(plain, &length);

    char *text = malloc(length + 1);
    memcpy(text, buffer, length);
    text[length] = '\0';

    /* Cleanup */
    gpgme_free(buffer);
    buffer = NULL;

    gpgme_release(context);
    gpgme_data_release(sign);

    return text;
}

/**
 * This function encrypts a file using a GPG key.
 *
 * @param input_path Path to the file to encrypt
 * @param output_path Path to write the encrypted file to
 * @param key Key to encrypt for
 *
 * @return Success
 */
bool encrypt_file(const char *input_path, const char *output_path,
                  gpgme_key_t key)
{
    gpgme_ctx_t context;
    gpgme_data_t decrypted;
    gpgme_data_t encrypted;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_data_new_from_file(&decrypted, input_path, 1);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error",
                    "create new decrypted GPGME data from file"), context,
                 gpgme_data_release(decrypted););

    error = gpgme_data_new(&encrypted);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "create new encrypted GPGME data"), context,
                 gpgme_data_release(decrypted); gpgme_data_release(encrypted););

    error = gpgme_data_set_file_name(encrypted, output_path);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "set file path of encrypted GPGME data"),
                 context, gpgme_data_release(decrypted);
                 gpgme_data_release(encrypted););

    error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                             key, NULL}, 0, decrypted, encrypted);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "encrypt GPGME data from file"), context,
                 gpgme_data_release(decrypted); gpgme_data_release(encrypted););

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(decrypted);
    gpgme_data_release(encrypted);

    return true;
}

/**
 * This function decrypts a file.
 *
 * @param input_path Path to the file to decrypt
 * @param output_path Path to write the decrypted file to
 *
 * @return Success
 */
bool decrypt_file(const char *input_path, const char *output_path)
{
    gpgme_ctx_t context;
    gpgme_data_t encrypted;
    gpgme_data_t decrypted;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_set_pinentry_mode(context, GPGME_PINENTRY_MODE_ASK);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "set pinentry mode of GPGME context to ask"), context,);

    error = gpgme_data_new_from_file(&encrypted, input_path, 1);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error",
                    "create new encrypted GPGME data from file"), context,
                 gpgme_data_release(encrypted););

    error = gpgme_data_new(&decrypted);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "create new decrypted GPGME data"), context,
                 gpgme_data_release(encrypted); gpgme_data_release(decrypted););

    error = gpgme_data_set_file_name(decrypted, output_path);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "set file path of decrypted GPGME data"),
                 context, gpgme_data_release(encrypted);
                 gpgme_data_release(decrypted););

    error = gpgme_op_decrypt(context, encrypted, decrypted);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "decrypt GPGME data from file"), context,
                 gpgme_data_release(encrypted); gpgme_data_release(decrypted););

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(decrypted);
    gpgme_data_release(encrypted);

    return true;
}

/**
 * This function signs a file using a GPG key.
 *
 * @param input_path Path to the file to sign
 * @param output_path Path to write the signed file to
 *
 * @return Success
 */
bool sign_file(const char *input_path, const char *output_path)
{
    gpgme_ctx_t context;
    gpgme_data_t plain;
    gpgme_data_t sign;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_set_pinentry_mode(context, GPGME_PINENTRY_MODE_ASK);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error",
                    "set pinentry mode of GPGME context to ask"), context,);

    error = gpgme_data_new_from_file(&plain, input_path, 1);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "create new unsigned GPGME data from file"),
                 context, gpgme_data_release(plain););

    error = gpgme_data_new(&sign);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "create new signed GPGME data"), context,
                 gpgme_data_release(plain); gpgme_data_release(sign););

    error = gpgme_data_set_file_name(sign, output_path);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "set file path of signed GPGME data"),
                 context, gpgme_data_release(plain); gpgme_data_release(sign););

    error = gpgme_op_sign(context, plain, sign, GPGME_SIG_MODE_NORMAL);
    HANDLE_ERROR(false, error, C_("GPGME Error", "sign GPGME data from file"),
                 context, gpgme_data_release(plain); gpgme_data_release(sign););

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(plain);
    gpgme_data_release(sign);

    return true;
}

/**
 * This function verifies a file.
 *
 * @param input_path Path to the file to verify
 * @param output_path Path to write the verified file to
 *
 * @return Success
 */
bool verify_file(const char *input_path, const char *output_path)
{
    gpgme_ctx_t context;
    gpgme_data_t sign;
    gpgme_data_t plain;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error, C_("GPGME Error", "create new GPGME context"),
                 context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 C_("GPGME Error", "set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_data_new(&sign);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "create new signed GPGME data"), context,
                 gpgme_data_release(sign););

    error = gpgme_data_set_file_name(sign, input_path);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "set file path of signed GPGME data"),
                 context, gpgme_data_release(sign););

    error = gpgme_data_new(&plain);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "create new unsigned GPGME data"), context,
                 gpgme_data_release(sign); gpgme_data_release(plain););

    error = gpgme_data_set_file_name(plain, output_path);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "set file path of unsigned GPGME data"),
                 context, gpgme_data_release(sign); gpgme_data_release(plain););

    error = gpgme_op_verify(context, sign, NULL, plain);
    HANDLE_ERROR(false, error,
                 C_("GPGME Error", "verify GPGME data from file"), context,
                 gpgme_data_release(sign); gpgme_data_release(plain););

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(sign);
    gpgme_data_release(plain);

    return true;
}
