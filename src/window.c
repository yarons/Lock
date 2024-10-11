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
};

G_DEFINE_TYPE(LockWindow, lock_window, ADW_TYPE_APPLICATION_WINDOW);

/**
 * This function initializes a LockWindow.
 *
 * @param window Window to be initialized
 */
static void lock_window_init(LockWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
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
