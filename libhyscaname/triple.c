#include "triple-types.h"
#include "hyscan-ame-splash.h"
#include <hyscan-ame-project.h>
#include <hyscan-ame-button.h>
#include <hyscan-ame-button.h>
#include <hyscan-gtk-param-tree.h>
#include <gmodule.h>

Global *tglobal;

enum
{
  DATE_SORT_COLUMN,
  TRACK_COLUMN,
  DATE_COLUMN,
  N_COLUMNS
};

void
ame_panel_destroy (gpointer data)
{
  AmePanel *panel = data;
  VisualWF *wf;
  VisualFL *fl;

  if (panel == NULL)
    return;

  g_free (panel->name);
  g_free (panel->sources);

  switch (panel->type)
    {
    case AME_PANEL_WATERFALL:
    case AME_PANEL_ECHO:
      wf = (VisualWF*)panel->vis_gui;

      g_array_unref (wf->colormaps);
      break;

    case AME_PANEL_FORWARDLOOK:
      fl = (VisualFL*)panel->vis_gui;

      g_object_unref (fl->coords);
      break;
    }

  g_free (panel);
}

AmePanel *
get_panel (Global *global,
           gint    panelx)
{
  AmePanel * panel;

  panel = g_hash_table_lookup (global->panels, GINT_TO_POINTER (panelx));

  if (panel == NULL)
    g_warning ("Panel %i not found.", panelx);

  return panel;
}

gint
get_panel_id_by_name (Global      *global,
                      const gchar *name)
{
  GHashTableIter iter;
  gpointer k;
  AmePanel *panel;

  g_return_val_if_fail (name != NULL, -1);

  g_hash_table_iter_init (&iter, global->panels);

  while (g_hash_table_iter_next (&iter, &k, (gpointer*)&panel))
    {
      if (g_str_equal (panel->name, name))
        return GPOINTER_TO_INT (k);
    }

  return -1;
}

void
depth_writer (GObject *emitter)
{
  guint32 first, last, i;
  HyScanAntennaPosition antenna;
  HyScanGeoGeodetic coord;
  GString *string = NULL;
  gchar *words = NULL;
  GError *error = NULL;

  HyScanDB * db = tglobal->db;
  HyScanCache * cache = tglobal->cache;
  gchar *project = tglobal->project_name;
  gchar *track = tglobal->track_name;

  HyScanNavData * dpt;
  HyScanmLoc * mloc;

  dpt = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, cache, project, track,
                                                 1, HYSCAN_SOURCE_NMEA_DPT,
                                                 HYSCAN_NMEA_FIELD_DEPTH));

  if (dpt == NULL)
    {
      g_warning ("Failed to open dpt parser.");
      return;
    }

  mloc = hyscan_mloc_new (db, cache, project, track);
  if (mloc == NULL)
    {
      g_warning ("Failed to open mLocation.");
      g_clear_object (&dpt);
      return;
    }

  string = g_string_new (NULL);
  g_string_append_printf (string, "%s;%s\n", project, track);

  hyscan_nav_data_get_range (dpt, &first, &last);
  antenna = hyscan_nav_data_get_position (dpt);

  for (i = first; i <= last; ++i)
    {
      gdouble val;
      gint64 time;
      gboolean status;
      gchar lat_str[1024] = {'\0'};
      gchar lon_str[1024] = {'\0'};
      gchar dpt_str[1024] = {'\0'};

      status = hyscan_nav_data_get (dpt, i, &time, &val);
      if (!status)
        continue;

      status = hyscan_mloc_get (mloc, time, &antenna, 0, 0, 0, &coord);
      if (!status)
        continue;

      g_ascii_formatd (lat_str, 1024, "%f", coord.lat);
      g_ascii_formatd (lon_str, 1024, "%f", coord.lon);
      g_ascii_formatd (dpt_str, 1024, "%f", val);
      g_string_append_printf (string, "%s;%s;%s\n", lat_str, lon_str, dpt_str);
    }

  words = g_string_free (string, FALSE);

  if (words == NULL)
    return;

  g_file_set_contents ("/srv/hyscan/lat-lon-depth.txt",
                       words, strlen (words), &error);

  if (error != NULL)
    {
      g_message ("Depth save failure: %s", error->message);
      g_error_free (error);
    }

  g_free (words);
}

void
button_set_active (GObject *object,
                   gboolean setting)
{
  if (object == NULL)
    return;

  g_object_set (object, "state", setting, NULL);
}

gint
run_manager (GObject *emitter)
{
  HyScanDBInfo *info;
  GtkWidget *dialog;
  gint res;

  info = hyscan_db_info_new (tglobal->db);
  dialog = hyscan_ame_project_new (tglobal->db, info, GTK_WINDOW (tglobal->gui.window));
  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res == HYSCAN_AME_PROJECT_OPEN || res == HYSCAN_AME_PROJECT_CREATE)
    {
      gchar *project;

      if (tglobal->control_s != NULL)
        start_stop (tglobal, FALSE);

      g_clear_pointer (&tglobal->project_name, g_free);
      hyscan_ame_project_get (HYSCAN_AME_PROJECT (dialog), &project, NULL);

      tglobal->project_name = g_strdup (project);
      hyscan_db_info_set_project (tglobal->db_info, project);

      g_clear_pointer (&tglobal->marks.loc_storage, g_hash_table_unref);
      tglobal->marks.loc_storage = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                        (GDestroyNotify) loc_store_free);

      hyscan_mark_model_set_project (tglobal->marks.model, tglobal->db, project);
      g_free (project);
    }

  g_object_unref (info);
  gtk_widget_destroy (dialog);

  return res;
}

void
run_param (GObject *emitter)
{
  GtkWidget *dialog, *content, *param;
  gint res;

  dialog = gtk_dialog_new_with_buttons("Параметры оборудования",
                                       GTK_WINDOW (tglobal->gui.window), 0,
                                       "Применить", GTK_RESPONSE_APPLY,
                                       "Откатить", GTK_RESPONSE_CANCEL,
                                       "Ок", GTK_RESPONSE_YES,
                                       "Отменить", GTK_RESPONSE_NO,
                                       NULL);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  param = hyscan_gtk_param_tree_new (HYSCAN_PARAM (tglobal->control), "/", FALSE);

  gtk_container_add (GTK_CONTAINER (content), param);
  gtk_widget_set_size_request (dialog, 600, 800);
  gtk_widget_show_all (dialog);

  do
    {
      res = gtk_dialog_run (GTK_DIALOG (dialog));

      if (res == GTK_RESPONSE_YES || res == GTK_RESPONSE_APPLY)
        hyscan_gtk_param_apply (HYSCAN_GTK_PARAM (param));
      if (res == GTK_RESPONSE_NO || res == GTK_RESPONSE_CANCEL)
        hyscan_gtk_param_discard (HYSCAN_GTK_PARAM (param));

    } while (res == GTK_RESPONSE_APPLY || res == GTK_RESPONSE_CANCEL);

  gtk_widget_destroy (dialog);
}

void
turn_meter_helper (AmePanel  *panel,
                   gboolean   state)
{
  void * layer = NULL;
  VisualWF *wf;

  /* Только вотерфольные панели. */
  if (panel->type != AME_PANEL_WATERFALL && panel->type != AME_PANEL_ECHO)
    return;

  wf = (VisualWF*)panel->vis_gui;

  layer = state ? (void*)wf->wf_metr : (void*)wf->wf_ctrl;
  hyscan_gtk_waterfall_layer_grab_input (HYSCAN_GTK_WATERFALL_LAYER (layer));
}

