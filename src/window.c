#include "window.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "application.h"
#include "entrydialog.h"
#include "config.h"

#include <gpgme.h>
#include "cryptography.h"

#define ACTION_MODE_TEXT 0
#define ACTION_MODE_FILE 1

#define HANDLE_ERROR_EMAIL(Return, Toast, ToastOverlay, Key, Email, FreeCode) if (Key == NULL) { \
        Toast = \
            adw_toast_new(g_strdup_printf \
                          (_("Failed to find key for email “%s”"), Email)); \
        adw_toast_set_timeout(Toast, 4); \
        adw_toast_overlay_add_toast(ToastOverlay, Toast); \
        \
        gpgme_key_release(Key); \
        \
        FreeCode \
        \
        return Return; \
    }

/**
 * This structure handles data of a window.
 */
struct _LockWindow {
    AdwApplicationWindow parent;

    AdwToastOverlay *toast_overlay;
    LockEntryDialog *encrypt_dialog;

    AdwViewStack *stack;
    unsigned int action_mode;

    /* Text */
    AdwViewStackPage *text_page;
    GtkRevealer *text_button_revealer;
    AdwSplitButton *text_button;
    GtkTextView *text_view;

    /* File */
    AdwViewStackPage *file_page;
    GFile *file_input;
    GFile *file_output;

    AdwActionRow *file_input_row;
    GtkButton *file_input_button;

    AdwActionRow *file_output_row;
    GtkButton *file_output_button;

    GtkButton *file_encrypt_button;
    GtkButton *file_decrypt_button;
    GtkButton *file_sign_button;
    GtkButton *file_verify_button;
};

G_DEFINE_TYPE(LockWindow, lock_window, ADW_TYPE_APPLICATION_WINDOW);

static void lock_window_encrypt_dialog_present(GSimpleAction * self,
                                               GVariant * parameter,
                                               LockWindow * window);
static void lock_window_stack_page_changed(AdwViewStack * self,
                                           GParamSpec * pspec,
                                           LockWindow * window);

/* Text */
static void lock_window_text_view_copy(AdwSplitButton * self,
                                       LockWindow * window);
static void lock_window_text_view_encrypt(LockEntryDialog * self,
                                          const char *email,
                                          LockWindow * window);
static void lock_window_text_view_decrypt(GSimpleAction * self,
                                          GVariant * parameter,
                                          LockWindow * window);
static void lock_window_text_view_sign(GSimpleAction * self,
                                       GVariant * parameter,
                                       LockWindow * window);
static void lock_window_text_view_verify(GSimpleAction * self,
                                         GVariant * parameter,
                                         LockWindow * window);

/* File */
static void lock_window_file_open(GObject * source_object, GAsyncResult * res,
                                  gpointer data);
static void lock_window_file_save(GObject * source_object, GAsyncResult * res,
                                  gpointer data);
static void lock_window_file_open_dialog_present(GtkButton * self,
                                                 LockWindow * window);
static void lock_window_file_save_dialog_present(GtkButton * self,
                                                 LockWindow * window);
static void lock_window_file_encrypt(LockEntryDialog * dialog,
                                     const char *email, LockWindow * window);
static void lock_window_file_decrypt(GtkButton * self, LockWindow * window);
static void lock_window_file_sign(GtkButton * self, LockWindow * window);
static void lock_window_file_verify(GtkButton * self, LockWindow * window);

/**
 * This function initializes a LockWindow.
 *
 * @param window Window to be initialized
 */
