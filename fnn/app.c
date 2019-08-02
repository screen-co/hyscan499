#include <stdlib.h>
#include <fnn-types.h>
#include <hyscan-fnn-flog.h>
#include <hyscan-fnn-splash.h>
#include <hyscan-fnn-project.h>
#include <hyscan-fnn-button.h>
#include <hyscan-gtk-con.h>
#include <hyscan-profile-hw.h>
#include <hyscan-profile-offset.h>
#include <hyscan-profile-db.h>
#include <gmodule.h>

#ifdef G_OS_WIN32 /* Входная точка для GUI wWinMain. */
  #include <Windows.h>
#endif

#ifndef G_OS_WIN32 /* Ловим сигналы ОС. */
 #include <signal.h>
 #include <errno.h>
#endif

#ifdef G_OS_WIN32
    #include <direct.h>
    #define getcwd _getcwd /* https://stackoverflow.com/q/2868680 */
#else
    #include <unistd.h>
#endif

#define GETTEXT_PACKAGE "hyscan-499"
#include <glib/gi18n.h>

enum
{
  CONNECTOR_CANCEL = 10,
  CONNECTOR_CLOSE,
};

/* Вот он, наш жирненький красавчик. */
Global global = {0,};
gint con_status = 0;

typedef gboolean (*ui_build_fn) (Global *);
typedef gboolean (*ui_config_fn) (GKeyFile *);

/* Функции уя. */
ui_build_fn        ui_build = NULL;
ui_config_fn       ui_config = NULL;
ui_config_fn       ui_setting = NULL;
ui_config_fn       ui_desetup = NULL;
ui_build_fn        ui_destroy = NULL;
ui_pack_fn         ui_pack = NULL;
ui_vadjust_fn      ui_adj_vis = NULL;


gboolean module_loader_helper (GModule     *module,
                               const gchar *name,
                               gpointer    *dst)
{
  g_module_symbol (module, name, dst);

  if (*dst == NULL)
    {
      g_warning ("Inconsistent UI: %s not found", name);
      return FALSE;
    }

  return TRUE;
}

/* Обработчик сигналов TERM и INT. */
void
shutdown_handler (gint signum)
{
  static int shutdown_counter = 0;

  shutdown_counter++;
  if (shutdown_counter == 1)
    {
      g_message ("Soft exit.");
      gtk_main_quit ();
    }
  if (shutdown_counter == 3)
    {
      g_message ("Hard exit.");
      exit (-1);
    }
}

void
destroy_cb (GtkWidget *w)
{
  g_message ("Desetup");
  ui_desetup (global.settings);
}

void
connector_finished (GtkAssistant *ass,
                    Global       *global)
{
  if (hyscan_gtk_con_get_result (HYSCAN_GTK_CON (ass)))
    {
      global->control = hyscan_gtk_con_get_control (HYSCAN_GTK_CON (ass));
      global->control_s = HYSCAN_SONAR (global->control);
      con_status = CONNECTOR_CLOSE;
    }
  else
    {
      con_status = CONNECTOR_CANCEL;
    }
  gtk_widget_destroy (GTK_WIDGET (ass));
  gtk_main_quit();
}

gchar **
va_to_strv (int           n,
            const gchar * first,
            va_list       args)
{
  gchar ** strv = NULL;
  const gchar * str = NULL;
  int count = 0;

  strv = g_realloc (strv, (sizeof (gchar*)) * (count + 1));
  strv[count] = g_strdup (first);
  ++count;

  do
    {
      str = va_arg (args, gchar*);
      strv = g_realloc (strv, (sizeof (gchar*)) * (count + 1));
      strv[count] = g_strdup (str);
      ++count;
    }
  while (str != NULL);

  return strv;
}

