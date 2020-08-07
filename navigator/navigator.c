#include <locale.h>
#include <libintl.h>
#include <hyscan-config.h>
#include "hyscan-gtk-navigator-app.h"

int
main (int    argc,
      char **argv)
{
  GtkApplication *navigator_app;
  gint status;
  gchar **args;
  gchar *db_uri = NULL;
  gchar **driver_paths = NULL;

  /* Параметры командной строки. */
  {
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] =
      {
        { "db-uri",  'd',   0, G_OPTION_ARG_STRING,       &db_uri,       "* DB uri", NULL },
        { "drivers", 'o',   0, G_OPTION_ARG_STRING_ARRAY, &driver_paths, "* Path to directories with driver(s)", NULL},
        { NULL }
      };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    context = g_option_context_new ("<db-uri>");
    g_option_context_set_help_enabled (context, TRUE);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    if (!g_option_context_parse_strv (context, &args, &error))
      {
        g_print ("%s\n", error->message);
        return -1;
      }

    if (db_uri == NULL)
      {
        g_print ("%s", g_option_context_get_help (context, FALSE, NULL));
        return -1;
      }

    g_option_context_free (context);
  }

  {
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, hyscan_config_get_locale_dir());
    bindtextdomain ("hyscangui", hyscan_config_get_locale_dir());
    bindtextdomain ("hyscangtk", hyscan_config_get_locale_dir());
    bindtextdomain ("hyscancore", hyscan_config_get_locale_dir());
    bindtextdomain ("libhyscanfnn", hyscan_config_get_locale_dir());
    bindtextdomain ("hyscanfnn-swui", hyscan_config_get_locale_dir());
    bindtextdomain ("hyscanfnn-evoui", hyscan_config_get_locale_dir());

    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    bind_textdomain_codeset ("hyscangui" , "UTF-8");
    bind_textdomain_codeset ("hyscangtk" , "UTF-8");
    bind_textdomain_codeset ("hyscancore" , "UTF-8");
    bind_textdomain_codeset ("libhyscanfnn" , "UTF-8");
    bind_textdomain_codeset ("hyscanfnn-swui", "UTF-8");
    bind_textdomain_codeset ("hyscanfnn-evoui", "UTF-8");

    textdomain (GETTEXT_PACKAGE);
  }

  navigator_app = hyscan_gtk_navigator_app_new (db_uri, driver_paths);
  status = g_application_run (G_APPLICATION (navigator_app), g_strv_length (args), args);
  g_object_unref (navigator_app);

  g_strfreev (args);

  return status;
}
