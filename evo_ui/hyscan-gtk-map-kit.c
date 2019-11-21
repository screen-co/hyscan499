#include "hyscan-gtk-map-kit.h"
#include "hyscan-gtk-mark-editor.h"
#include "hyscan-gtk-mark-export.h"
#include <hyscan-gtk-map-base.h>
#include <hyscan-gtk-map-control.h>
#include <hyscan-gtk-map-ruler.h>
#include <hyscan-gtk-map-grid.h>
#include <hyscan-gtk-map-pin.h>
#include <hyscan-gtk-map-nav.h>
#include <hyscan-gtk-map-track.h>
#include <hyscan-cached.h>
#include <hyscan-profile-map.h>
#include <hyscan-map-tile-loader.h>
#include <hyscan-db-info.h>
#include <hyscan-gtk-map-scale.h>
#include <hyscan-gtk-param-list.h>
#include <hyscan-gtk-map-wfmark.h>
#include <hyscan-object-model.h>
#include <hyscan-gtk-map-geomark.h>

#define GETTEXT_PACKAGE "hyscanfnn-evoui"
#include <glib/gi18n-lib.h>

#define DEFAULT_PROFILE_NAME "default"    /* Имя профиля карты по умолчанию. */
#define PRELOAD_STATE_DONE   1000         /* Статус кэширования тайлов 0 "Загрузка завершена". */

/* Столбцы в GtkTreeView списка слоёв. */
enum
{
  LAYER_VISIBLE_COLUMN,
  LAYER_KEY_COLUMN,
  LAYER_TITLE_COLUMN,
  LAYER_COLUMN,
};

/* Столбцы в GtkTreeView списка галсов. */
enum
{
  VISIBLE_COLUMN,
  DATE_SORT_COLUMN,
  TRACK_COLUMN,
  DATE_COLUMN
};

/* Столбцы в GtkTreeView списка меток. */
enum
{
  MARK_ID_COLUMN,
  MARK_NAME_COLUMN,
  MARK_MTIME_COLUMN,
  MARK_MTIME_SORT_COLUMN,
  MARK_TYPE_COLUMN,
  MARK_TYPE_NAME_COLUMN,
};

struct _HyScanGtkMapKitPrivate
{
  /* Модели данных. */
  HyScanDBInfo          *db_info;          /* Доступ к данным БД. */
  HyScanObjectModel     *mark_model;       /* Модель меток водопада. */
  HyScanObjectModel     *mark_geo_model;   /* Модель геометок. */
  HyScanMarkLocModel    *ml_model;         /* Модель местоположения меток водопада. */
  HyScanDB              *db;
  HyScanCache           *cache;
  HyScanNavModel        *nav_model;
  gchar                 *project_name;

  GHashTable            *profiles;         /* Хэш-таблица профилей карты. */
  gchar                 *profile_active;   /* Ключ активного профиля. */
  gboolean               profile_offline;  /* Признак оффлайн-профиля карты. */
  gchar                 *tile_cache_dir;   /* Путь к директории, в которой хранятся тайлы. */

  HyScanGeoGeodetic      center;           /* Географические координаты для виджета навигации. */

  /* Слои. */
  GtkListStore          *layer_store;      /* Модель параметров отображения слоёв. */
  HyScanGtkLayer        *track_layer;      /* Слой просмотра галсов. */
  HyScanGtkLayer        *wfmark_layer;     /* Слой с метками водопада. */
  HyScanGtkLayer        *geomark_layer;    /* Слой с метками водопада. */
  HyScanGtkLayer        *map_grid;         /* Слой координатной сетки. */
  HyScanGtkLayer        *ruler;            /* Слой линейки. */
  HyScanGtkLayer        *pin_layer;        /* Слой географических отметок. */
  HyScanGtkLayer        *way_layer;        /* Слой текущего положения по GPS-датчику. */

  /* Виджеты. */
  GtkWidget             *profiles_box;     /* Выпадающий список профилей карты. */
  GtkWidget             *layer_tool_stack; /* GtkStack с виджетами настроек каждого слоя. */
  GtkButton             *preload_button;   /* Кнопка загрузки тайлов. */
  GtkProgressBar        *preload_progress; /* Индикатор загрузки тайлов. */

  HyScanMapTileLoader   *loader;
  guint                  preload_tag;      /* Timeout-функция обновления виджета preload_progress. */
  gint                   preload_state;    /* Статус загрузки тайлов.
                                            * < 0                      - загрузка не началась,
                                            * 0 - PRELOAD_STATE_DONE-1 - прогресс загрузки,
                                            * >= PRELOAD_STATE_DONE    - загрузка завершена, не удалось загрузить
                                            *                            PRELOAD_STATE_DONE - preload_state тайлов */

  GtkTreeView           *track_tree;       /* GtkTreeView со списком галсов. */
  GtkListStore          *track_store;      /* Модель данных для track_tree. */
  GtkMenu               *track_menu;       /* Контекстное меню галса (по правой кнопке). */
  GtkWidget             *track_menu_find;  /* Пункт меню поиска галса на карте. */

  GtkTreeView           *mark_tree;       /* GtkTreeView со списком галсов. */
  GtkListStore          *mark_store;      /* Модель данных для track_tree. */

  GtkWidget             *locate_button;   /* Кнопка для определения текущего местоположения. */
  GtkWidget             *lat_spin;        /* Поля для ввода широты. */
  GtkWidget             *lon_spin;        /* Поля для ввода долготы. */

  GtkWidget             *mark_editor;     /* Редактор названия меток. */
  GtkWidget             *stbar_offline;   /* Статусбар оффлайн. */
  GtkWidget             *stbar_coord;     /* Статусбар координат. */
};

static void     hyscan_gtk_map_kit_set_tracks   (HyScanGtkMapKit      *kit,
                                                 gchar               **tracks);
static gchar ** hyscan_gtk_map_kit_get_tracks   (HyScanGtkMapKit      *kit);
static void     hyscan_gtk_map_kit_track_enable (HyScanGtkMapKit      *kit,
                                                 const gchar          *track_name,
                                                 gboolean              enable);
static void    hyscan_gtk_map_kit_on_changed_combo_box (
                                                 GtkComboBox          *combo,
                                                 HyScanGtkLayer       *layer);
#if !GLIB_CHECK_VERSION (2, 44, 0)
static gboolean g_strv_contains               (const gchar * const  *strv,
                                               const gchar          *str);
#endif

static GtkWidget *
create_map (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanGtkMap *map;
  gdouble *scales;
  gint scales_len;
  HyScanGtkLayer *control, *scale;

  map = HYSCAN_GTK_MAP (hyscan_gtk_map_new (priv->center));

  /* Устанавливаем допустимые масштабы. */
  scales = hyscan_gtk_map_create_scales2 (1.0 / 1000, HYSCAN_GTK_MAP_EQUATOR_LENGTH / 1000, 4, &scales_len);
  hyscan_gtk_map_set_scales_meter (map, scales, scales_len);
  g_free (scales);

  /* По умолчанию добавляем слой управления картой и масштаб. */
  control = hyscan_gtk_map_control_new ();
  scale = hyscan_gtk_map_scale_new ();
  hyscan_gtk_layer_set_visible (scale, TRUE);
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (map), control, "control");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (map), scale,   "scale");

  /* Чтобы виджет карты занял всё доступное место. */
  gtk_widget_set_hexpand (GTK_WIDGET (map), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (map), TRUE);

  return GTK_WIDGET (map);
}

/* Получает NULL-терминированный массив названий профилей.
 * Скопировано почти без изменений из hyscan_profile_common_list_profiles. */
static gchar **
list_profiles (const gchar *profiles_path)
{
  guint nprofiles = 0;
  gchar **profiles = NULL;
  GError *error = NULL;
  GDir *dir;

  dir = g_dir_open (profiles_path, 0, &error);
  if (error == NULL)
    {
      const gchar *filename;

      while ((filename = g_dir_read_name (dir)) != NULL)
        {
          gchar *fullname;

          fullname = g_build_path (G_DIR_SEPARATOR_S, profiles_path, filename, NULL);
          if (g_file_test (fullname, G_FILE_TEST_IS_REGULAR))
            {
              profiles = g_realloc (profiles, ++nprofiles * sizeof (gchar **));
              profiles[nprofiles - 1] = g_strdup (fullname);
            }
          g_free (fullname);
        }
    }
  else
    {
      g_warning ("HyScanGtkMapKit: %s", error->message);
      g_error_free (error);
    }

  g_dir_close (dir);

  profiles = g_realloc (profiles, ++nprofiles * sizeof (gchar **));
  profiles[nprofiles - 1] = NULL;

  return profiles;
}

static void
combo_box_update (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT (priv->profiles_box);

  GHashTableIter iter;
  const gchar *key;
  HyScanProfileMap *profile;

  gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (combo));

  g_hash_table_iter_init (&iter, priv->profiles);
  while (g_hash_table_iter_next (&iter, (gpointer *) &key, (gpointer *) &profile))
    {
      const gchar *title;

      title = hyscan_profile_get_name (HYSCAN_PROFILE (profile));
      gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo), key, title);
    }

  if (priv->profile_active != NULL)
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (combo), priv->profile_active);
}

static void
add_profile (HyScanGtkMapKit  *kit,
             const gchar      *profile_name,
             HyScanProfileMap *profile)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_hash_table_insert (priv->profiles, g_strdup (profile_name), g_object_ref (profile));
  combo_box_update (kit);
}

/* Переключает активный профиль карты. */
static void
on_profile_change (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkComboBox *combo = GTK_COMBO_BOX (priv->profiles_box);
  const gchar *active_id;

  active_id = gtk_combo_box_get_active_id (combo);

  if (active_id != NULL)
    hyscan_gtk_map_kit_set_profile_name (kit, active_id);
}

static void
on_editable_switch (GtkSwitch               *widget,
                    GParamSpec              *pspec,
                    HyScanGtkLayerContainer *container)
{
  hyscan_gtk_layer_container_set_changes_allowed (container, !gtk_switch_get_active (widget));
}

