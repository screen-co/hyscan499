#include "hyscan-gtk-map-kit.h"
#include "hyscan-gtk-mark-editor.h"
#include "hyscan-gtk-map-preload.h"
#include "hyscan-gtk-map-go.h"
#include "hyscan-gtk-paned.h"
#include <hyscan-gtk-map-base.h>
#include <hyscan-gtk-map-control.h>
#include <hyscan-gtk-map-ruler.h>
#include <hyscan-gtk-map-grid.h>
#include <hyscan-gtk-map-pin.h>
#include <hyscan-gtk-map-nav.h>
#include <hyscan-gtk-map-track.h>
#include <hyscan-cached.h>
#include <hyscan-profile-map.h>
#include <hyscan-gtk-map-scale.h>
#include <hyscan-gtk-map-wfmark.h>
#include <hyscan-planner-model.h>
#include <hyscan-gtk-map-geomark.h>
#include <hyscan-gtk-layer-list.h>
#include <hyscan-gtk-param-cc.h>

#define GETTEXT_PACKAGE "hyscanfnn-evoui"
#include <glib/gi18n-lib.h>
#include <hyscan-gtk-map-planner.h>
#include <hyscan-gtk-planner-editor.h>
#include <hyscan-gtk-planner-status.h>
#include <hyscan-gtk-planner-origin.h>
#include <hyscan-gtk-planner-zeditor.h>
#include <hyscan-planner-selection.h>
#include <hyscan-gtk-map-steer.h>
#include <hyscan-gtk-planner-list.h>
#include <hyscan-gtk-map-direction.h>
#include <hyscan-track-stats.h>
#include <hyscan-gtk-planner-list-bar.h>
#include <hyscan-gtk-planner-import.h>
#include <hyscan-gtk-param-merge.h>
#include <hyscan-param-merge.h>
#include <hyscan-nav-model.h>
#include <hyscan-config.h>

#define PROFILE_DIR          "map-profiles"
#define PROFILE_EXTENSION    ".ini"
#define DEFAULT_PROFILE_NAME "default"    /* Имя профиля карты по умолчанию. */
#define PRELOAD_STATE_DONE   1000         /* Статус кэширования тайлов 0 "Загрузка завершена". */

#define PLANNER_TOOLS_MODE    "planner-mode"
#define PLANNER_TAB_ZEDITOR   "zeditor"
#define PLANNER_TAB_ORIGIN    "origin"
#define PLANNER_TAB_STATUS    "status"

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
  MARK_OPERATOR_COLUMN,
  MARK_DESCRIPTION_COLUMN,
  MARK_LAT_COLUMN,
  MARK_LON_COLUMN,
  MARK_N_COLUMNS,
};

struct _HyScanGtkMapKitPrivate
{
  /* Модели данных. */
  HyScanGtkModelManager   *model_manager;    /* Менеджер Моделей. */
  HyScanDBInfo            *db_info;          /* Доступ к данным БД. */
  HyScanObjectModel       *mark_model;       /* Модель меток водопада. */
  HyScanPlannerModel      *planner_model;    /* Модель объектов планировщика. */
  HyScanPlannerSelection  *planner_selection;/* Модель выбранных объектов планировщика. */
  HyScanObjectModel       *mark_geo_model;   /* Модель геометок. */
  HyScanMarkLocModel      *ml_model;         /* Модель местоположения меток водопада. */
  HyScanMapTrackModel     *track_model;      /* Модель галсов карты. */
  HyScanUnits             *units;
  HyScanDB                *db;
  HyScanCache             *cache;
  HyScanNavModel          *nav_model;
  gchar                   *project_name;

  gchar                   *recording_track;  /* Имя текущего записываемого галс. */

  GHashTable              *profiles;         /* Хэш-таблица профилей карты. */
  gchar                   *profile_active;   /* Ключ активного профиля. */
  gboolean                 profile_offline;  /* Признак оффлайн-профиля карты. */
  gchar                   *tile_cache_dir;   /* Путь к директории, в которой хранятся тайлы. */

  HyScanGeoPoint           center;           /* Географические координаты для виджета навигации. */

  /* Слои. */
  HyScanGtkLayer          *base_layer;       /* Слой подложки. */
  HyScanGtkLayer          *track_layer;      /* Слой просмотра галсов. */
  HyScanGtkLayer          *wfmark_layer;     /* Слой с метками водопада. */
  HyScanGtkLayer          *geomark_layer;    /* Слой с метками водопада. */
  HyScanGtkLayer          *map_grid;         /* Слой координатной сетки. */
  HyScanGtkLayer          *ruler;            /* Слой линейки. */
  HyScanGtkLayer          *pin_layer;        /* Слой географических отметок. */
  HyScanGtkLayer          *way_layer;        /* Слой текущего положения по GPS-датчику. */
  HyScanGtkLayer          *planner_layer;    /* Слой планировщика. */

  /* Виджеты. */
  GtkWidget               *mark_manager;    /* Журнал меток. */

  GtkWidget               *profiles_box;     /* Выпадающий список профилей карты. */
  GtkWidget               *profile_param;    /* Виджет редактирования параметров профиля. */
  GtkWidget               *planner_stack;    /* GtkStack с виджетами настроек планировщика. */
  GtkButton               *preload_button;   /* Кнопка загрузки тайлов. */
  GtkProgressBar          *preload_progress; /* Индикатор загрузки тайлов. */
  GtkWidget               *steer_box;        /* Контейнер для виджета навигации по отклонениям от плана. */
  HyScanSonarRecorder     *recorder;         /* Управление записью галса. */

  GtkTreeView             *track_tree;       /* GtkTreeView со списком галсов. */
  GtkListStore            *track_store;      /* Модель данных для track_tree. */
  GtkMenu                 *track_menu;       /* Контекстное меню галса (по правой кнопке). */
  GtkWidget               *track_menu_find;  /* Пункт меню поиска галса на карте. */

  GtkTreeView             *mark_tree;       /* GtkTreeView со списком галсов. */
  GtkListStore            *mark_store;      /* Модель данных для track_tree. */

  GtkWidget               *locate_button;   /* Кнопка для определения текущего местоположения. */

  GtkWidget               *mark_editor;     /* Редактор названия меток. */
  GtkWidget               *stbar_offline;   /* Статусбар оффлайн. */
  GtkWidget               *stbar_coord;     /* Статусбар координат. */
  GtkWidget               *layer_list;
  GtkWidget               *steer;           /* Виджет HyScanGtkMapSteer - помощник навигации по галсу. */
  GtkWidget               *planner_list;    /* Виджет HyScanGtkPlannerList - список плановых галсов. */
};

static void     hyscan_gtk_map_kit_set_tracks   (HyScanGtkMapKit      *kit,
                                                 gchar               **tracks);
static gchar ** hyscan_gtk_map_kit_get_tracks   (HyScanGtkMapKit      *kit);
static void     hyscan_gtk_map_kit_track_enable (HyScanGtkMapKit      *kit,
                                                 const gchar          *track_name,
                                                 gboolean              enable);
static void     hyscan_gtk_map_kit_on_changed_combo_box (
                                                 GtkComboBox          *combo,
                                                 HyScanGtkLayer       *layer);

static void     hyscan_gtk_map_kit_project_name_changes (HyScanGtkMapKit  *kit);

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

  if (!g_file_test (profiles_path, G_FILE_TEST_IS_DIR))
    goto exit;

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

      g_dir_close (dir);
    }
  else
    {
      g_warning ("HyScanGtkMapKit: %s", error->message);
      g_error_free (error);
    }

exit:
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
               gboolean         visible,
               const gchar     *key,
               const gchar     *title)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  /* Регистрируем слой в карте. */
  if (layer != NULL)
    {
      hyscan_gtk_layer_set_visible (layer, visible);
      hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (kit->map), layer, key);
    }
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (priv->layer_list), layer, key, title);
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

     active = visible_tracks != NULL && g_strv_contains (visible_tracks, track_name);
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
    hyscan_gtk_map_track_view (HYSCAN_GTK_MAP_TRACK (priv->track_layer), track_name, FALSE, NULL);

  g_free (track_name);
}

