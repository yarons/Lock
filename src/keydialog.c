#include "keydialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "config.h"

/**
 * This structure handles data of a key dialog.
 */
struct _LockKeyDialog {
    AdwDialog parent;

    GtkRevealer *status_revealer;
    GtkListBox *key_box;

    GtkButton *import_button;
};

G_DEFINE_TYPE(LockKeyDialog, lock_key_dialog, ADW_TYPE_DIALOG);

/* UI */

static void lock_key_dialog_refresh(LockKeyDialog * dialog);

/**
 * This function initializes a LockKeyDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_key_dialog_init(LockKeyDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));
    lock_key_dialog_refresh(dialog);
}

/**
 * This function initializes a LockKeyDialog class.
 *
 * @param class Dialog class to be initialized
 */
static void lock_key_dialog_class_init(LockKeyDialogClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("keydialog.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         status_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         key_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         import_button);
}

/**
 * This function creates a new LockKeyDialog.
 *
 * @return LockKeyDialog
 */
LockKeyDialog *lock_key_dialog_new()
{
    return g_object_new(LOCK_TYPE_KEY_DIALOG, NULL);
}

/**** UI ****/

/**
 * This function refreshes the key list of a LockKeyDialog.
 *
 * @param dialog Dialog to refresh the list of
 */
static void lock_key_dialog_refresh(LockKeyDialog *dialog)
{
    if (gtk_list_box_get_row_at_index(dialog->key_box, 0) == NULL) {
        gtk_revealer_set_reveal_child(dialog->status_revealer, true);
    } else {
        gtk_revealer_set_reveal_child(dialog->status_revealer, false);
    }
}