/* Добавляет @layer на карту и в список слоёв. */
static void
add_layer_row (HyScanGtkMapKit *kit,
               HyScanGtkLayer  *layer,
               const gchar     *key,
               const gchar     *title)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeIter tree_iter;

  if (layer == NULL)
    return;

  /* Регистрируем слой в карте. */
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (kit->map), layer, key);

  /* Регистрируем слой в layer_store. */
  gtk_list_store_append (priv->layer_store, &tree_iter);
  gtk_list_store_set (priv->layer_store, &tree_iter,
                      LAYER_VISIBLE_COLUMN, hyscan_gtk_layer_get_visible (layer),
                      LAYER_KEY_COLUMN, key,
                      LAYER_TITLE_COLUMN, title,
                      LAYER_COLUMN, layer,
                      -1);
}

static void
on_enable_track (GtkCellRendererToggle *cell_renderer,
                 gchar                 *path,
                 gpointer               user_data)
{
  HyScanGtkMapKit *kit = user_data;
  HyScanGtkMapKitPrivate *priv = kit->priv;

  gboolean active;
  gchar *track_name;
  GtkTreePath *tree_path;
  GtkTreeIter iter;
  GtkTreeModel *tree_model = GTK_TREE_MODEL (priv->track_store);

  /* Узнаем, галочку какого галса изменил пользователь. */
  tree_path = gtk_tree_path_new_from_string (path);
  gtk_tree_model_get_iter (tree_model, &iter, tree_path);
  gtk_tree_path_free (tree_path);
  gtk_tree_model_get (tree_model, &iter, TRACK_COLUMN, &track_name, VISIBLE_COLUMN, &active, -1);

  /* Устанавливаем видимость галса. */
  hyscan_gtk_map_kit_track_enable (kit, track_name, !active);

  g_free (track_name);
}

static void
on_enable_layer (GtkCellRendererToggle *cell_renderer,
                 gchar                 *path,
                 gpointer               user_data)
{
  HyScanGtkMapKit *kit = user_data;
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeIter iter;
  GtkTreePath *tree_path;
  GtkTreeModel *tree_model = GTK_TREE_MODEL (priv->layer_store);

  gboolean visible;
  HyScanGtkLayer *layer;

  /* Узнаем, галочку какого слоя изменил пользователь. */
  tree_path = gtk_tree_path_new_from_string (path);
  gtk_tree_model_get_iter (tree_model, &iter, tree_path);
  gtk_tree_path_free (tree_path);
  gtk_tree_model_get (tree_model, &iter,
                      LAYER_COLUMN, &layer,
                      LAYER_VISIBLE_COLUMN, &visible, -1);

  /* Устанавливаем новое данных. */
  visible = !visible;
  hyscan_gtk_layer_set_visible (layer, visible);
  gtk_list_store_set (priv->layer_store, &iter, LAYER_VISIBLE_COLUMN, visible, -1);

  g_object_unref (layer);
}

/* Обновляет список активных галсов при измении модели. */
static void
track_tree_view_visible_changed (HyScanGtkMapKit     *kit,
                                 const gchar *const *visible_tracks)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeIter iter;
  gboolean valid;

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->track_store), &iter);
  while (valid)
   {
     gchar *track_name;
     gboolean active;

     gtk_tree_model_get (GTK_TREE_MODEL (priv->track_store), &iter, TRACK_COLUMN, &track_name, -1);

     active = g_strv_contains (visible_tracks, track_name);
     gtk_list_store_set (priv->track_store, &iter, VISIBLE_COLUMN, active, -1);

     g_free (track_name);

     valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->track_store), &iter);
   }
}

static gchar *
track_tree_view_get_selected (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeSelection *selection;
  GList *list;
  GtkTreeModel *tree_model = GTK_TREE_MODEL (priv->track_store);
  gchar *track_name = NULL;

  selection = gtk_tree_view_get_selection (priv->track_tree);
  list = gtk_tree_selection_get_selected_rows (selection, &tree_model);
  if (list != NULL)
    {
      GtkTreePath *path = list->data;
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->track_store), &iter, path))
          gtk_tree_model_get (GTK_TREE_MODEL (priv->track_store), &iter, TRACK_COLUMN, &track_name, -1);
    }
  g_list_free_full (list, (GDestroyNotify) gtk_tree_path_free);

  return track_name;
}

static void
on_locate_track_clicked (GtkButton *button,
                         gpointer   user_data)
{
  HyScanGtkMapKit *kit = user_data;
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar *track_name;

  track_name = track_tree_view_get_selected (kit);

  if (track_name != NULL)
    hyscan_gtk_map_track_view (HYSCAN_GTK_MAP_TRACK (priv->track_layer), track_name, FALSE);

  g_free (track_name);
}

static GtkWidget *
create_param_settings_window (HyScanGtkMapKit *kit,
                              const gchar     *title,
                              HyScanParam     *param)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *window;
  GtkWidget *frontend;
  GtkWidget *grid;
  GtkWidget *abar;
  GtkWidget *apply, *discard, *ok, *cancel;
  GtkSizeGroup *size;
  GtkWidget *toplevel;

  /* Настраиваем окошко. */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_set_default_size (GTK_WINDOW (window), 250, 300);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (priv->track_tree));
  if (GTK_IS_WINDOW (toplevel))
    gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (toplevel));

  g_signal_connect_swapped (window, "destroy", G_CALLBACK (gtk_widget_destroy), window);

  /* Виджет отображения параметров. */
  frontend = hyscan_gtk_param_list_new (param, "/", FALSE);
  hyscan_gtk_param_set_watch_period (HYSCAN_GTK_PARAM (frontend), 500);

  /* Кнопки сохранения настроек. */
  ok = gtk_button_new_with_label (_("OK"));
  cancel = gtk_button_new_with_label (_("Cancel"));
  discard = gtk_button_new_with_label (_("Restore"));
  apply = gtk_button_new_with_label (_("Apply"));

  g_signal_connect_swapped (apply,   "clicked", G_CALLBACK (hyscan_gtk_param_apply),   frontend);
  g_signal_connect_swapped (discard, "clicked", G_CALLBACK (hyscan_gtk_param_discard), frontend);
  g_signal_connect_swapped (ok,      "clicked", G_CALLBACK (hyscan_gtk_param_apply),   frontend);
  g_signal_connect_swapped (ok,      "clicked", G_CALLBACK (gtk_widget_destroy),       window);
  g_signal_connect_swapped (cancel,  "clicked", G_CALLBACK (gtk_widget_destroy),       window);

  abar = gtk_action_bar_new ();

  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), apply);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), discard);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), cancel);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), ok);

  size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_object_set_data_full (G_OBJECT (abar), "size-group", size, g_object_unref);

  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), apply);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), discard);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), ok);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), cancel);

  grid = gtk_grid_new ();
  g_object_set (grid, "margin", 12, NULL);

  /* Пакуем всё вместе. */
  gtk_grid_attach (GTK_GRID (grid), frontend, 0, 0, 4, 1);
  gtk_grid_attach (GTK_GRID (grid), abar,     0, 1, 4, 1);

  gtk_container_add (GTK_CONTAINER (window), grid);

  return window;
}

static void
on_configure_track_clicked (GtkButton *button,
                            gpointer   user_data)
{
  HyScanGtkMapKit *kit = user_data;
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeSelection *selection;
  GList *list;
  GtkTreeModel *tree_model = GTK_TREE_MODEL (priv->track_store);

  selection = gtk_tree_view_get_selection (priv->track_tree);
  list = gtk_tree_selection_get_selected_rows (selection, &tree_model);
  if (list != NULL)
    {
      GtkTreePath *path = list->data;
      GtkTreeIter iter;
      gchar *track_name;

      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->track_store), &iter, path))
        {
          GtkWidget *window;
          HyScanGtkMapTrackItem *track;

          gtk_tree_model_get (GTK_TREE_MODEL (priv->track_store), &iter, TRACK_COLUMN, &track_name, -1);
          track = hyscan_gtk_map_track_lookup (HYSCAN_GTK_MAP_TRACK (priv->track_layer), track_name);

          window = create_param_settings_window (kit, _("Track settings"), HYSCAN_PARAM (track));
          gtk_widget_show_all (window);

          g_object_unref (track);
          g_free (track_name);
        }
    }
  g_list_free_full (list, (GDestroyNotify) gtk_tree_path_free);

}

/* Функция вызывается при изменении списка галсов. */
static void
tracks_changed (HyScanDBInfo    *db_info,
                HyScanGtkMapKit *kit)
{
  GtkTreePath *null_path;
  GtkTreeIter tree_iter;
  GHashTable *tracks;
  GHashTableIter hash_iter;
  gpointer key, value;
  gchar **selected_tracks;

  HyScanGtkMapKitPrivate *priv = kit->priv;

  null_path = gtk_tree_path_new ();
  gtk_tree_view_set_cursor (priv->track_tree, null_path, NULL, FALSE);
  gtk_tree_path_free (null_path);

  gtk_list_store_clear (priv->track_store);

  tracks = hyscan_db_info_get_tracks (db_info);
  selected_tracks = hyscan_gtk_map_kit_get_tracks (kit);
  g_hash_table_iter_init (&hash_iter, tracks);
  while (g_hash_table_iter_next (&hash_iter, &key, &value))
    {
      GDateTime *local;
      gchar *time_str;
      HyScanTrackInfo *track_info = value;
      gboolean visible;

      if (track_info->id == NULL)
        {
          g_message ("Unable to open track <%s>", track_info->name);
          continue;
        }

      /* Добавляем в список галсов. */
      local = g_date_time_to_local (track_info->ctime);
      time_str = g_date_time_format (local, "%d.%m %H:%M");
      visible = g_strv_contains ((const gchar *const *) selected_tracks, track_info->name);

      gtk_list_store_append (priv->track_store, &tree_iter);
      gtk_list_store_set (priv->track_store, &tree_iter,
                          VISIBLE_COLUMN, visible,
                          DATE_SORT_COLUMN, g_date_time_to_unix (track_info->ctime),
                          TRACK_COLUMN, track_info->name,
                          DATE_COLUMN, time_str,
                          -1);

      g_free (time_str);
      g_date_time_unref (local);
    }

  g_strfreev (selected_tracks);
  g_hash_table_unref (tracks);
}

