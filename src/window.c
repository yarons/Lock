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

/**
 * This structure handles data of a window.
 */
struct _LockWindow {
    AdwApplicationWindow parent;

    AdwToastOverlay *toast_overlay;
    AdwViewStack *stack;

    unsigned int action_mode;

    LockEntryDialog *encrypt_dialog;
    AdwSplitButton *action_button;

    AdwViewStackPage *text_page;
    GtkTextView *text_view;

    AdwViewStackPage *file_page;
};

G_DEFINE_TYPE(LockWindow, lock_window, ADW_TYPE_APPLICATION_WINDOW);

static void lock_window_encrypt_dialog_present(GSimpleAction * self,
                                               GVariant * parameter,
                                               LockWindow * window);
static void lock_window_decrypt(GSimpleAction * self, GVariant * parameter,
                                LockWindow * window);

static void lock_window_sign(GSimpleAction *self, GVariant *parameter, LockWindow *window);


static void lock_window_stack_page_changed(AdwViewStack * self,
                                           GParamSpec * pspec,
                                           LockWindow * window);

static void lock_window_text_view_copy(AdwSplitButton * self,
                                       LockWindow * window);
static void lock_window_text_view_encrypt(LockEntryDialog * self, char *email,
                                          LockWindow * window);
static void lock_window_text_view_decrypt(LockWindow * window);

static void lock_window_text_view_sign(LockWindow *window);

/**
 * This function initializes a LockWindow.
 *
 * @param window Window to be initialized
 */
static void lock_window_init(LockWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));

    g_autoptr(GSimpleAction) encrypt_action =
        g_simple_action_new("encrypt", NULL);
    g_signal_connect(encrypt_action, "activate",
                     G_CALLBACK(lock_window_encrypt_dialog_present), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(encrypt_action));

    g_autoptr(GSimpleAction) decrypt_action =
        g_simple_action_new("decrypt", NULL);
    g_signal_connect(decrypt_action, "activate",
                     G_CALLBACK(lock_window_decrypt), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(decrypt_action));

    g_autoptr(GSimpleAction) sign_action = g_simple_action_new("sign", NULL);
    g_signal_connect(sign_action, "activate", G_CALLBACK(lock_window_sign), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(sign_action));


    g_signal_connect(window->stack, "notify::visible-child",
                     G_CALLBACK(lock_window_stack_page_changed), window);
    lock_window_stack_page_changed(window->stack, NULL, window);

    gtk_text_buffer_set_text(gtk_text_view_get_buffer(window->text_view),
                             _("Enter text …"), -1);
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
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         action_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_view);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_page);
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
 * This function handles decryption based on the action mode of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_decrypt(GSimpleAction *self, GVariant *parameter,
                                LockWindow *window)
{
    switch (window->action_mode) {
    case ACTION_MODE_TEXT:
        lock_window_text_view_decrypt(window);
        break;
    case ACTION_MODE_FILE:
        break;
    }
}

/**
 * This function handles signing based on the action mode of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_sign(GSimpleAction *self, GVariant *parameter, LockWindow *window)
{
    switch (window->action_mode) {
    case ACTION_MODE_TEXT:
        lock_window_text_view_sign(window);
        break;
    case ACTION_MODE_FILE:
        break;
    }
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
        break;
    }
    g_signal_connect(window->encrypt_dialog, "entered", G_CALLBACK(callback),
                     window);

    adw_dialog_present(ADW_DIALOG(window->encrypt_dialog), GTK_WIDGET(window));
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

    AdwSplitButton *action_button = window->action_button;

    if (visible_page == window->text_page) {
        adw_split_button_set_label(action_button, _("Copy"));
        g_signal_connect(action_button, "clicked",
                         G_CALLBACK(lock_window_text_view_copy), window);

        window->action_mode = ACTION_MODE_TEXT;
    } else if (visible_page == window->file_page) {
        adw_split_button_set_label(action_button, _("Save"));

        window->action_mode = ACTION_MODE_FILE;
    }
}

/**
 * This function gets the text from the text view of a LockWindow.
 *
 * @param window Window to get the text from
 *
 * @return Text
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
}

/**
 * This function encrypts text from the text view of a LockWindow.
 *
 * @param self Dialog where an input was confirmed
 * @param email Input entered in the dialog
 * @param window Window in which the dialog was present
 */
static void lock_window_text_view_encrypt(LockEntryDialog *self, char *email,
                                          LockWindow *window)
{
    gchar *plain = lock_window_text_view_get_text(window);
    AdwToast *toast;

    gpgme_key_t key = key_from_email(email);
    if (key == NULL) {
        toast =
            adw_toast_new(g_strdup_printf
                          (_("Failed to find key for email “%s”"), email));
        adw_toast_set_timeout(toast, 4);

        gpgme_key_release(key);

        adw_toast_overlay_add_toast(window->toast_overlay, toast);
        return;
    }

    gchar *armor = encrypt_text(plain, key);
    if (armor == NULL) {
        toast = adw_toast_new(_("Encryption failed"));
    } else {
        toast = adw_toast_new(_("Text encrypted"));

        lock_window_text_view_set_text(window, armor);
    }
    g_free(armor);
    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);
}

/**
 * This function decrypts text from the text view of a LockWindow.
 *
 * @param window Window containing the text view to decrypt
 */
static void lock_window_text_view_decrypt(LockWindow *window)
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
    g_free(text);
    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);
}

/**
 * This function signs text from the text view of a LockWindow.
 *
 * @param window Window containing the text view to decrypt
 */
static void lock_window_text_view_sign(LockWindow *window)
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
    g_free(armor);
    adw_toast_set_timeout(toast, 3);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);
}