void
turn_meter (GObject     *emitter,
            gboolean     state,
            gint         panelx)
{

  GHashTableIter iter;
  gpointer k, v;

  /* Если передан конкретный селектор, пробуем найти страницу для него. */
  v = g_hash_table_lookup (tglobal->panels, GINT_TO_POINTER (panelx));
  if (v != NULL)
    {
      turn_meter_helper ((AmePanel*)v, state);
    }
  else
    {
      g_hash_table_iter_init (&iter, tglobal->panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        turn_meter_helper ((AmePanel*)v, state);
    }
}

void
turn_mark_helper (AmePanel  *panel,
                  gboolean   state)
{
  void * layer = NULL;
  VisualWF *wf;

  /* Только вотерфольные панели. */
  if (panel->type != AME_PANEL_WATERFALL && panel->type != AME_PANEL_ECHO)
    return;

  wf = (VisualWF*)panel->vis_gui;

  layer = state ? (void*)wf->wf_mark : (void*)wf->wf_ctrl;
  hyscan_gtk_waterfall_layer_grab_input (HYSCAN_GTK_WATERFALL_LAYER (layer));
}

void
turn_marks (GObject     *emitter,
            gint         panelx)
{
  GHashTableIter iter;
  gpointer k, v;

  /* Сначала везде включаем вфконтрол, а потом в требуемом включаем метки. */
  g_hash_table_iter_init (&iter, tglobal->panels);
  while (g_hash_table_iter_next (&iter, &k, &v))
    turn_mark_helper ((AmePanel*)v, FALSE);

  v = g_hash_table_lookup (tglobal->panels, GINT_TO_POINTER (panelx));
  if (v != NULL)
    turn_mark_helper ((AmePanel*)v, TRUE);
}

void
hide_marks (GObject     *emitter,
            gboolean     state,
            gint         selector)
{
  gpointer v;

  v = g_hash_table_lookup (tglobal->panels, GINT_TO_POINTER (selector));
  if (v != NULL)
    {
      AmePanel *panel = v;
      VisualWF *wf;

      if (panel->type != AME_PANEL_WATERFALL && panel->type != AME_PANEL_ECHO)
        return;
      wf = (VisualWF*)panel->vis_gui;

      hyscan_gtk_waterfall_layer_set_visible (HYSCAN_GTK_WATERFALL_LAYER (wf->wf_mark), state);
    }

}

/* Функция изменяет режим окна full screen. */
gboolean
key_press (GtkWidget   *widget,
           GdkEventKey *event,
           Global      *global)
{
  if (event->keyval != GDK_KEY_F11)
    return FALSE;

  if (global->full_screen)
    gtk_window_unfullscreen (GTK_WINDOW (global->gui.window));
  else
    gtk_window_fullscreen (GTK_WINDOW (global->gui.window));

  global->full_screen = !global->full_screen;

  return FALSE;
}

gboolean
idle_key (GdkEvent *event)
{
  gtk_main_do_event (event);
  gdk_event_free (event);
  g_message ("Event! %i %i", event->key.keyval, event->key.state);
  return G_SOURCE_REMOVE;
}

void
nav_common (GtkWidget *target,
            guint      keyval,
            guint      state)
{
  GdkEvent *event;

  event = gdk_event_new (GDK_KEY_PRESS);
  event->key.keyval = keyval;
  event->key.state = state;

  gtk_window_set_focus (GTK_WINDOW (tglobal->gui.window), target);
  event->key.window = g_object_ref (gtk_widget_get_window (target));
  g_idle_add ((GSourceFunc)idle_key, (gpointer)event);
}

/* Макроселло для нав. ф-ий, чтобы их не плодить без надобности. */
#define NAV_FN(fname, button) void \
fname (GObject * emitter, \
       gpointer  udata) \
{ \
  AmePanel *panel = get_panel (tglobal, GPOINTER_TO_INT (udata)); \
  nav_common (panel->vis_gui->main, \
             button, GDK_CONTROL_MASK); \
}

NAV_FN (nav_del, GDK_KEY_Delete)
NAV_FN (nav_pg_up, GDK_KEY_Page_Up)
NAV_FN (nav_pg_down, GDK_KEY_Page_Down)
NAV_FN (nav_up, GDK_KEY_Up)
NAV_FN (nav_down, GDK_KEY_Down)
NAV_FN (nav_left, GDK_KEY_Left)
NAV_FN (nav_right, GDK_KEY_Right)

void
fl_prev (GObject *emitter,
         gint     panelx)
{
  gdouble value;
  GtkAdjustment *adj;
  VisualFL *fl;
  AmePanel *panel = get_panel (tglobal, panelx);

  fl = (VisualFL*)panel->vis_gui;
  adj = fl->position_range;

  value = gtk_adjustment_get_value (adj);
  gtk_adjustment_set_value (adj, value - 1);
}

void
fl_next (GObject *emitter,
         gint     panelx)
{
  gdouble value;
  GtkAdjustment *adj;
  VisualFL *fl;
  AmePanel *panel = get_panel (tglobal, panelx);

  fl = (VisualFL*)panel->vis_gui;
  adj = fl->position_range;

  value = gtk_adjustment_get_value (adj);
  gtk_adjustment_set_value (adj, value + 1);
}

/* Функция вызывается при изменении списка проектов. */
void
projects_changed (HyScanDBInfo *db_info,
                  Global       *global)
{
  GHashTable *projects = hyscan_db_info_get_projects (db_info);

  /* Если рабочий проект есть в списке, мониторим его. */
  if (g_hash_table_lookup (projects, global->project_name))
    hyscan_db_info_set_project (db_info, global->project_name);

  g_hash_table_unref (projects);
}

GtkTreePath *
ame_gtk_tree_model_get_last_path (GtkTreeView *tree)
{
  GtkTreeModel * model;
  GtkTreeIter cur, prev;

  model = gtk_tree_view_get_model (tree);

  /* Чтобы узнать последний элемент, я прохожусь по
   * всем, и запоминаю последний валидный итер. */
  gtk_tree_model_get_iter_first (model, &cur);
  do {
    prev = cur;
  } while (gtk_tree_model_iter_next (model, &cur));

  return gtk_tree_model_get_path (model, &prev);
}

GtkTreePath *
ame_gtk_tree_path_prev (GtkTreePath *path)
{
  gtk_tree_path_prev (path);
  return path;
}

GtkTreePath *
ame_gtk_tree_path_next (GtkTreePath *path)
{
  gtk_tree_path_next (path);
  return path;
}

void
track_scroller (GtkTreeView *tree,
                gboolean     to_top,
                gboolean     to_end)
{
  GtkTreePath *path = NULL; /* Не освобождать! */
  GtkTreePath *real = NULL;
  GtkTreePath *last = NULL;
  GtkTreePath *first = NULL;

  /* Текущий выбранный элемент, первый и последний. */
  gtk_tree_view_get_cursor (tree, &real, NULL);
  first = gtk_tree_path_new_first ();
  last = ame_gtk_tree_model_get_last_path (tree);

  if (to_top) /* Вверх. */
    {
      if (real == NULL || to_end)
        path = first;
      else if (0 == gtk_tree_path_compare (first, real))
        path = last;
      else
        path = ame_gtk_tree_path_prev (real);
    }
  else /* Вниз. */
    {
      if (real == NULL || to_end)
        path = last;
      else if (0 == gtk_tree_path_compare (last, real))
        path = first;
      else
        path = ame_gtk_tree_path_next (real);
    }

  gtk_tree_view_set_cursor (tree, path, NULL, FALSE);

  gtk_tree_path_free (real);
  gtk_tree_path_free (last);
  gtk_tree_path_free (first);
}

/* Функция вызывается при изменении списка галсов. */
void
tracks_changed (HyScanDBInfo *db_info,
                Global       *global)
{
  GtkTreePath *null_path;
  GtkTreeIter tree_iter;
  GHashTable *tracks;
  GHashTableIter hash_iter;
  gpointer key, value;
  gchar *cur_track_name;

  cur_track_name = g_strdup (global->track_name);

  null_path = gtk_tree_path_new ();
  gtk_tree_view_set_cursor (global->gui.track.tree, null_path, NULL, FALSE);
  gtk_tree_path_free (null_path);

  gtk_list_store_clear (GTK_LIST_STORE (global->gui.track.list));

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&hash_iter, tracks);

  while (g_hash_table_iter_next (&hash_iter, &key, &value))
    {
      HyScanTrackInfo *track_info = value;

      /* Добавляем в список галсов. */
      gtk_list_store_append (GTK_LIST_STORE (global->gui.track.list), &tree_iter);
      gtk_list_store_set (GTK_LIST_STORE (global->gui.track.list), &tree_iter,
                          DATE_SORT_COLUMN, g_date_time_to_unix (track_info->ctime),
                          TRACK_COLUMN, g_strdup (track_info->name),
                          DATE_COLUMN, g_date_time_format (track_info->ctime, "%d.%m %H:%M"),
                          -1);

      /* Подсвечиваем текущий галс. */
      if (g_strcmp0 (cur_track_name, track_info->name) == 0)
        {
          GtkTreePath *tree_path = gtk_tree_model_get_path (global->gui.track.list, &tree_iter);
          gtk_tree_view_set_cursor (global->gui.track.tree, tree_path, NULL, FALSE);
          gtk_tree_path_free (tree_path);
        }
    }

  g_hash_table_unref (tracks);
  g_free (cur_track_name);
}

