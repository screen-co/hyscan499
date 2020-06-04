#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "hyscan-gtk-configurator.h"
#include "hyscan-migrate-config.h"
#include <hyscan-config.h>

#ifdef G_OS_WIN32 /* Входная точка для GUI wWinMain. */
  #include <Windows.h>
#endif

int
main (int    argc,
      char **argv)
{
  HyScanConfigurator *model = NULL;
  HyScanMigrateConfig *migrate = NULL;
  GtkWidget *configurator;

  gboolean exit_if_configured = FALSE;
  gchar *path = NULL;
  gint result = 0;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, hyscan_config_get_locale_dir ());
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Разбор командной строки. */
  {
    GError *error = NULL;
    gchar **args;
    GOptionContext *context;
    GOptionEntry entries[] =
    {
      { "path",               'p',   0, G_OPTION_ARG_STRING, &path,    "Path to config directory",   NULL },
      { "exit-if-configured", 'e',   0, G_OPTION_ARG_NONE,   &exit_if_configured, "Exit if already configured", NULL },
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

  if (path == NULL)
    path = g_strdup (hyscan_config_get_user_files_dir ());

  /* Запускаем миграцию конфигурации при необходимости. */
  migrate = hyscan_migrate_config_new (path);
  if (hyscan_migrate_config_status (migrate) == HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED)
    {
      GtkWidget *dialog, *message;
      gint response;

      dialog = gtk_dialog_new_with_buttons (_("Configuration is outdated"), NULL, 0,
                                            _("_Update"), GTK_RESPONSE_OK,
                                            _("_Quit"), GTK_RESPONSE_CLOSE,
                                            NULL);

      message = gtk_label_new (_("Configuration is outdated. Would you like to update it to the latest version?"));
      g_object_set (message, "margin", 6, NULL);
      gtk_widget_set_margin_bottom (message, 18);
      gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), message);
      gtk_widget_show_all (dialog);

      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      switch (response)
        {
        case GTK_RESPONSE_OK:
          if (!hyscan_migrate_config_run (migrate, NULL))
            {
              result = -1;
              goto exit;
            }
          break;

        case GTK_RESPONSE_CLOSE:
        default:
          result = -1;
          goto exit;
        }
    }

  /* Проверяем, что хайскан сконфигурирован. */
  model = hyscan_configurator_new (path);
  if (exit_if_configured && hyscan_configurator_is_valid (model))
    goto exit;

  /* Открываем окно настроек при необходимости. */

  configurator = hyscan_gtk_configurator_new (model);
  g_signal_connect_after (configurator, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (configurator);
  gtk_main ();

exit:
  g_free (path);
  g_clear_object (&model);
  g_clear_object (&migrate);

  return result;
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