static void lock_window_init(LockWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));

    /* Page changed */
    g_signal_connect(window->stack, "notify::visible-child",
                     G_CALLBACK(lock_window_stack_page_changed), window);
    lock_window_stack_page_changed(window->stack, NULL, window);

    /* Text */
    g_signal_connect(window->text_button, "clicked",
                     G_CALLBACK(lock_window_text_view_copy), window);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(window->text_view),
                             _("Enter text …"), -1);
    // Encrypt
    g_autoptr(GSimpleAction) encrypt_text_action =
        g_simple_action_new("encrypt_text", NULL);
    g_signal_connect(encrypt_text_action, "activate",
                     G_CALLBACK(lock_window_encrypt_dialog_present), window);
    g_action_map_add_action(G_ACTION_MAP(window),
                            G_ACTION(encrypt_text_action));
    // Decrypt
    g_autoptr(GSimpleAction) decrypt_text_action =
        g_simple_action_new("decrypt_text", NULL);
    g_signal_connect(decrypt_text_action, "activate",
                     G_CALLBACK(lock_window_text_view_decrypt), window);
    g_action_map_add_action(G_ACTION_MAP(window),
                            G_ACTION(decrypt_text_action));
    // Sign
    g_autoptr(GSimpleAction) sign_text_action =
        g_simple_action_new("sign_text", NULL);
    g_signal_connect(sign_text_action, "activate",
                     G_CALLBACK(lock_window_text_view_sign), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(sign_text_action));
    // Verify
    g_autoptr(GSimpleAction) verify_text_action =
        g_simple_action_new("verify_text", NULL);
    g_signal_connect(verify_text_action, "activate",
                     G_CALLBACK(lock_window_text_view_verify), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(verify_text_action));

    /* File */
    g_signal_connect(window->file_input_button, "clicked",
                     G_CALLBACK(lock_window_file_open_dialog_present), window);
    g_signal_connect(window->file_output_button, "clicked",
                     G_CALLBACK(lock_window_file_save_dialog_present), window);
    // Encrypt
    g_signal_connect(window->file_encrypt_button, "clicked",
                     G_CALLBACK(lock_window_encrypt_dialog_present), window);
    // Decrypt
    g_signal_connect(window->file_decrypt_button, "clicked",
                     G_CALLBACK(lock_window_file_decrypt), window);
    // Sign
    g_signal_connect(window->file_sign_button, "clicked",
                     G_CALLBACK(lock_window_file_sign), window);
    // Verify
    g_signal_connect(window->file_verify_button, "clicked",
                     G_CALLBACK(lock_window_file_verify), window);
}

/**
 * This function initializes a LockWindow class.
 *
 * @param class Window class to be initialized
 */
static void lock_window_class_init(LockWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("window.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         toast_overlay);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         stack);

    /* Text */
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_button_revealer);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_view);

    /* File */
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_page);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_input_row);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_input_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_output_row);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_output_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_encrypt_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_decrypt_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_sign_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_verify_button);
}

/**
 * This function creates a new LockWindow.
 *
 * @param app Application to create the new window for
 *
 * @return LockWindow
 */
LockWindow *lock_window_new(LockApplication *app)
{
    return g_object_new(LOCK_TYPE_WINDOW, "application", app, NULL);
}

/**
 * This function opens a LockWindow.
 *
 * @param window Window to be opened
 * @param file File to be processed with the window
 */

void lock_window_open(LockWindow *window, GFile *file)
{
}

/**
 * This function presents the encrypt dialog of a LockWindow and handles encryption based on the action mode of the window.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_encrypt_dialog_present(GSimpleAction *self,
                                               GVariant *parameter,
                                               LockWindow *window)
{
    window->encrypt_dialog =
        lock_entry_dialog_new(_("Encrypt for"), _("Enter email …"),
                              GTK_INPUT_PURPOSE_EMAIL);

    void *callback = lock_window_text_view_encrypt;
    switch (window->action_mode) {
    case ACTION_MODE_TEXT:
        callback = lock_window_text_view_encrypt;
        break;
    case ACTION_MODE_FILE:
        callback = lock_window_file_encrypt;
        break;
    }
    g_signal_connect(window->encrypt_dialog, "entered", G_CALLBACK(callback),
                     window);

    adw_dialog_present(ADW_DIALOG(window->encrypt_dialog), GTK_WIDGET(window));

    /* Cleanup */
    callback = NULL;
}

/**
 * This function updates the UI on a change of a page of the stack of a LockWindow.
 *
 * @param self https://docs.gtk.org/gobject/signal.Object.notify.html
 * @param pspec https://docs.gtk.org/gobject/signal.Object.notify.html
 * @param window https://docs.gtk.org/gobject/signal.Object.notify.html
 */
static void lock_window_stack_page_changed(AdwViewStack *self,
                                           GParamSpec *pspec,
                                           LockWindow *window)
{
    GtkWidget *visible_child = adw_view_stack_get_visible_child(self);
    AdwViewStackPage *visible_page =
        adw_view_stack_get_page(self, visible_child);

    if (visible_page == window->text_page) {
        window->action_mode = ACTION_MODE_TEXT;

        gtk_revealer_set_reveal_child(window->text_button_revealer, true);
    } else if (visible_page == window->file_page) {
        window->action_mode = ACTION_MODE_FILE;

        gtk_revealer_set_reveal_child(window->text_button_revealer, false);
    }

    /* Cleanup */
    visible_child = NULL;
    visible_page = NULL;
}