void
active_mark_changed (HyScanGtkProjectViewer *marks_viewer,
                     Global                 *global)
{
  const gchar *mark_id;
  MarkAndLocation *mark;

  mark_id = hyscan_gtk_project_viewer_get_selected_item (marks_viewer);

  if ((mark = g_hash_table_lookup (global->marks.current, mark_id)) != NULL)
    {
      hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (global->gui.meditor),
                                       mark_id,
                                       mark->mark->name,
                                       mark->mark->operator_name,
                                       mark->mark->description,
                                       mark->lat,
                                       mark->lon);
    }
}

inline gboolean
ame_float_equal (gdouble a, gdouble b)
{
  return (ABS(a - b) < 1e-6);
}

gboolean
marks_equal (MarkAndLocation *a,
             MarkAndLocation *b)
{
  gboolean coords;
  gboolean name;
  gboolean size;

  if (a == NULL || b == NULL)
    {
      g_warning ("Null mark. It's a mistake");
      return FALSE;
    }

  coords = ame_float_equal (a->lat, b->lat) && ame_float_equal (a->lon, b->lon);
  name = g_strcmp0 (a->mark->name, b->mark->name) == 0;
  size = (a->mark->width == b->mark->width) && (a->mark->height == b->mark->height);

  return (coords && name && size);
}

void
mark_processor (gpointer        key,
                gpointer        value,
                GHashTable     *source,
                HyScanMarkSync *sync,
                gboolean        this_is_an_old_list)
{
  MarkAndLocation *our, *their;

  our = value;
  their = g_hash_table_lookup (source, key);

  /* Логика различается для текущего и предыдущего списка меток.
   * Если ищем в старом, то ненайденную создаем.
   * Если ищем в новом, то ненайденную удаляем. */
  if (this_is_an_old_list && their == NULL)
    {
      hyscan_mark_sync_remove (sync, key);
      return;
    }
  else if (this_is_an_old_list && their != NULL)
    {
      return;
    }

  if (their == NULL)
    {
      gdouble radius = sqrt (pow (our->mark->width, 2) + pow (our->mark->height, 2)) / 1e3;
      hyscan_mark_sync_set (sync, key, our->mark->name, our->lat, our->lon, radius);
      return;
    }

  /* Если была, но изменилась - удалить и переотправить*/
  if (!marks_equal (our, their))
    {
      gdouble radius = sqrt (pow (our->mark->width, 2) + pow (our->mark->height, 2)) / 1e3;
      hyscan_mark_sync_remove (sync, key);
      hyscan_mark_sync_set (sync, key, our->mark->name, our->lat, our->lon, radius);
      return;
    }
}

void
mark_sync_func (GHashTable *marks,
                Global     *global)
{
  GHashTableIter iter;
  gpointer key, value;

  if (global->marks.previous == NULL)
    {
      global->marks.previous = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                      (GDestroyNotify)mark_and_location_free);
      goto copy;
    }

  /* Надо пройтись сначала по старому, чтобы знать, что удалилось.
   * А потом по актуальному списку, посмотреть,
   * что появилось нового, а что поменялось */

  g_hash_table_iter_init (&iter, global->marks.previous);
  while (g_hash_table_iter_next (&iter, &key, &value))
    mark_processor (key, value, marks, global->marks.sync, TRUE);

  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    mark_processor (key, value, global->marks.previous, global->marks.sync, FALSE);

  g_hash_table_remove_all (global->marks.previous);

  copy:
  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_hash_table_insert (global->marks.previous, g_strdup (key), mark_and_location_copy (value));
}


/* создает LocStore. */
LocStore *
loc_store_new (HyScanDB    *db,
               HyScanCache *cache,
               const gchar *project,
               const gchar *track)
{
  LocStore *loc;

  loc = g_new0 (LocStore, 1);
  loc->track = g_strdup (track);
  loc->mloc = hyscan_mloc_new (db, cache, project, track);
  loc->projectors = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
                                           (GDestroyNotify)g_object_unref);
  loc->factory = hyscan_amplitude_factory_new (cache);
  hyscan_amplitude_factory_set_track (loc->factory, db, project, track);

  if (loc->mloc == NULL || loc->factory == NULL)
    {
      g_clear_pointer (&loc, loc_store_free);
    }

  return loc;
}

/* Возвращает (или создает и возврщает) прожектор для галса и источника. */
HyScanProjector *
get_projector (GHashTable       *locstores,
               gchar            *track,
               HyScanSourceType  source,
               HyScanmLoc       **mloc,
               HyScanAmplitude  **_amp)
{
  LocStore * store;
  HyScanProjector * pj;
  HyScanAmplitude * amp;
  gpointer pj_key = GINT_TO_POINTER (source);
  gpointer amp_key = GINT_TO_POINTER (10007 * source);

  /* Ищем хранилище проекторов для галса. */
  store = g_hash_table_lookup (locstores, track);

  if (store == NULL)
    return NULL;

  /* Ищем мини локейшн. */
  if (mloc != NULL)
    *mloc = store->mloc;

  /* Ищем проектор по источнику. */
  pj = g_hash_table_lookup (store->projectors, pj_key);

  /* Если нашли, возвращаем. */
  if (pj != NULL)
    {
      amp = g_hash_table_lookup (store->projectors, amp_key);
      if (_amp != NULL)
        *_amp = amp;

      return pj;
    }

  /* Если не нашли, создаем с помощью фабрики. */
  amp = hyscan_amplitude_factory_produce (store->factory, source);
  pj = hyscan_projector_new (amp);

  if (pj == NULL)
    return NULL;

  /* Закидываем проектор и амплитуддата в таблицу. */
  g_hash_table_insert (store->projectors, pj_key, pj);
  g_hash_table_insert (store->projectors, amp_key, amp);

  return pj;
}

/* Освобождение структуры LocStore */
void
loc_store_free (gpointer data)
{
  LocStore *loc = data;
  g_clear_pointer (&loc->track, g_free);
  g_clear_object  (&loc->mloc);
  g_clear_object  (&loc->factory);
  g_clear_pointer (&loc->projectors, g_hash_table_unref);
}

/* Создание структуры MarkAndLocation */
MarkAndLocation *
mark_and_location_new (HyScanWaterfallMark *mark,
                       gdouble              lat,
                       gdouble              lon)
{
  MarkAndLocation *dst = g_new (MarkAndLocation, 1);
  dst->mark = hyscan_waterfall_mark_copy (mark);
  dst->lat = lat;
  dst->lon = lon;

  return dst;
}

/* Копирование структуры MarkAndLocation */
MarkAndLocation *
mark_and_location_copy (gpointer data)
{
  MarkAndLocation *src = data;
  MarkAndLocation *dst = g_new (MarkAndLocation, 1);
  dst->mark = hyscan_waterfall_mark_copy (src->mark);
  dst->lat = src->lat;
  dst->lon = src->lon;

  return dst;
}

/* Освобождение структуры MarkAndLocation */
void
mark_and_location_free (gpointer data)
{
  MarkAndLocation *loc = data;
  hyscan_waterfall_mark_free (loc->mark);
  g_free (loc);
}

/* Функция определяет координаты метки. */
MarkAndLocation *
get_mark_coords (GHashTable             * locstores,
                 HyScanWaterfallMark    * mark,
                 Global                 * global)
{
  gdouble along, across;
  HyScanProjector *pj;
  HyScanAmplitude *amp;
  HyScanAntennaPosition apos;
  gint64 time;
  guint32 n;
  HyScanGeoGeodetic position;
  HyScanmLoc *mloc;

  if (!g_hash_table_contains (locstores, mark->track))
    {
      LocStore *ls;
      ls = loc_store_new (global->db, global->cache, global->project_name, mark->track);

      g_hash_table_insert (locstores, g_strdup (mark->track), ls);
    }

  pj = get_projector (locstores, mark->track, mark->source0, &mloc, &amp);

  // hyscan_projector_index_to_coord (pj, mark->index0, &along);
  hyscan_projector_count_to_coord (pj, mark->count0, &across, 0);

  apos = hyscan_amplitude_get_position (amp);
  hyscan_amplitude_get_amplitude (amp, mark->index0, &n, &time, NULL);

  if (mark->source0 == HYSCAN_SOURCE_SIDE_SCAN_PORT)
    across *= -1;

  hyscan_mloc_get (mloc, time, &apos, across, 0, 0, &position);
  #pragma message ("check axis names in the line above")

  return mark_and_location_new (mark, position.lat, position.lon);
}