static gboolean
on_button_press_event (GtkTreeView     *tree_view,
                       GdkEventButton  *event_button,
                       HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  if ((event_button->type == GDK_BUTTON_PRESS) && (event_button->button == GDK_BUTTON_SECONDARY))
    {
      gtk_menu_popup (priv->track_menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
    }

  return FALSE;
}

/* Отмечает все галочки в списке галсов. */
static void
track_view_select_all (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gboolean valid;
  GtkTreeIter iter;

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->track_store), &iter);
  while (valid)
    {
      gchar *track_name;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->track_store), &iter, TRACK_COLUMN, &track_name, -1);
      hyscan_gtk_map_kit_track_enable (kit, track_name, TRUE);
      g_free (track_name);

      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->track_store), &iter);
    }
}

/* Клик по заголовку с галочкой в таблице галсов. */
static void
on_enable_all_tracks (HyScanGtkMapKit *kit)
{
  gchar **selected;

  selected = hyscan_gtk_map_kit_get_tracks (kit);
  if (g_strv_length (selected) == 0)
    track_view_select_all (kit);
  else
    hyscan_gtk_map_kit_set_tracks (kit, NULL);

  g_strfreev (selected);
}

static GtkTreeView *
create_track_tree_view (HyScanGtkMapKit *kit,
                        GtkTreeModel    *tree_model)
{
  GtkWidget *image;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *visible_column, *track_column, *date_column;
  GtkTreeView *tree_view;

  /* Название галса. */
  renderer = gtk_cell_renderer_text_new ();
  track_column = gtk_tree_view_column_new_with_attributes (_("Track"), renderer,
                                                           "text", TRACK_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (track_column, TRACK_COLUMN);
  gtk_tree_view_column_set_expand (track_column, TRUE);
  gtk_tree_view_column_set_resizable (track_column, TRUE);
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  /* Дата создания галса. */
  renderer = gtk_cell_renderer_text_new ();
  date_column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer,
                                                          "text", DATE_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (date_column, DATE_SORT_COLUMN);

  /* Галочка с признаком видимости галса. */
  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (on_enable_track), kit);
  visible_column = gtk_tree_view_column_new_with_attributes (_("Show"), renderer,
                                                             "active", VISIBLE_COLUMN, NULL);
  image = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_MENU);
  gtk_tree_view_column_set_clickable (visible_column, TRUE);
  g_signal_connect_swapped (visible_column, "clicked", G_CALLBACK (on_enable_all_tracks), kit);
  gtk_widget_show (image);
  gtk_tree_view_column_set_widget (visible_column, image);

  tree_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (tree_model));
  gtk_tree_view_set_search_column (tree_view, TRACK_COLUMN);
  gtk_tree_view_append_column (tree_view, track_column);
  gtk_tree_view_append_column (tree_view, date_column);
  gtk_tree_view_append_column (tree_view, visible_column);

  gtk_tree_view_set_grid_lines (tree_view, GTK_TREE_VIEW_GRID_LINES_BOTH);

  return tree_view;
}

static GtkTreeView *
create_mark_tree_view (HyScanGtkMapKit *kit,
                       GtkTreeModel    *tree_model)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *type_column, *name_column, *date_column;
  GtkTreeView *tree_view;

  /* Тип метки. */
  renderer = gtk_cell_renderer_text_new ();
  type_column = gtk_tree_view_column_new_with_attributes ("", renderer,
                                                          "text", MARK_TYPE_NAME_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (type_column, MARK_TYPE_COLUMN);

  /* Название метки. */
  renderer = gtk_cell_renderer_text_new ();
  name_column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
                                                          "text", MARK_NAME_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (name_column, MARK_NAME_COLUMN);
  gtk_tree_view_column_set_expand (name_column, TRUE);
  gtk_tree_view_column_set_expand (name_column, TRUE);
  gtk_tree_view_column_set_resizable (name_column, TRUE);
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  /* Время последнего изменения метки. */
  renderer = gtk_cell_renderer_text_new ();
  date_column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer,
                                                          "text", MARK_MTIME_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (date_column, MARK_MTIME_SORT_COLUMN);

  tree_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (tree_model));
  gtk_tree_view_set_search_column (tree_view, MARK_NAME_COLUMN);
  gtk_tree_view_append_column (tree_view, name_column);
  gtk_tree_view_append_column (tree_view, date_column);
  gtk_tree_view_append_column (tree_view, type_column);

  gtk_tree_view_set_grid_lines (tree_view, GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

  return tree_view;
}

static void
on_track_change (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanGtkMapTrackItem *track_item = NULL;
  gchar *track_name;
  gboolean has_nmea;

  track_name = track_tree_view_get_selected (kit);

  if (track_name != NULL)
    track_item = hyscan_gtk_map_track_lookup (HYSCAN_GTK_MAP_TRACK (priv->track_layer), track_name);

  has_nmea = (track_item != NULL) && hyscan_gtk_map_track_item_has_nmea (track_item);
  gtk_widget_set_sensitive (priv->track_menu_find, has_nmea);

  g_clear_object (&track_item);
  g_free (track_name);
}

static void
create_track_menu (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *menu_item_settings;

  priv->track_menu = GTK_MENU (gtk_menu_new ());
  priv->track_menu_find = gtk_menu_item_new_with_label (_("Find track on the map"));
  g_signal_connect (priv->track_menu_find, "activate", G_CALLBACK (on_locate_track_clicked), kit);
  gtk_widget_show (priv->track_menu_find);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->track_menu), priv->track_menu_find);
  gtk_widget_set_sensitive (priv->track_menu_find, FALSE);

  menu_item_settings = gtk_menu_item_new_with_label (_("Track settings"));
  g_signal_connect (menu_item_settings, "activate", G_CALLBACK (on_configure_track_clicked), kit);
  gtk_widget_show (menu_item_settings);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->track_menu), menu_item_settings);

  /* Обработчик определяет доступные пункт при выборе галса. */
  g_signal_connect_swapped (gtk_tree_view_get_selection (priv->track_tree), "changed", G_CALLBACK (on_track_change), kit);
}

/* Ищет метку по её типу и идентификатору. */
static HyScanMark *
mark_find (HyScanGtkMapKit   *kit,
           HyScanObjectType   mark_type,
           const gchar       *id)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GHashTable *marks;

  if (mark_type == HYSCAN_MARK_WATERFALL)
    {
      HyScanMarkWaterfall *mark;

      marks = hyscan_object_model_get (priv->mark_model);
      mark = hyscan_mark_waterfall_copy (g_hash_table_lookup (marks, id));
      g_hash_table_unref (marks);

      return (HyScanMark *) mark;
    }

  else if (mark_type == HYSCAN_MARK_GEO)
    {
      HyScanMarkGeo *mark;

      marks = hyscan_object_model_get (priv->mark_geo_model);
      mark = hyscan_mark_geo_copy (g_hash_table_lookup (marks, id));
      g_hash_table_unref (marks);

      return (HyScanMark *) mark;
    }

  return NULL;
}

static gboolean
mark_get_location (HyScanGtkMapKit *kit,
                   const gchar     *mark_id,
                   HyScanMark      *mark,
                   gdouble         *latitude,
                   gdouble         *longitude)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gboolean found = FALSE;

  if (mark->type == HYSCAN_MARK_GEO)
    {
      HyScanMarkGeo *mark_geo = (HyScanMarkGeo *) mark;

      *latitude = mark_geo->center.lat;
      *longitude = mark_geo->center.lon;

      found = TRUE;
    }

  /* Положение метки из hyscan-mark-loc-model */
  else if (mark->type == HYSCAN_MARK_WATERFALL)
    {
      GHashTable *mark_locations;
      HyScanMarkLocation *location;

      mark_locations = hyscan_mark_loc_model_get (priv->ml_model);

      location = g_hash_table_lookup (mark_locations, mark_id);
      if (location != NULL && location->loaded)
        {
          *latitude = location->mark_geo.lat;
          *longitude = location->mark_geo.lon;

          found = TRUE;
        }

      g_hash_table_unref (mark_locations);
    }

  return found;
}

/* Обработчик выделения строки в списке меток:
 * устанавливает выбранную метку в редактор меток. */
static void
on_marks_selected (GtkTreeSelection *selection,
                   HyScanGtkMapKit   *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeIter iter;
  HyScanMark *mark;
  HyScanObjectType mark_type;

  gchar *mark_id;
  gdouble latitude, longitude;

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (priv->mark_editor), NULL, NULL, NULL, NULL, 0, 0);
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (priv->mark_store), &iter,
                      MARK_ID_COLUMN, &mark_id,
                      MARK_TYPE_COLUMN, &mark_type, -1);

  mark = mark_find (kit, mark_type, mark_id);
  if (mark == NULL)
    goto exit;

  if (!mark_get_location (kit, mark_id, mark, &latitude, &longitude))
    {
      latitude = 0.0;
      longitude = 0.0;
    }

  /* Открываем редактор метки. */
  hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (priv->mark_editor), mark_id,
                                   mark->name, mark->operator_name, mark->description,
                                   latitude, longitude);

  /* Выделяем метку на карте. */
  hyscan_gtk_map_wfmark_mark_highlight (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), mark_id);
  hyscan_gtk_map_geomark_mark_highlight (HYSCAN_GTK_MAP_GEOMARK (priv->geomark_layer), mark_id);

exit:
  if (mark->type == HYSCAN_MARK_WATERFALL)
    hyscan_mark_waterfall_free ((HyScanMarkWaterfall *) mark);
  else if (mark->type == HYSCAN_MARK_GEO)
    hyscan_mark_geo_free ((HyScanMarkGeo *) mark);
  g_free (mark_id);
}

