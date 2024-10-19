#ifndef KEY_ROW_H
#define KEY_ROW_H

#include <adwaita.h>

#include <gpgme.h>

#define LOCK_TYPE_KEY_ROW (lock_key_row_get_type())

G_DECLARE_FINAL_TYPE(LockKeyRow, lock_key_row, LOCK, KEY_ROW, AdwActionRow);

LockKeyRow *lock_key_row_new(const gchar * title, const gchar * subtitle,
                             const gchar * expiry_date,
                             const gchar * expiry_time);

#endif                          // KEY_ROW_H