#ifdef G_OS_WIN32
gchar *
win32_build_path (int n, ...)
{
  static gchar *exec_dir = NULL;
  gchar *path = NULL;
  gchar *utf8_path;
  va_list args;
  gchar ** args_str;

  /* ... If that directory's last component is "bin" or "lib",
   * its parent directory is returned, otherwise the directory itself. */
  if (exec_dir == NULL)
    {
      exec_dir = g_win32_get_package_installation_directory_of_module (NULL);
      if (exec_dir == NULL)
        {
          g_warning ("Something is totally wrong");
          return NULL;
        }

      g_message("installation dir: %s", exec_dir);
    }

  va_start (args, n);
  args_str = va_to_strv (n, exec_dir, args);
  va_end (args);

  utf8_path = g_build_filenamev (args_str);

  g_message ("Path: %s", utf8_path);
  return utf8_path;
}
#endif

static const gchar *
get_locale_dir (void)
{
  static gchar *locale_dir = NULL;

  if (locale_dir == NULL)
    {
      #ifdef G_OS_WIN32
        gchar *utf8_path;
        utf8_path = win32_build_path (2, "share", "locale", NULL);
        locale_dir = g_win32_locale_filename_from_utf8 (utf8_path);
        g_free (utf8_path);
      #else
        locale_dir = FNN_LOCALE_DIR;
      #endif

      g_message ("locale_dir: <%s>", locale_dir);
    }

  return locale_dir;
}

static const gchar *
get_profile_dir (void)
{
  static gchar *profile_dir = NULL;

  if (profile_dir == NULL)
    {
      #ifdef G_OS_WIN32
        profile_dir = win32_build_path (1, "share", NULL);
      #else
        profile_dir = FNN_PROFILE_DIR;
      #endif

      g_message ("profile_dir: <%s>", profile_dir);
    }

  return profile_dir;
}


int
main (int argc, char **argv)
{
  gint               cache_size = 0;

  gchar             *db_uri = NULL;            /* Адрес базы данных. */
  gchar             *project_name = NULL;      /* Название проекта. */
  gdouble            sound_velocity = 1500.0;  /* Скорость звука по умолчанию. */
  gdouble            ship_speed = 1.0;         /* Скорость движения судна. */
  gboolean           full_screen = FALSE;      /* Признак полноэкранного режима. */

  gchar             *settings_file = NULL;     /* Название файла с параметрами. */
  gchar             *config_file = NULL;       /* Название файла конфигурации. */
  GKeyFile          *config = NULL;
  GKeyFile          *hardware = NULL;

  gchar             *um_path = FNN_DEFAULT_UI;  /* Модуль с интерфейсом. */
  GModule           *ui_module = NULL;

  GtkBuilder        *common_builder = NULL;

  // HyScanFnnSplash   *splash = NULL;
  guint              sensor_label_writer_tag = 0;
  gchar             *hardware_profile = NULL;
  gchar            **driver_paths = NULL;              /* Путь к драйверам гидролокатора. */

  gboolean status;
  GOptionContext *context;

  global.canary = 123456789;

  /* Перенаправление логов в файл. */
  #ifdef FNN_LOGGING
  hyscan_fnn_flog_open ("hyscan", 1000000);
  #endif

  gtk_init (&argc, &argv);

  {
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, get_locale_dir());
    bindtextdomain ("hyscangui", get_locale_dir());
    bindtextdomain ("libhyscanfnn", get_locale_dir());
    bindtextdomain ("hyscanfnn-swui", get_locale_dir());
    bindtextdomain ("hyscanfnn-evoui", get_locale_dir());

    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    bind_textdomain_codeset ("hyscangui" , "UTF-8");
    bind_textdomain_codeset ("libhyscanfnn" , "UTF-8");
    bind_textdomain_codeset ("hyscanfnn-swui", "UTF-8");
    bind_textdomain_codeset ("hyscanfnn-evoui", "UTF-8");

    textdomain (GETTEXT_PACKAGE);
  }

  /* Разбор командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionGroup * group;

    GOptionEntry common_entries[] =
      {
        { "db-uri",          'd',   0, G_OPTION_ARG_STRING,  &db_uri,          "* DB uri", NULL },
        { "cache",           'c',   0, G_OPTION_ARG_INT,     &cache_size,      "* Cache size", NULL },

        { "sound-velocity",  'v',   0, G_OPTION_ARG_DOUBLE,  &sound_velocity,  "Sound velocity, m/s", NULL },
        { "ship-speed",      'e',   0, G_OPTION_ARG_DOUBLE,  &ship_speed,      "Ship speed, m/s", NULL },
        { "full-screen",     'f',   0, G_OPTION_ARG_NONE,    &full_screen,     "Full screen mode", NULL },

        { "hardware",        'p',   0, G_OPTION_ARG_STRING,       &hardware_profile, "* Hardware profile name", NULL },
        { "drivers",         'o',   0, G_OPTION_ARG_STRING_ARRAY, &driver_paths,     "* Path to directories with driver(s)", NULL},

        { "settings",        'l',   0, G_OPTION_ARG_STRING,  &settings_file,   "* Settings file", NULL },
        { "config",          'k',   0, G_OPTION_ARG_STRING,  &config_file,     "Configuration file", NULL },

        { "ui", 0,   G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &um_path,    "UI module", NULL },
        { NULL, }
      };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    context = g_option_context_new (NULL);
    g_option_context_set_description (context, "The items marked with * can be "
                                      "passed in config file in [common] group "
                                      "and long parameter as name");
    g_option_context_set_help_enabled (context, TRUE);
    g_option_context_set_ignore_unknown_options (context, FALSE);

    group = g_option_group_new ("common", "General parameters", "General parameters", NULL, NULL);
    g_option_group_add_entries (group, common_entries);
    g_option_context_add_group(context, group);
    if (!g_option_context_parse_strv (context, &args, &error))
      {
        g_print ("%s\n", error->message);
        return -1;
      }

    g_strfreev (args);
  }

#ifndef G_OS_WIN32 /* Ловим сигналы ОС. */
  {
    struct sigaction term_signal;

    /* Обработка сигналов TERM и INT. */
    sigemptyset (&term_signal.sa_mask);
    term_signal.sa_handler = shutdown_handler;
    term_signal.sa_flags = SA_RESTART;

    if ((sigaction (SIGTERM, &term_signal, NULL) != 0) ||
        (sigaction (SIGINT, &term_signal, NULL) != 0))
      {
        g_message ("can't setup signals handler: %s", strerror (errno));
        return -1;
      }
  }
