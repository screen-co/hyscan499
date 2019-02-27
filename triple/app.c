#include <triple-types.h>
#include <hyscan-ame-splash.h>
#include <hyscan-ame-project.h>
#include <hyscan-ame-button.h>
#include <hyscan-hw-connector.h>
#include <gmodule.h>

#ifndef G_OS_WIN32 /* Ловим сигналы ОС. */
 #include <signal.h>
 #include <errno.h>
#endif

/* Вот он, наш жирненький красавчик. */
Global global = {0,};

typedef gboolean (*ame_build) (Global *);
typedef gboolean (*ame_config) (GKeyFile *);

/* Обработчик сигналов TERM и INT. */
void shutdown_handler (gint signum)
{
  g_message ("term");
  gtk_main_quit ();
}

GArray *
make_svp_from_velocity (gdouble velocity)
{
  GArray *svp;
  HyScanSoundVelocity svp_val;

  svp = g_array_new (FALSE, FALSE, sizeof (HyScanSoundVelocity));
  svp_val.depth = 0.0;
  svp_val.velocity = global.sound_velocity;
  g_array_insert_val (svp, 0, svp_val);

  return svp;
}

GArray *
make_color_maps (gboolean profiler)
{
  guint32 kolors[2];
  GArray * colormaps;
  AmeColormap *new_map;
  #include "colormaps.h"

  colormaps = g_array_new (FALSE, TRUE, sizeof (AmeColormap*));
  g_array_set_clear_func (colormaps, ame_colormap_free);

  if (profiler)
    {
      new_map = g_new (AmeColormap, 1);
      new_map->name = g_strdup ("Причудливый Профилограф");
      new_map->colors = hyscan_tile_color_compose_colormap_pf (&new_map->len);
      new_map->len = 256;
      new_map->bg = WHITE_BG;
      g_array_append_vals (colormaps, &new_map, 1);

      return colormaps;
    }

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Желтый");
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 0.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Сепия");
  new_map->colors = g_memdup (sepia, 256 * sizeof (guint32));
  new_map->len = 256;
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Белый");
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Инверсия");
  kolors[0] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);new_map = g_new (AmeColormap, 1);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Зеленый");
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (0.2, 1.0, 0.2, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  return colormaps;
}