/* Обновляет имя метки mark_id в модели меток model. */
static void
update_mark (HyScanObjectModel *model,
             const gchar       *mark_id,
             const gchar       *name)
{
  HyScanMark *mark;

  mark = (HyScanMark *) hyscan_object_model_get_id (model, mark_id);
  if (mark == NULL)
    return;

  hyscan_mark_set_text (mark, name, mark->description, mark->operator_name);
  hyscan_object_model_modify_object (model, mark_id, (const HyScanObject *) mark);

  if (mark->type == HYSCAN_MARK_WATERFALL)
    hyscan_mark_waterfall_free ((HyScanMarkWaterfall *) mark);
  else if (mark->type == HYSCAN_MARK_GEO)
    hyscan_mark_geo_free ((HyScanMarkGeo *) mark);
}

/* Обработчик изменения метки в редакторе меток. */
static void
mark_modified (HyScanGtkMarkEditor *med,
               HyScanGtkMapKit     *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar *mark_id = NULL;
  gchar *title;

  hyscan_gtk_mark_editor_get_mark (med, &mark_id, &title, NULL, NULL);

  update_mark (priv->mark_model, mark_id, title);
  update_mark (priv->mark_geo_model, mark_id, title);

  g_free (mark_id);
  g_free (title);
}

static void
on_marks_activated (GtkTreeView        *treeview,
                    GtkTreePath        *path,
                    GtkTreeViewColumn  *col,
                    gpointer            user_data)
{
  HyScanGtkMapKit *kit = user_data;
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (treeview);
  if (gtk_tree_model_get_iter (model, &iter, path))
  {
    gchar *mark_id;
    HyScanObjectType mark_type;

    gtk_tree_model_get (model, &iter, MARK_ID_COLUMN, &mark_id, MARK_TYPE_COLUMN, &mark_type, -1);
    if (mark_type == HYSCAN_MARK_WATERFALL && priv->wfmark_layer != NULL)
      hyscan_gtk_map_wfmark_mark_view (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), mark_id, FALSE);
    else if (mark_type == HYSCAN_MARK_GEO && priv->geomark_layer != NULL)
      hyscan_gtk_map_geomark_mark_view (HYSCAN_GTK_MAP_GEOMARK (priv->geomark_layer), mark_id, FALSE);

    g_free (mark_id);
  }
}

static void
on_track_activated (GtkTreeView        *treeview,
                    GtkTreePath        *path,
                    GtkTreeViewColumn  *col,
                    gpointer            userdata)
{
  HyScanGtkMapKit *kit = userdata;
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (treeview);
  if (gtk_tree_model_get_iter(model, &iter, path))
  {
    gchar *track_name;

    gtk_tree_model_get (model, &iter, TRACK_COLUMN, &track_name, -1);
    hyscan_gtk_map_track_view (HYSCAN_GTK_MAP_TRACK (priv->track_layer), track_name, FALSE);

    g_free (track_name);
  }
}

static void
list_store_insert (HyScanGtkMapKit   *kit,
                   HyScanObjectModel *model,
                   gchar              *selected_id)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeIter tree_iter;
  GHashTable *marks;
  GHashTableIter hash_iter;

  HyScanMark *mark;
  gchar *mark_id;

  marks = hyscan_object_model_get (model);
  if (marks == NULL)
    return;

  g_hash_table_iter_init (&hash_iter, marks);

  while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &mark))
    {
      GDateTime *local;
      gchar *time_str;
      gchar *type_name;

      if (mark->type == HYSCAN_MARK_WATERFALL && ((HyScanMarkWaterfall *) mark)->track == NULL)
        continue;

      /* Добавляем в список меток. */
      local = g_date_time_new_from_unix_local (mark->mtime / 1000000);
      time_str = g_date_time_format (local, "%d.%m %H:%M");

      if (mark->type == HYSCAN_MARK_WATERFALL)
        type_name = "W";
      else if (mark->type == HYSCAN_MARK_GEO)
        type_name = "G";
      else
        type_name = "?";

      gtk_list_store_append (priv->mark_store, &tree_iter);
      gtk_list_store_set (priv->mark_store, &tree_iter,
                          MARK_ID_COLUMN, mark_id,
                          MARK_NAME_COLUMN, mark->name,
                          MARK_MTIME_COLUMN, time_str,
                          MARK_MTIME_SORT_COLUMN, mark->mtime,
                          MARK_TYPE_COLUMN, mark->type,
                          MARK_TYPE_NAME_COLUMN, type_name,
                          -1);

      if (g_strcmp0 (selected_id, mark_id) == 0)
        gtk_tree_selection_select_iter (gtk_tree_view_get_selection (priv->mark_tree), &tree_iter);

      g_free (time_str);
      g_date_time_unref (local);
    }

  g_hash_table_unref (marks);
}

static gchar *
mark_get_selected_id (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gchar *mark_id;

  selection = gtk_tree_view_get_selection (priv->mark_tree);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return NULL;

  gtk_tree_model_get (GTK_TREE_MODEL (priv->mark_store), &iter, MARK_ID_COLUMN, &mark_id, -1);

  return mark_id;
}

static void
on_marks_changed (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar *selected_id;
  GtkTreeSelection *selection;

  /* Блокируем обработку изменения выделения в списке меток. */
  selection = gtk_tree_view_get_selection (priv->mark_tree);
  g_signal_handlers_block_by_func (selection, on_marks_selected, kit);

  selected_id = mark_get_selected_id (kit);

  /* Удаляем все строки из mark_store. */
  gtk_list_store_clear (priv->mark_store);

  /* Добавляем метки из всех моделей. */
  if (priv->mark_model != NULL)
    list_store_insert (kit, priv->mark_model, selected_id);
  if (priv->mark_geo_model != NULL)
    list_store_insert (kit, priv->mark_geo_model, selected_id);

  /* Разблокируем обработку выделения. */
  g_signal_handlers_unblock_by_func (selection, on_marks_selected, kit);

  g_free (selected_id);
}

static GtkWidget *
create_mark_editor (HyScanGtkMapKit *kit,
                    GtkTreeView     *mark_tree)
{
  GtkWidget *mark_editor;
  GtkTreeSelection *selection;

  mark_editor = hyscan_gtk_mark_editor_new ();
  selection = gtk_tree_view_get_selection (mark_tree);

  g_signal_connect (selection,   "changed",       G_CALLBACK (on_marks_selected), kit);
  g_signal_connect (mark_editor, "mark-modified", G_CALLBACK (mark_modified),     kit);

  return mark_editor;
}

/* Добавляет в панель навигации виджет просмотра меток. */
static void
create_wfmark_toolbox (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *scrolled_window;

  /* Невозможно создать виджет, т.к. */
  if (priv->db == NULL)
    return;

  /* Виджет уже создан. Ничего не делаем. */
  if (priv->mark_tree != NULL)
    return;

  priv->mark_store = gtk_list_store_new (6,
                                         G_TYPE_STRING,  /* MARK_ID_COLUMN   */
                                         G_TYPE_STRING,  /* MARK_NAME_COLUMN   */
                                         G_TYPE_STRING,  /* MARK_MTIME_COLUMN */
                                         G_TYPE_INT64,   /* MARK_MTIME_SORT_COLUMN */
                                         G_TYPE_UINT,    /* MARK_TYPE_COLUMN */
                                         G_TYPE_STRING,  /* MARK_TYPE_NAME_COLUMN */
                                         -1);
  priv->mark_tree = create_mark_tree_view (kit, GTK_TREE_MODEL (priv->mark_store));
  priv->mark_editor = create_mark_editor (kit, priv->mark_tree);

  g_signal_connect (priv->mark_tree, "row-activated", G_CALLBACK (on_marks_activated), kit);

  /* Область прокрутки со списком меток. */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_hexpand (scrolled_window, FALSE);
  gtk_widget_set_vexpand (scrolled_window, FALSE);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (scrolled_window), 150);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), 120);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->mark_tree));

  /* Помещаем в панель навигации. */
  gtk_box_pack_start (GTK_BOX (kit->navigation), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (kit->navigation), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (kit->navigation), priv->mark_editor, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (kit->navigation), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
}

/* Выбор галсов проекта. */
static void
create_track_box (HyScanGtkMapKit *kit,
                  GtkBox          *box)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *scrolled_window;

  if (priv->db == NULL)
    return;

  priv->track_store = gtk_list_store_new (4,
                                          G_TYPE_BOOLEAN, /* VISIBLE_COLUMN   */
                                          G_TYPE_INT64,   /* DATE_SORT_COLUMN */
                                          G_TYPE_STRING,  /* TRACK_COLUMN     */
                                          G_TYPE_STRING   /* DATE_COLUMN      */);
  priv->track_tree = create_track_tree_view (kit, GTK_TREE_MODEL (priv->track_store));

  create_track_menu (kit);
  gtk_menu_attach_to_widget (priv->track_menu, GTK_WIDGET (priv->track_tree), NULL);

  g_signal_connect_swapped (priv->track_tree, "destroy", G_CALLBACK (gtk_widget_destroy), priv->track_menu);
  g_signal_connect (priv->track_tree, "button-press-event", G_CALLBACK (on_button_press_event), kit);
  g_signal_connect (priv->track_tree, "row-activated", G_CALLBACK (on_track_activated), kit);
  g_signal_connect (priv->db_info, "tracks-changed", G_CALLBACK (tracks_changed), kit);

  /* Область прокрутки со списком галсов. */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (scrolled_window, FALSE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  // gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), 200);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->track_tree));


  /* Помещаем в панель навигации. */
  gtk_box_pack_start (box, scrolled_window, TRUE, TRUE, 0);
}

static void
on_coordinate_change (GtkSwitch  *widget,
                      GParamSpec *pspec,
                      gdouble    *value)
{
  *value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
}

static void
on_locate_click (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanNavModelData data;

  if (hyscan_nav_model_get (priv->nav_model, &data, NULL))
    hyscan_gtk_map_move_to (HYSCAN_GTK_MAP (kit->map), data.coord);
}

static void
on_move_to_click (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  hyscan_gtk_map_move_to (HYSCAN_GTK_MAP (kit->map), priv->center);
}