static GtkWidget *
create_popup (HyScanGtkMapKit *kit,
              const gchar     *title,
              gboolean         modal)
{
  GtkWidget *popup, *toplevel;

  /* Настраиваем окошко. */
  popup = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (popup), title);
  gtk_window_set_modal (GTK_WINDOW (popup), modal);
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (kit->map));
  if (GTK_IS_WINDOW (toplevel))
    gtk_window_set_transient_for (GTK_WINDOW (popup), GTK_WINDOW (toplevel));

  return popup;
}

static GtkWidget *
create_param_settings_window (HyScanGtkMapKit  *kit,
                              const gchar      *title,
                              HyScanParamMerge *param)
{
  GtkWidget *window;
  GtkWidget *frontend;
  GtkWidget *grid;
  GtkWidget *abar;
  GtkWidget *apply, *discard, *ok, *cancel;
  GtkSizeGroup *size;

  /* Настраиваем окошко. */
  window = create_popup (kit, title, TRUE);
  gtk_window_set_default_size (GTK_WINDOW (window), 250, 300);

  /* Виджет отображения параметров. */
  frontend = hyscan_gtk_param_merge_new_full (param, "/", FALSE);
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
  GList *list, *link;
  HyScanParamMerge *merge;

  selection = gtk_tree_view_get_selection (priv->track_tree);
  list = gtk_tree_selection_get_selected_rows (selection, NULL);

  merge = hyscan_param_merge_new ();
  for (link = list; link != NULL; link = link->next)
    {
      GtkTreePath *path = link->data;
      GtkTreeIter iter;
      gchar *track_name;

      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->track_store), &iter, path))
        {
          HyScanMapTrackParam *param;

          gtk_tree_model_get (GTK_TREE_MODEL (priv->track_store), &iter, TRACK_COLUMN, &track_name, -1);
          param = hyscan_map_track_model_param (priv->track_model, track_name);
          hyscan_param_merge_add (merge, HYSCAN_PARAM (param));
          g_object_unref (param);

          g_free (track_name);
        }
    }

  if (hyscan_param_merge_bind (merge))
    {
      GtkWidget *window;

      window = create_param_settings_window (kit, _("Track settings"), merge);
      gtk_widget_show_all (window);
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

  if (event_button->button != GDK_BUTTON_SECONDARY)
    return GDK_EVENT_PROPAGATE;

  gtk_menu_popup_at_pointer (priv->track_menu, (const GdkEvent *) event_button);

  return GDK_EVENT_STOP;
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
  HyScanMapTrackParam *param = NULL;
  gchar *track_name;
  gboolean has_nmea = FALSE;

  track_name = track_tree_view_get_selected (kit);

  if (track_name != NULL && priv->track_model != NULL)
    {
      param = hyscan_map_track_model_param (priv->track_model, track_name);
      has_nmea = (param != NULL) && hyscan_map_track_param_has_rmc (param);
      g_object_unref (param);
    }

  gtk_widget_set_sensitive (priv->track_menu_find, has_nmea);
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

/* Обработчик выделения строки в списке меток:
 * устанавливает выбранную метку в редактор меток. */
static void
on_marks_selected (GtkTreeSelection *selection,
                   HyScanGtkMapKit   *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkTreeIter iter;
  GType mark_type;

  gchar *mark_id, *mark_name, *operator_name, *description;
  gdouble latitude, longitude;

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (priv->mark_editor), NULL, NULL, NULL, NULL, 0, 0);
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (priv->mark_store), &iter,
                      MARK_ID_COLUMN, &mark_id,
                      MARK_NAME_COLUMN, &mark_name,
                      MARK_OPERATOR_COLUMN, &operator_name,
                      MARK_DESCRIPTION_COLUMN, &description,
                      MARK_LAT_COLUMN, &latitude,
                      MARK_LON_COLUMN, &longitude,
                      MARK_TYPE_COLUMN, &mark_type, -1);

  if (mark_id == NULL)
    goto exit;

  /* Открываем редактор метки. */
  hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (priv->mark_editor), mark_id,
                                   mark_name, operator_name, description,
                                   latitude, longitude);

  /* Выделяем метку на карте. */
  hyscan_gtk_map_wfmark_mark_highlight (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), mark_id);
  hyscan_gtk_map_geomark_mark_highlight (HYSCAN_GTK_MAP_GEOMARK (priv->geomark_layer), mark_id);

exit:
  g_free (mark_id);
  g_free (mark_name);
  g_free (operator_name);
  g_free (description);
}

/* Обновляет имя метки mark_id в модели меток model. */
static void
update_mark (HyScanObjectModel *model,
             const gchar       *mark_id,
             const gchar       *name)
{
  HyScanMark *mark;
  gchar *description, *operator_name;

  mark = (HyScanMark *) hyscan_object_model_get_by_id (model, mark_id);
  if (mark == NULL)
    return;

  description = g_strdup (mark->description);
  operator_name = g_strdup (mark->operator_name);

  hyscan_mark_set_text (mark, name, description, operator_name);
  hyscan_object_model_modify (model, mark_id, (const HyScanObject *) mark);

  hyscan_object_free ((HyScanObject *) mark);
  g_free (description);
  g_free (operator_name);
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
    GType mark_type;

    gtk_tree_model_get (model, &iter, MARK_ID_COLUMN, &mark_id, MARK_TYPE_COLUMN, &mark_type, -1);
    if (mark_type == HYSCAN_TYPE_MARK_WATERFALL && priv->wfmark_layer != NULL)
      hyscan_gtk_map_wfmark_mark_view (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), mark_id, FALSE);
    else if (mark_type == HYSCAN_TYPE_MARK_GEO && priv->geomark_layer != NULL)
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
    hyscan_gtk_map_track_view (HYSCAN_GTK_MAP_TRACK (priv->track_layer), track_name, FALSE, NULL);

    g_free (track_name);
  }
}

/* Удаляет из таблицы меток строки пропавших меток и возвращает хэш таблицу остальных строк. */
static GHashTable *
mark_tree_filter_iters (HyScanGtkMapKit *kit,
                        GType            filter_type,
                        GHashTable      *marks)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GHashTable *gtk_iters;
  GtkTreeIter gtk_iter;
  gboolean valid;

  /* Удаляем строки, которых больше нет. */
  gtk_iters = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_iter_free);
  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->mark_store), &gtk_iter);
  while (valid)
    {
      GType mark_type;
      gchar *mark_id;

      /* Обрабатываем только метки указанного типа. */
      gtk_tree_model_get (GTK_TREE_MODEL (priv->mark_store), &gtk_iter, MARK_TYPE_COLUMN, &mark_type, -1);
      if (mark_type != filter_type)
        {
          valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->mark_store), &gtk_iter);
          continue;
        }

      gtk_tree_model_get (GTK_TREE_MODEL (priv->mark_store), &gtk_iter, MARK_ID_COLUMN, &mark_id, -1);
      if (g_hash_table_contains (marks, mark_id))
        {
          g_hash_table_insert (gtk_iters, mark_id, gtk_tree_iter_copy (&gtk_iter));
          valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->mark_store), &gtk_iter);
        }
      else
        {
          g_free (mark_id);
          valid = gtk_list_store_remove (priv->mark_store, &gtk_iter);
          continue;
        }
    }

  return gtk_iters;
}

/* Добавляет метку в список меток. */
static void
mark_tree_append (HyScanGtkMapKit *kit,
                  GtkTreeIter     *reuse_iter,
                  const gchar     *mark_id,
                  HyScanMark      *mark,
                  gdouble          lat,
                  gdouble          lon)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  GtkTreeIter tree_iter;

  GDateTime *local;
  gchar *time_str;
  gchar *type_name;

  if (mark->type == HYSCAN_TYPE_MARK_WATERFALL && ((HyScanMarkWaterfall *) mark)->track == NULL)
    return;

  /* Добавляем в список меток. */
  local = g_date_time_new_from_unix_local (mark->mtime / 1000000);
  time_str = g_date_time_format (local, "%d.%m %H:%M");

  if (mark->type == HYSCAN_TYPE_MARK_WATERFALL)
    type_name = "W";
  else if (mark->type == HYSCAN_TYPE_MARK_GEO)
    type_name = "G";
  else
    type_name = "?";

  if (reuse_iter == NULL)
    {
      gtk_list_store_append (priv->mark_store, &tree_iter);
      reuse_iter = &tree_iter;
    }

  gtk_list_store_set (priv->mark_store, reuse_iter,
                      MARK_ID_COLUMN, mark_id,
                      MARK_NAME_COLUMN, mark->name,
                      MARK_MTIME_COLUMN, time_str,
                      MARK_MTIME_SORT_COLUMN, mark->mtime,
                      MARK_TYPE_COLUMN, mark->type,
                      MARK_TYPE_NAME_COLUMN, type_name,
                      MARK_LAT_COLUMN, lat,
                      MARK_LON_COLUMN, lon,
                      -1);

  g_free (time_str);
  g_date_time_unref (local);
}