/* Возвращает таблицу меток с координатами. */
GHashTable *
make_marks_with_coords (HyScanMarkModel *model,
                        Global          *global)
{
  GHashTable *pure_marks;
  GHashTable *marks;
  GHashTableIter iter;
  MarkAndLocation *mark_ll;
  gpointer key, value;

  /* Достаем голенькие метки. */
  if ((pure_marks = hyscan_mark_model_get (model)) == NULL)
    return NULL;

  /* Заводим таблицу под метки. */
  marks = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)mark_and_location_free);

  /* Теперь приделываем к ним координаты. */
  g_hash_table_iter_init (&iter, pure_marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      HyScanWaterfallMark *wfmark = value;
      mark_ll = get_mark_coords (global->marks.loc_storage, wfmark, global);

      g_hash_table_insert (marks, g_strdup (wfmark->track), mark_ll);
    }

  return marks;
}

void
mark_model_changed (HyScanMarkModel *model,
                    Global          *global)
{
  GtkTreeIter tree_iter;
  GHashTable *marks;
  GHashTableIter iter;
  gpointer key, value;
  GtkListStore *ls;

  hyscan_gtk_project_viewer_clear (HYSCAN_GTK_PROJECT_VIEWER (global->gui.mark_view));

  /* Получаем крутецкий список меток с координатами. */
  marks = make_marks_with_coords (model, global);
  ls = hyscan_gtk_project_viewer_get_liststore (HYSCAN_GTK_PROJECT_VIEWER (global->gui.mark_view));

  /* Проходим по всем существующим меткам... */
  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const gchar *mark_id = key;
      MarkAndLocation *mark_ll = value;
      GDateTime *mtime;
      gchar *mtime_str;

      mtime = g_date_time_new_from_unix_local (mark_ll->mark->modification_time / 1e6);
      mtime_str = g_date_time_format (mtime, "%d.%m %H:%M");

      gtk_list_store_append (ls, &tree_iter);
      gtk_list_store_set (ls, &tree_iter,
                          0, mark_id,
                          1, mark_ll->mark->name,
                          2, mtime_str,
                          3, mark_ll->mark->modification_time,
                          -1);

      g_free (mtime_str);
      g_date_time_unref (mtime);
    }

  /* Сравниваем с предыдущими метками. Если надо, отсылаем наружу. */
  mark_sync_func (marks, global);
}

void
mark_modified (HyScanGtkMarkEditor *med,
               Global              *global)
{
  gchar *mark_id = NULL;
  GHashTable *marks;
  HyScanWaterfallMark *mark;

  hyscan_gtk_mark_editor_get_mark (med, &mark_id, NULL, NULL, NULL);

  if ((marks = hyscan_mark_model_get (global->marks.model)) == NULL)
    return;

  if ((mark = g_hash_table_lookup (marks, mark_id)) != NULL)
    {
      HyScanWaterfallMark *modified_mark = hyscan_waterfall_mark_copy (mark);
      g_clear_pointer (&modified_mark->name, g_free);
      g_clear_pointer (&modified_mark->operator_name, g_free);
      g_clear_pointer (&modified_mark->description, g_free);

      hyscan_gtk_mark_editor_get_mark (med,
                                       NULL,
                                       &modified_mark->name,
                                       &modified_mark->operator_name,
                                       &modified_mark->description);

      hyscan_mark_model_modify_mark (global->marks.model, mark_id, modified_mark);

      hyscan_waterfall_mark_free (modified_mark);
    }

  g_free (mark_id);
  g_hash_table_unref (marks);
}

/* Функция прокручивает список галсов. */
gboolean
track_scroll (GtkWidget *widget,
              GdkEvent  *event,
              Global    *global)
{
  gdouble position;
  gdouble step_x, step_y;
  gint size_x, size_y;

  position = gtk_adjustment_get_value (global->gui.track.scroll);
  gdk_event_get_scroll_deltas (event, &step_x, &step_y);
  gtk_tree_view_convert_bin_window_to_widget_coords (global->gui.track.tree, 0, 0, &size_x, &size_y);
  position += step_y * size_y;

  gtk_adjustment_set_value (global->gui.track.scroll, position);

  return TRUE;
}

gchar *
get_active_track (HyScanDBInfo *db_info)
{
  GHashTable *tracks;
  HyScanTrackInfo *current;
  GHashTableIter iter;
  gpointer key, val;
  gchar * active = NULL;

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&iter, tracks);
  while (g_hash_table_iter_next (&iter, &key, &val))
    {
      current = val;

      if (current->active)
        {
          active = g_strdup (current->name);
          break;
        }

    }

  g_hash_table_unref (tracks);
  return active;
}

gboolean
track_is_active (HyScanDBInfo *db_info,
                 const gchar  *name)
{
  GHashTable *tracks;
  HyScanTrackInfo *current;
  GHashTableIter iter;
  gpointer key, val;
  gboolean active = FALSE;

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&iter, tracks);
  while (g_hash_table_iter_next (&iter, &key, &val))
    {
      current = val;

      if (g_str_equal (current->name, name))
        {
          active = current->active;
          break;
        }

    }

  g_hash_table_unref (tracks);
  return active;
}

/* Обработчик изменения галса. */
void
track_changed (GtkTreeView *list,
               Global      *global)
{
  GValue value = G_VALUE_INIT;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  GHashTableIter htiter;
  const gchar *track_name;
  gpointer k, v;
  gboolean on_air;


  /* Определяем название нового галса. */
  gtk_tree_view_get_cursor (list, &path, NULL);
  if (path == NULL)
    return;

  if (!gtk_tree_model_get_iter (global->gui.track.list, &iter, path))
    return;

  g_clear_pointer (&global->track_name, g_free);

  /* Открываем новый галс. */
  gtk_tree_model_get_value (global->gui.track.list, &iter, TRACK_COLUMN, &value);
  track_name = g_value_get_string (&value);
  global->track_name = g_strdup (track_name);

  /* Выясняем, ведется ли в него запись. */
  on_air = track_is_active (global->db_info, track_name);

  g_hash_table_iter_init (&htiter, global->panels);
  while (g_hash_table_iter_next (&htiter, &k, &v))
    {
      VisualWF *wf;
      VisualFL *fl;
      AmePanel *panel = v;

      switch (panel->type)
        {
        case AME_PANEL_WATERFALL:
        case AME_PANEL_ECHO:
          wf = (VisualWF*) (panel->vis_gui);
          hyscan_gtk_waterfall_state_set_track (HYSCAN_GTK_WATERFALL_STATE (wf->wf),
                                                global->db, global->project_name, track_name);

          /* Записываемый галс включаем в режиме автопрокрутки. */
          hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (wf->wf), on_air);

          break;

        case AME_PANEL_FORWARDLOOK:
          fl = (VisualFL*) (panel->vis_gui);
          hyscan_forward_look_player_open (fl->player, global->db, global->cache,
                                           global->project_name, track_name);
          hyscan_fl_coords_set_project (fl->coords, global->db, global->project_name, track_name);

          if (on_air)
            hyscan_forward_look_player_real_time (fl->player);
          else
            hyscan_forward_look_player_pause (fl->player);

          break;
        }
    }

  g_value_unset (&value);
}

/* Функция обрабатывает данные для текущего индекса. */
void
position_changed (GtkAdjustment *range,
                  Global        *global)
{
}

gdouble
fl_current_scale (HyScanGtkForwardLook *fl)
{
  gdouble from_y, to_y, min_y, max_y, scale;
  GtkCifroArea *carea;

  carea = GTK_CIFRO_AREA (fl);

  gtk_cifro_area_get_view (carea, NULL, NULL, &from_y, &to_y);
  gtk_cifro_area_get_limits (carea, NULL, NULL, &min_y, &max_y);

  if (max_y == min_y)
    return -1;

  scale = 100.0 * (fabs (to_y - from_y) / fabs (max_y - min_y));

  if (isnan(scale) || isinf(scale))
    return -1;

  return scale;
}

/* Функция устанавливает масштаб отображения. */
void
zoom_changed (HyScanGtkWaterfall *wfall,
              gint                index,
              gdouble             gost,
              gint                panelx)
{
  gchar *text = NULL;
  AmePanel *panel = get_panel (tglobal, panelx);

  switch (panel->type)
    {
    case AME_PANEL_WATERFALL:
    case AME_PANEL_ECHO:
      break;

    case AME_PANEL_FORWARDLOOK:
    default:
      g_warning ("zoom_changed: wrong panel type!");
      return;
    }

  text = g_strdup_printf ("<small><b>1:%.0f</b></small>", gost);
  gtk_label_set_markup (panel->vis_gui->scale_value, text);
  g_free (text);
  return;
}