static gboolean
on_motion_show_coords (HyScanGtkMap    *map,
                       GdkEventMotion  *event,
                       HyScanGtkMapKit *kit)
{
  HyScanGeoGeodetic geo;
  gchar text[255];

  hyscan_gtk_map_point_to_geo (map, &geo, event->x, event->y);
  g_snprintf (text, sizeof (text), "%.6f° %.6f°", geo.lat, geo.lon);
  gtk_statusbar_push (GTK_STATUSBAR (kit->priv->stbar_coord), 0, text);

  return FALSE;
}

static gboolean
source_update_progress (gpointer data)
{
  HyScanGtkMapKit *kit = data;
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gint state;

  state = g_atomic_int_get (&priv->preload_state);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->preload_progress),
                                 CLAMP ((gdouble) state / PRELOAD_STATE_DONE, 0.0, 1.0));

  return G_SOURCE_CONTINUE;
}

static gboolean
source_progress_done (gpointer data)
{
  HyScanGtkMapKit *kit = data;
  HyScanGtkMapKitPrivate *priv = kit->priv;

  gint state;
  gchar *text = NULL;

  state = g_atomic_int_get (&priv->preload_state);

  if (state == PRELOAD_STATE_DONE)
    text = g_strdup (_("Done!"));
  else if (state > PRELOAD_STATE_DONE)
    text = g_strdup_printf (_("Done! Failed to load %d tiles"), state - PRELOAD_STATE_DONE);
  else
    g_warn_if_reached ();

  gtk_button_set_label (priv->preload_button, _("Start download"));
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->preload_progress), 1.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->preload_progress), text);
  g_free (text);

  g_signal_handlers_disconnect_by_data (priv->loader, kit);
  g_clear_object (&priv->loader);

  g_atomic_int_set (&priv->preload_state, -1);

  return G_SOURCE_REMOVE;
}

static void
on_preload_done (HyScanMapTileLoader *loader,
                 guint                failed,
                 HyScanGtkMapKit     *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  g_atomic_int_set (&priv->preload_state, PRELOAD_STATE_DONE + failed);

  g_source_remove (priv->preload_tag);
  g_idle_add (source_progress_done, kit);
}

static void
on_preload_progress (HyScanMapTileLoader *loader,
                     gdouble              fraction,
                     HyScanGtkMapKit     *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_atomic_int_set (&priv->preload_state, CLAMP ((gint) (PRELOAD_STATE_DONE * fraction), 0, PRELOAD_STATE_DONE - 1));
}

static void
preload_stop (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  hyscan_map_tile_loader_stop (priv->loader);
}

static void
preload_start (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanGtkLayer *layer;
  HyScanMapTileSource *source;
  gdouble from_x, to_x, from_y, to_y;
  GThread *thread;

  layer = hyscan_gtk_layer_container_lookup (HYSCAN_GTK_LAYER_CONTAINER (kit->map), HYSCAN_PROFILE_MAP_BASE_ID);
  g_return_if_fail (HYSCAN_IS_GTK_MAP_BASE (layer));

  source = hyscan_gtk_map_base_get_source (HYSCAN_GTK_MAP_BASE (layer));
  priv->loader = hyscan_map_tile_loader_new ();
  g_object_unref (source);

  gtk_button_set_label (priv->preload_button, _("Stop download"));
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->preload_progress), NULL);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->preload_progress), 0.0);

  /* Регистрируем обработчики сигналов для loader'а и обновление индикатора загрузки. */
  g_signal_connect (priv->loader, "progress", G_CALLBACK (on_preload_progress), kit);
  g_signal_connect (priv->loader, "done",     G_CALLBACK (on_preload_done),     kit);
  priv->preload_tag = g_timeout_add (500, source_update_progress, kit);

  /* Запускаем загрузку тайлов. */
  gtk_cifro_area_get_view (GTK_CIFRO_AREA (kit->map), &from_x, &to_x, &from_y, &to_y);
  thread = hyscan_map_tile_loader_start (priv->loader, source, from_x, to_x, from_y, to_y);
  g_thread_unref (thread);
}

static void
on_preload_click (GtkButton       *button,
                  HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  if (g_atomic_int_compare_and_exchange (&priv->preload_state, -1, 0))
    preload_start (kit);
  else
    preload_stop (kit);
}

/* Создаёт панель инструментов для слоя булавок и линейки. */
static GtkWidget *
create_ruler_toolbox (HyScanGtkLayer *layer,
                      const gchar    *label)
{
  GtkWidget *box;
  GtkWidget *ctrl_widget;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

  ctrl_widget = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (ctrl_widget), label);
  g_signal_connect_swapped (ctrl_widget, "clicked", G_CALLBACK (hyscan_gtk_map_pin_clear), layer);

  gtk_box_pack_start (GTK_BOX (box), ctrl_widget, TRUE, FALSE, 10);

  return box;
}
/* Список галсов и меток. */
static GtkWidget *
create_navigation_box (HyScanGtkMapKit *kit)
{
  GtkWidget *ctrl_box;

  ctrl_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_end (ctrl_box, 6);

  create_track_box (kit, GTK_BOX (ctrl_box));

  return ctrl_box;
}

/* Текстовые поля для ввода координат. */
static void
create_nav_input (HyScanGtkMapKit *kit,
                  GtkGrid         *grid)
{
  gint t=0;
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *move_button;

  priv->locate_button = gtk_button_new_from_icon_name ("network-wireless", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_margin_bottom (priv->locate_button, 5);
  gtk_button_set_label (GTK_BUTTON (priv->locate_button), _("My location"));
  gtk_widget_set_sensitive (priv->locate_button, FALSE);

  priv->lat_spin = gtk_spin_button_new_with_range (-90.0, 90.0, 0.001);
  g_signal_connect (priv->lat_spin, "notify::value", G_CALLBACK (on_coordinate_change), &priv->center.lat);

  priv->lon_spin = gtk_spin_button_new_with_range (-180.0, 180.0, 0.001);
  g_signal_connect (priv->lon_spin, "notify::value", G_CALLBACK (on_coordinate_change), &priv->center.lon);

  move_button = gtk_button_new_with_label (_("Go"));
  g_signal_connect_swapped (move_button, "clicked", G_CALLBACK (on_move_to_click), kit);

  gtk_grid_attach (grid, gtk_label_new (_("Lat")), 0, ++t, 1, 1);
  gtk_grid_attach (grid, priv->lat_spin, 1, t, 1, 1);
  gtk_grid_attach (grid, gtk_label_new (_("Lon")), 0, ++t, 1, 1);
  gtk_grid_attach (grid, priv->lon_spin, 1, t, 1, 1);
  gtk_grid_attach (grid, move_button, 0, ++t, 2, 1);
  gtk_grid_attach (grid, priv->locate_button, 0, ++t, 2, 1);
}

/* Загрузка тайлов. */
static void
create_preloader (HyScanGtkMapKit *kit,
                  GtkBox          *box)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *description;

  priv->preload_state = -1;

  priv->preload_button = GTK_BUTTON (gtk_button_new_with_label (_("Start download")));
  g_signal_connect (priv->preload_button, "clicked", G_CALLBACK (on_preload_click), kit);

  priv->preload_progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  gtk_progress_bar_set_show_text (priv->preload_progress, TRUE);

  description = gtk_label_new (_("Download visible area of map\nto make it available when offline"));
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);

  gtk_box_pack_start (box, description,                         TRUE, TRUE, 0);
  gtk_box_pack_start (box, GTK_WIDGET (priv->preload_progress), TRUE, TRUE, 0);
  gtk_box_pack_start (box, GTK_WIDGET (priv->preload_button),   TRUE, TRUE, 0);
}

/* Текущие координаты. */
static GtkWidget *
create_status_bar (HyScanGtkMapKit *kit)
{
  GtkWidget *statusbar_box;

  statusbar_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  kit->priv->stbar_offline = gtk_statusbar_new ();
  kit->priv->stbar_coord = gtk_statusbar_new ();
  g_object_set (kit->priv->stbar_offline, "margin", 0, NULL);
  g_object_set (kit->priv->stbar_coord, "margin", 0, NULL);
  g_signal_connect (kit->map, "motion-notify-event", G_CALLBACK (on_motion_show_coords), kit);

  gtk_box_pack_start (GTK_BOX (statusbar_box), kit->priv->stbar_offline, FALSE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (statusbar_box), kit->priv->stbar_coord,   FALSE, TRUE, 10);

  return statusbar_box;
}

static void
layer_changed (GtkTreeSelection *selection,
               HyScanGtkMapKit  *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkWidget *layer_tools;

  /* Получаем данные выбранного слоя. */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      HyScanGtkLayer *layer;
      gchar *layer_key;

      gtk_tree_model_get (model, &iter,
                          LAYER_COLUMN, &layer,
                          LAYER_KEY_COLUMN, &layer_key,
                          -1);
      if (!hyscan_gtk_layer_grab_input (layer))
        hyscan_gtk_layer_container_set_input_owner (HYSCAN_GTK_LAYER_CONTAINER (kit->map), NULL);

      layer_tools = gtk_stack_get_child_by_name (GTK_STACK (priv->layer_tool_stack), layer_key);

      g_free (layer_key);
      g_object_unref (layer);
    }
  else
    {
      hyscan_gtk_layer_container_set_input_owner (HYSCAN_GTK_LAYER_CONTAINER (kit->map), NULL);
      layer_tools = NULL;
    }



  if (layer_tools != NULL)
    {
      gtk_stack_set_visible_child (GTK_STACK (priv->layer_tool_stack), layer_tools);
      gtk_widget_show_all (GTK_WIDGET (priv->layer_tool_stack));
    }
  else
    {
      gtk_widget_hide (GTK_WIDGET (priv->layer_tool_stack));
    }
}

static GtkWidget *
create_layer_tree_view (HyScanGtkMapKit *kit,
                        GtkTreeModel    *tree_model)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *visible_column, *layer_column;
  GtkWidget *tree_view;
  GtkTreeSelection *selection;

  /* Галочка с признаком видимости галса. */
  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (on_enable_layer), kit);
  visible_column = gtk_tree_view_column_new_with_attributes (_("Show"), renderer,
                                                             "active", LAYER_VISIBLE_COLUMN, NULL);

  /* Название галса. */
  renderer = gtk_cell_renderer_text_new ();
  layer_column = gtk_tree_view_column_new_with_attributes (_("Layer"), renderer,
                                                           "text", LAYER_TITLE_COLUMN, NULL);
  gtk_tree_view_column_set_expand (layer_column, TRUE);

  tree_view = gtk_tree_view_new_with_model (tree_model);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), layer_column);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), visible_column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  g_signal_connect (selection, "changed", G_CALLBACK (layer_changed), kit);

  return tree_view;
}

