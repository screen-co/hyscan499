/* hyscan-gtk-navigator-app.c
 *
 * Copyright 2020 Screen LLC, Alexey Sakhnov <alexsakhnov@gmail.com>
 *
 * This file is part of HyScanFnn library.
 *
 * HyScanFnn is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HyScanFnn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Alternatively, you can license this code under a commercial license.
 * Contact the Screen LLC in this case - <info@screen-co.ru>.
 */

/* HyScanFnn имеет двойную лицензию.
 *
 * Во-первых, вы можете распространять HyScanFnn на условиях Стандартной
 * Общественной Лицензии GNU версии 3, либо по любой более поздней версии
 * лицензии (по вашему выбору). Полные положения лицензии GNU приведены в
 * <http://www.gnu.org/licenses/>.
 *
 * Во-вторых, этот программный код можно использовать по коммерческой
 * лицензии. Для этого свяжитесь с ООО Экран - <info@screen-co.ru>.
 */

/**
 * SECTION: hyscan-gtk-navigator-app
 * @Short_description: Приложение для судоводителя.
 * @Title: HyScanGtkNavigatorApp
 *
 * GUI-приложение #HyScanGtkNavigatorApp предназначено для судоводителя, работающего
 * в паре с оператором за отдельным планшетом (компьтером).
 *
 * Работа на разных устройствах позволяет оператору и судоводителю работать на разных
 * рабочих местах и просматривать в приложении различные виджеты, не мешаю друг другу.
 *
 * Приложение судоводителя скомпановано для удобной работы с картой и планами галсов,
 * в нём отсуствуют вкладки акустических изображений и возможность управления локатором.
 *
 * Предполагает подключение судоводителя к базе данных оператора по tcp и получение
 * данных навигации с датчика GNSS оператора через udp-ретранслятор.
 *
 */

#include "hyscan-gtk-navigator-app.h"
#include "hyscan-gtk-map-go.h"
#include "hyscan-gtk-map-preload.h"
#include <hyscan-units.h>
#include <hyscan-cached.h>
#include <hyscan-gtk-map-builder.h>
#include <hyscan-gtk-area.h>
#include <hyscan-control-model.h>
#include <hyscan-config.h>
#include <glib/gi18n-lib.h>
#include <hyscan-gtk-con.h>
#include <hyscan-fnn-project.h>
#include <hyscan-gtk-dev-indicator.h>

#define SETTING_FILENAME "navigator.ini"      /* Название файла конфигурации. */
#define CACHE_SIZE       255                  /* Размер кэша, Мб. */

enum
{
  PROP_O,
  PROP_DB_URI,
  PROP_DRIVER_PATHS,
};

struct _HyScanGtkNavigatorAppPrivate
{
  gchar                    *db_uri;           /* URI базы данных. */
  gchar                   **driver_paths;     /* Пути к директориям с драйверами. */
  gchar                    *settings_file;    /* Путь к файлу с настройками. */
  GKeyFile                 *settings;         /* GKeyFile настроек. */
  HyScanControl            *control;          /* Объект управления оборудованием. */
  HyScanControlModel       *control_model;    /* Асинхронная модель оборудования. */
  HyScanGtkModelManager    *model_manager;    /* Объект для доступа к моделям. */
  HyScanGtkMapBuilder      *builder;          /* Объект для создания карты. */
  GtkWidget                *window;           /* Главное окно программы. */
};

static void        hyscan_gtk_navigator_app_set_property             (GObject               *object,
                                                                      guint                  prop_id,
                                                                      const GValue          *value,
                                                                      GParamSpec            *pspec);
static void        hyscan_gtk_navigator_app_object_constructed       (GObject               *object);
static void        hyscan_gtk_navigator_app_object_finalize          (GObject               *object);
static void        hyscan_gtk_navigator_app_activate                 (GApplication          *application);
static void        hyscan_gtk_navigator_app_shutdown                 (GApplication          *application);
static void        hyscan_gtk_navigator_app_connector_window         (HyScanGtkNavigatorApp *navigator_app);
static void        hyscan_gtk_navigator_app_connector_close          (GtkAssistant          *assistant,
                                                                      HyScanGtkNavigatorApp *navigator_app);
