#ifndef ENTRY_DIALOG_H
#define ENTRY_DIALOG_H

#include <adwaita.h>
#include "window.h"

#define LOCK_TYPE_ENTRY_DIALOG (lock_entry_dialog_get_type())

G_DECLARE_FINAL_TYPE(LockEntryDialog, lock_entry_dialog, LOCK, ENTRY_DIALOG,
                     AdwDialog);

LockEntryDialog *lock_entry_dialog_new(const gchar * title,
                                       const gchar * placeholder_text,
                                       GtkInputPurpose purpose);

const gchar *lock_entry_dialog_get_text(LockEntryDialog * dialog);

#endif                          // ENTRY_DIALOG_H
