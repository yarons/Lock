#include "window.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "application.h"
#include "config.h"

/**
 * This structure handles data of a window.
 */
struct _LockWindow {
    AdwApplicationWindow parent;

    AdwViewStack *stack;
    AdwSplitButton *action_button;

    AdwViewStackPage *text_page;
    AdwToastOverlay *text_toast;
    GtkTextView *text_view;

    AdwViewStackPage *file_page;
};

G_DEFINE_TYPE(LockWindow, lock_window, ADW_TYPE_APPLICATION_WINDOW);

static void lock_window_stack_page_changed(AdwViewStack * self,
                                           GParamSpec * pspec,
                                           LockWindow * window);

static void lock_window_text_view_copy(AdwSplitButton * self,
                                       LockWindow * window);

/**
 * This function initializes a LockWindow.
 *
 * @param window Window to be initialized
 */
static void lock_window_init(LockWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));

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
                                         stack);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         action_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_toast);
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
    } else if (visible_page == window->file_page) {
        adw_split_button_set_label(action_button, _("Save"));
    }
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

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->text_view);

    GtkTextIter start_iter;
    GtkTextIter end_iter;

    AdwToast *toast = adw_toast_new(_("Text copied"));
    adw_toast_set_timeout(toast, 2);

    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    gchar *text =
        gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, true);
    gdk_clipboard_set_text(active_clipboard, text);

    adw_toast_overlay_add_toast(window->text_toast, toast);
}