/* Функция устанавливает масштаб отображения. */
gboolean
scale_set (Global   *global,
           gboolean  scale_up,
           gint      panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gchar *text = NULL;
  const gdouble *scales;
  gint n_scales, i_scale;
  gdouble scale;
  AmePanel *panel = get_panel (global, panelx);

  switch (panel->type)
    {
    case AME_PANEL_WATERFALL:
    case AME_PANEL_ECHO:
      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_control_zoom (wf->wf_ctrl, scale_up);

      i_scale = hyscan_gtk_waterfall_get_scale (HYSCAN_GTK_WATERFALL (wf->wf),
                                                &scales, &n_scales);
      scale = scales[i_scale];
      text = g_strdup_printf ("<small><b>1:%.0f</b></small>", scale);
      break;

    case AME_PANEL_FORWARDLOOK:
      {
        GtkCifroArea *carea;
        GtkCifroAreaZoomType zoom_dir;

        fl = (VisualFL*)panel->vis_gui;
        carea = GTK_CIFRO_AREA (fl->fl);

        scale = fl_current_scale (fl->fl);

        if (scale_up && (scale < 5.0))
          return FALSE;

        if (!scale_up && (scale > 100.0))
          return FALSE;

        zoom_dir = (scale_up) ? GTK_CIFRO_AREA_ZOOM_IN : GTK_CIFRO_AREA_ZOOM_OUT;
        gtk_cifro_area_zoom (carea, zoom_dir, zoom_dir, 0.0, 0.0);

        scale = fl_current_scale (fl->fl);
        if (scale == -1)
          text = g_strdup_printf ("<small><b>N/A</b></small>");
        else
          text = g_strdup_printf ("<small><b>%.0f%%</b></small>", scale);
      }
      break;

    default:
      g_warning ("scale_set: wrong panel type!");
    }

  gtk_label_set_markup (panel->vis_gui->scale_value, text);
  g_free (text);
  return TRUE;
}

#define INCR(color,next) if (incr_##color)   \
                    {                        \
                      if (++color == 255)      \
                        {incr_##color = FALSE;\
                         next = TRUE; continue;} \
                    }
#define DECR(color,next) if (decr_##color)   \
                    {                        \
                      if (--color == 0)      \
                        {decr_##color = FALSE;\
                         next = TRUE; continue;} \
                    }
guint32*
hyscan_tile_color_compose_colormap_pf (guint *length)
{
  guint32 *out;
  guint len = 1022;
  len = 1120; //m_edition
  guint i;
  gboolean incr_g = FALSE;
  gboolean decr_b = FALSE;
  gboolean incr_r = FALSE;
  gboolean decr_g = FALSE;
  gboolean decr_r = TRUE;
  guchar r = 0, g = 0, b = 255;
  out = g_malloc0 (len * sizeof (guint32));

  out[0] = hyscan_tile_color_converter_c2i (255, 255, 255, 255);

  r=99;
  g=0;
  b=255;

  for (i = 1; i < len; ++i)
    {
      out[i] = hyscan_tile_color_converter_c2i (r, g, b, 255);
      DECR(r, incr_g);
      INCR(g, decr_b);
      DECR(b, incr_r);
      INCR(r, decr_g);
      DECR(g, decr_g);

      // INCR (g, decr_b);
      // DECR (b, incr_r);
      // INCR (r, decr_g);
      // DECR (g, decr_g);
    }

  if (length != NULL)
    *length = len;
  return out;
}

/* Функция устанавливает новую палитру. */
gboolean
color_map_set (Global *global,
               guint   desired_cmap,
               gint    panelx)
{
  VisualWF *wf;
  gchar   *text;
  AmeColormap *colormap;
  AmePanel *panel = get_panel (global, panelx);

  if (desired_cmap >= MAX_COLOR_MAPS)
    return FALSE;

  /* Проверяем тип панели. */
  if (panel->type != AME_PANEL_WATERFALL && panel->type != AME_PANEL_ECHO)
    {
      g_warning ("color_map_set: wrong panel type");
      return FALSE;
    }

  wf = (VisualWF*)panel->vis_gui;
  colormap = g_array_index (wf->colormaps, AmeColormap*, desired_cmap);

  hyscan_gtk_waterfall_set_colormap_for_all (wf->wf,
                                             colormap->colors,
                                             colormap->len,
                                             colormap->bg);
  hyscan_gtk_waterfall_set_substrate (wf->wf, colormap->bg);

  /* Проверяем, существует ли вообще виджет. */
  if (wf->common.colormap_value != NULL)
    {
      text = g_strdup_printf ("<small><b>%s</b></small>", colormap->name);
      gtk_label_set_markup (wf->common.colormap_value, text);
      g_free (text);
    }

  return TRUE;
}