static void
on_marks_changed (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GHashTable *marks;
  GHashTableIter hash_iter;
  gchar *mark_id;

  /* Добавляем метки из всех моделей. */
  if (priv->ml_model != NULL && (marks = hyscan_mark_loc_model_get (priv->ml_model)) != NULL)
    {
      HyScanMarkLocation *location;
      GHashTable *gtk_iters;

      /* Фильтруем строки, которые уже есть. */
      gtk_iters = mark_tree_filter_iters (kit, HYSCAN_TYPE_MARK_WATERFALL, marks);

      g_hash_table_iter_init (&hash_iter, marks);
      while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &location))
        {
          GtkTreeIter *reuse_iter = g_hash_table_lookup (gtk_iters, mark_id);
          mark_tree_append (kit, reuse_iter, mark_id, (HyScanMark *) location->mark,
                            location->mark_geo.lat, location->mark_geo.lon);
        }

      g_hash_table_unref (gtk_iters);
      g_hash_table_unref (marks);
    }

  if (priv->mark_geo_model != NULL && (marks = hyscan_object_model_get (priv->mark_geo_model)) != NULL)
    {
      HyScanMarkGeo *mark;
      GHashTable *gtk_iters;

      /* Фильтруем строки, которые уже есть. */
      gtk_iters = mark_tree_filter_iters (kit, HYSCAN_TYPE_MARK_GEO, marks);

      g_hash_table_iter_init (&hash_iter, marks);
      while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &mark))
        {
          GtkTreeIter *reuse_iter = g_hash_table_lookup (gtk_iters, mark_id);
          mark_tree_append (kit, reuse_iter, mark_id, (HyScanMark *) mark,
                            mark->center.lat, mark->center.lon);
        }

      g_hash_table_unref (gtk_iters);
      g_hash_table_unref (marks);
    }
}

static GtkWidget *
create_mark_editor (HyScanGtkMapKit *kit,
                    GtkTreeView     *mark_tree)
{
  GtkWidget *mark_editor;
  GtkTreeSelection *selection;

  mark_editor = hyscan_gtk_mark_editor_new (kit->priv->units);
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

  priv->mark_store = gtk_list_store_new (MARK_N_COLUMNS,
                                         G_TYPE_STRING,  /* MARK_ID_COLUMN   */
                                         G_TYPE_STRING,  /* MARK_NAME_COLUMN   */
                                         G_TYPE_STRING,  /* MARK_MTIME_COLUMN */
                                         G_TYPE_INT64,   /* MARK_MTIME_SORT_COLUMN */
                                         G_TYPE_GTYPE,   /* MARK_TYPE_COLUMN */
                                         G_TYPE_STRING,  /* MARK_TYPE_NAME_COLUMN */
                                         G_TYPE_STRING,  /* MARK_OPERATOR_COLUMN */
                                         G_TYPE_STRING,  /* MARK_DESCRIPTION_COLUMN */
                                         G_TYPE_DOUBLE,  /* MARK_LAT_COLUMN */
                                         G_TYPE_DOUBLE,  /* MARK_LON_COLUMN */
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
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (priv->track_tree), GTK_SELECTION_MULTIPLE);

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
on_locate_click (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanNavStateData data;

  if (hyscan_nav_state_get (HYSCAN_NAV_STATE (priv->nav_model), &data, NULL))
    hyscan_gtk_map_move_to (HYSCAN_GTK_MAP (kit->map), data.coord);
}

static gboolean
on_motion_show_coords (HyScanGtkMap    *map,
                       GdkEventMotion  *event,
                       HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanGeoPoint geo;
  gchar text[255];
  gchar *lat, *lon;

  hyscan_gtk_map_point_to_geo (map, &geo, event->x, event->y);
  lat = hyscan_units_format (priv->units, HYSCAN_UNIT_TYPE_LAT, geo.lat, 6);
  lon = hyscan_units_format (priv->units, HYSCAN_UNIT_TYPE_LON, geo.lon, 6);
  g_snprintf (text, sizeof (text), "%s, %s", lat, lon);
  gtk_statusbar_push (GTK_STATUSBAR (kit->priv->stbar_coord), 0, text);

  g_free (lat);
  g_free (lon);

  return FALSE;
}

/* Создаёт панель инструментов для слоя булавок и линейки. */
static GtkWidget *
create_ruler_toolbox (HyScanGtkLayer *layer,
                      const gchar    *label)
{
  GtkWidget *ctrl_widget;

  ctrl_widget = gtk_button_new_from_icon_name ("user-trash-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_label (GTK_BUTTON (ctrl_widget), label);
  g_signal_connect_swapped (ctrl_widget, "clicked", G_CALLBACK (hyscan_gtk_map_pin_clear), layer);

  return ctrl_widget;
}

static void
geo_units_change (GtkToggleButton   *button,
                  GParamSpec        *pspec,
                  HyScanGtkMapKit   *kit)
{
  HyScanUnitsGeo geo_units;

  if (!gtk_toggle_button_get_active (button))
    return;

  geo_units = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "unit"));
  hyscan_units_set_geo (kit->priv->units, geo_units);
}

/* Создаёт панель инструментов для слоя сетки. */
static GtkWidget *
create_grid_toolbox (HyScanGtkMapKit *kit)
{
  GtkWidget *box, *btn_widget = NULL;
  HyScanUnitsGeo geo_units;
  guint i;
  struct {
    HyScanUnitsGeo  value;
    const gchar    *label;
    GtkWidget      *widget;
  } btn[] = {
      { HYSCAN_UNITS_GEO_DD,     N_("Decimal Degrees"), NULL },
      { HYSCAN_UNITS_GEO_DDMM,   N_("Degree Minute"), NULL },
      { HYSCAN_UNITS_GEO_DDMMSS, N_("Degree Minute Second"), NULL },
  };

  geo_units = hyscan_units_get_geo (kit->priv->units);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box), gtk_label_new (_("Coordinate Units")), FALSE, TRUE, 0);

  for (i = 0; i < G_N_ELEMENTS (btn); i++)
    {
      btn_widget = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (btn_widget), _(btn[i].label));
      if (geo_units == btn[i].value)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn_widget), TRUE);

      g_object_set_data (G_OBJECT (btn_widget), "unit", GINT_TO_POINTER (btn[i].value));
      g_signal_connect (btn_widget, "notify::active", G_CALLBACK (geo_units_change), kit);
      gtk_box_pack_start (GTK_BOX (box), btn_widget, FALSE, TRUE, 0);
    }

  return box;
}

static void
track_layer_draw_change (GtkToggleButton   *button,
                         GParamSpec        *pspec,
                         HyScanGtkMapTrack *track_layer)
{
  HyScanGtkMapTrackDrawType draw_type;

  if (!gtk_toggle_button_get_active (button))
    return;

  draw_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "draw-type"));
  hyscan_gtk_map_track_set_draw_type (track_layer, draw_type);
}

/* Создаёт панель инструментов для слоя галсов. */
static GtkWidget *
create_track_toolbox (HyScanGtkMapKit *kit)
{
  GtkWidget *bar, *beam, *box;

  bar = gtk_radio_button_new_with_label_from_widget (NULL, _("Distance bars"));
  g_object_set_data (G_OBJECT (bar), "draw-type", GINT_TO_POINTER (HYSCAN_GTK_MAP_TRACK_BAR));
  g_signal_connect (bar, "notify::active", G_CALLBACK (track_layer_draw_change), kit->priv->track_layer);

  beam = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (bar), _("Coverage"));
  g_object_set_data (G_OBJECT (beam), "draw-type", GINT_TO_POINTER (HYSCAN_GTK_MAP_TRACK_BEAM));
  g_signal_connect (beam, "notify::active", G_CALLBACK (track_layer_draw_change), kit->priv->track_layer);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box), bar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), beam, FALSE, TRUE, 0);

  return box;
}