static void
hyscan_gtk_map_kit_scale (HyScanGtkMapKit *kit,
                          gint             steps)
{
  HyScanGtkMap *map = HYSCAN_GTK_MAP (kit->map);
  gdouble from_x, to_x, from_y, to_y;
  gint scale_idx;

  gtk_cifro_area_get_view (GTK_CIFRO_AREA (map), &from_x, &to_x, &from_y, &to_y);

  scale_idx = hyscan_gtk_map_get_scale_idx (map, NULL);
  hyscan_gtk_map_set_scale_idx (map, scale_idx + steps, (from_x + to_x) / 2.0, (from_y + to_y) / 2.0);
}

static void
hyscan_gtk_map_kit_scale_down (HyScanGtkMapKit *kit)
{
  hyscan_gtk_map_kit_scale (kit, 1);
}

static void
hyscan_gtk_map_kit_scale_up (HyScanGtkMapKit *kit)
{
  hyscan_gtk_map_kit_scale (kit, -1);
}

/* Кнопки управления виджетом. */
static GtkWidget *
create_control_box (HyScanGtkMapKit *kit)
{
  gint t = 0;
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *ctrl_box;

  ctrl_box = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (ctrl_box), TRUE);
  gtk_grid_set_column_spacing (GTK_GRID (ctrl_box), 5);
  gtk_grid_set_row_spacing (GTK_GRID (ctrl_box), 3);

  /* Выпадающий список с профилями. */
  priv->profiles_box = gtk_combo_box_text_new ();
  g_signal_connect_swapped (priv->profiles_box , "changed", G_CALLBACK (on_profile_change), kit);

  gtk_grid_attach (GTK_GRID (ctrl_box), priv->profiles_box,                             0, ++t, 5, 1);
  gtk_grid_attach (GTK_GRID (ctrl_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, ++t, 5, 1);

  {
    GtkWidget *scale_up, *scale_down;

    scale_down = gtk_button_new_from_icon_name ("list-remove-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_halign (scale_down, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (scale_down, GTK_ALIGN_CENTER);

    scale_up = gtk_button_new_from_icon_name ("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_halign (scale_up, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (scale_up, GTK_ALIGN_CENTER);

    g_signal_connect_swapped (scale_up,   "clicked", G_CALLBACK (hyscan_gtk_map_kit_scale_up),   kit);
    g_signal_connect_swapped (scale_down, "clicked", G_CALLBACK (hyscan_gtk_map_kit_scale_down), kit);

    gtk_grid_attach (GTK_GRID (ctrl_box), gtk_label_new (_("Scale")),                     0, ++t, 3, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), scale_down,                                     3, t,   1, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), scale_up,                                       4, t,   1, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, ++t, 5, 1);
  }

  /* Слои. */
  {
    GtkWidget *lock_switch, *layers_list;

    lock_switch = gtk_switch_new ();
    gtk_switch_set_active (GTK_SWITCH (lock_switch),
                           !hyscan_gtk_layer_container_get_changes_allowed (HYSCAN_GTK_LAYER_CONTAINER (kit->map)));

    g_signal_connect (lock_switch, "notify::active", G_CALLBACK (on_editable_switch), kit->map);

    layers_list = create_layer_tree_view (kit, GTK_TREE_MODEL (priv->layer_store));

    gtk_grid_attach (GTK_GRID (ctrl_box), gtk_label_new (_("Lock layers")), 0, ++t, 3, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), lock_switch,                      3,   t, 2, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), layers_list,                      0, ++t, 5, 1);
  }

  /* Контейнер для панели инструментов каждого слоя. */
  {
    GtkWidget *layer_tools;

    priv->layer_tool_stack = gtk_stack_new ();
    gtk_stack_set_homogeneous (GTK_STACK (priv->layer_tool_stack), FALSE);

    /* Устаналиваем виджеты с инструментами для каждого слоя. */
    layer_tools = create_ruler_toolbox (priv->ruler, _("Remove ruler"));
    g_object_set_data (G_OBJECT (priv->ruler), "toolbox-cb", "ruler");
    gtk_stack_add_titled (GTK_STACK (priv->layer_tool_stack), layer_tools, "ruler", "Ruler");

    layer_tools = create_ruler_toolbox (priv->pin_layer, _("Remove all pins"));
    g_object_set_data (G_OBJECT (priv->pin_layer), "toolbox-cb", "pin");
    gtk_stack_add_titled (GTK_STACK (priv->layer_tool_stack), layer_tools, "pin", "Pin");

    gtk_grid_attach (GTK_GRID (ctrl_box), GTK_WIDGET (priv->layer_tool_stack),            0, ++t, 5, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, ++t, 5, 1);
  }

  /* Стек с инструментами. */
  {
    GtkWidget *stack_switcher, *stack, *stack_box;

    stack = gtk_stack_new ();

    stack_box = gtk_grid_new ();
    g_object_set (stack_box,
                  "margin", 6,
                  "row-spacing", 6,
                  "column-spacing", 6,
                  "halign", GTK_ALIGN_CENTER,
                  NULL);
    gtk_grid_set_column_spacing (GTK_GRID (stack_box), 2);
    gtk_grid_set_row_spacing (GTK_GRID (stack_box), 2);
    create_nav_input (kit, GTK_GRID (stack_box));
    gtk_stack_add_titled (GTK_STACK (stack), stack_box, "navigate", _("Go to"));

    stack_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    g_object_set (stack_box,
                  "margin", 6,
                  "halign", GTK_ALIGN_CENTER,
                  NULL);
    create_preloader (kit, GTK_BOX (stack_box));
    gtk_stack_add_titled (GTK_STACK (stack), stack_box, "cache", _("Download"));

    stack_switcher = gtk_stack_switcher_new ();
    gtk_widget_set_halign (stack_switcher, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top (stack_switcher, 5);
    gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (stack_switcher), GTK_STACK (stack));

    gtk_grid_attach (GTK_GRID (ctrl_box), stack_switcher,                                 0, ++t, 5, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), stack,                                          0, ++t, 5, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, ++t, 5, 1);
  }

  return ctrl_box;
}

/* Создаёт слои, если соответствующие им модели данных доступны. */
static void
create_layers (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  priv->map_grid = hyscan_gtk_map_grid_new ();
  priv->ruler = hyscan_gtk_map_ruler_new ();
  priv->pin_layer = hyscan_gtk_map_pin_new ();

  hyscan_gtk_layer_set_visible (priv->map_grid, TRUE);
  hyscan_gtk_layer_set_visible (priv->ruler, TRUE);
  hyscan_gtk_layer_set_visible (priv->pin_layer, TRUE);

  /* Слой с галсами. */
  if (priv->db != NULL)
    priv->track_layer = hyscan_gtk_map_track_new (priv->db, priv->cache);
}

/* Создает модели данных. */
static void
hyscan_gtk_map_kit_model_create (HyScanGtkMapKit *kit,
                                 HyScanDB        *db)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  priv->profiles = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  priv->cache = HYSCAN_CACHE (hyscan_cached_new (300));
  if (db != NULL)
    {
      priv->db = g_object_ref (db);
      priv->db_info = hyscan_db_info_new (db);
    }

  kit->map = create_map (kit);
  create_layers (kit);

  priv->layer_store = gtk_list_store_new (4,
                                          G_TYPE_BOOLEAN,        /* LAYER_VISIBLE_COLUMN */
                                          G_TYPE_STRING,         /* LAYER_KEY_COLUMN     */
                                          G_TYPE_STRING,         /* LAYER_TITLE_COLUMN   */
                                          HYSCAN_TYPE_GTK_LAYER  /* LAYER_COLUMN         */);

  add_layer_row (kit, priv->track_layer,   "track",   _("Tracks"));
  add_layer_row (kit, priv->ruler,         "ruler",   _("Ruler"));
  add_layer_row (kit, priv->pin_layer,     "pin",     _("Pin"));
  add_layer_row (kit, priv->map_grid,      "grid",    _("Grid"));
}

static void
hyscan_gtk_map_kit_view_create (HyScanGtkMapKit *kit)
{
  kit->navigation = create_navigation_box (kit);
  kit->control    = create_control_box (kit);
  kit->status_bar = create_status_bar (kit);
}

static void
hyscan_gtk_map_kit_model_init (HyScanGtkMapKit   *kit,
                               HyScanGeoGeodetic *center)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  if (priv->db_info != NULL && priv->project_name != NULL)
    hyscan_db_info_set_project (priv->db_info, priv->project_name);

  /* Устанавливаем центр карты. */
  {
    priv->center = *center;
    hyscan_gtk_map_move_to (HYSCAN_GTK_MAP (kit->map), priv->center);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->lat_spin), priv->center.lat);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->lon_spin), priv->center.lon);
  }

  /* Добавляем профиль по умолчанию. */
  {
    HyScanProfileMap *profile;

    profile = hyscan_profile_map_new_default (priv->tile_cache_dir);

    add_profile (kit, DEFAULT_PROFILE_NAME, profile);
    hyscan_gtk_map_kit_set_profile_name (kit, DEFAULT_PROFILE_NAME);

    g_object_unref (profile);
  }
}

/* Получает ключи видимых слоёв. */
static gchar **
hyscan_gtk_map_kit_get_layers (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeIter iter;
  gboolean valid;
  GArray *array;

  array = g_array_new (TRUE, FALSE, sizeof (gchar *));

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->layer_store), &iter);
  while (valid)
   {
     HyScanGtkLayer *layer;
     gchar *layer_key;

     gtk_tree_model_get (GTK_TREE_MODEL (priv->layer_store), &iter,
                         LAYER_KEY_COLUMN, &layer_key,
                         LAYER_COLUMN, &layer,
                         -1);

     if (hyscan_gtk_layer_get_visible (layer))
       g_array_append_val (array, layer_key);
     else
       g_free (layer_key);

     g_object_unref (layer);

     valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->layer_store), &iter);
   }

  return (gchar **) g_array_free (array, FALSE);
}

