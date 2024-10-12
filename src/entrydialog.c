#include "entrydialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "config.h"

/**
 * This structure handles data of a window.
 */
struct _LockEntryDialog {
    AdwDialog parent;

    GtkEntry *entry;

    GtkButton *confirm_button;
};

G_DEFINE_TYPE(LockEntryDialog, lock_entry_dialog, ADW_TYPE_DIALOG);

static void lock_entry_dialog_entry_confirm(GtkButton * self,
                                            LockEntryDialog * dialog);

/**
 * This function initializes a LockEntryDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_entry_dialog_init(LockEntryDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));

    g_signal_connect(dialog->confirm_button, "clicked",
                     G_CALLBACK(lock_entry_dialog_entry_confirm), dialog);
}

/**
 * This function initializes a LockEntryDialog class.
 *
 * @param class Dialog class to be initialized
 */
static void lock_entry_dialog_class_init(LockEntryDialogClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("entrydialog.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEntryDialog, entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEntryDialog, confirm_button);

    g_signal_new("entered", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, 0,
                 NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1,
                 G_TYPE_STRING);
}

/**
 * This function creates a new LockEntryDialog.
 *
 * @param title Title of the dialog
 *
 * @return LockEntryDialog
 */
LockEntryDialog *lock_entry_dialog_new(gchar *placeholder_text)
{
    return g_object_new(LOCK_TYPE_ENTRY_DIALOG, NULL);
}

/**
 * This function gets text from the entry of a LockEntryDialog.
 *
 * @param dialog Dialog to get the text from
 *
 * @return Text
 */
const char *lock_entry_dialog_get_text(LockEntryDialog *dialog)
{
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(dialog->entry);

    return gtk_entry_buffer_get_text(buffer);
}

/**
 * This function handles input confirmation of a LockEntryDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_entry_dialog_entry_confirm(GtkButton *self,
                                            LockEntryDialog *dialog)
{
    const char *text = lock_entry_dialog_get_text(dialog);

    if (!(strlen(text) > 0) || text == NULL)
        return;

    g_signal_emit_by_name(dialog, "entered", text);

    adw_dialog_close(ADW_DIALOG(dialog));
}