static void
hyscan_gtk_map_planner_mode_changed (GtkToggleButton *togglebutton,
                                     gpointer         user_data)
{
  HyScanGtkMapKit *kit = user_data;
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanGtkMapPlanner *planner = HYSCAN_GTK_MAP_PLANNER (kit->priv->planner_layer);
  HyScanGtkMapPlannerMode mode;
  const gchar *child_name;

  if (!gtk_toggle_button_get_active (togglebutton))
    return;

  mode = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (togglebutton), PLANNER_TOOLS_MODE));

  if (mode == HYSCAN_GTK_MAP_PLANNER_MODE_ORIGIN)
    child_name = PLANNER_TAB_ORIGIN;
  else if (mode == HYSCAN_GTK_MAP_PLANNER_MODE_ZONE)
    child_name = PLANNER_TAB_ZEDITOR;
  else
    child_name = PLANNER_TAB_STATUS;

  hyscan_gtk_map_planner_set_mode (planner, mode);
  gtk_stack_set_visible_child_name (GTK_STACK (priv->planner_stack), child_name);
}

static GtkWidget*
create_planner_mode_btn (HyScanGtkMapKit         *kit,
                         GtkWidget               *from,
                         const gchar             *icon,
                         const gchar              *tooltip,
                         HyScanGtkMapPlannerMode  mode)
{
  GtkWidget *button;

  button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (from));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));
  gtk_button_set_image_position (GTK_BUTTON (button), GTK_POS_TOP);
  gtk_button_set_label (GTK_BUTTON (button), tooltip);

  g_object_set_data (G_OBJECT (button), PLANNER_TOOLS_MODE, GUINT_TO_POINTER (mode));
  g_signal_connect (button, "toggled", G_CALLBACK (hyscan_gtk_map_planner_mode_changed), kit);

  return button;
}

static GtkWidget *
create_planner_zeditor (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *box, *scroll_wnd, *zeditor, *topo_checkbox;

  scroll_wnd = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_wnd), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_wnd), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  zeditor = hyscan_gtk_planner_zeditor_new (priv->planner_model, priv->planner_selection);
  gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (zeditor), GTK_TREE_VIEW_GRID_LINES_BOTH);
  gtk_container_add (GTK_CONTAINER (scroll_wnd), zeditor);

  topo_checkbox = gtk_check_button_new_with_label(_("Topo coordinates (x, y)"));
  g_object_bind_property (topo_checkbox, "active",
                          zeditor, "geodetic", G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box), topo_checkbox, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), scroll_wnd, TRUE, TRUE, 0);

  return box;
}

/* Создаёт панель инструментов для слоя планировщика. */
static GtkWidget *
create_planner_toolbox (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *box;
  GtkWidget *tab_switch;
  GtkWidget *origin_label;
  GtkWidget *tab_status, *tab_origin, *tab_zeditor;
  GtkWidget *zone_mode, *track_mode, *origin_mode;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  priv->planner_stack = gtk_stack_new ();
  gtk_stack_set_vhomogeneous (GTK_STACK (priv->planner_stack), FALSE);

  tab_switch = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  track_mode = create_planner_mode_btn (kit, NULL, "document-new-symbolic", _("Track"),
                                        HYSCAN_GTK_MAP_PLANNER_MODE_TRACK);
  zone_mode = create_planner_mode_btn (kit, track_mode, "folder-new-symbolic", _("Zone"),
                                       HYSCAN_GTK_MAP_PLANNER_MODE_ZONE);
  origin_mode = create_planner_mode_btn (kit, zone_mode, "go-home-symbolic", _("Origin"),
                                         HYSCAN_GTK_MAP_PLANNER_MODE_ORIGIN);

  gtk_style_context_add_class (gtk_widget_get_style_context (tab_switch), GTK_STYLE_CLASS_LINKED);
  gtk_box_pack_start (GTK_BOX (tab_switch), track_mode, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (tab_switch), zone_mode, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (tab_switch), origin_mode, TRUE, TRUE, 0);

  tab_zeditor = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  tab_status = hyscan_gtk_planner_status_new (HYSCAN_GTK_MAP_PLANNER (priv->planner_layer));
  tab_origin = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  origin_label = gtk_label_new (_("Enter parameters of coordinate system or pick it on the map"));
  gtk_label_set_max_width_chars (GTK_LABEL (origin_label), 10);
  gtk_label_set_line_wrap (GTK_LABEL (origin_label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (origin_label), 0.0);
  gtk_box_pack_start (GTK_BOX (tab_origin), origin_label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (tab_origin), hyscan_gtk_planner_origin_new (priv->planner_model), FALSE, TRUE, 0);

  gtk_stack_add_named (GTK_STACK (priv->planner_stack), tab_status, PLANNER_TAB_STATUS);
  gtk_stack_add_named (GTK_STACK (priv->planner_stack), tab_zeditor, PLANNER_TAB_ZEDITOR);
  gtk_stack_add_named (GTK_STACK (priv->planner_stack), tab_origin, PLANNER_TAB_ORIGIN);

  gtk_box_pack_start (GTK_BOX (box), tab_switch, TRUE, FALSE, 6);
  gtk_box_pack_start (GTK_BOX (box), priv->planner_stack, TRUE, FALSE, 6);

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
static GtkWidget *
create_nav_input (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *preload, *go;
  GtkWidget *toplevel;

  toplevel = gtk_notebook_new ();

  go = hyscan_gtk_map_go_new (HYSCAN_GTK_MAP (kit->map));
  g_object_set (go, "margin", 6, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (toplevel), go, gtk_label_new (_("Go to")));

  preload = hyscan_gtk_map_preload_new (HYSCAN_GTK_MAP (kit->map), HYSCAN_GTK_MAP_BASE (priv->base_layer));
  g_object_set (preload, "margin", 6, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (toplevel), preload, gtk_label_new (_("Download")));

  return toplevel;
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

static void
profile_config_apply_clicked (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  HyScanParam *param;
  HyScanProfileMap *profile, *new_profile;
  gchar *file_name, *base_name;
  const gchar *user_dir;

  profile = g_hash_table_lookup (priv->profiles, priv->profile_active);
  if (profile == NULL)
    return;

  hyscan_gtk_param_apply (HYSCAN_GTK_PARAM (priv->profile_param));

  user_dir = hyscan_config_get_user_files_dir ();
  if (user_dir == NULL)
    return;

  /* Создаём копию профиля в пользовательской директории. */
  base_name = g_strdup_printf ("%s%s", priv->profile_active, PROFILE_EXTENSION);
  file_name = g_build_path (G_DIR_SEPARATOR_S, user_dir, PROFILE_DIR, base_name, NULL);
  new_profile = hyscan_profile_map_copy (profile, file_name);

  /* Записываем копию профиля на диск и считываем её обратно. */
  param = hyscan_gtk_layer_container_get_param (HYSCAN_GTK_LAYER_CONTAINER (kit->map));
  hyscan_profile_map_set_param (new_profile, param);
  hyscan_profile_write (HYSCAN_PROFILE (new_profile));
  if (hyscan_profile_read (HYSCAN_PROFILE (new_profile)))
    add_profile (kit, priv->profile_active, new_profile);

  g_object_unref (param);
  g_free (file_name);
  g_free (base_name);
}

static void
profile_param_clear (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  priv->profile_param = NULL;
  gtk_widget_set_sensitive (priv->profiles_box, TRUE);
}

static void
edit_profile_btn_clicked (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *window, *box, *a_bar;
  GtkWidget *btn_apply, *btn_ok, *btn_cancel;
  HyScanProfileMap *profile;
  HyScanParam *map_param;
  gchar *title;

  if (priv->profile_param != NULL || priv->profile_active == NULL)
    return;

  profile = g_hash_table_lookup (priv->profiles, priv->profile_active);
  if (profile == NULL)
    return;

  map_param = hyscan_gtk_layer_container_get_param (HYSCAN_GTK_LAYER_CONTAINER (kit->map));

  title = g_strdup_printf (_("%s - Edit Profile"), hyscan_profile_get_name (HYSCAN_PROFILE (profile)));
  window = create_popup (kit, title, FALSE);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);
  g_free (title);

  gtk_widget_set_sensitive (priv->profiles_box, FALSE);

  g_signal_connect_swapped (window, "destroy", G_CALLBACK (profile_param_clear), kit);

  priv->profile_param = hyscan_gtk_param_cc_new_full (map_param, "/", FALSE);
  btn_apply = gtk_button_new_with_mnemonic (_("_Apply"));
  btn_ok = gtk_button_new_with_mnemonic (_("_OK"));
  btn_cancel = gtk_button_new_with_mnemonic (_("_Cancel"));

  g_signal_connect_swapped (btn_apply,  "clicked", G_CALLBACK (profile_config_apply_clicked), kit);
  g_signal_connect_swapped (btn_ok,     "clicked", G_CALLBACK (profile_config_apply_clicked), kit);
  g_signal_connect_swapped (btn_ok,     "clicked", G_CALLBACK (gtk_widget_destroy), window);
  g_signal_connect_swapped (btn_cancel, "clicked", G_CALLBACK (gtk_widget_destroy), window);

  a_bar = gtk_action_bar_new ();
  gtk_action_bar_pack_end (GTK_ACTION_BAR (a_bar), btn_ok);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (a_bar), btn_apply);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (a_bar), btn_cancel);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box), priv->profile_param, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), a_bar,               FALSE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show_all (window);

  g_object_unref (map_param);
}