#if !GLIB_CHECK_VERSION (2, 44, 0)
static gboolean
g_strv_contains (const gchar * const *strv,
                 const gchar         *str)
{
  g_return_val_if_fail (strv != NULL, FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  for (; *strv != NULL; strv++)
    {
      if (g_str_equal (str, *strv))
        return TRUE;
    }

  return FALSE;
}
#endif

/* Устанавливает видимость слоёв с ключами layers. */
static void
hyscan_gtk_map_kit_set_layers (HyScanGtkMapKit  *kit,
                               gchar           **layers)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeIter iter;
  gboolean valid;

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->layer_store), &iter);
  while (valid)
   {
     gchar *layer_key;
     HyScanGtkLayer *layer;
     gboolean visible;

     gtk_tree_model_get (GTK_TREE_MODEL (priv->layer_store), &iter,
                         LAYER_KEY_COLUMN, &layer_key,
                         LAYER_COLUMN, &layer,
                         -1);

     visible = g_strv_contains ((const gchar *const *) layers, layer_key);
     gtk_list_store_set (priv->layer_store, &iter, LAYER_VISIBLE_COLUMN, visible, -1);
     hyscan_gtk_layer_set_visible (layer, visible);

     g_free (layer_key);
     g_object_unref (layer);

     valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->layer_store), &iter);
   }
}

/* Устанавливает видимость галса track_name. */
static void
hyscan_gtk_map_kit_track_enable (HyScanGtkMapKit *kit,
                                 const gchar     *track_name,
                                 gboolean         enable)
{
  gchar **tracks;
  gboolean track_enabled;
  const gchar **tracks_new;
  gint i, j;

  tracks = hyscan_gtk_map_kit_get_tracks (kit);
  track_enabled = g_strv_contains ((const gchar *const *) tracks, track_name);

  /* Галс уже и так в нужном состоянии. */
  if ((track_enabled != 0) == (enable != 0))
    {
      g_free (tracks);
      return;
    }

  /* Новый массив = старый массив ± track_name + NULL. */
  tracks_new = g_new (const gchar *, g_strv_length (tracks) + (enable ? 1 : -1) + 1);

  for (i = 0, j = 0; tracks[i] != NULL; ++i)
    {
      if (!enable && g_strcmp0 (tracks[i], track_name) == 0)
        continue;

      tracks_new[j++] = tracks[i];
    }

  if (enable)
    tracks_new[j++] = track_name;

  tracks_new[j] = NULL;

  hyscan_gtk_map_kit_set_tracks (kit, (gchar **) tracks_new);

  g_strfreev (tracks);
  g_free (tracks_new);
}

/* Устанавливает видимость галсов с названиями tracks. */
static void
hyscan_gtk_map_kit_set_tracks (HyScanGtkMapKit  *kit,
                               gchar           **tracks)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  if (priv->track_layer != NULL)
    hyscan_gtk_map_track_set_tracks (HYSCAN_GTK_MAP_TRACK (priv->track_layer), tracks);

  track_tree_view_visible_changed (kit, (const gchar *const *) tracks);
}

/* Устанавливает видимость галсов с названиями tracks. */
static gchar **
hyscan_gtk_map_kit_get_tracks (HyScanGtkMapKit  *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  if (priv->track_layer != NULL)
    return hyscan_gtk_map_track_get_tracks (HYSCAN_GTK_MAP_TRACK (priv->track_layer));

  return g_new0 (gchar *, 1);
}

/* @briefФункция create_wfmark_layer_toolbox создаёт панель инструментов для слоя меток с
 * акустическим изображением.
 * @param layer - указатель на слой с метками с акустическим изображением.
 * */
static GtkWidget *
create_wfmark_layer_toolbox (HyScanGtkLayer *layer)
{
  GtkWidget *box, *combo;

  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), NULL, _("Show acoustic image"));
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), NULL, _("Show mark border only"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

  g_signal_connect (GTK_COMBO_BOX (combo), "changed",
                    G_CALLBACK (hyscan_gtk_map_kit_on_changed_combo_box), layer);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start(GTK_BOX(box), combo, TRUE, TRUE, 10);

  return box;
}
/* @brief Функция-обработчик hyscan_gtk_map_kit_on_changed_combo_box передёт режим отображения
 * меток с акустическим изображением в соответствующий слой.
 * @param combo - указатель на выпадающий список;
 * @param data  - указатель на название проекта.
 * */
static void
hyscan_gtk_map_kit_on_changed_combo_box (GtkComboBox    *combo,
                                         HyScanGtkLayer *layer)
{
  hyscan_gtk_map_wfmark_set_show_mode (HYSCAN_GTK_MAP_WFMARK (layer),
                                       gtk_combo_box_get_active (combo));
}
/**
 * hyscan_gtk_map_kit_new_map:
 * @center: центр карты
 * @db: указатель на #HyScanDB
 * @cache_dir: папка для кэширования тайлов
 *
 * Returns: указатель на структуру #HyScanGtkMapKit, для удаления
 *          hyscan_gtk_map_kit_free().
 */
HyScanGtkMapKit *
hyscan_gtk_map_kit_new (HyScanGeoGeodetic *center,
                        HyScanDB          *db,
                        const gchar       *cache_dir)
{
  HyScanGtkMapKit *kit;

  kit = g_new0 (HyScanGtkMapKit, 1);
  kit->priv = g_new0 (HyScanGtkMapKitPrivate, 1);
  kit->priv->tile_cache_dir = g_strdup (cache_dir);

  hyscan_gtk_map_kit_model_create (kit, db);
  hyscan_gtk_map_kit_view_create (kit);
  hyscan_gtk_map_kit_model_init (kit, center);

  return kit;
}

void
hyscan_gtk_map_kit_set_project (HyScanGtkMapKit *kit,
                                const gchar     *project_name)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  /* Очищаем список активных галсов, только если уже был открыт другой проект. */
  if (priv->project_name != NULL)
    hyscan_gtk_map_kit_set_tracks (kit, (gchar **) { NULL });

  g_free (priv->project_name);
  priv->project_name = g_strdup (project_name);

  if (priv->db_info)
    hyscan_db_info_set_project (priv->db_info, priv->project_name);

  /* Устанавливаем проект во всех сущностях. */
  if (priv->mark_geo_model != NULL)
    hyscan_object_model_set_project (priv->mark_geo_model, priv->db, priv->project_name);

  if (priv->mark_model != NULL)
    hyscan_object_model_set_project (priv->mark_model, priv->db, priv->project_name);

  if (priv->ml_model != NULL)
    hyscan_mark_loc_model_set_project (priv->ml_model, priv->project_name);

  if (priv->track_layer != NULL)
    hyscan_gtk_map_track_set_project (HYSCAN_GTK_MAP_TRACK (priv->track_layer), priv->project_name);
  if (priv->wfmark_layer != NULL)
    hyscan_gtk_map_wfmark_set_project ( HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), priv->project_name);
}

gboolean
hyscan_gtk_map_kit_set_profile_name (HyScanGtkMapKit *kit,
                                     const gchar     *profile_name)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanGtkMap *map = HYSCAN_GTK_MAP (kit->map);
  HyScanProfileMap *profile;

  g_return_val_if_fail (profile_name != NULL, FALSE);

  profile = g_hash_table_lookup (priv->profiles, profile_name);
  if (profile == NULL)
    return FALSE;

  g_free (priv->profile_active);
  priv->profile_active = g_strdup (profile_name);
  hyscan_profile_map_set_offline (profile, priv->profile_offline);
  hyscan_profile_map_apply (profile, map);

  g_signal_handlers_block_by_func (priv->profiles_box, on_profile_change, kit);
  gtk_combo_box_set_active_id (GTK_COMBO_BOX (priv->profiles_box), priv->profile_active);
  g_signal_handlers_unblock_by_func (priv->profiles_box, on_profile_change, kit);

  gtk_statusbar_push (GTK_STATUSBAR (priv->stbar_offline), 0,
                      priv->profile_offline ? _("Offline") : _("Online"));

  return TRUE;
}

gchar *
hyscan_gtk_map_kit_get_profile_name (HyScanGtkMapKit   *kit)
{
  return g_strdup (kit->priv->profile_active);
}

/**
 * hyscan_gtk_map_kit_load_profiles:
 * @kit:
 * @profile_dir: директория с ini-профилями карты
 *
 * Добавляет дополнительные профили карты, загружая из папки profile_dir
 */
void
hyscan_gtk_map_kit_load_profiles (HyScanGtkMapKit *kit,
                                  const gchar     *profile_dir)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar **config_files;
  guint conf_i;

  config_files = list_profiles (profile_dir);
  for (conf_i = 0; config_files[conf_i] != NULL; ++conf_i)
    {
      HyScanProfileMap *profile;
      gchar *profile_name, *extension_ptr;

      profile = hyscan_profile_map_new (priv->tile_cache_dir, config_files[conf_i]);

      /* Обрезаем имя файла по последней точке - это будет название профиля. */
      profile_name = g_path_get_basename (config_files[conf_i]);
      extension_ptr = g_strrstr (profile_name, ".");
      if (extension_ptr != NULL)
        *extension_ptr = '\0';

      /* Читаем профиль и добавляем его в карту. */
      if (hyscan_profile_read (HYSCAN_PROFILE (profile)))
        add_profile (kit, profile_name, profile);

      g_free (profile_name);
      g_object_unref (profile);
    }

  g_strfreev (config_files);
}

/**
 * hyscan_gtk_map_kit_add_nav:
 * @kit:
 * @sensor: датчик GPS-ресивера
 * @sensor_name: имя датчика
 * @offset: (nullable): смещение антенны
 * @delay_time: время задержки навигационных данных
 */
