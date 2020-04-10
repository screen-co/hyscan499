#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <hyscan-gtk-configurator.h>

#define GETTEXT_PACKAGE "hyscan-499"
#include <hyscan-config.h>

#ifdef G_OS_WIN32 /* Входная точка для GUI wWinMain. */
  #include <Windows.h>
#endif

int
main (int argc, char **argv)
{
  GtkWidget *configurator;
  const gchar *config_dir;
  gboolean exit_if_configured = FALSE;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, hyscan_config_get_locale_dir ());
  bindtextdomain ("libhyscanfnn", hyscan_config_get_locale_dir ());
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

  config_dir = hyscan_config_get_user_files_dir ();
  configurator = hyscan_gtk_configurator_new (config_dir);

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
          LPWSTR    lpCmdLine,
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