/* Кнопки управления виджетом. */
static GtkWidget *
create_control_box (HyScanGtkMapKit *kit)
{
  gint t = 0;
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *ctrl_box, *edit_profile_btn;

  ctrl_box = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (ctrl_box), TRUE);
  gtk_grid_set_column_spacing (GTK_GRID (ctrl_box), 5);
  gtk_grid_set_row_spacing (GTK_GRID (ctrl_box), 3);

  /* Выпадающий список с профилями. */
  priv->profiles_box = gtk_combo_box_text_new ();
  g_signal_connect_swapped (priv->profiles_box , "changed", G_CALLBACK (on_profile_change), kit);

  edit_profile_btn = gtk_button_new_from_icon_name ("document-edit-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (edit_profile_btn, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (edit_profile_btn, GTK_ALIGN_CENTER);
  g_signal_connect_swapped (edit_profile_btn, "clicked", G_CALLBACK (edit_profile_btn_clicked), kit);

  gtk_grid_attach (GTK_GRID (ctrl_box), priv->profiles_box,                             0, ++t, 4, 1);
  gtk_grid_attach (GTK_GRID (ctrl_box), edit_profile_btn,                               4,   t, 1, 1);
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
    GtkWidget *lock_switch;
    GtkWidget *scrolled_wnd;

    /* Контейнер с прокруткой для списка слоёв. */
    scrolled_wnd = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled_wnd), priv->layer_list);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_wnd), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (scrolled_wnd), TRUE);
    gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (scrolled_wnd), TRUE);

    lock_switch = gtk_switch_new ();
    gtk_widget_set_halign (lock_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_active (GTK_SWITCH (lock_switch),
                           !hyscan_gtk_layer_container_get_changes_allowed (HYSCAN_GTK_LAYER_CONTAINER (kit->map)));

    g_signal_connect (lock_switch, "notify::active", G_CALLBACK (on_editable_switch), kit->map);

    gtk_grid_attach (GTK_GRID (ctrl_box), gtk_label_new (_("Lock layers")), 0, ++t, 3, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), lock_switch,                      3,   t, 2, 1);
    gtk_grid_attach (GTK_GRID (ctrl_box), scrolled_wnd,                     0, ++t, 5, 1);
  }

  /* Контейнер для панели инструментов каждого слоя. */
  {
    /* Устаналиваем виджеты с инструментами для каждого слоя. */
    hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "ruler",
                                     create_ruler_toolbox (priv->ruler, _("Remove ruler")));
    hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "pin",
                                     create_ruler_toolbox (priv->pin_layer, _("Remove all pins")));
    hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "base",
                                     create_nav_input (kit));
    hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "grid",
                                     create_grid_toolbox (kit));
    if (priv->track_layer != NULL)
      {
       hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "track",
                                        create_track_toolbox (kit));
      }
  }

  gtk_grid_attach (GTK_GRID (ctrl_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 0, ++t, 5, 1);

  return ctrl_box;
}

/* Создаёт слои, если соответствующие им модели данных доступны. */
static void
create_layers (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  priv->base_layer = hyscan_gtk_map_base_new (priv->cache);
  priv->map_grid = hyscan_gtk_map_grid_new (priv->units);
  priv->ruler = hyscan_gtk_map_ruler_new ();
  priv->pin_layer = hyscan_gtk_map_pin_new ();

  /* Слой с галсами. */
  priv->track_layer = hyscan_gtk_map_track_new (priv->track_model);

  priv->layer_list = hyscan_gtk_layer_list_new (HYSCAN_GTK_LAYER_CONTAINER (kit->map));

  add_layer_row (kit, priv->base_layer,  TRUE,  "base",   _("Base Map"));
  if (priv->track_layer != NULL)
    add_layer_row (kit, priv->track_layer, FALSE, "track",  _("Tracks"));
  add_layer_row (kit, priv->ruler,       TRUE,  "ruler",  _("Ruler"));
  add_layer_row (kit, priv->pin_layer,   TRUE,  "pin",    _("Pin"));
  add_layer_row (kit, priv->map_grid,    TRUE,  "grid",   _("Grid"));
}

/* Создает модели данных. */
static void
hyscan_gtk_map_kit_model_create (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  priv->profiles = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  priv->db = hyscan_gtk_model_manager_get_db (priv->model_manager);
  priv->db_info = hyscan_gtk_model_manager_get_track_model (priv->model_manager);
  priv->track_model = hyscan_map_track_model_new (priv->db, priv->cache);
  hyscan_map_track_model_set_project (priv->track_model, priv->project_name);

  kit->map = create_map (kit);
  create_layers (kit);
}

static void
hyscan_gtk_map_kit_view_create (HyScanGtkMapKit *kit)
{
  kit->navigation = create_navigation_box (kit);
  kit->control    = create_control_box (kit);
  kit->status_bar = create_status_bar (kit);
  kit->paned = hyscan_gtk_paned_new ();
  hyscan_gtk_paned_set_center_widget (HYSCAN_GTK_PANED (kit->paned), kit->map);
}

static void
hyscan_gtk_map_kit_model_init (HyScanGtkMapKit   *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  /* Добавляем профиль по умолчанию. */
  {
    HyScanProfileMap *profile;

    profile = hyscan_profile_map_new_default (priv->tile_cache_dir);

    add_profile (kit, DEFAULT_PROFILE_NAME, profile);
    hyscan_gtk_map_kit_set_profile_name (kit, DEFAULT_PROFILE_NAME);

    g_object_unref (profile);
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

  if (priv->track_model != NULL)
    hyscan_map_track_model_set_tracks (priv->track_model, tracks);

  track_tree_view_visible_changed (kit, (const gchar *const *) tracks);
}

/* Устанавливает видимость галсов с названиями tracks. */
static gchar **
hyscan_gtk_map_kit_get_tracks (HyScanGtkMapKit  *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  if (priv->track_model != NULL)
    return hyscan_map_track_model_get_tracks (priv->track_model);

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

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX(box), combo, TRUE, TRUE, 0);

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

static void
on_cog_len_change (GtkSpinButton   *widget,
                   GParamSpec      *pspec,
                   HyScanGtkMapNav *nav_layer)
{
  gdouble minutes_value;

  minutes_value = gtk_spin_button_get_value (widget);
  hyscan_gtk_map_nav_set_cog_len (nav_layer, minutes_value * 60.0);
}

static void
on_hdg_len_change (GtkSpinButton   *widget,
                   GParamSpec      *pspec,
                   HyScanGtkMapNav *nav_layer)
{
  gdouble km_value;

  km_value = gtk_spin_button_get_value (widget);
  hyscan_gtk_map_nav_set_hdg_len (nav_layer, km_value * 1000.0);
}

static GtkWidget*
nav_tools (HyScanGtkMapKit *kit)
{
  GtkWidget *grid, *cog_spin, *hdg_spin;
  gint t = -1;

  grid = gtk_grid_new ();
  gtk_widget_set_halign (GTK_WIDGET (grid), GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);

  cog_spin = gtk_spin_button_new_with_range (0, 60, 1);
  hdg_spin = gtk_spin_button_new_with_range (0, 100, 0.1);

  g_signal_connect (cog_spin, "notify::value", G_CALLBACK (on_cog_len_change), kit->priv->way_layer);
  g_signal_connect (hdg_spin, "notify::value", G_CALLBACK (on_hdg_len_change), kit->priv->way_layer);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (cog_spin), 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (hdg_spin), 0.2);

  gtk_grid_attach (GTK_GRID (grid), gtk_label_new (_("COG Predictor, min")), 0, ++t, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), cog_spin, 1, t, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), gtk_label_new (_("HDG Predictor, km")), 0, ++t, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), hdg_spin, 1, t, 1, 1);

  return grid;
}

