#include "keyrow.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "config.h"

#include <gpgme.h>

/**
 * This structure handles data of a key row.
 */
struct _LockKeyRow {
    AdwActionRow parent;
};

G_DEFINE_TYPE(LockKeyRow, lock_key_row, ADW_TYPE_ACTION_ROW);

/**
 * This function initializes a LockKeyRow.
 *
 * @param row Row to be initialized
 */
static void lock_key_row_init(LockKeyRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));
}

/**
 * This function initializes a LockKeyRow class.
 *
 * @param class Row class to be initialized
 */
static void lock_key_row_class_init(LockKeyRowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("keyrow.ui"));
}

/**
 * This function creates a new LockKeyRow.
 *
 * @param key Key of the row
 *
 * @return LockKeyRow
 */
LockKeyRow *lock_key_row_new(const gchar *title, const gchar *subtitle,
                             const gchar *expiry_date, const gchar *expiry_time)
{
    gchar *tooltip_text;
    if (expiry_date == NULL || expiry_time == NULL) {
        tooltip_text = _("Key does not expire");
    } else {
        tooltip_text = g_strdup_printf(C_
                                       ("First formatter: YYYY-mm-dd; Second formatter: HH:MM",
                                        "Expires %s at %s"), expiry_date,
                                       expiry_time);
    }

    return g_object_new(LOCK_TYPE_KEY_ROW, "title", title, "subtitle", subtitle,
                        "tooltip-text", tooltip_text, NULL);
}
