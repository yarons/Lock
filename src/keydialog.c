#include "keydialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "keyrow.h"
#include "config.h"

#include <gpgme.h>
#include <time.h>
#include "cryptography.h"
#include "threading.h"

/**
 * This structure handles data of a key dialog.
 */
struct _LockKeyDialog {
    AdwDialog parent;

    LockWindow *window;

    GtkButton *refresh_button;
    AdwToastOverlay *toast_overlay;
    GtkBox *content_box;

    AdwStatusPage *status_page;
    GtkListBox *key_box;

    gboolean import_success;
    GtkButton *import_button;
    GFile *import_file;
};

G_DEFINE_TYPE(LockKeyDialog, lock_key_dialog, ADW_TYPE_DIALOG);

/* UI */
static void lock_key_dialog_refresh(GtkButton * self, LockKeyDialog * dialog);

gboolean lock_key_dialog_import_on_completed(LockKeyDialog * dialog);

/* Import */
static void lock_key_dialog_import_file_present(GtkButton * self,
                                                LockKeyDialog * dialog);

/**
 * This function initializes a LockKeyDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_key_dialog_init(LockKeyDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));

    g_signal_connect(dialog->refresh_button, "clicked",
                     G_CALLBACK(lock_key_dialog_refresh), dialog);
    lock_key_dialog_refresh(NULL, dialog);

    g_signal_connect(dialog->import_button, "clicked",
                     G_CALLBACK(lock_key_dialog_import_file_present), dialog);
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
                                         refresh_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         toast_overlay);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         content_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         status_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         key_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyDialog,
                                         import_button);
}

/**
 * This function creates a new LockKeyDialog.
 *
 * @param window Window in which the dialog is presented
 *
 * @return LockKeyDialog
 */
LockKeyDialog *lock_key_dialog_new(LockWindow *window)
{
    LockKeyDialog *dialog = g_object_new(LOCK_TYPE_KEY_DIALOG, NULL);

    /* TODO: implement g_object_class_install_property() */
    dialog->window = window;

    return dialog;
}

/**** UI ****/

/**
 * This function refreshes the key list of a LockKeyDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_key_dialog_refresh(GtkButton *self, LockKeyDialog *dialog)
{
    gtk_list_box_remove_all(dialog->key_box);

    size_t expiry_date_length = strlen("YYYY-mm-dd") + 1;
    gchar *expiry_date = malloc(expiry_date_length * sizeof(char));

    size_t expiry_time_length = strlen("HH:MM") + 1;
    gchar *expiry_time = malloc(expiry_time_length * sizeof(char));

    time_t expiry_timestamp;
    struct tm *expiry;

    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    if (error)
        return;

    error = gpgme_op_keylist_start(context, NULL, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (key->subkeys->expires == 0) {
            expiry_date = NULL;
            expiry_time = NULL;
        } else {
            expiry_timestamp = (time_t) key->subkeys->expires;
            expiry = localtime(&expiry_timestamp);

            strftime(expiry_date, expiry_date_length, "%Y-%m-%d", expiry);
            strftime(expiry_time, expiry_time_length, "%H:%M", expiry);
        }

        gtk_list_box_append(dialog->key_box,
                            GTK_WIDGET(lock_key_row_new
                                       (dialog, key->uids->uid,
                                        key->subkeys->fpr, expiry_date,
                                        expiry_time)));
    }

    /* Cleanup */
    gpgme_release(context);
    gpgme_key_release(key);

    g_free(expiry_date);
    expiry_date = NULL;

    g_free(expiry_time);
    expiry_time = NULL;

    if (gtk_list_box_get_row_at_index(dialog->key_box, 0) == NULL) {
        gtk_widget_set_visible(GTK_WIDGET(dialog->key_box), false);
        gtk_box_set_spacing(dialog->content_box, 0);

        gtk_widget_set_visible(GTK_WIDGET(dialog->status_page), true);
    } else {
        gtk_widget_set_visible(GTK_WIDGET(dialog->key_box), true);
        gtk_box_set_spacing(dialog->content_box, 20);

        gtk_widget_set_visible(GTK_WIDGET(dialog->status_page), false);
    }
}

/**
 * This functions returns the window of a LockKeyDialog.
 *
 * @param dialog Dialog to get the window of
 *
 * @return LockWindow
 */
LockWindow *lock_key_dialog_get_window(LockKeyDialog *dialog)
{
    return dialog->window;
}

/**
 * This function adds a toast the toast_overlay of a LockKeyDialog.
 *
 * @param dialog Dialog to add the toast to
 * @param toast Toast to add to the dialog
 */
void lock_key_dialog_add_toast(LockKeyDialog *dialog, AdwToast *toast)
{
    adw_toast_overlay_add_toast(dialog->toast_overlay, toast);
}

/**** Import ****/

/**
 * This function opens the import file of a LockKeyDialog.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_key_dialog_import_file_open(GObject *source_object,
                                             GAsyncResult *res, gpointer data)
{
    GtkFileDialog *file = GTK_FILE_DIALOG(source_object);
    LockKeyDialog *dialog = LOCK_KEY_DIALOG(data);

    dialog->import_file = gtk_file_dialog_open_finish(file, res, NULL);
    if (dialog->import_file == NULL) {
        lock_key_dialog_import_on_completed(dialog);

        /* Cleanup */
        g_object_unref(file);
        file = NULL;

        dialog = NULL;

        return;
    }

    thread_import_key(dialog);
}

/**
 * This function opens an open file dialog for a LockKeyDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_key_dialog_import_file_present(GtkButton *self,
                                                LockKeyDialog *dialog)
{
    GtkFileDialog *file = gtk_file_dialog_new();
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_open(file, GTK_WINDOW(dialog->window),
                         cancel, lock_key_dialog_import_file_open, dialog);
}

/**
 * This function imports a key in a LockKeyDialog.
 *
 * @param dialog https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_key_dialog_import(LockKeyDialog *dialog)
{
    char *path = g_file_get_path(dialog->import_file);

    dialog->import_success = key_import(path);

    /* Cleanup */
    g_free(path);
    path = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_key_dialog_import_on_completed, dialog);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for key imports and is supposed to be called via g_idle_add().
 *
 * @param dialog https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_key_dialog_import_on_completed(LockKeyDialog *dialog)
{
    AdwToast *toast;

    if (dialog->import_file == NULL) {
        toast = adw_toast_new(_("Could not open file"));
    } else if (!dialog->import_success) {
        toast = adw_toast_new(_("Import failed"));
    } else {
        toast = adw_toast_new(_("Key(s) imported"));
    }

    adw_toast_set_timeout(toast, 2);
    adw_toast_overlay_add_toast(dialog->toast_overlay, toast);

    lock_key_dialog_refresh(NULL, dialog);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}