/**
 * hyscan_gtk_map_kit_new_map:
 * @center: центр карты
 * @db: указатель на #HyScanDB
 * @cache: указатель на #HyScanCache
 * @units: единицы измерения
 * @cache_dir: папка для кэширования тайлов
 *
 * Returns: указатель на структуру #HyScanGtkMapKit, для удаления
 *          hyscan_gtk_map_kit_free().
 */
HyScanGtkMapKit *
hyscan_gtk_map_kit_new (HyScanGeoPoint        *center,
                        HyScanGtkModelManager *model_manager,
                        HyScanCache           *cache,
                        HyScanUnits           *units,
                        const gchar           *cache_dir)
{
  HyScanGtkMapKit *kit;
  const gchar **profile_dirs;
  gint i;

  g_return_val_if_fail (HYSCAN_IS_GTK_MODEL_MANAGER (model_manager), NULL);

  kit = g_new0 (HyScanGtkMapKit, 1);
  kit->priv = g_new0 (HyScanGtkMapKitPrivate, 1);
  kit->priv->tile_cache_dir = g_strdup (cache_dir);
  kit->priv->cache = g_object_ref (cache);
  kit->priv->center = *center;
  kit->priv->units = g_object_ref (units);

  /* Сохраняем указатель на Менеджер Моделей. */
  kit->priv->model_manager = g_object_ref (model_manager);
  kit->priv->project_name = g_strdup (hyscan_gtk_model_manager_get_project_name (kit->priv->model_manager));
  /* Подключаем сигнал изменения названия проекта. */
  g_signal_connect_swapped (kit->priv->model_manager,
                            "notify::project-name",
                            G_CALLBACK (hyscan_gtk_map_kit_project_name_changes),
                            kit);

  hyscan_gtk_map_kit_model_create (kit);
  hyscan_gtk_map_kit_view_create (kit);
  hyscan_gtk_map_kit_model_init (kit);

  profile_dirs = hyscan_config_get_profile_dirs ();
  for (i = 0; profile_dirs[i] != NULL; ++i)
    {
      gchar *profile_dir;
      profile_dir = g_build_filename (profile_dirs[i], PROFILE_DIR, NULL);
      hyscan_gtk_map_kit_load_profiles (kit, profile_dir);
      g_free (profile_dir);
    }

  return kit;
}

static void
hyscan_gtk_map_kit_project_name_changes (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  /* Очищаем список активных галсов, только если уже был открыт другой проект. */
  if (priv->project_name != NULL)
    hyscan_gtk_map_kit_set_tracks (kit, (gchar **) { NULL });

  g_free (priv->project_name);
  priv->project_name = g_strdup (hyscan_gtk_model_manager_get_project_name (priv->model_manager));

  if (priv->track_model != NULL)
    hyscan_map_track_model_set_project (priv->track_model, priv->project_name);
  if (priv->wfmark_layer != NULL)
    hyscan_gtk_map_wfmark_set_project (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), priv->project_name);
  if (priv->planner_model != NULL)
    hyscan_object_model_set_project (HYSCAN_OBJECT_MODEL (priv->planner_model), priv->db, priv->project_name);
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
  hyscan_profile_map_apply (profile, map, "base");

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
      gchar *profile_name;

      if (!g_str_has_suffix (config_files[conf_i], PROFILE_EXTENSION))
        continue;

      profile = hyscan_profile_map_new (priv->tile_cache_dir, config_files[conf_i]);
      /* Убираем расширение из имени файла - это будет название профиля. */
      profile_name = g_path_get_basename (config_files[conf_i]);
      profile_name[strlen (profile_name) - strlen (PROFILE_EXTENSION)] = '\0';

      /* Читаем профиль и добавляем его в карту. */
      if (hyscan_profile_read (HYSCAN_PROFILE (profile)))
        add_profile (kit, profile_name, profile);

      g_free (profile_name);
      g_object_unref (profile);
    }

  g_strfreev (config_files);
}

static void
hyscan_gtk_map_kit_add_record (HyScanGtkMapKit *kit,
                               const gchar     *track_id)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar *active_track;
  HyScanPlannerTrack *track;

  g_message ("Adding record: %s", track_id);
  active_track = hyscan_planner_selection_get_active_track (priv->planner_selection);
  if (active_track == NULL)
    return;

  track = (HyScanPlannerTrack *) hyscan_object_model_get_by_id (HYSCAN_OBJECT_MODEL (priv->planner_model), active_track);
  if (!HYSCAN_IS_PLANNER_TRACK (track))
    {
      g_free (active_track);
      return;
    }

  if (track->records == NULL || !g_strv_contains ((const gchar *const *) track->records, track_id))
    {
      hyscan_planner_track_record_append (track, track_id);
      hyscan_object_model_modify (HYSCAN_OBJECT_MODEL (priv->planner_model), active_track,
                                  (const HyScanObject *) track);
    }

  g_free (active_track);
  hyscan_planner_track_free (track);
}

/* Обработчик сигнала HyScanDBInfo::tracks-changed.
 * Добавляет в список плановых галсов информацию о записанных галсах. */
static void
hyscan_gtk_map_kit_rec_watcher (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GHashTable *tracks;
  const gchar *record_id = NULL;

  /* Находим текущий записываемый галс. */
  tracks = hyscan_db_info_get_tracks (priv->db_info);
  if (tracks != NULL)
    {
      GHashTableIter iter;
      HyScanTrackInfo *track_info;

      g_hash_table_iter_init (&iter, tracks);
      while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &track_info) && record_id == NULL)
        {
          if (!track_info->record)
            continue;

          record_id = track_info->id;
        }
    }

  if (g_strcmp0 (record_id, priv->recording_track) != 0)
    {
      g_free (priv->recording_track);
      priv->recording_track = g_strdup (record_id);
      if (priv->recording_track != NULL)
        hyscan_gtk_map_kit_add_record (kit, priv->recording_track);
    }

  g_clear_pointer (&tracks, g_hash_table_destroy);
}

static void
planner_stack_switch_tracks (HyScanPlannerSelection  *selection,
                             const gchar * const     *tracks,
                             GtkStack                *stack)
{
  gchar *zone_id;
  gboolean track_selected;

  track_selected = (tracks[0] != NULL);
  if (track_selected)
    {
      gtk_stack_set_visible_child_name (stack, "track-editor");
      return;
    }

  zone_id = hyscan_planner_selection_get_zone (selection, NULL);
  if (zone_id != NULL)
    {
      gtk_stack_set_visible_child_name (stack, "zone-editor");
      g_free (zone_id);
    }
}

static void
planner_stack_switch_zone (HyScanPlannerSelection  *selection,
                           GtkStack                *stack)
{
  gchar **tracks;

  tracks = hyscan_planner_selection_get_tracks (selection);
  planner_stack_switch_tracks (selection, (const gchar *const *) tracks, stack);
  g_strfreev (tracks);
}

