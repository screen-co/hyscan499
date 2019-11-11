#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <hyscan-gtk-configurator.h>

#define GETTEXT_PACKAGE "hyscan-499"

#ifdef G_OS_WIN32 /* Входная точка для GUI wWinMain. */
  #include <Windows.h>
#endif

static const gchar *
configurator_get_locale_dir (void)
{
  static gchar *locale_dir = NULL;

  if (locale_dir == NULL)
    {
#ifdef G_OS_WIN32
      gchar *utf8_path;
      gchar *install_dir;

      install_dir = g_win32_get_package_installation_directory_of_module (NULL);
      utf8_path = g_build_filename (install_dir, "share", "locale", NULL);
      locale_dir = g_win32_locale_filename_from_utf8 (utf8_path);

      g_free (install_dir);
      g_free (utf8_path);
#else
      locale_dir = FNN_LOCALE_DIR;
#endif
    }

  return locale_dir;
}

int
main (int argc, char **argv)
{
  GtkWidget *configurator;
  gchar *config_dir;
  gboolean exit_if_configured = FALSE;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, configurator_get_locale_dir ());
  bindtextdomain ("libhyscanfnn", configurator_get_locale_dir ());
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bind_textdomain_codeset ("libhyscanfnn", "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Разбор командной строки. */
  {
    GError *error = NULL;
    gchar **args;
    GOptionContext *context;
    GOptionEntry entries[] =
    {
      { "exit-if-configured", 'e',   0, G_OPTION_ARG_NONE,  &exit_if_configured, "Exit if already configured", NULL },
      { NULL, }
    };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    gtk_init (&argc, &argv);

    context = g_option_context_new (NULL);
    g_option_context_set_summary (context, "Configure HyScan DB path and map cache location before start");
    g_option_context_set_help_enabled (context, TRUE);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse_strv (context, &args, &error))
      {
        g_print ("%s\n", error->message);
        return -1;
      }

    g_strfreev (args);
  }

  config_dir = g_build_path (G_DIR_SEPARATOR_S, g_get_user_config_dir (), "hyscan",  NULL);
  configurator = hyscan_gtk_configurator_new (config_dir);
  g_free (config_dir);

  if (exit_if_configured && hyscan_gtk_configurator_configured (HYSCAN_GTK_CONFIGURATOR (configurator)))
    {
      gtk_widget_destroy (configurator);
    }
  else
    {
      g_signal_connect_after (configurator, "destroy", G_CALLBACK (gtk_main_quit), NULL);
      gtk_widget_show_all (configurator);
      gtk_main ();
    }

  return 0;
}

#ifdef G_OS_WIN32
/* Точка входа в приложение для Windows. */
int WINAPI
wWinMain (HINSTANCE hInst,
          HINSTANCE hPreInst,
          LPSTR     lpCmdLine,
          int       nCmdShow)
{
  int argc;
  char **argv;

  gint result;

  argv = g_win32_get_command_line ();
  argc = g_strv_length (argv);

  result = main (argc, argv);

  g_strfreev (argv);

  return result;
}
#endif