#endif

  /* Грузим модуль построения интерфейса. */
  {
    gchar *module_path;
    gboolean status = FALSE;

    module_path = g_build_filename (".", um_path, NULL);
    ui_module = g_module_open (module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    if (ui_module != NULL)
      {
        status  = module_loader_helper (ui_module, "build_interface", (gpointer *) &ui_build);
        status &= module_loader_helper (ui_module, "destroy_interface", (gpointer *) &ui_destroy);

        status &= module_loader_helper (ui_module, "kf_config", (gpointer *) &ui_config);
        status &= module_loader_helper (ui_module, "kf_setting", (gpointer *) &ui_setting);
        status &= module_loader_helper (ui_module, "kf_desetup", (gpointer *) &ui_desetup);

        status &= module_loader_helper (ui_module, "panel_pack", (gpointer *)  &ui_pack);
        status &= module_loader_helper (ui_module, "panel_adjust_visibility", (gpointer *)  &ui_adj_vis);
      }
    else
      {
        g_warning ("UI <%s> not loaded", module_path);
      }
    g_free (module_path);

    /* Если что-то не получилось загрузить, то уходим отсюда. */
    if (!status)
      goto exit;

    global.ui.pack = ui_pack;
    global.ui.adjust_visibility = ui_adj_vis;
  }

  fnn_init (&global);


  /* конфигурационные файлы. */
  /* Конфиг файл ИНТЕРФЕЙСА. */
  if (config_file != NULL)
    {
      config = g_key_file_new ();
      status = g_key_file_load_from_file (config, config_file, G_KEY_FILE_NONE, NULL);

      if (!status)
        {
          g_message ("failed to load config file <%s>", config_file);
          g_clear_pointer (&config, g_key_file_unref);
        }

      /* Верифицируем и пытаемся считать значения из конфига. */
      if (db_uri == NULL)
        db_uri = keyfile_string_read_helper (config, "common", "db-uri");
      if (cache_size == 0)
        cache_size = keyfile_uint_read_helper (config, "common", "cache", 2048);
      if (hardware_profile == NULL)
        hardware_profile = keyfile_string_read_helper (config, "common", "hardware");
      if (driver_paths == NULL)
        driver_paths = keyfile_strv_read_helper (config, "common", "drivers");
      if (settings_file == NULL)
        settings_file = keyfile_string_read_helper (config, "common", "settings");
    }

  /* Файл c настройками. */
  if (settings_file == NULL)
    settings_file = g_build_filename (g_get_user_config_dir (), "hyscan",
                                      "settings.ini", NULL);

  global.settings = g_key_file_new ();
  g_key_file_load_from_file (global.settings, settings_file, G_KEY_FILE_NONE, NULL);



  /***
   *     ___   ___         ___         ___
   *      | |   | |       |   | |\  |   | |       |   | |   |
   *      + |   +-        |-+-| | + |   + |       |-+-| | + |
   *      | |   | |       |   | |  \|   | |       |   | |/ \|
   *     ---   ---                     ---
   * Подключение к БД и локатору.
   * - если заданы дб ИЛИ дб+гл, то просто подключаемся
   * - если не задано ничего, то запускаем коннектор
   */

  if (db_uri != NULL)
    {
      /* Подключение к базе данных. */
      global.db = hyscan_db_new (db_uri);
      hyscan_exit_if_w_param (global.db == NULL, "can't connect to db '%s'", db_uri);
    }
  else
    {
      gchar *folder;
      /* К О С Т Ы Л И
       * О С Т Ы Л И
       * С Т Ы Л И
       * Т Ы Л И
       * Ы Л И
       * Л И
       * И
       */
      /* беру первую попавшуюся БД. Мне насрать. */
      folder = g_build_filename (g_get_user_config_dir (), "hyscan","db-profiles", NULL);
      {
        const gchar *filename;
        GError *error = NULL;
        GDir *dir;

        if (!g_file_test (folder, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS))
          {
            g_warning ("HyScanFNN: directory %s doesn't exist", folder);
            // return;
          }

        dir = g_dir_open (folder, 0, &error);
        if (error != NULL)
          {
            g_warning ("HyScanFNN: %s", error->message);
            g_clear_pointer (&error, g_error_free);
            // return;
          }

        while ((filename = g_dir_read_name (dir)) != NULL)
          {
            gchar *fullname;
            HyScanProfileDB *profile;

            fullname = g_build_filename (folder, filename, NULL);
            profile = hyscan_profile_db_new (fullname);
            hyscan_profile_read (HYSCAN_PROFILE (profile));

            g_free (fullname);
            global.db = hyscan_profile_db_connect (profile);

            if (global.db != NULL)
              break;
          }

        g_dir_close (dir);
      }

    }

  /* теперь запускаю педрильный коннектор*/
  {
      GtkWidget *con;

      con = hyscan_gtk_con_new (get_profile_dir(), driver_paths, global.settings, global.db);
      g_signal_connect_swapped (con, "close", G_CALLBACK (g_print), "close\n");
      g_signal_connect_swapped (con, "cancel", G_CALLBACK (g_print), "cancel\n");
      g_signal_connect (con, "cancel", G_CALLBACK (connector_finished), &global);
      g_signal_connect (con, "close", G_CALLBACK (connector_finished), &global);


      gtk_widget_show_all (con);
      gtk_main ();

      hyscan_exit_if (con_status != CONNECTOR_CLOSE, "Connector closed or failed");
  }

  /* К этому моменту подвезли global.control и global.db. Настраиваю контрол. */
  if (global.control != NULL)
    {
      guint32 n_sources;
      const gchar * const * sensors;
      const HyScanSourceType * source;
      guint32 i;

      hyscan_control_device_bind (global.control);
      hyscan_control_writer_set_db (global.control, global.db);

      global.infos = g_hash_table_new (g_direct_hash, g_direct_equal);

      sensors = hyscan_control_sensors_list (global.control);
      for (; sensors != NULL && *sensors != NULL; ++sensors)
        {
          g_print ("Sensor found: %s\n", *sensors);
          hyscan_sensor_set_enable (HYSCAN_SENSOR (global.control), *sensors, TRUE);
        }

      source = hyscan_control_sources_list (global.control, &n_sources);
      for (i = 0; i < n_sources; ++i)
        {
          const HyScanSonarInfoSource *info;

          info = hyscan_control_source_get_info (global.control, source[i]);
          if (info == NULL)
            continue;

          g_print ("Source found: %s\n", hyscan_source_get_id_by_type (source[i]));
          g_hash_table_insert (global.infos, GINT_TO_POINTER (source[i]), (void*)info);
        }
    }

  /***
   *     ___   ___   ___         ___   ___   ___         ___         ___         ___   ___   ___   ___
   *    |   | |   | |   |     | |     |       |         |   | |\  |   | |         |   |   | |   | |     |  /
   *    |-+-  |-+-  |   |     | |-+-  |       +         |-+-| | + |   + |         +   |-+-  |-+-| |     |-+
   *    |     |  \  |   | |   | |     |       |         |   | |  \|   | |         |   |  \  |   | |     |  \
   *                 ---   ---   ---   ---                           ---                           ---
   * проект, галс, настройки
   */

  /* SV. */
  global.sound_velocity = sound_velocity;
  global.ship_speed = ship_speed;

  /* cache*/
  if (cache_size == 0)
   cache_size = 2048;
  global.cache = HYSCAN_CACHE (hyscan_cached_new (cache_size));

  /* Проект пытаемся считать из файла. */
  project_name = keyfile_string_read_helper (global.settings, "common", "project");

  if (project_name == NULL)
    {
      GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      HyScanDBInfo *info = hyscan_db_info_new (global.db);
      GtkWidget *dialog;
      gint res;

      dialog = hyscan_fnn_project_new (global.db, info, GTK_WINDOW (window));
      res = gtk_dialog_run (GTK_DIALOG (dialog));
      hyscan_fnn_project_get (HYSCAN_FNN_PROJECT (dialog), &project_name, NULL);
      gtk_widget_destroy (dialog);
      gtk_widget_destroy (window);

      /* Если был нажат крестик - сорян, сваливаем отсюда. */
      if (res != HYSCAN_FNN_PROJECT_OPEN && res != HYSCAN_FNN_PROJECT_CREATE)
        goto exit;

      g_object_unref (info);
    }

  /* Монитор базы данных. */
  global.db_info = hyscan_db_info_new (global.db);
  g_signal_connect (global.db_info, "projects-changed", G_CALLBACK (projects_changed), &global);
  g_signal_connect (global.db_info, "tracks-changed", G_CALLBACK (tracks_changed), &global);

  /* Конфигурация. */
  global.sound_velocity = sound_velocity;
  global.full_screen = full_screen;
  global.project_name = g_strdup (project_name);

  // splash = hyscan_fnn_splash_new ();
  // hyscan_fnn_splash_start (splash, "Подключение");

  /***
   *           ___   ___   ___         ___         ___   ___   ___   ___   ___   ___   ___
   *    |   | |     |     |   |         |   |\  |   |   |     |   | |     |   | |     |
   *    |   |  -+-  |-+-  |-+-          +   | + |   +   |-+-  |-+-  |-+-  |-+-| |     |-+-
   *    |   |     | |     |  \          |   |  \|   |   |     |  \  |     |   | |     |
   *     ---   ---   ---               ---               ---                     ---   ---
   *
   */

  common_builder = gtk_builder_new ();
  gtk_builder_set_translation_domain (common_builder, GETTEXT_PACKAGE);
  gtk_builder_add_from_resource (common_builder, "/org/triple/gtk/common.ui", NULL);

  /* Основное окно программы. */
  global.gui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_after (G_OBJECT (global.gui.window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (global.gui.window), "key-press-event", G_CALLBACK (key_press), &global);
  set_window_title (&global);
  gtk_window_set_default_size (GTK_WINDOW (global.gui.window), 1600, 900);

  /* Навигационные данные. */
  if (global.control != NULL)
    global.gui.nav = hyscan_gtk_nav_indicator_new (HYSCAN_SENSOR (global.control));

  /* Галсы. */
  global.gui.track.view = GTK_WIDGET (gtk_builder_get_object (common_builder, "track.view"));
  global.gui.track.tree = GTK_TREE_VIEW (gtk_builder_get_object (common_builder, "track.tree"));
  global.gui.track.list = GTK_TREE_MODEL (gtk_builder_get_object (common_builder, "track.list"));
  global.gui.track.scroll = GTK_ADJUSTMENT (gtk_builder_get_object (common_builder, "track.scroll"));
  hyscan_exit_if ((global.gui.track.tree == NULL) ||
                  (global.gui.track.list == NULL) ||
                  (global.gui.track.scroll == NULL),
                  "incorrect track control ui");
  gtk_builder_add_callback_symbol (common_builder, "track_scroll", G_CALLBACK (track_scroll));
  gtk_builder_add_callback_symbol (common_builder, "track_changed", G_CALLBACK (track_changed));

  /* Сортировка списка галсов. */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (global.gui.track.list), 0, GTK_SORT_DESCENDING);

  /* Список меток. */
  global.marks.model = hyscan_mark_model_new (HYSCAN_MARK_WATERFALL);
  // TODO: перепроверить, что в ран-менеджере, что в трек-чейнджед, что здесь
  global.marks.loc_storage = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                    (GDestroyNotify) loc_store_free);
  global.gui.mark_view = hyscan_gtk_project_viewer_new ();
  global.gui.meditor = hyscan_gtk_mark_editor_new ();

  hyscan_mark_model_set_project (global.marks.model, global.db, global.project_name);
  g_signal_connect (global.marks.model, "changed", G_CALLBACK (mark_model_changed), &global);
  g_signal_connect (global.gui.meditor, "mark-modified", G_CALLBACK (mark_modified), &global);
  g_signal_connect (global.gui.mark_view, "item-changed", G_CALLBACK (active_mark_changed), &global);

  gtk_builder_connect_signals (common_builder, &global);

  /***
   *  ___   ___         ___         ___
   * |   | |   | |\  | |     |     |
   * |-+-  |-+-| | + | |-+-  |      -+-
   * |     |   | |  \| |___  |___   ___|
   *
   * Создаем панели.
   */
  global.panels = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, fnn_panel_destroy);

  hardware = g_key_file_new ();

  ui_config (config);
  // Вызываем постройку билдера!
  ui_build (&global);
  g_key_file_unref (hardware);

  ui_setting (global.settings);

  if (full_screen)
    gtk_window_fullscreen (GTK_WINDOW (global.gui.window));

  g_signal_connect (global.gui.window, "destroy", G_CALLBACK (destroy_cb), NULL);
  gtk_widget_show_all (global.gui.window);
  update_panels (&global, NULL);
  gtk_main ();

  if (sensor_label_writer_tag > 0)
    g_source_remove (sensor_label_writer_tag);
  // hyscan_fnn_splash_start (splash, "Отключение");

  if (global.control != NULL)
    hyscan_sonar_stop (global.control_s);

  /***
   *     ___         ___   ___               ___
   *    |     |     |     |   | |\  | |   | |   |
   *    |     |     |-+-  |-+-| | + | |   | |-+-
   *    |     |     |     |   | |  \| |   | |
   *     ---   ---   ---               ---
   *
   */
  // ui_desetup (global.settings);
  ui_destroy (&global);


exit:

  fnn_deinit (&global);
  if (global.settings != NULL)
    {
      keyfile_string_write_helper(global.settings, "common", "version", "2019.01");
      g_key_file_save_to_file (global.settings, settings_file, NULL);
    }

  #ifdef FNN_LOGGING
  hyscan_fnn_flog_close ();
  #endif

  g_clear_pointer (&config, g_key_file_unref);

  g_clear_pointer (&ui_module, g_module_close);

  g_clear_object (&common_builder);

  g_clear_pointer (&global.settings, g_key_file_unref);
  g_clear_object (&global.cache);
  g_clear_object (&global.db_info);
  g_clear_object (&global.db);

  g_clear_pointer (&global.panels, g_hash_table_unref);

  g_free (global.track_name);
  g_free (db_uri);
  g_free (project_name);
  g_free (config_file);
  g_free (settings_file);

  g_option_context_free (context);


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