void
color_map_up (GtkWidget *widget,
              gint       panelx)
{
  guint desired_cmap;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired_cmap = panel->vis_current.colormap + 1;
  if (color_map_set (tglobal, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_up failed");
}

void
color_map_down (GtkWidget *widget,
                gint       panelx)
{
  guint desired_cmap;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired_cmap = panel->vis_current.colormap - 1;
  if (color_map_set (tglobal, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_down failed");
}

void
color_map_cyclic (GtkWidget *widget,
                  gint       panelx)
{
  guint desired_cmap;
  VisualWF *wf;
  AmePanel *panel = get_panel (tglobal, panelx);

  if (panel->type != AME_PANEL_WATERFALL && panel->type != AME_PANEL_ECHO)
    {
      g_warning ("color_map_cyclic: wrong panel type");
      return;
    }

  wf = (VisualWF*)panel->vis_gui;
  desired_cmap = panel->vis_current.colormap + 1;
  if (desired_cmap >= wf->colormaps->len)
    desired_cmap = 0;

  if (color_map_set (tglobal, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_cyclic failed");
}

/* Функция устанавливает порог чувствительности. */
void
sensitivity_label (AmePanel *panel,
                   gdouble   sens)
{
  VisualFL *fl = (VisualFL*)panel->vis_gui;
  gchar *text;

  if (fl->common.sensitivity_value == NULL)
    return;

  text = g_strdup_printf ("<small><b>%.0f</b></small>", sens);
  gtk_label_set_markup (fl->common.sensitivity_value, text);
  g_free (text);
}

gboolean
sensitivity_set (Global  *global,
                 gdouble  desired_sensitivity,
                 guint    panelx)
{
  VisualFL *fl;
  AmePanel *panel = get_panel (global, panelx);

  if (desired_sensitivity < 1.0)
   return FALSE;
  if (desired_sensitivity > 10.0)
   return FALSE;

  if (panel->type != AME_PANEL_FORWARDLOOK)
    {
      g_warning ("sensitivity_set: wrong panel type!");
      return FALSE;
    }


  fl = (VisualFL*)panel->vis_gui;
  hyscan_gtk_forward_look_set_sensitivity (fl->fl, desired_sensitivity);
  gtk_widget_queue_draw (GTK_WIDGET (fl->fl));

  sensitivity_label (panel, desired_sensitivity);

  return TRUE;
}

void
sensitivity_up (GtkWidget *widget,
                gint       panelx)
{
  gdouble desired_sensitivity;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired_sensitivity = panel->vis_current.sensitivity + 1;
  if (sensitivity_set (tglobal, desired_sensitivity, panelx))
    panel->vis_current.sensitivity = desired_sensitivity;
  else
    g_warning ("sens_up failed");
}

void
sensitivity_down (GtkWidget *widget,
                  gint       panelx)
{
  gdouble desired_sensitivity;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired_sensitivity = panel->vis_current.sensitivity - 1;
  if (sensitivity_set (tglobal, desired_sensitivity, panelx))
    panel->vis_current.sensitivity = desired_sensitivity;
  else
    g_warning ("sens_down failed");
}

const HyScanDataSchemaEnumValue *
signal_finder (Global           *global,
               AmePanel         *panel,
               HyScanSourceType  source,
               gint              n)
{
  HyScanSonarInfoSource * info;
  GList *link;

  /* Информация об источнике. */
  info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
  hyscan_return_val_if_fail (info != NULL && info->presets != NULL, NULL);

  /* Ищем сигнал. */
  link = g_list_nth (info->presets, n);
  if (link == NULL)
    link = info->presets;

  return (HyScanDataSchemaEnumValue*) link->data;
}

void
signal_label (AmePanel    *panel,
              const gchar *name)
{
  gchar *text;

  if (panel->gui.signal_value != NULL)
    {
      text = g_strdup_printf ("<small><b>%s</b></small>", name);
      gtk_label_set_markup (panel->gui.signal_value, text);
      g_free (text);
    }
}

/* Функция устанавливает излучаемый сигнал. */
gboolean
signal_set (Global *global,
            guint   sig_num,
            gint    panelx)
{
  HyScanSourceType *iter;
  const HyScanDataSchemaEnumValue *sig, *prev_sig = NULL;
  AmePanel *panel = get_panel (global, panelx);

  g_message ("Signal_set: Sonar#%i, Signal %i", panelx, sig_num);

  if (sig_num == 0)
    return FALSE;

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSourceType source = *iter;

      g_message ("Setting signal for %s", hyscan_source_get_name_by_type (source));
      if (source == HYSCAN_SOURCE_PROFILER_ECHO)
        continue;

      sig = signal_finder (global, panel, source, sig_num);
      hyscan_return_val_if_fail (sig != NULL, FALSE);

      if (prev_sig != NULL && !g_str_equal (sig->name, prev_sig->name))
        {
          g_warning ("Signal names not equal!");
        }

      prev_sig = sig;
      /* Устанавливаем. */
      status = hyscan_sonar_generator_set_preset (global->control_s, source, sig->value);
      if (!status)
        return FALSE;
    }

  signal_label (panel, sig->name);
  return TRUE;
}

void
signal_up (GtkWidget *widget,
           gint      panelx)
{
  guint desired_signal;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired_signal = panel->current.signal + 1;
  if (signal_set (tglobal, desired_signal, panelx))
    panel->current.signal = desired_signal;
  else
    g_warning ("signal_up failed");
}

void
signal_down (GtkWidget *widget,
             gint      panelx)
{
  guint desired_signal;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired_signal = panel->current.signal - 1;
  if (signal_set (tglobal, desired_signal, panelx))
    panel->current.signal = desired_signal;
  else
    g_warning ("signal_down failed");
}

void
tvg_label (AmePanel *panel,
           gdouble   gain0,
           gdouble   step)
{
  gchar *gain_text, *step_text;

  if (panel->gui.tvg0_value != NULL)
    {
      gain_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", gain0);
      gtk_label_set_markup (panel->gui.tvg0_value, gain_text);
      g_free (gain_text);
    }
  if (panel->gui.tvg_value != NULL)
    {
      step_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", step);
      gtk_label_set_markup (panel->gui.tvg_value, step_text);
      g_free (step_text);
    }
}

/* Функция устанавливает параметры ВАРУ. */
gboolean
tvg_set (Global  *global,
         gdouble *gain0,
         gdouble  step,
         gint     panelx)
{
  HyScanSourceType *iter;

  AmePanel *panel = get_panel (global, panelx);

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSonarInfoSource *info;
      HyScanSourceType source = *iter;

      g_message ("setting tvg for %s", hyscan_source_get_name_by_type (source));

      /* Проверяем gain0. */
      info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
      hyscan_return_val_if_fail (info != NULL && info->tvg != NULL, TRUE); // TODO do something
      *gain0 = CLAMP (*gain0, info->tvg->min_gain,info->tvg->max_gain);

      status = hyscan_sonar_tvg_set_linear_db (global->control_s, source, *gain0, step);
      if (!status)
        return FALSE;
    }

  tvg_label (panel, *gain0, step);
  return TRUE;
}

void
tvg0_up (GtkWidget *widget,
         gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.gain0 + 0.5;

  if (tvg_set (tglobal, &desired, panel->current.gain_step, panelx))
    panel->current.gain0 = desired;
  else
    g_warning ("tvg0_up failed");
}

void
tvg0_down (GtkWidget *widget,
           gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.gain0 - 0.5;

  if (tvg_set (tglobal, &desired, panel->current.gain_step, panelx))
    panel->current.gain0 = desired;
  else
    g_warning ("tvg0_down failed");
}

void
tvg_up (GtkWidget *widget,
         gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.gain_step + 0.5;

  if (tvg_set (tglobal, &panel->current.gain0, desired, panelx))
    panel->current.gain_step = desired;
  else
    g_warning ("tvg_up failed");
}

void
tvg_down (GtkWidget *widget,
           gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.gain_step - 0.5;

  if (tvg_set (tglobal, &panel->current.gain0, desired, panelx))
    panel->current.gain_step = desired;
  else
    g_warning ("tvg_down failed");
}

void
auto_tvg_label (AmePanel *panel,
                gdouble   level,
                gdouble   sensitivity)
{
  gchar *sens_text, *level_text;

  if (panel->gui.tvg_level_value != NULL)
    {
      level_text = g_strdup_printf ("<small><b>%.1f</b></small>", level);
      gtk_label_set_markup (panel->gui.tvg_level_value, level_text);
      g_free (level_text);
    }

  if (panel->gui.tvg_sens_value != NULL)
    {
      sens_text = g_strdup_printf ("<small><b>%.1f</b></small>", sensitivity);
      gtk_label_set_markup (panel->gui.tvg_sens_value, sens_text);
      g_free (sens_text);
    }
}

/* Функция устанавливает параметры автоВАРУ. */
gboolean
auto_tvg_set (Global   *global,
              gdouble   level,
              gdouble   sensitivity,
              gint      panelx)
{
  gboolean status;
  AmePanel *panel = get_panel (global, panelx);

  HyScanSourceType *iter;

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      g_message ("setting auto-tvg for %s", hyscan_source_get_name_by_type (*iter));
      status = hyscan_sonar_tvg_set_auto (global->control_s, *iter, level, sensitivity);
      if (!status)
        return FALSE;
    }

  /* Теперь печатаем что и куда надо. */
  auto_tvg_label (panel, level, sensitivity);
  return TRUE;
}

void
tvg_level_up (GtkWidget *widget,
              gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.level + 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (tglobal, desired, panel->current.sensitivity, panelx))
    panel->current.level = desired;
  else
    g_warning ("tvg_level_up failed");
}

void
tvg_level_down (GtkWidget *widget,
                gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.level - 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (tglobal, desired, panel->current.sensitivity, panelx))
    panel->current.level = desired;
  else
    g_warning ("tvg_level_down failed");
}

void
tvg_sens_up (GtkWidget *widget,
             gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.sensitivity + 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (tglobal, panel->current.level, desired, panelx))
    panel->current.sensitivity = desired;
  else
    g_warning ("tvg_sens_up failed");
}

void
tvg_sens_down (GtkWidget *widget,
               gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.sensitivity + 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (tglobal, panel->current.level, desired, panelx))
    panel->current.sensitivity = desired;
  else
    g_warning ("tvg_sens_down failed");
}

void
distance_label (AmePanel *panel,
                gdouble   distance)
{
  gchar *text;
  if (panel->gui.distance_value != NULL)
  {
    text = g_strdup_printf ("<small><b>%.0f м</b></small>", distance);
    gtk_label_set_markup (panel->gui.distance_value, text);
    g_free (text);
  }
}

/* Функция устанавливает рабочую дистанцию. */
gboolean
distance_set (Global  *global,
              gdouble  meters,
              gint     panelx)
{
  gdouble receive_time;
  gboolean status;
  HyScanSourceType *iter;
  AmePanel *panel = get_panel (global, panelx);

  if (meters < 1.0)
    return FALSE;

  receive_time = meters / (global->sound_velocity / 2.0);
  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      g_message ("setting distance for %s", hyscan_source_get_name_by_type (*iter));
      status = hyscan_sonar_receiver_set_time (global->control_s, *iter, receive_time, 0);
      if (!status)
        return FALSE;
    }

  distance_label (panel, meters);
  return TRUE;
}

void
distance_up (GtkWidget *widget,
             gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.distance + 5.0;
  if (distance_set (tglobal, desired, panelx))
    panel->current.distance = desired;
  else
    g_warning ("distance_up failed");
}

void
distance_down (GtkWidget *widget,
               gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.distance - 5.0;
  if (distance_set (tglobal, desired, panelx))
    panel->current.distance = desired;
  else
    g_warning ("distance_down failed");
}


void
void_callback (gpointer data)
{
  g_message ("This is a void callback. ");
}