static void        hyscan_gtk_navigator_app_main_window              (HyScanGtkNavigatorApp *navigator_app);
static GtkWidget * hyscan_gtk_navigator_app_compose_main_window      (HyScanGtkNavigatorApp *navigator_app);
static void        hyscan_gtk_navigator_app_add_actions              (HyScanGtkNavigatorApp *navigator_app);
static void        hyscan_gtk_navigator_app_builder_save             (HyScanGtkNavigatorApp *navigator_app);
static void        hyscan_gtk_navigator_app_fullscreen_action        (GSimpleAction         *action,
                                                                      GVariant              *state,
                                                                      gpointer               user_data);
static void        hyscan_gtk_navigator_app_activate_toggle          (GSimpleAction         *action,
                                                                      GVariant              *parameter,
                                                                      gpointer               user_data);
static void        hyscan_gtk_navigator_app_about_activated          (GSimpleAction         *action,
                                                                      GVariant              *parameter,
                                                                      gpointer               user_data);
static void        hyscan_gtk_navigator_app_quit_activated           (GSimpleAction         *action,
                                                                      GVariant              *parameter,
                                                                      gpointer               user_data);
static void        hyscan_gtk_navigator_app_open_activated           (GSimpleAction         *action,
                                                                      GVariant              *parameter,
                                                                      gpointer               user_data);
static void        hyscan_gtk_navigator_app_project_changed          (HyScanGtkNavigatorApp *navigator_app);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkNavigatorApp, hyscan_gtk_navigator_app, GTK_TYPE_APPLICATION)

