#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "application.h"
#include "config.h"

/**
 * This function is the entry point of the program.
 * 
 * @param argc Number of arguments passed
 * @param argv Arguments passed
 *
 * @return Exit code
 */
int main(int argc, char *argv[])
{
    // Localization
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    // GUI
    int status = g_application_run(G_APPLICATION(lock_application_new()), argc,
                                   argv);

    return status;
}