static void
add_steer (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *direction;
  GtkWidget *autostart, *autoselect;
  HyScanSteer *steer;

  /* Отклонение от курса. */
  if (priv->nav_model == NULL || priv->planner_selection == NULL)
    return;

  steer = hyscan_steer_new (HYSCAN_NAV_STATE (priv->nav_model), priv->planner_selection, priv->recorder);
  priv->steer = hyscan_gtk_map_steer_new (steer);

  direction = hyscan_gtk_map_direction_new (HYSCAN_GTK_MAP (kit->map), priv->planner_selection, HYSCAN_NAV_STATE (priv->nav_model));
  autostart = gtk_check_button_new_with_mnemonic (_("_Autostart recording"));
  g_object_bind_property (autostart, "active", steer, "autostart", G_BINDING_SYNC_CREATE);
  autoselect = gtk_check_button_new_with_mnemonic (_("_Select track automatically"));
  g_object_bind_property (autoselect, "active", steer, "autoselect", G_BINDING_SYNC_CREATE);

  gtk_container_add (GTK_CONTAINER (direction), autostart);
  gtk_container_add (GTK_CONTAINER (direction), autoselect);
  gtk_box_pack_start (GTK_BOX (priv->steer_box), priv->steer, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (priv->steer_box), direction,   TRUE, TRUE, 0);
  g_object_unref (steer);
}

/**
 * hyscan_gtk_map_kit_add_nav:
 * @kit:
 * @sensor: датчик GPS-ресивера
 * @sensor_name: имя датчика
 * @recorder: объект управления записью галсов
 * @offset: (nullable): смещение антенны
 * @delay_time: время задержки навигационных данных
 */
void
hyscan_gtk_map_kit_add_nav (HyScanGtkMapKit           *kit,
                            HyScanSensor              *sensor,
                            const gchar               *sensor_name,
                            HyScanSonarRecorder       *recorder,
                            const HyScanAntennaOffset *offset,
                            gdouble                    delay_time)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *box;

  g_return_if_fail (priv->nav_model == NULL);

  /* Навигационные данные. */
  priv->nav_model = hyscan_nav_model_new ();
  hyscan_nav_model_set_sensor (priv->nav_model, sensor);
  hyscan_nav_model_set_sensor_name (priv->nav_model, sensor_name);
  hyscan_nav_model_set_offset (priv->nav_model, offset);
  hyscan_nav_model_set_delay (priv->nav_model, delay_time);

  /* Определение местоположения. */
  priv->locate_button = gtk_button_new_from_icon_name ("network-wireless-signal-good-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_label (GTK_BUTTON (priv->locate_button), _("My location"));
  g_signal_connect_swapped (priv->locate_button, "clicked", G_CALLBACK (on_locate_click), kit);

  /* Слой с траекторией движения судна. */
  priv->way_layer = hyscan_gtk_map_nav_new (HYSCAN_NAV_STATE (priv->nav_model));
  add_layer_row (kit, priv->way_layer, FALSE, "nav", _("Navigation"));

  priv->recorder = g_object_ref (recorder);

  /* Контейнер для виджета навигации по галсу (сам виджет может быть, а может не быть). */
  priv->steer_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box), priv->steer_box, FALSE, TRUE, 6);
  gtk_box_pack_start (GTK_BOX (box), priv->locate_button, FALSE, TRUE, 6);

  hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "nav", box);

  add_steer (kit);
}

/**
 * hyscan_gtk_map_kit_add_marks:
 * @kit: указатель на структуру с GUI
 *
 * Создаёт модели с данными о метках водопада и их местоположении, и гео-метках,
 * создаются виджеты GUI-а.
 */
void
hyscan_gtk_map_kit_add_marks (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  priv->mark_geo_model = hyscan_gtk_model_manager_get_geo_mark_model (priv->model_manager),
  priv->mark_model = hyscan_gtk_model_manager_get_acoustic_mark_model (priv->model_manager);
  priv->ml_model = hyscan_gtk_model_manager_get_acoustic_mark_loc_model (priv->model_manager);

  g_signal_connect_swapped (priv->ml_model, "changed", G_CALLBACK (on_marks_changed), kit);
  g_signal_connect_swapped (priv->mark_geo_model, "changed", G_CALLBACK (on_marks_changed), kit);

  /* Слой с метками. */
  priv->wfmark_layer = hyscan_gtk_map_wfmark_new (priv->ml_model, priv->db, priv->cache, priv->units);
  hyscan_gtk_map_wfmark_set_project (HYSCAN_GTK_MAP_WFMARK (priv->wfmark_layer), priv->project_name);
  add_layer_row (kit, priv->wfmark_layer, FALSE, "wfmark", _("Waterfall Marks"));
  hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "wfmark",
                                   create_wfmark_layer_toolbox (priv->wfmark_layer));

  /* Слой с геометками. */
  priv->geomark_layer = hyscan_gtk_map_geomark_new (priv->mark_geo_model);
  add_layer_row (kit, priv->geomark_layer, FALSE, "geomark", _("Geo Marks"));

  /* Виджет навигации по меткам. */
  create_wfmark_toolbox (kit);
}

void
hyscan_gtk_map_kit_add_planner (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;

  g_return_if_fail (priv->planner_model == NULL);
  g_return_if_fail (priv->db != NULL);

  /* Модель данных планировщика. */
  priv->planner_model = hyscan_planner_model_new ();
  priv->planner_selection = hyscan_planner_selection_new (priv->planner_model);

  /* Слой планировщика. */
  priv->planner_layer = hyscan_gtk_map_planner_new (priv->planner_model, priv->planner_selection);
  add_layer_row (kit, priv->planner_layer, FALSE, "planner", _("Planner"));

  /* Виджет настроек слоя. */
  hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (priv->layer_list), "planner", create_planner_toolbox (kit));

  /* Виджет со списком галсов. */
  {
    GtkWidget *vbox, *hbox, *stack;
    GtkWidget *list_bar, *track_editor, *zone_editor, *scroll_wnd;
    HyScanTrackStats *track_stats;
    HyScanPlannerStats *planner_stats;

    track_stats = hyscan_track_stats_new (priv->db_info, priv->planner_model, priv->cache);
    planner_stats = hyscan_planner_stats_new (track_stats, priv->planner_model);
    g_object_unref (track_stats);

    /* Список схемы галсов помещаем в GtkScrolledWindow. */
    priv->planner_list = hyscan_gtk_planner_list_new (priv->planner_model, priv->planner_selection, planner_stats,
                                                      HYSCAN_GTK_MAP_PLANNER (priv->planner_layer));
    hyscan_gtk_planner_list_enable_binding (HYSCAN_GTK_PLANNER_LIST (priv->planner_list), priv->db_info);

    scroll_wnd = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (scroll_wnd, FALSE);
    gtk_widget_set_vexpand (scroll_wnd, FALSE);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_wnd), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_wnd), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scroll_wnd), 50);
    gtk_container_add (GTK_CONTAINER (scroll_wnd), priv->planner_list);

    /* Панель управления списком схемы галсов. */
    list_bar = hyscan_gtk_planner_list_bar_new (HYSCAN_GTK_PLANNER_LIST (priv->planner_list));

    /* Стек виджетов для редактирования параметров галсов и зон. */
    stack = gtk_stack_new ();
    gtk_widget_set_margin_top (stack, 6);

    track_editor = hyscan_gtk_planner_editor_new (priv->planner_model, priv->planner_selection);
    zone_editor = create_planner_zeditor (kit);
    gtk_stack_add_named (GTK_STACK (stack), track_editor, "track-editor");
    gtk_stack_add_named (GTK_STACK (stack), zone_editor, "zone-editor");

    /* Переключение виджетов в стеке в зависимости от выбранного объекта. */
    g_signal_connect (priv->planner_selection, "tracks-changed", G_CALLBACK (planner_stack_switch_tracks), stack);
    g_signal_connect (priv->planner_selection, "zone-changed", G_CALLBACK (planner_stack_switch_zone), stack);

    /* Компануем все виджеты вместе. */
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start (GTK_BOX (vbox), list_bar, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), scroll_wnd, TRUE, TRUE, 0);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), stack, FALSE, TRUE, 0);

    hyscan_gtk_paned_add (HYSCAN_GTK_PANED (kit->paned), hbox, "tracks", _("Planned tracks"), NULL);
  }

  /* Устанавливаем проект и БД. */
  if (priv->project_name != NULL)
    hyscan_object_model_set_project (HYSCAN_OBJECT_MODEL (priv->planner_model), priv->db, priv->project_name);

  /* Отслеживаем запись в базу данных. */
  g_signal_connect_swapped (priv->db_info, "tracks-changed", G_CALLBACK (hyscan_gtk_map_kit_rec_watcher), kit);

  add_steer (kit);
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
 * hyscan_gtk_map_kit_set_smooth:
 * @kit: указатель на #HyScanGtkMapKit
 * @smooth: сглаживать данные GPS или нет
 *
 *
 */
