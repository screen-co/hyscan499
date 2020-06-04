#include "hyscan-migrate-config.h"
#include <hyscan-config.h>

int
main (int    argc,
      char **argv)
{
  gchar *path = NULL;
  HyScanMigrateConfig *migrate_config;

  /* Разбор командной строки. */
  {
    GError *error = NULL;
    gchar **args;
    GOptionContext *context;
    GOptionEntry entries[] =
    {
      { "path",               'p',   0, G_OPTION_ARG_STRING, &path,    "Path to config directory",   NULL },
      { NULL, }
    };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    context = g_option_context_new (NULL);
    g_option_context_set_summary (context, "Migrate HyScan configuration");
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

  migrate_config = hyscan_migrate_config_new (path);
  switch (hyscan_migrate_config_status (migrate_config))
    {
    case HYSCAN_MIGRATE_CONFIG_STATUS_LATEST:
      g_print ("Configuration is up to date\n");
      break;

    case HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY:
      g_print ("Configuration is not found\n");
      break;

    case HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED:
      g_print ("Migrating config...\n");
      if (hyscan_migrate_config_run (migrate_config, NULL))
        g_print ("Migration finished successfully\n");
      else
        g_print ("Migration failed\n");
      break;

    default:
      g_print ("Failed to read configuration\n");
    }

  g_object_unref (migrate_config);
  g_free (path);

  return 0;
}
