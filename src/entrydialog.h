#ifndef ENTRY_DIALOG_H
#define ENTRY_DIALOG_H

#include <adwaita.h>
#include "window.h"

#define LOCK_TYPE_ENTRY_DIALOG (lock_entry_dialog_get_type())

G_DECLARE_FINAL_TYPE(LockEntryDialog, lock_entry_dialog, LOCK, ENTRY_DIALOG,
                     AdwDialog);

LockEntryDialog *lock_entry_dialog_new(gchar * title, gchar * placeholder_text,
                                       GtkInputPurpose purpose);

const char *lock_entry_dialog_get_text(LockEntryDialog * dialog);

#endif                          // ENTRY_DIALOG_H