static void
hyscan_gtk_navigator_app_class_init (HyScanGtkNavigatorAppClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  application_class->shutdown = hyscan_gtk_navigator_app_shutdown;
  application_class->activate = hyscan_gtk_navigator_app_activate;


  object_class->set_property = hyscan_gtk_navigator_app_set_property;

  object_class->constructed = hyscan_gtk_navigator_app_object_constructed;
  object_class->finalize = hyscan_gtk_navigator_app_object_finalize;

  g_object_class_install_property (object_class, PROP_DB_URI,
    g_param_spec_string ("db-uri", "Db URI", "Database URI", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_DRIVER_PATHS,
    g_param_spec_boxed ("driver-paths", "Drivers Paths", "String array of paths where to search for drivers", G_TYPE_STRV,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_navigator_app_init (HyScanGtkNavigatorApp *gtk_navigator_app)
{
  gtk_navigator_app->priv = hyscan_gtk_navigator_app_get_instance_private (gtk_navigator_app);
}

static void
hyscan_gtk_navigator_app_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  HyScanGtkNavigatorApp *gtk_navigator_app = HYSCAN_GTK_NAVIGATOR_APP (object);
  HyScanGtkNavigatorAppPrivate *priv = gtk_navigator_app->priv;

  switch (prop_id)
    {
    case PROP_DB_URI:
      priv->db_uri = g_value_dup_string (value);
      break;

    case PROP_DRIVER_PATHS:
      priv->driver_paths = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_navigator_app_object_constructed (GObject *object)
{
  HyScanGtkNavigatorApp *navigator_app = HYSCAN_GTK_NAVIGATOR_APP (object);
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  HyScanDB *db;
  HyScanCache *cache;

  G_OBJECT_CLASS (hyscan_gtk_navigator_app_parent_class)->constructed (object);

  db = hyscan_db_new (priv->db_uri);
  if (db == NULL)
    return;

  cache = HYSCAN_CACHE (hyscan_cached_new (CACHE_SIZE));
  priv->model_manager = hyscan_gtk_model_manager_new (NULL, db, cache, NULL);
  priv->settings_file = g_build_filename (hyscan_config_get_user_files_dir (), SETTING_FILENAME, NULL);
  priv->settings = g_key_file_new ();
  g_key_file_load_from_file (priv->settings, priv->settings_file, G_KEY_FILE_NONE, NULL);

  g_signal_connect_swapped (priv->model_manager, "notify::project-name",
                            G_CALLBACK (hyscan_gtk_navigator_app_project_changed),
                            navigator_app);

  g_object_unref (db);
  g_object_unref (cache);
}

static void
hyscan_gtk_navigator_app_object_finalize (GObject *object)
{
  HyScanGtkNavigatorApp *gtk_navigator_app = HYSCAN_GTK_NAVIGATOR_APP (object);
  HyScanGtkNavigatorAppPrivate *priv = gtk_navigator_app->priv;

  g_clear_object (&priv->model_manager);
  g_clear_object (&priv->control_model);
  g_clear_object (&priv->control);
  g_clear_object (&priv->builder);
  g_clear_pointer (&priv->settings, g_key_file_unref);
  g_clear_pointer (&priv->driver_paths, g_strfreev);
  g_free (priv->db_uri);
  g_free (priv->settings_file);

  G_OBJECT_CLASS (hyscan_gtk_navigator_app_parent_class)->finalize (object);
}

/* Обработчик изменения названия проекта. */
static void
hyscan_gtk_navigator_app_project_changed (HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  const gchar *project;

  project = hyscan_gtk_model_manager_get_project_name (priv->model_manager);
  if (priv->window != NULL)
    {
      gchar *title;

      title = g_strdup_printf (_("Navigator - %s"), project);
      gtk_window_set_title (GTK_WINDOW (priv->window), title);

      g_free (title);
    }
}

/* Действие "win.fullscreen" - "Развернуть на весь экран". */
static void
hyscan_gtk_navigator_app_fullscreen_action (GSimpleAction *action,
                                            GVariant      *state,
                                            gpointer       user_data)
{
  if (g_variant_get_boolean (state))
    gtk_window_fullscreen (user_data);
  else
    gtk_window_unfullscreen (user_data);

  g_simple_action_set_state (action, state);
}

/* Изменяет состояние пунктов меню boolean на обратное. */
static void
hyscan_gtk_navigator_app_activate_toggle (GSimpleAction *action,
                                          GVariant      *parameter,
                                          gpointer       user_data)
{
  GVariant *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

/* Действие "app.about" - "О программе". */
static void
hyscan_gtk_navigator_app_about_activated (GSimpleAction *action,
                                          GVariant      *parameter,
                                          gpointer       user_data)
{
  GtkApplication *application = GTK_APPLICATION (user_data);
  gtk_show_about_dialog (gtk_application_get_active_window (application),
                         "program-name", _("Navigator"),
                         "title", _("About Navigator"),
                         "comments", _("Navigator application for HyScan 5"),
                         NULL);

}

/* Действие "app.open" - "Открыть проект...". */
static void
hyscan_gtk_navigator_app_open_activated (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{                                                                  
  HyScanGtkNavigatorApp *navigator_app = HYSCAN_GTK_NAVIGATOR_APP (user_data);
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  HyScanDB *db;
  HyScanDBInfo *db_info;
  gchar *project_name = NULL;
  GtkWidget *dialog;

  db = hyscan_gtk_model_manager_get_db (priv->model_manager);
  db_info = hyscan_gtk_model_manager_get_track_model (priv->model_manager);

  dialog = hyscan_fnn_project_new (db, db_info, GTK_WINDOW (priv->window));
  gtk_dialog_run (GTK_DIALOG (dialog));
  hyscan_fnn_project_get (HYSCAN_FNN_PROJECT (dialog), &project_name, NULL);
  gtk_widget_destroy (dialog);
  
  if (project_name != NULL)
    hyscan_gtk_model_manager_set_project_name (priv->model_manager, project_name);

  g_free (project_name);
  g_object_unref (db);
  g_object_unref (db_info);
}

/* Действие "app.exit" - "Выход". */
static void
hyscan_gtk_navigator_app_quit_activated (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
  GApplication *app = G_APPLICATION (user_data);
  HyScanGtkNavigatorAppPrivate *priv = HYSCAN_GTK_NAVIGATOR_APP (app)->priv;

  if (priv->window != NULL)
    gtk_widget_destroy (priv->window);
}

/* Записывает параметры карты в GKeyFile. */
static void
hyscan_gtk_navigator_app_builder_save (HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;

  hyscan_gtk_map_builder_file_write (priv->builder, priv->settings);
}

/* Регистрирует доступные действия GAction. */
static void
hyscan_gtk_navigator_app_add_actions (HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  GPropertyAction *action;

  /* Действия окна "win.*". */
  GActionEntry win_entries[] =
  {
    { "fullscreen", hyscan_gtk_navigator_app_activate_toggle, NULL, "false", hyscan_gtk_navigator_app_fullscreen_action },
  };

  /* Действия приложения "app.*". */
  GActionEntry app_entries[] =
  {
    { "open", hyscan_gtk_navigator_app_open_activated, NULL, NULL, NULL },
    { "about", hyscan_gtk_navigator_app_about_activated, NULL, NULL, NULL },
    { "exit", hyscan_gtk_navigator_app_quit_activated, NULL, NULL, NULL },
  };

  /* Действия приложения "app.*". */
  action = g_property_action_new ("offline", priv->builder, "offline");
  g_action_map_add_action (G_ACTION_MAP (navigator_app), G_ACTION (action));
  g_action_map_add_action_entries (G_ACTION_MAP (navigator_app), app_entries, G_N_ELEMENTS (app_entries), navigator_app);

  /* Действия окна "win.*". */
  g_action_map_add_action_entries (G_ACTION_MAP (priv->window), win_entries, G_N_ELEMENTS (win_entries), priv->window);

  g_object_unref (action);
}

/* Компонует основное окно программы. */
static GtkWidget *
hyscan_gtk_navigator_app_compose_main_window (HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  GtkWidget *right_col;
  GtkWidget *area;
  GtkWidget *central;
  GtkWidget *status_bar;
  GtkWidget *main_box;
  GtkWidget *window;

  window = gtk_application_window_new (GTK_APPLICATION (navigator_app));
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);

  /* Устанавливаем иконки приложения. */
  {
    GList *icon_list = NULL;
    guint i;
    const gchar *icons[] =
    {
      "/org/hyscan/icons/main-16x16.png",
      "/org/hyscan/icons/main-32x32.png",
      "/org/hyscan/icons/main-64x64.png",
      "/org/hyscan/icons/main-128x128.png",
      "/org/hyscan/icons/main-256x256.png",
    };

    for (i = 0; i < G_N_ELEMENTS (icons); i++)
      icon_list = g_list_prepend (icon_list, gdk_pixbuf_new_from_resource (icons[i], NULL));

    gtk_window_set_default_icon_list (icon_list);
    g_list_free_full (icon_list, g_object_unref);
  }

  /* Правая колонка и центральная часть помещаются в HyScanGtkArea. */
  right_col = hyscan_gtk_map_builder_get_right_col (priv->builder);
  gtk_widget_set_margin_top (right_col, 6);
  gtk_widget_set_margin_start (right_col, 6);

  central = hyscan_gtk_map_builder_get_central (priv->builder);

  area = hyscan_gtk_area_new ();
  hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (area), central);
  hyscan_gtk_area_set_right (HYSCAN_GTK_AREA (area), right_col);

  /* Строка состояния. */
  status_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  g_object_set (status_bar, "margin", 3, NULL);
  gtk_box_pack_start (GTK_BOX (status_bar), hyscan_gtk_map_builder_get_bar (priv->builder), TRUE, TRUE, 0);
  if (priv->control != NULL)
    gtk_box_pack_end (GTK_BOX (status_bar), hyscan_gtk_dev_indicator_new (priv->control), FALSE, FALSE, 0);

  /* Основная коробка. */
  main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_box), area, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_box), status_bar, FALSE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), main_box);

  g_signal_connect_swapped (window, "destroy", G_CALLBACK (hyscan_gtk_navigator_app_builder_save), navigator_app);

  return window;
}

/* Создаёт и показывает главное окно программы. */
static void
hyscan_gtk_navigator_app_main_window (HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  gchar *project;

  /* Считываем проект из настроек, его мог поменять коннектор. */
  project = g_key_file_get_string (priv->settings, "common", "project", NULL);
  hyscan_gtk_model_manager_set_project_name (priv->model_manager, project);
  g_free (project);

  /* Создаём и настраиваем билдер карты. */
  priv->builder = hyscan_gtk_map_builder_new (priv->model_manager);
  hyscan_gtk_map_builder_add_planner (priv->builder, FALSE);
  if (priv->control_model != NULL)
    hyscan_gtk_map_builder_add_nav (priv->builder, priv->control_model, NULL);

  /* Добавляем инструменты управления картой. */
  {
    GtkWidget *preload, *go;
    GtkWidget *notebook;
    GtkWidget *map = hyscan_gtk_map_builder_get_map (priv->builder);
    const gchar *base_id = "base";

    notebook = gtk_notebook_new ();

    go = hyscan_gtk_map_go_new (HYSCAN_GTK_MAP (map));
    g_object_set (go, "margin", 6, NULL);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), go, gtk_label_new (_("Go to")));

    preload = hyscan_gtk_map_preload_new (HYSCAN_GTK_MAP (map), base_id);
    g_object_set (preload, "margin", 6, NULL);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), preload, gtk_label_new (_("Download")));

    hyscan_gtk_map_builder_layer_set_tools (priv->builder, base_id, notebook);
  }

  /* Считываем настройки карты. */
  hyscan_gtk_map_builder_file_read (priv->builder, priv->settings);

  /* Компануем окно программы. */
  priv->window = hyscan_gtk_navigator_app_compose_main_window (navigator_app);

  hyscan_gtk_navigator_app_project_changed (navigator_app);
  hyscan_gtk_navigator_app_add_actions (navigator_app);

  gtk_widget_show_all (GTK_WIDGET (priv->window));
}