int
main (int argc, char **argv)
{
  gint               cache_size = 2048;        /* Размер кэша по умолчанию. */

  gchar             *db_uri = NULL;            /* Адрес базы данных. */
  gchar             *project_name = NULL;      /* Название проекта. */
  gdouble            sound_velocity = 1500.0;  /* Скорость звука по умолчанию. */
  GArray            *svp;                      /* Профиль скорости звука. */
  gdouble            ship_speed = 1.8;         /* Скорость движения судна. */
  gboolean           full_screen = FALSE;      /* Признак полноэкранного режима. */

  gchar             *config_file = NULL;       /* Название файла конфигурации. */
  GKeyFile          *config = NULL;
  GKeyFile          *hardware = NULL;

  gchar             *um_path = "hardware.ui";       /* Модуль с интерфейсом. */
  GModule           *ui_module = NULL;
  ame_build          ui_build = NULL;
  ame_build          ui_destroy = NULL;
  ame_config         ui_config = NULL;

  gboolean           need_ss = FALSE;
  gboolean           need_pf = FALSE;
  gboolean           need_es = FALSE;
  gboolean           need_fl = FALSE;

  GtkBuilder        *common_builder = NULL;

  // HyScanAmeSplash   *splash = NULL;
  guint              sensor_label_writer_tag = 0;
  gchar             *hardware_profile_name = NULL;

  gboolean status;

  gtk_init (&argc, &argv);
  init_triple (&global);

  /* Разбор командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionContext *context;

    GOptionEntry common_entries[] =
      {
        { "db-uri",          'd',   0, G_OPTION_ARG_STRING,  &db_uri,          "HyScan DB uri", NULL },

        { "sound-velocity",  'v',   0, G_OPTION_ARG_DOUBLE,  &sound_velocity,  "Sound velocity, m/s", NULL },
        { "ship-speed",      'e',   0, G_OPTION_ARG_DOUBLE,  &ship_speed,      "Ship speed, m/s", NULL },
        { "full-screen",     'f',   0, G_OPTION_ARG_NONE,    &full_screen,     "Full screen mode", NULL },

        { "ss",              0,     0, G_OPTION_ARG_NONE,    &need_ss,         "Enable ss panel", NULL },
        { "pf",              0,     0, G_OPTION_ARG_NONE,    &need_pf,         "Enable pf panel", NULL },
        { "fl",              0,     0, G_OPTION_ARG_NONE,    &need_fl,         "Enable fl panel", NULL },
        { "es",              0,     0, G_OPTION_ARG_NONE,    &need_es,         "Enable es panel", NULL },

        { "hardware-profile",'p',   0, G_OPTION_ARG_STRING,  &hardware_profile_name,     "Specify hardware profile name"
                                                              ", otherwise it will be obtained from config-file.", NULL },
        { "ui", 0,   G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &um_path,    "UI module", NULL },
        { NULL, }
      };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    context = g_option_context_new ("[config-file]");
    g_option_context_set_help_enabled (context, TRUE);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    g_option_context_add_main_entries (context, common_entries, NULL);
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

    if (args[1] == NULL)
      {
        g_print ("%s", g_option_context_get_help (context, FALSE, NULL));
        return -1;
      }
    config_file = g_strdup (args[1]);

    g_option_context_free (context);
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

    module_path = g_build_filename (".", um_path, NULL);
    ui_module = g_module_open (module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
    g_free (module_path);

    if (ui_module != NULL)
      {
        g_module_symbol (ui_module, "build_interface", (gpointer *) &ui_build);
        g_module_symbol (ui_module, "destroy_interface", (gpointer *) &ui_destroy);
        g_module_symbol (ui_module, "kf_config", (gpointer *) &ui_config);
      }

    /* Если нет этих двух функций, то всё, суши вёсла. */
    if ((ui_build == NULL) || (ui_destroy == NULL))
      {
        g_warning ("failed to load UI builer (%p; %p)", ui_build, ui_destroy);
        goto exit;
      }
  }

  /* Открываем конфиг файл. */
  if (config_file != NULL)
    {
      config = g_key_file_new ();
      status = g_key_file_load_from_file (config, config_file,
                                          G_KEY_FILE_NONE, NULL);

      if (!status)
        g_message ("failed to load config file <%s>", config_file);
    }

  /* SV. */
  global.sound_velocity = sound_velocity;
  svp = make_svp_from_velocity (sound_velocity);

  /* Кэш. */
  cache_size = keyfile_uint_read_helper (config, "common", "cache", 2048);
  cache_size = MAX (256, cache_size);
  global.cache = HYSCAN_CACHE (hyscan_cached_new (cache_size));

  /* Подключение к базе данных. */
  global.db = hyscan_db_new (db_uri);
  hyscan_exit_if_w_param (global.db == NULL, "can't connect to db '%s'", db_uri);

  /* Проект пытаемся считать из файла. */
  project_name = keyfile_string_read_helper (config, "common", "project");

  if (project_name == NULL)
    {
      GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      HyScanDBInfo *info = hyscan_db_info_new (global.db);
      GtkWidget *dialog;
      gint res;

      dialog = hyscan_ame_project_new (global.db, info, GTK_WINDOW (window));
      res = gtk_dialog_run (GTK_DIALOG (dialog));
      hyscan_ame_project_get (HYSCAN_AME_PROJECT (dialog), &project_name, NULL);
      gtk_widget_destroy (dialog);
      gtk_widget_destroy (window);

      /* Если был нажат крестик - сорян, сваливаем отсюда. */
      if (res != HYSCAN_AME_PROJECT_OPEN && res != HYSCAN_AME_PROJECT_CREATE)
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

  // splash = hyscan_ame_splash_new ();
  // hyscan_ame_splash_start (splash, "Подключение");

  /***
   *     ___   ___         ___   ___         ___   ___               ___   ___   ___   ___   ___
   *    |     |   | |\  | |   | |   |       |     |   | |\  | |\  | |     |       |     |   |   | |\  |
   *     -+-  |   | | + | |-+-| |-+-        |     |   | | + | | + | |-+-  |       +     +   |   | | + |
   *        | |   | |  \| |   | |  \        |     |   | |  \| |  \| |     |       |     |   |   | |  \|
   *     ---   ---                           ---   ---               ---   ---         ---   ---
   *
   * Подключение к гидролокатору.
   */

  /* Пути к дровам и имя файла с профилем аппаратного обеспечения. */
  {
    gchar *sonar_profile_name;
    gchar **driver_paths;      /* Путь к драйверам гидролокатора. */
    HyScanHWConnector *connector;
    gboolean check;

    driver_paths = keyfile_strv_read_helper (config, "common", "paths");
    /* Если не задано название профиля, читаем его из конфига. */
    if (hardware_profile_name == NULL)
      sonar_profile_name = keyfile_string_read_helper (config, "common", "hardware");
    else
      sonar_profile_name = hardware_profile_name;


    /* Првоеряем, что пути к драйверам и имя профиля на месте. */
    connector = hyscan_hw_connector_new ();
    hyscan_hw_connector_set_driver_paths (connector, driver_paths);

    /* Читаем профиль. */
    hyscan_hw_connector_read (connector, sonar_profile_name);

    check = hyscan_hw_connector_check (connector);

    if (check)
      {
        global.control = hyscan_hw_connector_connect (connector);
        global.control_s = HYSCAN_SONAR (global.control);
      }

    g_strfreev (driver_paths);

    if (!check || global.control == NULL)
      goto no_sonar;
  }

  if (global.control != NULL)
    {
      const HyScanSonarInfoSource *info;

      hyscan_control_writer_set_db (global.control, global.db);

      global.infos = g_hash_table_new (g_direct_hash, g_direct_equal);

      if (NULL != (info = hyscan_control_source_get_info (global.control, FORWARDLOOK)))
        {g_message ("FORWARDLOOK");g_hash_table_insert (global.infos, GINT_TO_POINTER (FORWARDLOOK), (void*)info);}
      if (NULL != (info = hyscan_control_source_get_info (global.control, PROFILER)))
        {g_message ("PROFILER");g_hash_table_insert (global.infos, GINT_TO_POINTER (PROFILER), (void*)info);}
      if (NULL != (info = hyscan_control_source_get_info (global.control, ECHOSOUNDER)))
        {g_message ("ECHOSOUNDER");g_hash_table_insert (global.infos, GINT_TO_POINTER (ECHOSOUNDER), (void*)info);}
      if (NULL != (info = hyscan_control_source_get_info (global.control, HYSCAN_SOURCE_PROFILER_ECHO)))
        {g_message ("HYSCAN_SOURCE_PROFILER_ECHO");g_hash_table_insert (global.infos, GINT_TO_POINTER (HYSCAN_SOURCE_PROFILER_ECHO), (void*)info);}
      if (NULL != (info = hyscan_control_source_get_info (global.control, STARBOARD)))
        {g_message ("STARBOARD");g_hash_table_insert (global.infos, GINT_TO_POINTER (STARBOARD), (void*)info);}
      if (NULL != (info = hyscan_control_source_get_info (global.control, PORTSIDE)))
        {g_message ("PORTSIDE");g_hash_table_insert (global.infos, GINT_TO_POINTER (PORTSIDE), (void*)info);}
    }

  if (global.control != NULL)
    g_signal_connect (global.control, "sensor-data", G_CALLBACK (sensor_cb), &global);

  /* Закончили подключение к гидролокатору. */
  no_sonar:

  /***
   *           ___   ___   ___         ___         ___   ___   ___   ___   ___   ___   ___
   *    |   | |     |     |   |         |   |\  |   |   |     |   | |     |   | |     |
   *    |   |  -+-  |-+-  |-+-          +   | + |   +   |-+-  |-+-  |-+-  |-+-| |     |-+-
   *    |   |     | |     |  \          |   |  \|   |   |     |  \  |     |   | |     |
   *     ---   ---   ---               ---               ---                     ---   ---
   *
   */

  common_builder = gtk_builder_new_from_resource ("/org/triple/gtk/common.ui");

  /* Основное окно программы. */
  global.gui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_after (G_OBJECT (global.gui.window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (global.gui.window), "key-press-event", G_CALLBACK (key_press), &global);
  gtk_window_set_title (GTK_WINDOW (global.gui.window), "ГБОГКПФ");
  gtk_window_set_default_size (GTK_WINDOW (global.gui.window), 1600, 900);

  /* Навигационные данные. */
  global.gui.nav = hyscan_gtk_nav_indicator_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (global.gui.nav), GTK_ORIENTATION_HORIZONTAL);

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
  gtk_builder_add_callback_symbol (common_builder, "position_changed", G_CALLBACK (position_changed));

  /* Сортировка списка галсов. */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (global.gui.track.list), 0, GTK_SORT_DESCENDING);

  /* Список меток. */
  global.marks.sync = hyscan_mark_sync_new ("127.0.0.1", 10010);
  global.marks.model = hyscan_mark_model_new ();
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
  global.panels = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, ame_panel_destroy);

  hardware = g_key_file_new ();

  /* Проверяем флаги на панели */
  if (!need_ss && !need_pf && !need_fl && !need_es)
    {
      g_warning ("You must explicitly choose panels");
      g_warning ("Enabling: SS, PF, FL");
      need_ss = need_pf = need_fl = TRUE;
    }

  if (need_ss)
    { /* ГБО */
      GtkWidget *main_widget;
      AmePanel *panel = g_new0 (AmePanel, 1);
      VisualWF *vwf = g_new0 (VisualWF, 1);

      panel->name = g_strdup ("ГБО");
      panel->short_name = g_strdup ("SS");
      panel->type = AME_PANEL_WATERFALL;

      panel->sources = g_new0 (HyScanSourceType, 3);
      panel->sources[0] = HYSCAN_SOURCE_SIDE_SCAN_STARBOARD;
      panel->sources[1] = HYSCAN_SOURCE_SIDE_SCAN_PORT;
      panel->sources[2] = HYSCAN_SOURCE_INVALID;

      panel->vis_gui = (VisualCommon*)vwf;

      vwf->colormaps = make_color_maps (FALSE);

      vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global.cache));
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

      main_widget = make_overlay (vwf->wf,
                                  &vwf->wf_grid, &vwf->wf_ctrl,
                                  &vwf->wf_mark, &vwf->wf_metr,
                                  global.marks.model);

      g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (automove_switched), &global);
      g_signal_connect (vwf->wf, "waterfall-zoom", G_CALLBACK (zoom_changed), GINT_TO_POINTER (X_SIDESCAN));

      hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), ship_speed);
      hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
      hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
      hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

      vwf->common.main = main_widget;
      g_hash_table_insert (global.panels, GINT_TO_POINTER (X_SIDESCAN), panel);
    }

  if (need_pf)
    { /* ПФ */
      GtkWidget *main_widget;
      AmePanel *panel = g_new0 (AmePanel, 1);
      VisualWF *vwf = g_new0 (VisualWF, 1);

      panel->name = g_strdup ("Профилограф");
      panel->short_name = g_strdup ("PF");
      panel->type = AME_PANEL_ECHO;

      panel->sources = g_new0 (HyScanSourceType, 3);
      panel->sources[0] = HYSCAN_SOURCE_PROFILER;
      panel->sources[1] = HYSCAN_SOURCE_PROFILER_ECHO;
      panel->sources[2] = HYSCAN_SOURCE_INVALID;

      panel->vis_gui = (VisualCommon*)vwf;

      vwf->colormaps = make_color_maps (TRUE);

      vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global.cache));
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

      main_widget = make_overlay (vwf->wf,
                                  &vwf->wf_grid, &vwf->wf_ctrl,
                                  &vwf->wf_mark, &vwf->wf_metr,
                                  global.marks.model);

      hyscan_gtk_waterfall_grid_set_condence (vwf->wf_grid, 10.0);
      hyscan_gtk_waterfall_grid_set_grid_color (vwf->wf_grid, hyscan_tile_color_converter_c2i (32, 32, 32, 255));

      g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (automove_switched), &global);
      g_signal_connect (vwf->wf, "waterfall-zoom", G_CALLBACK (zoom_changed), GINT_TO_POINTER (X_PROFILER));

      hyscan_gtk_waterfall_state_echosounder (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), PROFILER);
      hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), ship_speed / 10);
      hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
      hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
      hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

      vwf->common.main = main_widget;
      g_hash_table_insert (global.panels, GINT_TO_POINTER (X_PROFILER), panel);
    }

  if (need_es)
    { /* Эхолот */
      GtkWidget *main_widget;
      AmePanel *panel = g_new0 (AmePanel, 1);
      VisualWF *vwf = g_new0 (VisualWF, 1);

      panel->name = g_strdup ("Эхолот");
      panel->short_name = g_strdup ("ES");
      panel->type = AME_PANEL_ECHO;

      panel->sources = g_new0 (HyScanSourceType, 2);
      panel->sources[0] = HYSCAN_SOURCE_ECHOSOUNDER;
      panel->sources[1] = HYSCAN_SOURCE_INVALID;

      panel->vis_gui = (VisualCommon*)vwf;

      vwf->colormaps = make_color_maps (TRUE);

      vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global.cache));
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

      main_widget = make_overlay (vwf->wf,
                                  &vwf->wf_grid, &vwf->wf_ctrl,
                                  &vwf->wf_mark, &vwf->wf_metr,
                                  global.marks.model);

      hyscan_gtk_waterfall_grid_set_condence (vwf->wf_grid, 10.0);
      hyscan_gtk_waterfall_grid_set_grid_color (vwf->wf_grid, hyscan_tile_color_converter_c2i (32, 32, 32, 255));

      g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (automove_switched), &global);
      g_signal_connect (vwf->wf, "waterfall-zoom", G_CALLBACK (zoom_changed), GINT_TO_POINTER (X_ECHOSOUND));

      hyscan_gtk_waterfall_state_echosounder (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), ECHOSOUNDER);
      hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), ship_speed / 10);
      hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
      hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
      hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

      vwf->common.main = main_widget;
      g_hash_table_insert (global.panels, GINT_TO_POINTER (X_ECHOSOUND), panel);
    }

  if (need_fl)
    { /* ВСЛ */
      AmePanel *panel = g_new0 (AmePanel, 1);
      VisualFL *vfl = g_new0 (VisualFL, 1);

      panel->name = g_strdup ("Курсовой");
      panel->short_name = g_strdup ("FL");
      panel->type = AME_PANEL_FORWARDLOOK;

      panel->sources = g_new0 (HyScanSourceType, 2);
      panel->sources[0] = HYSCAN_SOURCE_FORWARD_LOOK;
      panel->sources[1] = HYSCAN_SOURCE_INVALID;

      panel->vis_gui = (VisualCommon*)vfl;

      vfl->fl = HYSCAN_GTK_FORWARD_LOOK (hyscan_gtk_forward_look_new ());
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vfl->fl), TRUE);

      vfl->player = hyscan_gtk_forward_look_get_player (vfl->fl);
      vfl->coords = hyscan_fl_coords_new (vfl->fl, global.cache);

      /* Управление воспроизведением FL. */
      vfl->play_control = GTK_WIDGET (gtk_builder_get_object (common_builder, "fl_play_control"));
      hyscan_exit_if (vfl->play_control == NULL, "can't load play control ui");

      vfl->position = GTK_SCALE (gtk_builder_get_object (common_builder, "position"));
      vfl->coords_label = GTK_LABEL (gtk_builder_get_object (common_builder, "fl_latlong"));
      g_signal_connect (vfl->coords, "coords", G_CALLBACK (fl_coords_callback), vfl->coords_label);
      hyscan_exit_if (vfl->position == NULL, "incorrect play control ui");
      vfl->position_range = hyscan_gtk_forward_look_get_adjustment (vfl->fl);
      gtk_range_set_adjustment (GTK_RANGE (vfl->position), vfl->position_range);

      hyscan_forward_look_player_set_sv (vfl->player, global.sound_velocity);

      vfl->common.main = GTK_WIDGET (vfl->fl);
      g_hash_table_insert (global.panels, GINT_TO_POINTER (X_FORWARDL), panel);
    }
  g_key_file_unref (hardware);


  // Вызываем постройку билдера!
  ui_build (&global);
  ui_config (config);