/* Функция устанавливает яркость отображения. */
gboolean
brightness_set (Global  *global,
                gdouble  new_brightness,
                gdouble  new_black,
                gint     panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gchar *text_bright;
  gchar *text_black;
  gdouble b, g, w;
  AmePanel *panel = get_panel (global, panelx);

  if (new_brightness < 1.0)
    return FALSE;
  if (new_brightness > 100.0)
    return FALSE;
  if (new_black < 0.0)
    return FALSE;
  if (new_black > 100.0)
    return FALSE;

  text_bright = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_brightness);
  text_black = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_black);

  switch (panel->type)
    {
    case AME_PANEL_WATERFALL:
      b = 0;
      w = 1.0 - (new_brightness / 100.0) * 0.99;
      g = 1.25 - 0.5 * (new_brightness / 100.0);

      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w);
      gtk_label_set_markup (wf->common.brightness_value, text_bright);
      break;

    case AME_PANEL_ECHO:
      b = new_black * 0.0000025;
      w = 1.0 - (new_brightness / 100.0) * 0.99;
      w *= w;
      g = 1;
      if (b >= w)
        {
          g_message ("BBC error");
          return FALSE;
        }

      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w);

      gtk_label_set_markup (wf->common.brightness_value, text_bright);
      gtk_label_set_markup (wf->common.black_value, text_black);
      break;

    case AME_PANEL_FORWARDLOOK:
      fl = (VisualFL*)panel->vis_gui;
      hyscan_gtk_forward_look_set_brightness (fl->fl, new_brightness);
      gtk_label_set_markup (fl->common.brightness_value, text_bright);
      break;

    default:
      g_warning ("brightness_set: wrong panel type!");
    }

  g_free (text_bright);
  g_free (text_black);

  return TRUE;
}

void
brightness_up (GtkWidget *widget,
               gint       panelx)
{
  gdouble new_brightness;
  AmePanel *panel = get_panel (tglobal, panelx);

  new_brightness = panel->vis_current.brightness;

  switch (panel->type)
    {
      case AME_PANEL_WATERFALL:
      case AME_PANEL_ECHO:
        {
          if (new_brightness < 50.0)
            new_brightness += 10.0;
          else if (new_brightness < 90.0)
            new_brightness += 5.0;
          else
            new_brightness += 1.0;
        }
        break;

      case AME_PANEL_FORWARDLOOK:
        {
          if (new_brightness < 10.0)
            new_brightness += 1.0;
          else if (new_brightness < 30.0)
            new_brightness += 2.0;
          else if (new_brightness < 50.0)
            new_brightness += 5.0;
          else
            new_brightness += 10.0;
        }
        break;

      default:
        g_warning ("brightness_up: wrong panel type");
        return;
    }

  new_brightness = CLAMP (new_brightness, 1.0, 100.0);

  if (brightness_set (tglobal, new_brightness, panel->vis_current.black, panelx))
    panel->vis_current.brightness = new_brightness;
  else
    g_warning ("brightness_up failed");
}

void
brightness_down (GtkWidget *widget,
                 gint        panelx)
{
  gdouble new_brightness;
  AmePanel *panel = get_panel (tglobal, panelx);

  new_brightness = panel->vis_current.brightness;

  switch (panel->type)
    {
      case AME_PANEL_WATERFALL:
      case AME_PANEL_ECHO:
        {
          if (new_brightness > 90.0)
            new_brightness = new_brightness - 1.0;
          else if (new_brightness > 50.0)
            new_brightness = new_brightness - 5.0;
          else
            new_brightness = new_brightness - 10.0;
        }
        break;

      case AME_PANEL_FORWARDLOOK:
        {
          if (new_brightness <= 10.0)
            new_brightness -= 1.0;
          else if (new_brightness <= 30.0)
            new_brightness -= 2.0;
          else if (new_brightness <= 50.0)
            new_brightness -= 5.0;
          else
            new_brightness -= 10.0;
        }
        break;

      default:
        g_warning ("brightness_down: wrong panel type");
        return;
    }

  new_brightness = CLAMP (new_brightness, 1.0, 100.0);

  if (brightness_set (tglobal, new_brightness, panel->vis_current.black, panelx))
    panel->vis_current.brightness = new_brightness;
  else
    g_warning ("brightness_down failed");
}

void
black_up (GtkWidget *widget,
          gint       panelx)
{
  gdouble new_black;
  AmePanel *panel = get_panel (tglobal, panelx);

  new_black = panel->vis_current.black;

  switch (panel->type)
    {
      case AME_PANEL_WATERFALL:
      case AME_PANEL_ECHO:
        {
          if (new_black < 50.0)
            new_black += 10.0;
          else if (new_black < 90.0)
            new_black += 5.0;
          else
            new_black += 1.0;
        }
        break;

      case AME_PANEL_FORWARDLOOK:
        {
          if (new_black < 10.0)
            new_black += 1.0;
          else if (new_black < 30.0)
            new_black += 2.0;
          else if (new_black < 50.0)
            new_black += 5.0;
          else
            new_black += 10.0;
        }
        break;

      default:
        g_warning ("black_up: wrong panel type");
        return;
    }

  new_black = CLAMP (new_black, 0.0, 100.0);

  if (brightness_set (tglobal, panel->vis_current.brightness, new_black, panelx))
    panel->vis_current.black = new_black;
  else
    g_warning ("black_up failed");
}

void
black_down (GtkWidget *widget,
            gint       panelx)
{
  gdouble new_black;
  AmePanel *panel = get_panel (tglobal, panelx);

  new_black = panel->vis_current.black;

  switch (panel->type)
    {
      case AME_PANEL_WATERFALL:
      case AME_PANEL_ECHO:
        {
          if (new_black > 90.0)
            new_black = new_black - 1.0;
          else if (new_black > 50.0)
            new_black = new_black - 5.0;
          else
            new_black = new_black - 10.0;
        }
        break;

      case AME_PANEL_FORWARDLOOK:
        {
          if (new_black <= 10.0)
            new_black -= 1.0;
          else if (new_black <= 30.0)
            new_black -= 2.0;
          else if (new_black <= 50.0)
            new_black -= 5.0;
          else
            new_black -= 10.0;
        }
        break;

      default:
        g_warning ("black_down: wrong panel type");
        return;
    }

  new_black = CLAMP (new_black, 0.0, 100.0);

  if (brightness_set (tglobal, panel->vis_current.brightness, new_black, panelx))
    panel->vis_current.black = new_black;
  else
    g_warning ("black_down failed");
}

void
scale_up (GtkWidget *widget,
          gint       panelx)
{
  scale_set (tglobal, TRUE, panelx);
}

void
scale_down (GtkWidget *widget,
            gint       panelx)
{
  scale_set (tglobal, FALSE, panelx);
}

/* Функция переключает режим отображения виджета ВС.
 * Попрошу заметить, возвращается ЛОЖЬ для того, чтобы свитчер нормально переключился. */
gboolean
mode_changed (GtkWidget *widget,
              gboolean   state,
              gint       panelx)
{
  VisualFL *fl;
  HyScanGtkForwardLookViewType type;

  AmePanel *panel = get_panel (tglobal, panelx);

  if (panel->type != AME_PANEL_FORWARDLOOK)
    {
      g_message ("mode_changed: wrong panel type!");
      return TRUE;
    }

  fl = (VisualFL*)panel->vis_gui;
  type = state ? HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET : HYSCAN_GTK_FORWARD_LOOK_VIEW_SURFACE;
  hyscan_gtk_forward_look_set_type (fl->fl, type);
  return FALSE;
}

/* Вкл и выкл спецобработки.
 * Попрошу заметить, возвращается ЛОЖЬ для того, чтобы свитчер нормально переключился. */
gboolean
pf_special (GtkWidget  *widget,
            gboolean    state,
            gint        panelx)
{
  VisualWF *wf;
  HyScanTileFlags flags = 0;
  AmePanel *panel = get_panel (tglobal, panelx);

  if (panel->type != AME_PANEL_ECHO)
    {
      g_message ("pf_special: wrong panel type!");
      return TRUE;
    }

  wf = (VisualWF*)panel->vis_gui;
  flags = hyscan_gtk_waterfall_state_get_tile_flags (HYSCAN_GTK_WATERFALL_STATE (wf->wf));

  if (state)
    flags |= HYSCAN_TILE_PROFILER;
  else
    flags &= ~HYSCAN_TILE_PROFILER;

  hyscan_gtk_waterfall_state_set_tile_flags (HYSCAN_GTK_WATERFALL_STATE (wf->wf), flags);

  return FALSE;
}