void
hyscan_gtk_map_kit_add_nav (HyScanGtkMapKit           *kit,
                            HyScanSensor              *sensor,
                            const gchar               *sensor_name,
                            const HyScanAntennaOffset *offset,
                            gdouble                    delay_time)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_return_if_fail (priv->nav_model == NULL);

  /* Навигационные данные. */
  priv->nav_model = hyscan_nav_model_new ();
  hyscan_nav_model_set_sensor (priv->nav_model, sensor);
  hyscan_nav_model_set_sensor_name (priv->nav_model, sensor_name);
  hyscan_nav_model_set_offset (priv->nav_model, offset);
  hyscan_nav_model_set_delay (priv->nav_model, delay_time);

  /* Определение местоположения. */
  g_signal_connect_swapped (priv->locate_button, "clicked", G_CALLBACK (on_locate_click), kit);
  gtk_widget_set_sensitive (priv->locate_button, TRUE);

  /* Слой с траекторией движения судна. */
  priv->way_layer = hyscan_gtk_map_nav_new (priv->nav_model);
  add_layer_row (kit, priv->way_layer, "nav", _("Navigation"));
}

void
hyscan_gtk_map_kit_add_marks_wf (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_return_if_fail (priv->mark_model == NULL);
  g_return_if_fail (priv->db != NULL);

  /* Модели меток водопада и их местоположения. */
  priv->mark_model = hyscan_object_model_new (HYSCAN_TYPE_OBJECT_DATA_WFMARK);
  priv->ml_model = hyscan_mark_loc_model_new (priv->db, priv->cache);

  g_signal_connect_swapped (priv->mark_model, "changed", G_CALLBACK (on_marks_changed), kit);

  /* Слой с метками. */
  priv->wfmark_layer = hyscan_gtk_map_wfmark_new (priv->ml_model, priv->db, priv->cache);
  add_layer_row (kit, priv->wfmark_layer, "wfmark", _("Waterfall Marks"));

  gtk_stack_add_titled (GTK_STACK (priv->layer_tool_stack),
                        create_wfmark_layer_toolbox (priv->wfmark_layer),
                        "wfmark", "Wfmark");

  /* Виджет навигации по меткам. */
  create_wfmark_toolbox (kit);

  /* Устанавливаем проект и БД. */
  if (priv->project_name != NULL)
    {
      hyscan_object_model_set_project (priv->mark_model, priv->db, priv->project_name);
      hyscan_mark_loc_model_set_project (priv->ml_model, priv->project_name);
      hyscan_gtk_map_wfmark_set_project (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), priv->project_name);
    }
}

void
hyscan_gtk_map_kit_add_marks_geo (HyScanGtkMapKit   *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_return_if_fail (priv->mark_geo_model == NULL);
  g_return_if_fail (priv->db != NULL);

  /* Модель геометок. */
  priv->mark_geo_model = hyscan_object_model_new (HYSCAN_TYPE_OBJECT_DATA_GEOMARK);
  g_signal_connect_swapped (priv->mark_geo_model, "changed", G_CALLBACK (on_marks_changed), kit);

  /* Слой с геометками. */
  priv->geomark_layer = hyscan_gtk_map_geomark_new (priv->mark_geo_model);
  add_layer_row (kit, priv->geomark_layer, "geomark", _("Geo Marks"));

  /* Виджет навигации по меткам. */
  create_wfmark_toolbox (kit);

  if (priv->project_name != NULL)
    hyscan_object_model_set_project (priv->mark_geo_model, priv->db, priv->project_name);
}

/**
 * hyscan_gtk_map_kit_get_offline:
 * @kit: указатель на #HyScanGtkMapKit
 *
 * Returns: признак режима работы оффлайн
 */
gboolean
hyscan_gtk_map_kit_get_offline (HyScanGtkMapKit *kit)
{
  return kit->priv->profile_offline;
}

/**
 * hyscan_gtk_map_kit_set_offline:
 * @kit: указатель на #HyScanGtkMapKit
 * @offline: признак режима работы оффлайн
 *
 * Устанавливает режим работы @offline и применяет текущий профиль в этом режиме
 */
void
hyscan_gtk_map_kit_set_offline (HyScanGtkMapKit   *kit,
                                gboolean           offline)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar *profile_name;

  profile_name = g_strdup (priv->profile_active);

  priv->profile_offline = offline;
  hyscan_gtk_map_kit_set_profile_name (kit, profile_name);

  g_free (profile_name);
}

/**
 * hyscan_gtk_map_kit_free:
 * @kit: указатель на структуру #HyScanGtkMapKit
 */
void
hyscan_gtk_map_kit_free (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_free (priv->profile_active);
  g_free (priv->tile_cache_dir);
  g_free (priv->project_name);
  g_hash_table_destroy (priv->profiles);
  g_clear_object (&priv->cache);
  g_clear_object (&priv->ml_model);
  g_clear_object (&priv->db);
  g_clear_object (&priv->db_info);
  g_clear_object (&priv->mark_model);
  g_clear_object (&priv->mark_geo_model);
  g_clear_object (&priv->layer_store);
  g_clear_object (&priv->track_store);
  g_clear_object (&priv->mark_store);
  g_free (priv);

  g_free (kit);
}

GtkTreeView *
hyscan_gtk_map_kit_get_track_view (HyScanGtkMapKit *kit,
                                   gint            *track_col)
{
  if (track_col != NULL)
    *track_col = TRACK_COLUMN;
  return kit->priv->track_tree;
}

/**
 * hyscan_gtk_map_kit_kf_setup:
 * @kit
 * @kf
 *
 * Загружает настройки карты из файла @kf
 */
void
hyscan_gtk_map_kit_kf_setup (HyScanGtkMapKit *kit,
                             GKeyFile        *kf)
{
  gchar *profile_name = NULL;
  gboolean offline;
  HyScanGeoGeodetic center;
  gchar **layers, **tracks;
  gchar *cache_dir;

  /* Считываем настройки из файла конфигурации. */
  center.lat   = g_key_file_get_double      (kf, "evo-map", "lat", NULL);
  center.lon   = g_key_file_get_double      (kf, "evo-map", "lon", NULL);
  profile_name = g_key_file_get_string      (kf, "evo-map", "profile", NULL);
  layers       = g_key_file_get_string_list (kf, "evo-map", "layers", NULL, NULL);
  tracks       = g_key_file_get_string_list (kf, "evo-map", "tracks", NULL, NULL);
  cache_dir    = g_key_file_get_string      (kf, "evo-map", "cache", NULL);

  if (g_key_file_has_key (kf, "evo-map", "offline", NULL))
    offline = g_key_file_get_boolean (kf, "evo-map", "offline", NULL);
  else
    offline = TRUE;

  /* Применяем настройки. */
  hyscan_gtk_map_move_to (HYSCAN_GTK_MAP (kit->map), center);

  if (cache_dir != NULL)
    {
      GHashTableIter iter;
      HyScanProfileMap *profile;

      g_free (kit->priv->tile_cache_dir);
      kit->priv->tile_cache_dir = cache_dir;

      g_hash_table_iter_init (&iter, kit->priv->profiles);
      while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &profile))
        hyscan_profile_map_set_cache_dir (profile, kit->priv->tile_cache_dir);

      /* Инициируем повторное применение профиля, чтобы обновить настройки кэша. */
      if (profile_name == NULL)
        profile_name = g_strdup (kit->priv->profile_active);
    }

  if (profile_name != NULL)
    {
      hyscan_gtk_map_kit_set_profile_name (kit, profile_name);
      g_free (profile_name);
    }

  if (layers != NULL)
    {
      hyscan_gtk_map_kit_set_layers (kit, layers);
      g_strfreev (layers);
    }

  if (tracks != NULL)
    {
      hyscan_gtk_map_kit_set_tracks (kit, tracks);
      g_strfreev (tracks);
    }

  hyscan_gtk_map_kit_set_offline (kit, offline);
}

/**
 * hyscan_gtk_map_kit_kf_desetup:
 * @kit
 * @kf
 *
 * Сохраняет настройки карты в файл @kf
 */
void
hyscan_gtk_map_kit_kf_desetup (HyScanGtkMapKit *kit,
                               GKeyFile        *kf)
{
  gchar *proifle;
  gchar **layers, **tracks;
  HyScanGeoGeodetic geod;
  HyScanGeoCartesian2D  c2d;
  gdouble from_x, to_x, from_y, to_y;

  gtk_cifro_area_get_view (GTK_CIFRO_AREA (kit->map), &from_x, &to_x, &from_y, &to_y);
  c2d.x = (to_x + from_x) / 2.0;
  c2d.y = (to_y + from_y) / 2.0;
  hyscan_gtk_map_value_to_geo (HYSCAN_GTK_MAP (kit->map), &geod, c2d);

  proifle = hyscan_gtk_map_kit_get_profile_name (kit);
  layers = hyscan_gtk_map_kit_get_layers (kit);
  tracks = hyscan_gtk_map_kit_get_tracks (kit);

  g_key_file_set_double      (kf, "evo-map", "lat",     geod.lat);
  g_key_file_set_double      (kf, "evo-map", "lon",     geod.lon);
  g_key_file_set_string      (kf, "evo-map", "profile", proifle);
  /* Не сохраняем "evo-map" > "cache", т.к. он настраивается через конфигуратор. */
  /* g_key_file_set_string      (kf, "evo-map", "cache",   kit->priv->tile_cache_dir); */
  g_key_file_set_boolean     (kf, "evo-map", "offline", hyscan_gtk_map_kit_get_offline (kit));
  g_key_file_set_string_list (kf, "evo-map", "layers", (const gchar *const *) layers, g_strv_length (layers));
  g_key_file_set_string_list (kf, "evo-map", "tracks", (const gchar *const *) tracks, g_strv_length (tracks));

  g_free (proifle);
  g_strfreev (tracks);
  g_strfreev (layers);
}

void
hyscan_gtk_map_kit_get_mark_backends (HyScanGtkMapKit     *kit,
                                      HyScanObjectModel  **geo,
                                      HyScanMarkLocModel **wf)
{
  if (geo != NULL)
    *geo = g_object_ref (kit->priv->mark_geo_model);
  if (wf != NULL)
    *wf = g_object_ref (kit->priv->ml_model);
}