/* Обрабатывает завершение коннектора. */
static void
hyscan_gtk_navigator_app_connector_close (GtkAssistant          *assistant,
                                          HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  gboolean success;

  success = hyscan_gtk_con_get_result (HYSCAN_GTK_CON (assistant));
  if (!success)
    {
      gtk_widget_destroy (GTK_WIDGET (assistant));
      g_application_quit (G_APPLICATION (navigator_app));
      return;
    }

  priv->control = hyscan_gtk_con_get_control (HYSCAN_GTK_CON (assistant));
  if (priv->control != NULL)
    {
      hyscan_control_device_bind (priv->control);
      priv->control_model = hyscan_control_model_new (priv->control);
    }

  gtk_widget_destroy (GTK_WIDGET (assistant));
  hyscan_gtk_navigator_app_main_window (navigator_app);
}

/* Открывает окно коннектора. */
static void
hyscan_gtk_navigator_app_connector_window (HyScanGtkNavigatorApp *navigator_app)
{
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;
  GtkWidget *connector;
  HyScanDB *db;

  db = hyscan_gtk_model_manager_get_db (priv->model_manager);

  connector = hyscan_gtk_con_new ((gchar**)hyscan_config_get_profile_dirs(),
                                  priv->driver_paths, priv->settings, db);

  gtk_application_add_window (GTK_APPLICATION (navigator_app), GTK_WINDOW (connector));

  g_signal_connect (connector, "cancel", G_CALLBACK (hyscan_gtk_navigator_app_connector_close), navigator_app);
  g_signal_connect (connector, "close", G_CALLBACK (hyscan_gtk_navigator_app_connector_close), navigator_app);

  gtk_widget_show_all (connector);

  g_object_unref (db);
}