/* Функция переключает автосдвижку. */
gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           gint        panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gboolean current;
  AmePanel *panel = get_panel (tglobal, panelx);

  switch (panel->type)
    {
    case AME_PANEL_WATERFALL:
    case AME_PANEL_ECHO:
      wf = (VisualWF*)panel->vis_gui;
      current = hyscan_gtk_waterfall_automove (wf->wf, state);
      break;

    case AME_PANEL_FORWARDLOOK:

      fl = (VisualFL*)panel->vis_gui;
      if (state)
        hyscan_forward_look_player_real_time (fl->player);
      else
        hyscan_forward_look_player_pause (fl->player);
      current = state;
      break;

    default:
      g_warning ("live_view: wrong panel type!");
    }

  button_set_active (G_OBJECT (panel->vis_gui->live_view), current);

  return TRUE;
}

gboolean
automove_switched (GtkWidget  *widget,
                   gboolean    state)
{
  static gboolean now_switching = FALSE;

  if (now_switching)
    return TRUE;

  now_switching = TRUE;
  automove_state_changed (widget, state, tglobal);
  now_switching = FALSE;

  return FALSE;
}

void
automove_state_changed (GtkWidget  *widget,
                        gboolean    state,
                        Global     *global)
{
  GHashTableIter iter;
  gpointer k, v;

  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, &k, &v))
    live_view (NULL, state, GPOINTER_TO_INT (k));

  g_message ("automove_state_changed!");
}


/* Функция включает/выключает излучение. */
gboolean
start_stop (Global    *global,
            gboolean   state)
{
  GHashTableIter iter;
  gpointer k, v;

  if (global->control_s == NULL)
    {
      g_message ("I ain't got no sonar.");
      return FALSE;
    }

  /* Включаем излучение. */
  if (state)
    {
      g_message ("Start sonars. Dry is %s", global->dry ? "ON" : "OFF");
      gint n_tracks = -1;
      gboolean status = TRUE;

      /* Закрываем текущий открытый галс. */
      g_clear_pointer (&global->track_name, g_free);

      /* Проходим по всем страницам и включаем системы локаторов. */
      g_hash_table_iter_init (&iter, global->panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        {
          AmePanel *panel = v;
          gint panelx = GPOINTER_TO_INT (k);

          /* Излучение НЕ в режиме сух. пов. */
          if (!global->dry)
            status &= signal_set (global, panel->current.signal, panelx);

          /* Приемник. */
          status &= distance_set (global, panel->current.distance, panelx);

          /* TVG */
          switch (panel->type)
            {
              /* TODO: механизм определения, где автотвг, где просто твг.
                 Сейчас пригодно только для АМЭ.
               */
            case AME_PANEL_WATERFALL:
              status &= auto_tvg_set (global, panel->current.level, panel->current.sensitivity, panelx);
              break;
            case AME_PANEL_ECHO:
            case AME_PANEL_FORWARDLOOK:
              status &= tvg_set (global, &panel->current.gain0, panel->current.gain_step, panelx);
              break;

            default:
              g_warning ("start_stop: wrong panel type");

            }

          /* Если не удалось запустить какую-либо подсистему. */
          if (!status)
            return FALSE;
        }

      /* Число галсов в проекте. */
      if (n_tracks < 0)
        {
          gint32 project_id;
          gchar **tracks;
          gchar **strs;

          project_id = hyscan_db_project_open (global->db, global->project_name);
          tracks = hyscan_db_track_list (global->db, project_id);

          for (strs = tracks; strs != NULL && *strs != NULL; strs++)
            {
              n_tracks++;
            }

          hyscan_db_close (global->db, project_id);
          g_free (tracks);
        }

      /* Включаем запись нового галса. */
      global->track_name = g_strdup_printf ("%d%s", ++n_tracks, global->dry ? DRY_TRACK_SUFFIX : "");

      status = hyscan_sonar_start (global->control_s, global->project_name,
                                   global->track_name, HYSCAN_TRACK_SURVEY);

      if (!status)
        {
          g_message ("Sonar startup failed @ %i",__LINE__);
          return FALSE;
        }

      /* Если локатор включён, переходим в режим онлайн. */
      gtk_widget_set_sensitive (GTK_WIDGET (global->gui.track.tree), FALSE);
    }

  /* Выключаем излучение и блокируем режим онлайн. */
  else
    {
      g_message ("Stop sonars");
      hyscan_sonar_stop (global->control_s);
      gtk_widget_set_sensitive (GTK_WIDGET (global->gui.track.tree), TRUE);
    }

  /* Устанавливаем live_view для всех виджетов. */
  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, &k, &v))
    live_view (NULL, state, GPOINTER_TO_INT (k));

  return TRUE;
}

/* Функция включает/выключает сухую поверку. */
gboolean
set_dry (Global    *global,
         gboolean   state)
{
  global->dry = state;

  return TRUE;
}

void
sensor_cb (HyScanSensor     *sensor,
           const gchar      *name,
           HyScanSourceType  source,
           gint64            recv_time,
           HyScanBuffer     *buffer,
           Global           *global)
{
  /* Ищем строки RMC и DPT. */
  if (source != HYSCAN_SOURCE_NMEA_RMC && source != HYSCAN_SOURCE_NMEA_DPT)
    return;

  /* Напрямую загоняем их в виджет. */
  hyscan_gtk_nav_indicator_push (HYSCAN_GTK_NAV_INDICATOR (global->gui.nav), buffer);
}

GtkWidget*
make_layer_btn (HyScanGtkWaterfallLayer *layer,
                GtkWidget               *from)
{
  GtkWidget *button;
  const gchar *icon;

  icon = hyscan_gtk_waterfall_layer_get_mnemonic (layer);
  button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (from));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input),
                            layer);

  return button;
}

void
fl_coords_callback (HyScanFlCoords *coords,
                    GtkLabel       *label)
{
  gboolean status;
  gdouble lat;
  gdouble lon;
  gchar *text;

  status = hyscan_fl_coords_get_coords (coords, &lat, &lon);

  text = g_strdup_printf ("Широта: %f; Долгота: %f", lat, lon);
  gtk_widget_set_visible (GTK_WIDGET (label), status);
  gtk_label_set_text (label, text);
}

GtkWidget *
make_overlay (HyScanGtkWaterfall          *wf,
              HyScanGtkWaterfallGrid     **_grid,
              HyScanGtkWaterfallControl  **_ctrl,
              HyScanGtkWaterfallMark     **_mark,
              HyScanGtkWaterfallMeter    **_meter,
              HyScanMarkModel             *mark_model)
{
  GtkWidget * container = gtk_grid_new ();

  HyScanGtkWaterfallGrid *grid = hyscan_gtk_waterfall_grid_new (wf);
  HyScanGtkWaterfallControl *ctrl = hyscan_gtk_waterfall_control_new (wf);
  HyScanGtkWaterfallMark *mark = hyscan_gtk_waterfall_mark_new (wf, mark_model);
  HyScanGtkWaterfallMeter *meter = hyscan_gtk_waterfall_meter_new (wf);
  GtkWidget *hscroll = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL,
                                           hyscan_gtk_waterfall_grid_get_hadjustment (grid));
  GtkWidget *vscroll = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
                                           hyscan_gtk_waterfall_grid_get_vadjustment (grid));
  gtk_range_set_inverted (GTK_RANGE (vscroll), TRUE);
  hyscan_gtk_waterfall_control_set_wheel_behaviour (ctrl, TRUE);

  if (_grid != NULL)
    *_grid = grid;
  if (_ctrl != NULL)
    *_ctrl = ctrl;
  if (_mark != NULL)
    *_mark = mark;
  if (_meter != NULL)
    *_meter = meter;

  gtk_grid_attach (GTK_GRID (container), GTK_WIDGET (wf), 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (container), hscroll, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (container), vscroll, 1, 0, 1, 1);

  gtk_widget_set_hexpand (GTK_WIDGET (wf), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (wf), TRUE);

  return GTK_WIDGET (container);
}

void
ame_colormap_free (gpointer data)
{
  AmeColormap *cmap = *(AmeColormap**)data;
  if (cmap == NULL)
    return;

  g_clear_pointer (&cmap->colors, g_free);
  g_clear_pointer (&cmap->name, g_free);

  g_free (cmap);
}

void
init_triple (Global *ext_global)
{
  tglobal = ext_global;
}