/***
 *     ___   ___   ___   ___               ___               ___               ___   ___
 *      | | |     |     |   | |   | |       |         |  /  |   | |     |   | |     |
 *      + | |-+-  |-+-  |-+-| |   | |       +         | +   |-+-| |     |   | |-+-   -+-
 *      | | |     |     |   | |   | |       |         |/    |   | |     |   | |         |
 *     ---   ---               ---   ---                           ---   ---   ---   ---
 *
 * Начальные значения.
 */
  {
    GHashTableIter iter;
    gpointer k, v;
    g_hash_table_iter_init (&iter, global.panels);
    while (g_hash_table_iter_next (&iter, &k, &v))
      {
        gint panelx = GPOINTER_TO_INT (k);
        AmePanel *panel = v;
        panel->current.distance =    keyfile_double_read_helper (config, panel->name, "sonar.cur_distance", 50);
        panel->current.signal =      keyfile_double_read_helper (config, panel->name, "sonar.cur_signal", 1);
        panel->current.gain0 =       keyfile_double_read_helper (config, panel->name, "sonar.cur_gain0", 0);
        panel->current.gain_step =   keyfile_double_read_helper (config, panel->name, "sonar.cur_gain_step", 10);
        panel->current.level =       keyfile_double_read_helper (config, panel->name, "sonar.cur_level", 0.5);
        panel->current.sensitivity = keyfile_double_read_helper (config, panel->name, "sonar.cur_sensitivity", 0.6);

        panel->vis_current.brightness =  keyfile_double_read_helper (config, panel->name, "cur_brightness",   80.0);
        panel->vis_current.colormap =    keyfile_double_read_helper (config, panel->name, "cur_color_map",    0);
        panel->vis_current.black =       keyfile_double_read_helper (config, panel->name, "cur_black",        0);
        panel->vis_current.sensitivity = keyfile_double_read_helper (config, panel->name, "cur_sensitivity",  8.0);

        if (panel->type == AME_PANEL_WATERFALL)
          color_map_set (&global, panel->vis_current.colormap, panelx);
        else if (panel->type == AME_PANEL_ECHO)
          color_map_set (&global, panel->vis_current.colormap, panelx);
        else if (panel->type == AME_PANEL_FORWARDLOOK)
          sensitivity_set (&global, panel->vis_current.sensitivity, panelx);

        brightness_set (&global, panel->vis_current.brightness, panel->vis_current.black, panelx);
        scale_set (&global, FALSE, panelx);

        /* Если нет локатора, нечего и задавать. */
        if (global.control == NULL)
          continue;

        /* Для локаторов мы ничего не задаем, но лейблы всёрно надо проинициализировать. */
        distance_label (panel, panel->current.distance);
        tvg_label (panel, panel->current.gain0, panel->current.gain_step);
        auto_tvg_label (panel, panel->current.level, panel->current.sensitivity);

        /* С сигналом чуть сложней, т.к. надо найти сигнал и вытащить из него имя. */
        {
          const HyScanDataSchemaEnumValue * signal;
          signal = signal_finder (&global, panel, *panel->sources, panel->current.signal);
          if (signal == NULL)
            {
              panel->current.signal = 0;
              signal = signal_finder (&global, panel, *panel->sources, panel->current.signal);
            }
          if (signal != NULL)
            signal_label (panel, signal->name);
        }
      }
  }

  if (full_screen)
    gtk_window_fullscreen (GTK_WINDOW (global.gui.window));

  gtk_widget_show_all (global.gui.window);
  gtk_main ();

  if (sensor_label_writer_tag > 0)
    g_source_remove (sensor_label_writer_tag);
  // hyscan_ame_splash_start (splash, "Отключение");

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
  ui_destroy (&global);

  if (config != NULL)
    {
      GHashTableIter iter;
      gpointer k, v;
      g_hash_table_iter_init (&iter, global.panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        {
          AmePanel *panel = v;
          keyfile_double_write_helper (config, panel->name, "sonar.cur_distance", panel->current.distance);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_signal", panel->current.signal);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_gain0", panel->current.gain0);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_gain_step", panel->current.gain_step);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_level", panel->current.level);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_sensitivity", panel->current.sensitivity);

          keyfile_double_write_helper (config, panel->name, "cur_brightness",          panel->vis_current.brightness);
          keyfile_double_write_helper (config, panel->name, "cur_color_map",           panel->vis_current.colormap);
          keyfile_double_write_helper (config, panel->name, "cur_black",               panel->vis_current.black);
          keyfile_double_write_helper (config, panel->name, "cur_sensitivity",         panel->vis_current.sensitivity);
        }

      keyfile_string_write_helper (config, "common",  "project", global.project_name);

      g_key_file_save_to_file (config, config_file, NULL);
      g_key_file_unref (config);
    }

exit:

  g_clear_pointer (&ui_module, g_module_close);

  g_clear_object (&common_builder);

  g_clear_object (&global.cache);
  g_clear_object (&global.db_info);
  g_clear_object (&global.db);

  g_hash_table_unref (global.panels);

  g_free (global.track_name);
  g_free (db_uri);
  g_free (project_name);
  g_free (config_file);

  // g_clear_object (&splash);

  return 0;
}