/* Активация приложения. */
static void
hyscan_gtk_navigator_app_activate (GApplication *application)
{
  HyScanGtkNavigatorApp *navigator_app = HYSCAN_GTK_NAVIGATOR_APP (application);
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;

  /* Проверяем подключение к БД (=> наличие моделей). */
  if (priv->model_manager == NULL)
    {
      GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (NULL,
                                       flags,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       "Failed to connect to database “%s”", priv->db_uri);

      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      return;
    }

  /* Открываем окно коннектора. */
  hyscan_gtk_navigator_app_connector_window (navigator_app);
}

static void
hyscan_gtk_navigator_app_shutdown (GApplication *application)
{
  HyScanGtkNavigatorApp *navigator_app = HYSCAN_GTK_NAVIGATOR_APP (application);
  HyScanGtkNavigatorAppPrivate *priv = navigator_app->priv;

  /* Сохраняем настройки. */
  if (priv->model_manager != NULL && priv->settings != NULL)
    {
      const gchar *project = NULL;

      project = hyscan_gtk_model_manager_get_project_name (priv->model_manager);
      if (project != NULL)
        g_key_file_set_string (priv->settings, "common", "project", project);
      g_key_file_save_to_file (priv->settings, priv->settings_file, NULL);
    }

  G_APPLICATION_CLASS (hyscan_gtk_navigator_app_parent_class)->shutdown (application);
}

/**
 * hyscan_gtk_navigator_app_new:
 * @db_uri: URI для подключения к БД
 * @driver_paths: NULL-терминированный массив каталогов для поиска драйверов
 *
 * Функция создаёт приложения для судоводителя.
 *
 * Returns: (transfer full): новый объект HyScanGtkNavigatorApp, для удаления g_object_unref().
 */
GtkApplication *
hyscan_gtk_navigator_app_new (const gchar  *db_uri,
                              gchar       **driver_paths)
{
  return g_object_new (HYSCAN_TYPE_GTK_NAVIGATOR_APP,
                       "application-id", "org.hyscan.navigator",
                       "db-uri", db_uri,
                       "driver-paths", driver_paths,
                       NULL);
}