/**
 * This function gets the text from the text view of a LockWindow.
 *
 * @param window Window to get the text from
 *
 * @return Text. Freeing required
 */
gchar *lock_window_text_view_get_text(LockWindow *window)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->text_view);

    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    return gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, true);
}

/**
 * This functions sets the text of the text view of a LockWindow.
 *
 * @param window Window to set the text in
 * @param text Text to overwrite the text views buffer with
 */
static void lock_window_text_view_set_text(LockWindow *window, const char *text)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->text_view);

    gtk_text_buffer_set_text(buffer, text, -1);
}

/**
 * This function copies text from the text view of a LockWindow.
 *
 * @param self https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.SplitButton.clicked.html
 * @param window https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.SplitButton.clicked.html
 */
static void lock_window_text_view_copy(AdwSplitButton *self, LockWindow *window)
{
    GdkClipboard *active_clipboard =
        gdk_display_get_clipboard(gdk_display_get_default());

    AdwToast *toast = adw_toast_new(_("Text copied"));
    adw_toast_set_timeout(toast, 2);

    gchar *text = lock_window_text_view_get_text(window);
    gdk_clipboard_set_text(active_clipboard, text);

    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(text);
    text = NULL;
}

/**
 * This function encrypts text from the text view of a LockWindow.
 *
 * @param self Dialog where an input was confirmed
 * @param email Input entered in the dialog
 * @param window Window in which the dialog was present
 */
static void lock_window_text_view_encrypt(LockEntryDialog *self,
                                          const char *email, LockWindow *window)
{
    gchar *plain = lock_window_text_view_get_text(window);
    AdwToast *toast;

    gpgme_key_t key = key_from_email(email);
    HANDLE_ERROR_EMAIL(, toast, window->toast_overlay, key, email,
                       g_free(plain);
                       plain = NULL;
        );

    gchar *armor = encrypt_text(plain, key);
    if (armor == NULL) {
        toast = adw_toast_new(_("Encryption failed"));
    } else {
        toast = adw_toast_new(_("Text encrypted"));

        lock_window_text_view_set_text(window, armor);
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(plain);
    plain = NULL;

    g_free(armor);
    armor = NULL;
}

/**
 * This function decrypts text from the text view of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_text_view_decrypt(GSimpleAction *self,
                                          GVariant *parameter,
                                          LockWindow *window)
{
    gchar *armor = lock_window_text_view_get_text(window);
    AdwToast *toast;

    gchar *text = decrypt_text(armor);
    if (text == NULL) {
        toast = adw_toast_new(_("Decryption failed"));
    } else {
        toast = adw_toast_new(_("Text decrypted"));

        lock_window_text_view_set_text(window, text);
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(armor);
    armor = NULL;

    g_free(text);
    text = NULL;
}

/**
 * This function signs text from the text view of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_text_view_sign(GSimpleAction *self, GVariant *parameter,
                                       LockWindow *window)
{
    gchar *plain = lock_window_text_view_get_text(window);
    AdwToast *toast;

    gchar *armor = sign_text(plain);
    if (armor == NULL) {
        toast = adw_toast_new(_("Signing failed"));
    } else {
        toast = adw_toast_new(_("Text signed"));

        lock_window_text_view_set_text(window, armor);
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(plain);
    plain = NULL;

    g_free(armor);
    armor = NULL;
}

/**
 * This function verifies text from the text view of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_text_view_verify(GSimpleAction *self,
                                         GVariant *parameter,
                                         LockWindow *window)
{
    gchar *armor = lock_window_text_view_get_text(window);
    AdwToast *toast;

    gchar *text = verify_text(armor);
    if (text == NULL) {
        toast = adw_toast_new(_("Verification failed"));
    } else {
        toast = adw_toast_new(_("Text verified"));

        lock_window_text_view_set_text(window, text);
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(armor);
    armor = NULL;

    g_free(text);
    text = NULL;
}

/**
 * This function opens the input file of a LockWindow.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_window_file_open(GObject *source_object, GAsyncResult *res,
                                  gpointer data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    LockWindow *window = LOCK_WINDOW(data);

    window->file_input = gtk_file_dialog_open_finish(dialog, res, NULL);
    if (window->file_input == NULL) {
        /* Cleanup */
        g_object_unref(dialog);
        dialog = NULL;

        window = NULL;

        return;
    }

    adw_action_row_set_subtitle(window->file_input_row,
                                g_file_get_basename(window->file_input));

    /* Cleanup */
    g_object_unref(dialog);
    dialog = NULL;

    window = NULL;
}