void
hyscan_gtk_map_kit_set_smooth (HyScanGtkMapKit   *kit,
                               gboolean           smooth)
{
  if (kit->priv->nav_model == NULL)
    return;

  hyscan_nav_model_set_delay (kit->priv->nav_model, smooth ? 1.0 : 0.0);
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
  g_free (priv->recording_track);
  g_hash_table_destroy (priv->profiles);
  g_clear_object (&priv->cache);
  g_clear_object (&priv->ml_model);
  g_clear_object (&priv->db);
  g_clear_object (&priv->db_info);
  g_clear_object (&priv->mark_model);
  g_clear_object (&priv->mark_geo_model);
  g_clear_object (&priv->track_store);
  g_clear_object (&priv->mark_store);
  g_clear_object (&priv->planner_model);
  g_clear_object (&priv->planner_selection);
  g_clear_object (&priv->recorder);
  g_clear_object (&priv->units);
  g_clear_object (&priv->track_model);
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
  HyScanGeoPoint center;
  gchar **layers, **tracks;
  gchar *cache_dir;
  gint planner_cols;

  /* Считываем настройки из файла конфигурации. */
  center.lat   = g_key_file_get_double      (kf, "evo-map", "lat", NULL);
  center.lon   = g_key_file_get_double      (kf, "evo-map", "lon", NULL);
  profile_name = g_key_file_get_string      (kf, "evo-map", "profile", NULL);
  layers       = g_key_file_get_string_list (kf, "evo-map", "layers", NULL, NULL);
  tracks       = g_key_file_get_string_list (kf, "evo-map", "tracks", NULL, NULL);
  cache_dir    = g_key_file_get_string      (kf, "evo-map", "cache", NULL);
  planner_cols = g_key_file_has_key         (kf, "evo-map", "planner-cols", NULL) ?
                 g_key_file_get_integer     (kf, "evo-map", "planner-cols", NULL) :
                 HYSCAN_GTK_PLANNER_LIST_INVALID;

  offline      = g_key_file_has_key         (kf, "evo-map", "offline", NULL) ?
                 g_key_file_get_boolean     (kf, "evo-map", "offline", NULL) :
                 TRUE;

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
      hyscan_gtk_layer_list_set_visible_ids (HYSCAN_GTK_LAYER_LIST (kit->priv->layer_list), layers);
      g_strfreev (layers);
    }

  if (tracks != NULL)
    {
      hyscan_gtk_map_kit_set_tracks (kit, tracks);
      g_strfreev (tracks);
    }

  hyscan_gtk_map_kit_set_offline (kit, offline);

  if (planner_cols != HYSCAN_GTK_PLANNER_LIST_INVALID)
    hyscan_gtk_planner_list_set_visible_cols (HYSCAN_GTK_PLANNER_LIST (kit->priv->planner_list), planner_cols);
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
  HyScanGeoPoint geod;
  HyScanGeoCartesian2D  c2d;
  gdouble from_x, to_x, from_y, to_y;
  gint planner_cols;

  gtk_cifro_area_get_view (GTK_CIFRO_AREA (kit->map), &from_x, &to_x, &from_y, &to_y);
  c2d.x = (to_x + from_x) / 2.0;
  c2d.y = (to_y + from_y) / 2.0;
  hyscan_gtk_map_value_to_geo (HYSCAN_GTK_MAP (kit->map), &geod, c2d);

  proifle = hyscan_gtk_map_kit_get_profile_name (kit);
  layers = hyscan_gtk_layer_list_get_visible_ids (HYSCAN_GTK_LAYER_LIST (kit->priv->layer_list));
  planner_cols = hyscan_gtk_planner_list_get_visible_cols (HYSCAN_GTK_PLANNER_LIST (kit->priv->planner_list));
  tracks = hyscan_gtk_map_kit_get_tracks (kit);

  g_key_file_set_double      (kf, "evo-map", "lat",     geod.lat);
  g_key_file_set_double      (kf, "evo-map", "lon",     geod.lon);
  g_key_file_set_string      (kf, "evo-map", "profile", proifle);
  /* Не сохраняем "evo-map" > "cache", т.к. он настраивается через конфигуратор. */
  /* g_key_file_set_string      (kf, "evo-map", "cache",   kit->priv->tile_cache_dir); */
  g_key_file_set_boolean     (kf, "evo-map", "offline", hyscan_gtk_map_kit_get_offline (kit));
  g_key_file_set_string_list (kf, "evo-map", "layers", (const gchar *const *) layers, g_strv_length (layers));
  g_key_file_set_string_list (kf, "evo-map", "tracks", (const gchar *const *) tracks, g_strv_length (tracks));
  g_key_file_set_integer     (kf, "evo-map", "planner-cols", planner_cols);

  g_free (proifle);
  g_strfreev (tracks);
  g_strfreev (layers);
}

HyScanObjectModel *
hyscan_gtk_map_kit_get_planner (HyScanGtkMapKit *kit)
{
  return kit->priv->planner_model != NULL ? g_object_ref (HYSCAN_OBJECT_MODEL (kit->priv->planner_model)) : NULL;
}

/**
 * hyscan_gtk_map_kit_run_planner_import:
 * @kit: указатель на HyScanGtkMapKit
 *
 * Создаёт окно с виджетом импорта данных планировщика
 */
void
hyscan_gtk_map_kit_run_planner_import (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  GtkWidget *window;
  GtkWidget *box, *import, *abar;
  GtkWidget *start_btn, *close_btn;

  window = create_popup (kit, _("Import Track Plan"), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 200);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  import = hyscan_gtk_planner_import_new (priv->db, priv->project_name);
  g_object_set (import, "margin", 6, NULL);

  start_btn = gtk_button_new_with_mnemonic (_("_Import"));
  close_btn = gtk_button_new_with_mnemonic (_("_Close"));

  g_object_bind_property (import, "ready", start_btn, "sensitive", G_BINDING_SYNC_CREATE);
  g_signal_connect_swapped (start_btn, "clicked", G_CALLBACK (hyscan_gtk_planner_import_start), import);
  g_signal_connect_swapped (close_btn, "clicked", G_CALLBACK (hyscan_gtk_planner_import_stop), import);
  g_signal_connect_swapped (close_btn, "clicked", G_CALLBACK (gtk_widget_destroy), window);

  abar = gtk_action_bar_new ();
  gtk_action_bar_pack_start (GTK_ACTION_BAR (abar), start_btn);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), close_btn);

  gtk_box_pack_start (GTK_BOX (box), import, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), abar, FALSE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);
}

HyScanTrackPlan *
hyscan_gtk_map_kit_get_track_plan (HyScanGtkMapKit *kit)
{
  HyScanGtkMapKitPrivate *priv = kit->priv;
  gchar *active_track;
  HyScanPlannerTrack *track;
  HyScanTrackPlan *track_plan;

  if (priv->planner_selection == NULL)
    return NULL;

  active_track = hyscan_planner_selection_get_active_track (priv->planner_selection);
  if (active_track == NULL)
    return NULL;

  track = (HyScanPlannerTrack *) hyscan_object_model_get_by_id (HYSCAN_OBJECT_MODEL (priv->planner_model), active_track);
  g_free (active_track);

  if (!HYSCAN_IS_PLANNER_TRACK (track))
    return NULL;

  track_plan = hyscan_planner_track_get_plan (track);
  hyscan_planner_track_free (track);

  return track_plan;
}