/**
 * This function opens the output file of a LockWindow.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_window_file_save(GObject *source_object,
                                  GAsyncResult *res, gpointer data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    LockWindow *window = LOCK_WINDOW(data);

    window->file_output = gtk_file_dialog_save_finish(dialog, res, NULL);
    if (window->file_output == NULL) {
        /* Cleanup */
        g_object_unref(dialog);
        dialog = NULL;

        window = NULL;

        return;
    }

    adw_action_row_set_subtitle(window->file_output_row,
                                g_file_get_basename(window->file_output));

    /* Cleanup */
    g_object_unref(dialog);
    dialog = NULL;

    window = NULL;
}

/**
 * This function opens an open file dialog for a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void
lock_window_file_open_dialog_present(GtkButton *self, LockWindow *window)
{
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_open(dialog, GTK_WINDOW(window),
                         cancel, lock_window_file_open, window);
}

/**
 * This function opens a save file dialog for a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void
lock_window_file_save_dialog_present(GtkButton *self, LockWindow *window)
{
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_save(dialog, GTK_WINDOW(window),
                         cancel, lock_window_file_save, window);
}

/**
 * This function encrypts the file of a LockWindow.
 *
 * @param self Dialog where an input was confirmed
 * @param email Input entered in the dialog
 * @param window Window in which the dialog was present
 */
static void lock_window_file_encrypt(LockEntryDialog *dialog, const char *email,
                                     LockWindow *window)
{
    char *input_path = g_file_get_path(window->file_input);
    char *output_path = g_file_get_path(window->file_output);
    AdwToast *toast;

    gpgme_key_t key = key_from_email(email);
    HANDLE_ERROR_EMAIL(, toast, window->toast_overlay, key, email,
                       /* Cleanup */
                       g_free(input_path);
                       input_path = NULL; g_free(output_path);
                       output_path = NULL;
        );

    bool success = encrypt_file(input_path, output_path, key);
    if (!success) {
        toast = adw_toast_new(_("Encryption failed"));
    } else {
        toast = adw_toast_new(_("File encrypted"));
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(input_path);
    input_path = NULL;

    g_free(output_path);
    output_path = NULL;
}

/**
 * This function decrypts the file of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_window_file_decrypt(GtkButton *self, LockWindow *window)
{
    // FIXME: cannot write to file until GPGME version 1.24

    char *input_path = g_file_get_path(window->file_input);
    char *output_path = g_file_get_path(window->file_output);
    AdwToast *toast;

    bool success = decrypt_file(input_path, output_path);
    if (!success) {
        toast = adw_toast_new(_("Decryption failed"));
    } else {
        toast = adw_toast_new(_("File decrypted"));
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(input_path);
    input_path = NULL;

    g_free(output_path);
    output_path = NULL;
}

/**
 * This function signs the file of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_window_file_sign(GtkButton *self, LockWindow *window)
{
    char *input_path = g_file_get_path(window->file_input);
    char *output_path = g_file_get_path(window->file_output);
    AdwToast *toast;

    bool success = sign_file(input_path, output_path);
    if (!success) {
        toast = adw_toast_new(_("Signing failed"));
    } else {
        toast = adw_toast_new(_("File signed"));
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(input_path);
    input_path = NULL;

    g_free(output_path);
    output_path = NULL;
}

/**
 * This function verifies the file of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_window_file_verify(GtkButton *self, LockWindow *window)
{
    // FIXME: cannot write to file until GPGME version 1.24

    char *input_path = g_file_get_path(window->file_input);
    char *output_path = g_file_get_path(window->file_output);
    AdwToast *toast;

    bool success = verify_file(input_path, output_path);
    if (!success) {
        toast = adw_toast_new(_("Verification failed"));
    } else {
        toast = adw_toast_new(_("File verified"));
    }

    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(input_path);
    input_path = NULL;

    g_free(output_path);
    output_path = NULL;
}
