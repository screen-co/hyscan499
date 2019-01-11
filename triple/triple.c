#include "triple-types.h"
#include "hyscan-ame-splash.h"
#include "ui-builder.h"
#include "ui.h"
#include "hyscan-ame-project.h"

enum
{
  DATE_SORT_COLUMN,
  TRACK_COLUMN,
  DATE_COLUMN,
  N_COLUMNS
};

AmePanel *
get_panel (Global *global,
           gint    panelx)
{
  return g_hash_table_lookup (global->panels, GINT_TO_POINTER (panelx));
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

  HyScanDB * db = global.db;
  HyScanCache * cache = global.cache;
  gchar *project = global.project_name;
  gchar *track = global.track_name;

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
switch_page (GObject     *emitter,
             const gchar *page)
{
  GtkStack * lstack = GTK_STACK (global.gui.lstack);
  GtkStack * rstack = GTK_STACK (global.gui.rstack);
  GtkRevealer * sidebar = GTK_REVEALER (global.gui.side_revealer);
  GtkRevealer * bottbar = GTK_REVEALER (global.gui.bott_revealer);

  const gchar * old = gtk_stack_get_visible_child_name (lstack);
  // HyScanAmeFixed * lold = HYSCAN_AME_FIXED (gtk_stack_get_visible_child (lstack));
  HyScanAmeFixed * rold = HYSCAN_AME_FIXED (gtk_stack_get_visible_child (rstack));

  if (g_str_equal (old, "И_ПФд"))
    hyscan_ame_fixed_set_state (rold, 4, FALSE);
  if (g_str_equal (old, "И_ГБОд"))
    hyscan_ame_fixed_set_state (rold, 4, FALSE);

  g_message ("Opening page <%s>", page);

  gtk_stack_set_visible_child_name (lstack, page);
  gtk_stack_set_visible_child_name (rstack, page);

  if (g_str_equal(page, "ГАЛС"))
    gtk_revealer_set_reveal_child (sidebar, TRUE);
  else if (g_str_equal(page, "И_ГБОм"))
    {
      gtk_revealer_set_reveal_child (sidebar, TRUE);
      turn_marks (NULL, W_SIDESCAN);
    }
  else if (g_str_equal(page, "И_ПФм"))
    {
      gtk_revealer_set_reveal_child (sidebar, TRUE);
      turn_marks (NULL, W_PROFILER);
    }
  else
    {
      gtk_revealer_set_reveal_child (sidebar, FALSE);
      turn_marks (NULL, W_LAST);
      turn_meter (NULL, FALSE, W_LAST);
    }


  if (g_str_equal (page, "И_ГК") ||
      g_str_equal (page, "И_ГКд"))
    gtk_revealer_set_reveal_child (bottbar, TRUE);
  else
    gtk_revealer_set_reveal_child (bottbar, FALSE);

}

void
turn_meter (GObject     *emitter,
            gboolean     state,
            gint         selector)
{
  void * layer0 = NULL;
  void * layer1 = NULL;

  if (!state)
    {
      layer0 = global.GPF.wf_ctrl;
      layer1 = global.GSS.wf_ctrl;
    }
  else if (selector == W_SIDESCAN)
    {
      layer0 = global.GSS.wf_metr;
      layer1 = global.GPF.wf_ctrl;
    }
  else if (selector == W_PROFILER)
    {
      layer0 = global.GPF.wf_metr;
      layer1 = global.GSS.wf_ctrl;
    }

  hyscan_gtk_waterfall_layer_grab_input (HYSCAN_GTK_WATERFALL_LAYER (layer0));
  hyscan_gtk_waterfall_layer_grab_input (HYSCAN_GTK_WATERFALL_LAYER (layer1));
}

void
turn_marks (GObject     *emitter,
            gint         selector)
{
  void * layer0 = NULL;
  void * layer1 = NULL;

  if (selector == W_SIDESCAN)
    {
      layer0 = global.GSS.wf_mark;
      layer1 = global.GPF.wf_ctrl;
    }
  else if (selector == W_PROFILER)
    {
      layer0 = global.GPF.wf_mark;
      layer1 = global.GSS.wf_ctrl;
    }
  else
    {
      layer0 = global.GPF.wf_ctrl;
      layer1 = global.GSS.wf_ctrl;
    }

  hyscan_gtk_waterfall_layer_grab_input (HYSCAN_GTK_WATERFALL_LAYER (layer0));
  hyscan_gtk_waterfall_layer_grab_input (HYSCAN_GTK_WATERFALL_LAYER (layer1));
}
void
hide_marks (GObject     *emitter,
            gboolean     state,
            gint         selector)
{
  HyScanGtkWaterfallLayer *layer = NULL;

  if (selector == W_SIDESCAN)
    layer = HYSCAN_GTK_WATERFALL_LAYER (global.GSS.wf_mark);
  else if (selector == W_PROFILER)
    layer = HYSCAN_GTK_WATERFALL_LAYER (global.GPF.wf_mark);

  hyscan_gtk_waterfall_layer_set_visible (layer, state);
}

HyScanDataSchemaEnumValue *
find_signal_by_name (HyScanDataSchemaEnumValue ** where,
                     HyScanDataSchemaEnumValue  * what)
{
  const gchar * name = what->name;
  HyScanDataSchemaEnumValue * current;

  if (where == NULL)
    return NULL;

  for (; *where != NULL; ++where)
    {
      current = *where;

      if (g_strcmp0 (current->name, name) == 0)
        return current;
    }

  return NULL;
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

  gtk_window_set_focus (GTK_WINDOW (global.gui.window), target);
  event->key.window = g_object_ref (gtk_widget_get_window (target));
  g_idle_add ((GSourceFunc)idle_key, (gpointer)event);
}

#define NAV_FN(fname, button) void \
                              fname (GObject * emitter, \
                                     gpointer udata) \
                              {gint sel = GPOINTER_TO_INT (udata); \
                               nav_common (global.gui.disp_widgets[sel], \
                                           button, GDK_CONTROL_MASK);}

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
  AmePanel *panel = get_panel (&global, panelx);

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
  AmePanel *panel = get_panel (&global, panelx);

  fl = (VisualFL*)panel->vis_gui;
  adj = fl->position_range;

  value = gtk_adjustment_get_value (adj);
  gtk_adjustment_set_value (adj, value + 1);
}


void
run_manager (GObject *emitter)
{
  HyScanDBInfo *info;
  GtkWidget *dialog;
  gint res;

  info = hyscan_db_info_new (global.db);
  dialog = hyscan_ame_project_new (global.db, info, GTK_WINDOW (global.gui.window));
  res = gtk_dialog_run (GTK_DIALOG (dialog));

  // TODO: start/stop?
  if (res == HYSCAN_AME_PROJECT_OPEN || res == HYSCAN_AME_PROJECT_CREATE)
    {
      gchar *project;
      hyscan_ame_project_get (HYSCAN_AME_PROJECT (dialog), &project, NULL);

      start_stop (NULL, FALSE);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_all_switch), FALSE);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_dry_switch), FALSE);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_SIDESCAN]), FALSE);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_PROFILER]), FALSE);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_FORWARDL]), FALSE);

      g_clear_pointer (&global.project_name, g_free);
      global.project_name = g_strdup (project);
      g_message ("global.project_name set to <<%s>>", global.project_name);
      hyscan_db_info_set_project (global.db_info, project);
      // hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.control), project);
      // hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.control), project);

      g_clear_pointer (&global.marks.loc_storage, g_hash_table_unref);
      global.marks.loc_storage = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                        (GDestroyNotify) loc_store_free);


      hyscan_mark_model_set_project (global.marks.model, global.db, project);
      g_free (project);
    }

  g_object_unref (info);
  gtk_widget_destroy (dialog);
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

GtkTreeView *
list_scroll_tree_view_resolver (gint list)
{
  GtkTreeView * view = NULL;

  if (list == L_MARKS)
    view = hyscan_gtk_project_viewer_get_tree_view (HYSCAN_GTK_PROJECT_VIEWER (global.gui.mark_view));
  else if (list == L_TRACKS)
    view = global.gui.track_view;
  else
    g_message ("Wrong list selector passed (%i)", list);

  return view;
}

void
list_scroll_up (GObject *emitter,
                gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, TRUE, FALSE);
}

void
list_scroll_down (GObject *emitter,
                  gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, FALSE, FALSE);
}

void
list_scroll_start (GObject *emitter,
                   gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, TRUE, TRUE);
}
void
list_scroll_end (GObject *emitter,
                 gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, FALSE, TRUE);
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
  gtk_tree_view_set_cursor (global->gui.track_view, null_path, NULL, FALSE);
  gtk_tree_path_free (null_path);

  gtk_list_store_clear (GTK_LIST_STORE (global->gui.track_list));

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&hash_iter, tracks);

  while (g_hash_table_iter_next (&hash_iter, &key, &value))
    {
      HyScanTrackInfo *track_info = value;

      if (g_str_has_prefix (track_info->name, PF_TRACK_PREFIX))
        continue;

      /* Добавляем в список галсов. */
      gtk_list_store_append (GTK_LIST_STORE (global->gui.track_list), &tree_iter);
      gtk_list_store_set (GTK_LIST_STORE (global->gui.track_list), &tree_iter,
                          DATE_SORT_COLUMN, g_date_time_to_unix (track_info->ctime),
                          TRACK_COLUMN, g_strdup (track_info->name),
                          DATE_COLUMN, g_date_time_format (track_info->ctime, "%d.%m %H:%M"),
                          -1);

      /* Подсвечиваем текущий галс. */
      if (g_strcmp0 (cur_track_name, track_info->name) == 0)
        {
          GtkTreePath *tree_path = gtk_tree_model_get_path (global->gui.track_list, &tree_iter);
          gtk_tree_view_set_cursor (global->gui.track_view, tree_path, NULL, FALSE);
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
mark_manager_changed (HyScanMarkModel *model,
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

  position = gtk_adjustment_get_value (global->gui.track_range);
  gdk_event_get_scroll_deltas (event, &step_x, &step_y);
  gtk_tree_view_convert_bin_window_to_widget_coords (global->gui.track_view, 0, 0, &size_x, &size_y);
  position += step_y * size_y;

  gtk_adjustment_set_value (global->gui.track_range, position);

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
  const gchar *track_name;
  gchar *pftrack;

  /* Определяем название нового галса. */
  gtk_tree_view_get_cursor (list, &path, NULL);
  if (path == NULL)
    return;

  if (!gtk_tree_model_get_iter (global->gui.track_list, &iter, path))
    return;

  g_clear_pointer (&global->track_name, g_free);
  /* Открываем новый галс. */

  gtk_tree_model_get_value (global->gui.track_list, &iter, TRACK_COLUMN, &value);
  track_name = g_value_get_string (&value);
  global->track_name = g_strdup (track_name);

  hyscan_forward_look_player_open (global->GFL.fl_player, global->db, global->cache,
                                   global->project_name, track_name);
  hyscan_fl_coords_set_project (global->GFL.fl_coords, global->db, global->project_name, track_name);
  hyscan_gtk_waterfall_state_set_track (HYSCAN_GTK_WATERFALL_STATE (global->GSS.wf),
                                        global->db, global->project_name, track_name);

  pftrack = g_strdup_printf (PF_TRACK_PREFIX "%s", track_name);
  hyscan_gtk_waterfall_state_set_track (HYSCAN_GTK_WATERFALL_STATE (global->GPF.wf),
                                        global->db, global->project_name, pftrack);
  g_free (pftrack);

  g_value_unset (&value);

    /* Режим отображения. */
  if (track_is_active (global->db_info, track_name))
    {
      hyscan_forward_look_player_real_time (global->GFL.fl_player);
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global->GSS.wf), TRUE);
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global->GPF.wf), TRUE);
    }
  else
    {
      hyscan_forward_look_player_pause (global->GFL.fl_player);
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global->GSS.wf), FALSE);
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global->GPF.wf), FALSE);
    }
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

  scale = 100.0 * (fabs (to_y - from_y) / fabs (max_y - min_y));

  return scale;
}

/* Функция устанавливает масштаб отображения. */
gboolean
scale_set (Global   *global,
           gboolean  scale_up,
           gpointer  user_data,
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
      }
      break;

    default:
      g_warning ("scale_set: wrong panel type!");
    }

  text = g_strdup_printf ("<small><b>%.0f%%</b></small>", scale);
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

  for (i = 1; i <= 1120; ++i)
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
  text = g_strdup_printf ("<small><b>%s</b></small>", colormap->name);
  gtk_label_set_markup (wf->common.colormap_value, text);
  g_free (text);

  return TRUE;
}

void
color_map_up (GtkWidget *widget,
              gint       panelx)
{
  guint desired_cmap;
  AmePanel *panel = get_panel (&global, panelx);

  desired_cmap = panel->vis_current.colormap + 1;
  if (color_map_set (&global, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_up failed");
}

void
color_map_down (GtkWidget *widget,
                gint       panelx)
{
  guint desired_cmap;
  AmePanel *panel = get_panel (&global, panelx);

  desired_cmap = panel->vis_current.colormap - 1;
  if (color_map_set (&global, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_down failed");
}

/* Функция устанавливает порог чувствительности. */
gboolean
sensitivity_set (Global  *global,
                 gdouble  desired_sensitivity,
                 guint    panelx)
{
  VisualFL *fl;
  gchar *text;
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

  text = g_strdup_printf ("<small><b>%.0f</b></small>", desired_sensitivity);
  gtk_label_set_markup (fl->common.sensitivity_value, text);
  g_free (text);

  return TRUE;
}

void
sens_up (GtkWidget *widget,
         gint       panelx)
{
  gdouble desired_sensitivity;
  AmePanel *panel = get_panel (&global, panelx);

  desired_sensitivity = panel->vis_current.sensitivity + 1;
  if (sensitivity_set (&global, desired_sensitivity, panelx))
    panel->vis_current.sensitivity = desired_sensitivity;
  else
    g_warning ("sens_up failed");
}

void
sens_down (GtkWidget *widget,
           gint       panelx)
{
  gdouble desired_sensitivity;
  AmePanel *panel = get_panel (&global, panelx);

  desired_sensitivity = panel->vis_current.sensitivity - 1;
  if (sensitivity_set (&global, desired_sensitivity, panelx))
    panel->vis_current.sensitivity = desired_sensitivity;
  else
    g_warning ("sens_down failed");
}

/* Функция устанавливает излучаемый сигнал. */
gboolean
signal_set (Global *global,
            guint   sig_num,
            gint    panelx)
{
  gchar *text;
  HyScanSourceType *iter;
  HyScanDataSchemaEnumValue *sig, *prev_sig = NULL;
  AmePanel *panel = get_panel (global, panelx);

  g_message ("Signal_set: Sonar#%i, Signal %i", panelx, sig_num);

  if (sig_num == 0)
    return FALSE;

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSourceType source = *iter;
      HyScanSonarInfoSource * info;
      GList *link;

      info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
      hyscan_return_val_if_fail (info != NULL && info->generator != NULL, FALSE);

      /* Ищем нужный сигнал. */
      link = g_list_nth (info->generator->presets, sig_num);
      hyscan_return_val_if_fail (link != NULL, FALSE);
      sig = link->data;
      hyscan_return_val_if_fail (sig != NULL, FALSE);

      if (prev_sig != NULL && !g_str_equal (sig->name, prev_sig->name))
        {
          g_warning ("Signal names not equal!");
        }

      /* Устанавливаем. */
      status = hyscan_sonar_generator_set_preset (global->control_s, source, sig->value);
      if (!status)
        return FALSE;
    }

  text = g_strdup_printf ("<small><b>%s</b></small>", sig->name);
  gtk_label_set_markup (panel->gui.signal_value, text);
  g_free (text);

  return TRUE;
}

void
signal_up (GtkWidget *widget,
           gint      panelx)
{
  guint desired_signal;
  AmePanel *panel = get_panel (&global, panelx);

  desired_signal = panel->current.cur_signal + 1;
  if (signal_set (&global, desired_signal, panelx))
    panel->current.cur_signal = desired_signal;
  else
    g_warning ("signal_up failed");
}

void
signal_down (GtkWidget *widget,
             gint      panelx)
{
  guint desired_signal;
  AmePanel *panel = get_panel (&global, panelx);

  desired_signal = panel->current.cur_signal - 1;
  if (signal_set (&global, desired_signal, panelx))
    panel->current.cur_signal = desired_signal;
  else
    g_warning ("signal_down failed");
}

/* Функция устанавливает параметры ВАРУ. */
gboolean
tvg_set (Global  *global,
         gdouble *gain0,
         gdouble  step,
         gint     panelx)
{
  gchar *gain_text, *step_text;
  HyScanSourceType *iter;

  AmePanel *panel = get_panel (global, panelx);

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSonarInfoSource *info;
      HyScanSourceType source = *iter;

      /* Проверяем gain0. */
      info = g_hash_table_lookup (global->panels, GINT_TO_POINTER (source));
      hyscan_return_val_if_fail (info != NULL && info->tvg != NULL, FALSE);
      *gain0 = CLAMP (*gain0, info->tvg->min_gain,info->tvg->max_gain);

      status = hyscan_sonar_tvg_set_linear_db (global->control_s, source, *gain0, step);
      if (!status)
        return FALSE;
    }

  gain_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", *gain0);
  step_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", step);

  gtk_label_set_markup (panel->gui.tvg0_value, gain_text);
  gtk_label_set_markup (panel->gui.tvg_value, step_text);

  g_free (gain_text);
  g_free (step_text);

  return TRUE;
}

void
tvg0_up (GtkWidget *widget,
         gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_gain0 + 0.5;

  if (tvg_set (&global, &desired, panel->current.cur_gain_step, panelx))
    panel->current.cur_gain0 = desired;
  else
    g_warning ("tvg0_up failed");
}

void
tvg0_down (GtkWidget *widget,
           gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_gain0 - 0.5;

  if (tvg_set (&global, &desired, panel->current.cur_gain_step, panelx))
    panel->current.cur_gain0 = desired;
  else
    g_warning ("tvg0_down failed");
}

void
tvg_up (GtkWidget *widget,
         gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_gain_step + 0.5;

  if (tvg_set (&global, &panel->current.cur_gain0, desired, panelx))
    panel->current.cur_gain_step = desired;
  else
    g_warning ("tvg_up failed");
}

void
tvg_down (GtkWidget *widget,
           gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_gain_step - 0.5;

  if (tvg_set (&global, &panel->current.cur_gain0, desired, panelx))
    panel->current.cur_gain_step = desired;
  else
    g_warning ("tvg_down failed");
}


/* Функция устанавливает параметры автоВАРУ. */
gboolean
auto_tvg_set (Global   *global,
              gdouble   level,
              gdouble   sensitivity,
              gint      panelx)
{
  gboolean status;
  gchar *sens_text, *level_text;
  AmePanel *panel = get_panel (global, panelx);

  HyScanSourceType *iter;

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      status = hyscan_sonar_tvg_set_auto (global->control_s, *iter, level, sensitivity);
      if (!status)
        return FALSE;
    }

  /* Теперь печатаем что и куда надо. */
  sens_text = g_strdup_printf ("<small><b>%.1f</b></small>", sensitivity);
  level_text = g_strdup_printf ("<small><b>%.1f</b></small>", level);

  gtk_label_set_markup (panel->gui.tvg_sens_value, sens_text);
  gtk_label_set_markup (panel->gui.tvg_level_value, level_text);

  return TRUE;
}

void
tvg_level_up (GtkWidget *widget,
              gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_level + 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (&global, desired, panel->current.cur_sensitivity, panelx))
    panel->current.cur_level = desired;
  else
    g_warning ("tvg_level_up failed");
}

void
tvg_level_down (GtkWidget *widget,
                gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_level - 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (&global, desired, panel->current.cur_sensitivity, panelx))
    panel->current.cur_level = desired;
  else
    g_warning ("tvg_level_down failed");
}

void
tvg_sens_up (GtkWidget *widget,
             gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_sensitivity + 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (&global, panel->current.cur_level, desired, panelx))
    panel->current.cur_sensitivity = desired;
  else
    g_warning ("tvg_sens_up failed");
}

void
tvg_sens_down (GtkWidget *widget,
               gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_sensitivity + 0.1;
  desired = CLAMP (desired, 0.0, 1.0);

  if (auto_tvg_set (&global, panel->current.cur_level, desired, panelx))
    panel->current.cur_sensitivity = desired;
  else
    g_warning ("tvg_sens_down failed");
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
  gchar *text;
  AmePanel *panel = get_panel (global, panelx);

  if (meters < 1.0)
    return FALSE;

  receive_time = meters / (global->sound_velocity / 2.0);
  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      status = hyscan_sonar_receiver_set_time (global->control_s, *iter, receive_time, 0);
      if (!status)
        return FALSE;
    }

  text = g_strdup_printf ("<small><b>%.0f м</b></small>", meters);
  gtk_label_set_markup (panel->gui.distance_value, text);
  g_free (text);

  return TRUE;
}

void
distance_up (GtkWidget *widget,
             gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_distance + 5.0;
  if (distance_set (&global, desired, panelx))
    panel->current.cur_distance = desired;
  else
    g_warning ("distance_up failed");
}

void
distance_down (GtkWidget *widget,
               gint       panelx)
{
  gdouble desired;
  AmePanel *panel = get_panel (&global, panelx);

  desired = panel->current.cur_distance - 5.0;
  if (distance_set (&global, desired, panelx))
    panel->current.cur_distance = desired;
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
  AmePanel *panel = get_panel (&global, panelx);

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

  if (brightness_set (&global, new_brightness, panel->vis_current.black, panelx))
    panel->vis_current.brightness = new_brightness;
  else
    g_warning ("brightness_up failed");
}

void
brightness_down (GtkWidget *widget,
                 gint        panelx)
{
  gdouble new_brightness;
  AmePanel *panel = get_panel (&global, panelx);

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

  if (brightness_set (&global, new_brightness, panel->vis_current.black, panelx))
    panel->vis_current.brightness = new_brightness;
  else
    g_warning ("brightness_down failed");
}

void
black_up (GtkWidget *widget,
          gint       panelx)
{
  gdouble new_black;
  AmePanel *panel = get_panel (&global, panelx);

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

  new_black = CLAMP (new_black, 1.0, 100.0);

  if (brightness_set (&global, panel->vis_current.brightness, new_black, panelx))
    panel->vis_current.black = new_black;
  else
    g_warning ("black_up failed");
}

void
black_down (GtkWidget *widget,
            gint       panelx)
{
  gdouble new_black;
  AmePanel *panel = get_panel (&global, panelx);

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

  new_black = CLAMP (new_black, 1.0, 100.0);

  if (brightness_set (&global, panel->vis_current.brightness, new_black, panelx))
    panel->vis_current.black = new_black;
  else
    g_warning ("black_down failed");
}

void
scale_up (GtkWidget *widget,
          gint       panelx)
{
  scale_set (&global, TRUE, NULL, panelx);
}

void
scale_down (GtkWidget *widget,
            gint       panelx)
{
  scale_set (&global, FALSE, NULL, panelx);
}

/* Функция переключает режим отображения виджета ВС. */
void
mode_changed (GtkWidget *widget,
              gboolean   state,
              gint       panelx)
{
  VisualFL *fl;
  HyScanGtkForwardLookViewType type;

  AmePanel *panel = get_panel (&global, panelx);

  if (panel->type != AME_PANEL_FORWARDLOOK)
    {
      g_message ("mode_changed: wrong panel type!");
      return;
    }

  fl = (VisualFL*)panel->vis_gui;
  type = state ? HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET : HYSCAN_GTK_FORWARD_LOOK_VIEW_SURFACE;
  hyscan_gtk_forward_look_set_type (fl->fl, type);
  return;
}

gboolean
pf_special (GtkWidget  *widget,
            gboolean    state,
            gint        selector)
{
  VisualWF *wf;
  HyScanTileFlags flags = 0;
  HyScanGtkForwardLookViewType type;

  AmePanel *panel = get_panel (&global, panelx);

  if (panel->type != AME_PANEL_ECHO)
    {
      g_message ("pf_special: wrong panel type!");
      return FALSE;
    }

  wf = (VisualWF*)panel->vis_gui;
  flags = hyscan_gtk_waterfall_state_get_tile_flags (wf->wf);

  if (state)
    flags |= HYSCAN_TILE_PROFILER;
  else
    flags &= ~HYSCAN_TILE_PROFILER;

  hyscan_gtk_waterfall_state_set_tile_flags (wf->wf, flags);
  return TRUE;
}

/* Функция переключает автосдвижку. */
gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           gint        panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  AmePanel *panel = get_panel (&global, panelx);

  switch (panel->type)
    {
    case AME_PANEL_WATERFALL:
    case AME_PANEL_ECHO:
      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_automove (wf->wf, state);
      break;

    case AME_PANEL_FORWARDLOOK:

      fl = (VisualFL*)panel->vis_gui;
      if (state)
        hyscan_forward_look_player_real_time (fl->fl_player);
      else
        hyscan_forward_look_player_pause (fl->fl_player);
      break;

    default:
      g_warning ("live_view: wrong panel type!");

    }

  // TODO: live_view for ALL?
  hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (panel->vis_gui->live_view), state);

  return TRUE;
}

void
live_view_off (GtkWidget  *widget,
               gboolean    state,
               Global     *global)
{
  g_message ("live_view_off!");
  live_view (NULL, state, -1);
}

void
widget_swap (GObject  *emitter,
             gpointer  user_data)
{
  /* В userdata лежит option.
   * -2 ROTATE циклическая прокрутка
   * -1 ALL - все три,
   * 0 - гбо, 1 - пф, 2 - гк.
   */

  gint option = GPOINTER_TO_INT (user_data);
  gint selector;
  GtkWidget *will_go_right = NULL; /* Виджет, который сейчас слева. */
  GtkWidget *will_go_left = NULL; /* Требуемый виджет. */
  GList *v_kids, *h_kids;
  void (*pack_func) (GtkBox*, GtkWidget*, gboolean, gboolean, guint) = NULL; /* Функция упаковки виджета в бокс справа. */
  gchar *text;

  if (option == ROTATE)
    {
      gint new_selector;
      selector = global.view_selector;

      /* Magic time! -1 -> 0 -> 1 -> 2 -> -1... */
      new_selector = (selector + 2) % 4 - 1;
      g_message ("Changing selector from %i to %i", selector, new_selector);
      selector = new_selector;
    }
  else
    {
      selector = option;
    }

  /* Если 0 - показываем все три и готово. */
  if (selector == ALL)
    {
      widget_swap (emitter, GINT_TO_POINTER (W_SIDESCAN));
      gtk_widget_set_visible (global.gui.sub_center_box, TRUE);
      text = g_strdup_printf ("<small><b>%s</b></small>", "Всё");
      goto curr_print;
    }
  else
    {
      gtk_widget_set_visible (global.gui.sub_center_box, FALSE);
    }

  will_go_left = GTK_WIDGET (global.gui.disp_widgets[selector]);

  v_kids = gtk_container_get_children (GTK_CONTAINER (global.gui.sub_center_box));
  h_kids = gtk_container_get_children (GTK_CONTAINER (global.gui.center_box));

  will_go_right = GTK_WIDGET (h_kids->data);

  if (v_kids == NULL || v_kids->next == NULL || v_kids->next->next != NULL)
    {
      g_warning ("sub_center_box children error: ");
      if (v_kids == NULL)
        g_message ("  no children");
      else if (v_kids->next == NULL)
        g_message ("  only one child");
      else if (v_kids->next->next != NULL)
        g_message ("  too many children");
      return;
    }

  if (will_go_left == v_kids->data)
    pack_func = gtk_box_pack_start;
  else if (will_go_left == v_kids->next->data)
    pack_func = gtk_box_pack_end;
  else
    goto no_swap_needed;

  gtk_container_remove (GTK_CONTAINER (global.gui.center_box), will_go_right);
  gtk_container_remove (GTK_CONTAINER (global.gui.sub_center_box), will_go_left);

  gtk_box_pack_start (GTK_BOX (global.gui.center_box), will_go_left, TRUE, TRUE, 0);
  pack_func (GTK_BOX (global.gui.sub_center_box), will_go_right, TRUE, TRUE, 0);

no_swap_needed:
  text = g_strdup_printf ("<small><b>%s</b></small>", global.gui.widget_names[selector]);

curr_print:
  gtk_label_set_markup (GTK_LABEL (global.gui.current_view), text);
  g_free (text);

  gtk_revealer_set_reveal_child (GTK_REVEALER (global.gui.bott_revealer), selector == W_FORWARDL);

  global.view_selector = selector;
}

/* Функция включает/выключает излучение. */
void
start_stop (GtkWidget *widget,
            gboolean   state)
{
  HyScanAmeButton * button = HYSCAN_AME_BUTTON (widget);
  GHashTableIter iter;
  gpointer k, v;

  /* Включаем излучение. */
  if (state)
    {
      g_message ("Start sonars. Dry is %s", global.dry ? "ON" : "OFF");
      gint n_tracks = -1;
      gboolean status = TRUE;

      /* Закрываем текущий открытый галс. */
      g_clear_pointer (&global.track_name, g_free);

      /* Проходим по всем страницам и включаем системы локаторов. */
      g_hash_table_iter_init (&iter, global.panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        {
          AmePanel *panel = v;
          gint panelx = GPOINTER_TO_INT (k);

          /* Излучение НЕ в режиме сух. пов. */
          if (!global.dry)
            status &= signal_set (&global, panel->current.cur_signal, panelx);

          /* Приемник. */
          status &= distance_set (&global, panel->current.cur_distance, panelx);

          /* TVG */
          switch (panel->type)
            {
            case W_SIDESCAN:
              status &= auto_tvg_set (&global, panel->current.cur_level, panel->current.cur_sensitivity, panelx);
            case W_PROFILER:
            case W_FORWARDL:
            default:
              status &= tvg_set (&global, &panel->current.cur_gain0, panel->current.cur_gain_step, panelx);
            }

          /* Если не удалось запустить какую-либо подсистему. */
          if (!status)
            {
              hyscan_ame_button_set_state (button, FALSE);
              return;
            }
        }

      /* Число галсов в проекте. */
      if (n_tracks < 0)
        {
          gint32 project_id;
          gchar **tracks;
          gchar **strs;

          project_id = hyscan_db_project_open (global.db, global.project_name);
          tracks = hyscan_db_track_list (global.db, project_id);

          for (strs = tracks; strs != NULL && *strs != NULL; strs++)
            {
              n_tracks++;
            }

          hyscan_db_close (global.db, project_id);
          g_free (tracks);
        }

      /* Включаем запись нового галса. */
      global.track_name = g_strdup_printf ("%d%s", ++n_tracks, global.dry ? DRY_TRACK_SUFFIX : "");

      status = hyscan_sonar_start (global.control_s, global.project_name,
                                   global.track_name, HYSCAN_TRACK_SURVEY);

      if (!status)
        {
          g_message ("Sonar startup failed @ %i",__LINE__);
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }

      /* Если локатор включён, переходим в режим онлайн. */
      gtk_widget_set_sensitive (GTK_WIDGET (global.gui.track_view), FALSE);

      /* Включаем live_view для всех локаторов. */
      g_hash_table_iter_init (&iter, global.panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        {
          live_view (NULL, TRUE, GPOINTER_TO_INT (k));
        }
    }

  /* Выключаем излучение и блокируем режим онлайн. */
  else
    {
      g_message ("Stop sonars");
      hyscan_sonar_stop (global.control_s);
      hyscan_forward_look_player_pause (global.GFL.fl_player);
      hyscan_ame_button_set_state (button, FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (global.gui.track_view), TRUE);
    }

  start_stop_disabler (widget, state);

  return;
}

gboolean
sensor_label_writer (Global *global)
{
  g_mutex_lock (&global->nmea.lock);

  if (global->nmea.tmd_value != NULL)
    gtk_label_set_text (global->nmea.l_tmd, global->nmea.tmd_value);
  if (global->nmea.lat_value != NULL)
    gtk_label_set_text (global->nmea.l_lat, global->nmea.lat_value);
  if (global->nmea.lon_value != NULL)
    gtk_label_set_text (global->nmea.l_lon, global->nmea.lon_value);
  if (global->nmea.trk_value != NULL)
    gtk_label_set_text (global->nmea.l_trk, global->nmea.trk_value);
  if (global->nmea.spd_value != NULL)
    gtk_label_set_text (global->nmea.l_spd, global->nmea.spd_value);
  if (global->nmea.dpt_value != NULL)
    gtk_label_set_text (global->nmea.l_dpt, global->nmea.dpt_value);

  g_mutex_unlock (&global->nmea.lock);

  return G_SOURCE_CONTINUE;
}

void
nav_printer (gchar       **target,
             const gchar  *format,
             ...)
{
  va_list args;

  if (*target != NULL)
    g_clear_pointer (target, g_free);

  va_start (args, format);
  *target = g_strdup_vprintf (format, args);
  va_end (args);
}

void
sensor_cb (HyScanSensor             *sensor,
           const gchar              *name,
           HyScanSourceType          source,
           gint64                    recv_time,
           HyScanBuffer             *buffer,
           Global                   *global)
{
  gchar **nmea;
  const gchar * rmc = NULL, *dpt = NULL, *raw_data;
  guint32 size;
  guint i;
  gdouble time = 0, date, lat, lon, trk, spd, dpt_;

  /* Ищем строки RMC и DPT. */
  if (source != HYSCAN_SOURCE_NMEA_RMC && source != HYSCAN_SOURCE_NMEA_DPT)
    return;

  raw_data = hyscan_buffer_get_data (buffer, &size);

  nmea = g_strsplit_set (raw_data, "\r\n", -1);
  for (i = 0; (nmea != NULL) && (nmea[i] != NULL); i++)
    {
      if (g_str_has_prefix (nmea[i]+3, "RMC"))
        rmc = nmea[i];
      else if (g_str_has_prefix (nmea[i]+3, "DPT"))
        dpt = nmea[i];
    }

  /* Обновляем данные виджета навигации. */
  g_mutex_lock (&global->nmea.lock);

  if (hyscan_nmea_parser_parse_string (global->nmea.time, rmc, &time) &&
      hyscan_nmea_parser_parse_string (global->nmea.date, rmc, &date))
    {
      GTimeZone *tz;
      GDateTime *local, *utc;
      gchar *dtstr;

      tz = g_time_zone_new ("+03:00");
      utc = g_date_time_new_from_unix_utc (date + time);
      local = g_date_time_to_timezone (utc, tz);

      dtstr = g_date_time_format (local, "%d.%m.%y %H:%M:%S");

      nav_printer (&global->nmea.tmd_value, "%s", dtstr);

      g_free (dtstr);
      g_date_time_unref (local);
      g_date_time_unref (utc);
      g_time_zone_unref (tz);
    }

  if (hyscan_nmea_parser_parse_string (global->nmea.lat, rmc, &lat))
    nav_printer (&global->nmea.lat_value, "%f °%s", lat, lat > 0 ? "С.Ш." : "Ю.Ш.");
  if (hyscan_nmea_parser_parse_string (global->nmea.lon, rmc, &lon))
    nav_printer (&global->nmea.lon_value, "%f °%s", lon, lon > 0 ? "В.Д." : "З.Д.");
  if (hyscan_nmea_parser_parse_string (global->nmea.trk, rmc, &trk))
    nav_printer (&global->nmea.trk_value, "%f °", trk);
  if (hyscan_nmea_parser_parse_string (global->nmea.spd, rmc, &spd))
    nav_printer (&global->nmea.spd_value, "%f м/с", spd * 0.514);
  if (hyscan_nmea_parser_parse_string (global->nmea.dpt, dpt, &dpt_))
    nav_printer (&global->nmea.dpt_value, "%f м", dpt_);

  g_mutex_unlock (&global->nmea.lock);
  g_strfreev (nmea);
}

gboolean
start_stop_disabler (GtkWidget *sw,
                     gboolean   state)
{
  if (sw == global.gui.start_stop_dry_switch)
    {
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (global.gui.start_stop_all_switch), !state);
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_SIDESCAN]), !state);
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_FORWARDL]), !state);
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_PROFILER]), !state);
    }
  else
    {
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (global.gui.start_stop_dry_switch), !state);

      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_all_switch), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_SIDESCAN]), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_PROFILER]), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_FORWARDL]), state);
    }

  return FALSE;
}

/* Функция включает/выключает сухую поверку. */
gboolean
start_stop_dry (GtkWidget *widget,
                gboolean   state)
{
  global.dry = state;

  start_stop (widget, state);

  return TRUE;
}

// gboolean
// ame_key_func (gpointer user_data)
// {
//   Global *global = user_data;
//   gint keycode;

//   keycode = g_atomic_int_get (&global->urpc_keycode);

//   if (keycode == URPC_KEYCODE_NO_ACTION)
//     return G_SOURCE_CONTINUE;

//   switch (keycode)
//     {
//     default:
//       g_message ("This key is not supported yet. ");
//     }

//   /* Сбрасываем значение кнопки, если его больше никто не перетер. */
//   g_atomic_int_compare_and_exchange (&global->urpc_keycode, keycode, URPC_KEYCODE_NO_ACTION);

//   return G_SOURCE_CONTINUE;
// }

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

void
ame_colormap_free (gpointer data)
{
  AmeColormap *cmap = data;
  if (cmap == NULL)
    return;

  g_clear_pointer (cmap->colors, g_free);
  g_clear_pointer (cmap->name, g_free);

  g_free (cmap);
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
      g_array_append_val (colormaps, new_map);

      return colormaps;
    }

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Вонючий Калькулятор");
  new_map->colors = g_memdup (orange, 256 * sizeof (guint32));
  new_map->len = 256;
  new_map->bg = BLACK_BG;
  g_array_append_val (colormaps, new_map);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Максимум Ретро");
  new_map->colors = g_memdup (sepia, 256 * sizeof (guint32));
  new_map->len = 256;
  new_map->bg = BLACK_BG;
  g_array_append_val (colormaps, new_map);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Белоснежный");
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_val (colormaps, new_map);

  new_map = g_new (AmeColormap, 1);
  new_map->name = g_strdup ("Киберпанк");
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.11, 1.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (0.83, 0.0, 1.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_val (colormaps, new_map);

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

  GtkBuilder        *common_builder = NULL;
  GtkWidget         *fl_play_control = NULL;

  // HyScanAmeSplash   *splash = NULL;
  guint              sensor_label_writer_tag = 0;

  gboolean status;

  gtk_init (&argc, &argv);

  /* Разбор командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionContext *context;

    GOptionEntry common_entries[] =
      {
        { "cache-size",      'c',   0, G_OPTION_ARG_INT,     &cache_size,      "Cache size, Mb", NULL },
        { "db-uri",          'd',   0, G_OPTION_ARG_STRING,  &db_uri,          "HyScan DB uri", NULL },

        { "sound-velocity",  'v',   0, G_OPTION_ARG_DOUBLE,  &sound_velocity,  "Sound velocity, m/s", NULL },
        { "ship-speed",      'e',   0, G_OPTION_ARG_DOUBLE,  &ship_speed,      "Ship speed, m/s", NULL },
        { "full-screen",     'f',   0, G_OPTION_ARG_NONE,    &full_screen,     "Full screen mode", NULL },
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
        return 0;
      }

    if (args[1] != NULL)
      config_file = g_strdup (args[1]);

    g_option_context_free (context);
    g_strfreev (args);
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
  cache_size = MAX (256, cache_size);
  global.cache = HYSCAN_CACHE (hyscan_cached_new (cache_size));

  /* Подключение к базе данных. */
  global.db = hyscan_db_new (db_uri);
  hyscan_exit_if_w_param (global.db == NULL, "can't connect to db '%s'", db_uri);

  /* Проект пытаемся считать из файла. */
  project_name = keyfile_string_read_helper (config, "common", "project");

  if (project_name == NULL)
    {
      HyScanDBInfo *info = hyscan_db_info_new (global.db);
      GtkWidget *dialog;
      gint res;

      dialog = hyscan_ame_project_new (global.db, info, NULL);
      res = gtk_dialog_run (GTK_DIALOG (dialog));
      hyscan_ame_project_get (HYSCAN_AME_PROJECT (dialog), &project_name, NULL);

      /* Если был нажат крестик - сорян, сваливаем отсюда. */
      if (res != HYSCAN_AME_PROJECT_OPEN && res != HYSCAN_AME_PROJECT_CREATE)
        {
          gtk_widget_destroy (dialog);
          goto exit;
        }

      gtk_widget_destroy (dialog);
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
    HyScanHWProfile *hw_profile;
    gboolean check;

    driver_paths = keyfile_strv_read_helper (config, "common", "paths");
    sonar_profile_name = keyfile_string_read_helper (config, "common", "hardware");

    /* Првоеряем, что пути к драйверам и имя профиля на месте. */

    hw_profile = hyscan_hw_profile_new (sonar_profile_name);
    hyscan_hw_profile_set_driver_paths (hw_profile, driver_paths);

    /* Читаем профиль. */
    hyscan_hw_profile_read (hw_profile);

    check = hyscan_hw_profile_check (hw_profile);

    if (check)
      {
        global.control = hyscan_hw_profile_connect (hw_profile);
        global.control_s = HYSCAN_SONAR (global.control);
      }

    g_strfreev (driver_paths);
    g_object_unref (hw_profile);

    if (!check || global.control == NULL)
      goto no_sonar;
  }

  if (global.control != NULL)
    {
      const HyScanSonarInfoSource *info;
      global.infos = g_hash_table_new (g_direct_hash, g_direct_equal);

      info = hyscan_control_source_get_info (global.control, FORWARDLOOK);
      if (info != NULL)
        g_hash_table_insert (global.infos, GINT_TO_POINTER (FORWARDLOOK), (void*)info);
      info = hyscan_control_source_get_info (global.control, PROFILER);
      if (info != NULL)
        g_hash_table_insert (global.infos, GINT_TO_POINTER (PROFILER), (void*)info);
      info = hyscan_control_source_get_info (global.control, ECHOSOUNDER);
      if (info != NULL)
        g_hash_table_insert (global.infos, GINT_TO_POINTER (ECHOSOUNDER), (void*)info);
      info = hyscan_control_source_get_info (global.control, STARBOARD);
      if (info != NULL)
        g_hash_table_insert (global.infos, GINT_TO_POINTER (STARBOARD), (void*)info);
      info = hyscan_control_source_get_info (global.control, PORTSIDE);
      if (info != NULL)
        g_hash_table_insert (global.infos, GINT_TO_POINTER (PORTSIDE), (void*)info);
    }

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
  /* Билдеры. */
  common_builder = gtk_builder_new_from_resource ("/org/triple/gtk/common.ui");

  /* Основное окно программы. */
  global.gui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (global.gui.window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (global.gui.window), "key-press-event", G_CALLBACK (key_press), &global);
  gtk_window_set_title (GTK_WINDOW (global.gui.window), "ГБОГКПФ");
  gtk_window_set_default_size (GTK_WINDOW (global.gui.window), 1600, 900);

  /* Основная раскладка окна. */
  global.gui.grid = gtk_grid_new ();

  /* Создадим парсеры. */
  {
    global.nmea.l_tmd = GTK_LABEL (gtk_builder_get_object (common_builder, "tmd"));
    global.nmea.l_lat = GTK_LABEL (gtk_builder_get_object (common_builder, "lat"));
    global.nmea.l_lon = GTK_LABEL (gtk_builder_get_object (common_builder, "lon"));
    global.nmea.l_trk = GTK_LABEL (gtk_builder_get_object (common_builder, "trk"));
    global.nmea.l_spd = GTK_LABEL (gtk_builder_get_object (common_builder, "spd"));
    global.nmea.l_dpt = GTK_LABEL (gtk_builder_get_object (common_builder, "dpt"));

    global.nmea.time = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_RMC, HYSCAN_NMEA_FIELD_TIME);
    global.nmea.date = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_RMC, HYSCAN_NMEA_FIELD_DATE);
    global.nmea.lat = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_RMC, HYSCAN_NMEA_FIELD_LAT);
    global.nmea.lon = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_RMC, HYSCAN_NMEA_FIELD_LON);
    global.nmea.trk = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_RMC, HYSCAN_NMEA_FIELD_TRACK);
    global.nmea.spd = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_RMC, HYSCAN_NMEA_FIELD_SPEED);
    global.nmea.dpt = hyscan_nmea_parser_new_empty (HYSCAN_SOURCE_NMEA_DPT, HYSCAN_NMEA_FIELD_DEPTH);

    g_mutex_init (&global.nmea.lock);
  }

  /* Слева у нас список галсов и меток. */
  {
    GtkWidget *track_control, *mark_list, *mark_editor;
    global.gui.left_box = gtk_grid_new ();
    track_control = GTK_WIDGET (gtk_builder_get_object (common_builder, "track_control"));
    hyscan_exit_if (track_control == NULL, "can't load track control ui");

    global.gui.track_view = GTK_TREE_VIEW (gtk_builder_get_object (common_builder, "track_view"));
    global.gui.track_list = GTK_TREE_MODEL (gtk_builder_get_object (common_builder, "track_list"));
    global.gui.track_range = GTK_ADJUSTMENT (gtk_builder_get_object (common_builder, "track_range"));
    hyscan_exit_if ((global.gui.track_view == NULL) ||
                    (global.gui.track_list == NULL) ||
                    (global.gui.track_range == NULL),
                    "incorrect track control ui");
    /* Сортировка списка галсов. */
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (global.gui.track_list), 0, GTK_SORT_DESCENDING);

    /* Список меток. */
    global.marks.model = hyscan_mark_model_new ();
    global.gui.mark_view = mark_list = hyscan_gtk_project_viewer_new ();
    global.gui.meditor = mark_editor = hyscan_gtk_mark_editor_new ();

    g_object_set (track_control, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                                 "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mark_list, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                             "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mark_editor, "vexpand", FALSE, "valign", GTK_ALIGN_END,
                                "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);

    gtk_grid_attach (GTK_GRID (global.gui.left_box), track_control, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (global.gui.left_box), mark_list, 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (global.gui.left_box), mark_editor, 0, 2, 1, 1);


    /* Список галсов кладем слева. */
    global.marks.loc_storage = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                      (GDestroyNotify) loc_store_free);

    g_signal_connect (global.marks.model, "changed", G_CALLBACK (mark_manager_changed), &global);
    g_signal_connect (mark_editor, "mark-modified", G_CALLBACK (mark_modified), &global);
    g_signal_connect (mark_list, "item-changed", G_CALLBACK (active_mark_changed), &global);

    hyscan_mark_model_set_project (global.marks.model, global.db, global.project_name);
  }

  /* Ништяк: скелет гуя сделан. Теперь подцепим тут нужные сигналы. */
  gtk_builder_add_callback_symbol (common_builder, "track_scroll", G_CALLBACK (track_scroll));
  gtk_builder_add_callback_symbol (common_builder, "track_changed", G_CALLBACK (track_changed));
  gtk_builder_add_callback_symbol (common_builder, "position_changed", G_CALLBACK (position_changed));
  gtk_builder_connect_signals (common_builder, &global);

  // hyscan_ame_splash_stop (splash);

  /***
   *  ___   ___         ___         ___
   * |   | |   | |\  | |     |     |
   * |-+-  |-+-| | + | |-+-  |      -+-
   * |     |   | |  \| |___  |___   ___|
   *
   * Создаем панели.
   */
  { /* ГБО */
    AmePanel *panel = g_new0 (AmePanel, 1);
    VisualWF *vwf = g_new0 (VisualWF, 1);

    panel->name = g_strdup ("SideScan");
    panel->type = AME_PANEL_WATERFALL;

    panel->sources = g_new0 (HyScanSourceType, 3);
    panel->sources[0] = HYSCAN_SOURCE_SIDE_SCAN_STARBOARD;
    panel->sources[1] = HYSCAN_SOURCE_SIDE_SCAN_PORT;
    panel->sources[2] = HYSCAN_SOURCE_INVALID;

    panel->vis_gui = (VisualCommon*)vwf;

    vwf->colormaps = make_color_maps (FALSE);

    vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global.cache));
    gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

    GtkWidget *ss_ol = make_overlay (vwf->wf,
                                     &vwf->wf_grid, &vwf->wf_ctrl,
                                     &vwf->wf_mark, &vwf->wf_metr,
                                     global.marks.model);

    g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (live_view_off), &global);
    g_signal_connect_swapped (vwf->wf, "waterfall-zoom", G_CALLBACK (scale_set), &global);

    hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), ship_speed);
    hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
    hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
    hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

    g_hash_table_insert (global.panels, panel, GINT_TO_POINTER (X_SIDESCAN));
  }

  { /* ПФ */
    AmePanel *panel = g_new0 (AmePanel, 1);
    VisualWF *vwf = g_new0 (VisualWF, 1);

    panel->name = g_strdup ("Profiler");
    panel->type = AME_PANEL_ECHO;

    panel->sources = g_new0 (HyScanSourceType, 2);
    panel->sources[0] = HYSCAN_SOURCE_SIDE_SCAN_PROFILER;
    // TODO echosounder?
    panel->sources[1] = HYSCAN_SOURCE_INVALID;

    panel->vis_gui = (VisualCommon*)vwf;

    vwf->colormaps = make_color_maps (TRUE);

    vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global.cache));
    gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

    GtkWidget *ss_ol = make_overlay (vwf->wf,
                                     &vwf->wf_grid, &vwf->wf_ctrl,
                                     &vwf->wf_mark, &vwf->wf_metr,
                                     global.marks.model);

    hyscan_gtk_waterfall_grid_set_condence (vwf->wf_grid, 10.0);
    hyscan_gtk_waterfall_grid_set_grid_color (vwf->wf_grid, hyscan_tile_color_converter_c2i (32, 32, 32, 255));

    g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (live_view_off), &global);
    g_signal_connect_swapped (vwf->wf, "waterfall-zoom", G_CALLBACK (scale_set), &global);

    hyscan_gtk_waterfall_state_echosounder (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), PROFILER);
    hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), ship_speed / 10);
    hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
    hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
    hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

    g_hash_table_insert (global.panels, panel, GINT_TO_POINTER (X_PROFILER));
  }

  { /* ВСЛ */
    AmePanel *panel = g_new0 (AmePanel, 1);
    VisualFL *vfl = g_new0 (VisualFL, 1);

    panel->name = g_strdup ("ForwardLook");
    panel->type = AME_PANEL_FORWARDLOOK;

    panel->sources = g_new0 (HyScanSourceType, 2);
    panel->sources[0] = HYSCAN_SOURCE_FORWARD_LOOK;
    panel->sources[1] = HYSCAN_SOURCE_INVALID;

    panel->vis_gui = (VisualCommon*)vfl;

    vfl->fl = HYSCAN_GTK_FORWARD_LOOK (hyscan_gtk_forward_look_new ());
    gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vfl->fl), TRUE);

    vfl->fl_player = hyscan_gtk_forward_look_get_player (vfl->fl);
    vfl->fl_coords = hyscan_fl_coords_new (vfl->fl, global.cache);
    hyscan_forward_look_player_set_cache (vfl->fl_player, global.cache, NULL);

    /* Управление воспроизведением FL. */
    fl_play_control = GTK_WIDGET (gtk_builder_get_object (common_builder, "fl_play_control"));
    hyscan_exit_if (fl_play_control == NULL, "can't load play control ui");

    vfl->position = GTK_SCALE (gtk_builder_get_object (common_builder, "position"));
    vfl->coords_label = GTK_LABEL (gtk_builder_get_object (common_builder, "fl_latlong"));
    g_signal_connect (vfl->fl_coords, "coords", G_CALLBACK (fl_coords_callback), vfl->coords_label);
    hyscan_exit_if (vfl->position == NULL, "incorrect play control ui");
    vfl->position_range = hyscan_gtk_forward_look_get_adjustment (vfl->fl);
    gtk_range_set_adjustment (GTK_RANGE (vfl->position), vfl->position_range);

    hyscan_forward_look_player_set_sv (wfl->fl_player, global.sound_velocity);

    g_hash_table_insert (global.panels, panel, GINT_TO_POINTER (X_FORWARDL));
  }

  /*  */
  global.gui.widget_names[W_SIDESCAN] = "ГБО";
  global.gui.widget_names[W_PROFILER] = "Профилограф";
  global.gui.widget_names[W_FORWARDL] = "Курсовой";

  /* Центральная зона: два box'a (наполним позже). */
  global.gui.sub_center_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  global.gui.center_box     = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);

  global.gui.disp_widgets[W_SIDESCAN] = g_object_ref (ss_ol);
  global.gui.disp_widgets[W_PROFILER] = g_object_ref (pf_ol);
  global.gui.disp_widgets[W_FORWARDL] = g_object_ref (global.GFL.fl);

  gint i;
  for (i = W_SIDESCAN; i < W_LAST; ++i)
    {
      GObject *obj = G_OBJECT (global.gui.disp_widgets[i]);

      g_object_set (obj, "vexpand", TRUE, "valign", GTK_ALIGN_FILL, "hexpand", TRUE, "halign", GTK_ALIGN_FILL, NULL);
    }

  gtk_box_pack_end (GTK_BOX (global.gui.center_box), global.gui.sub_center_box, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (global.gui.center_box), global.gui.disp_widgets[W_SIDESCAN], TRUE, TRUE, 1);

  gtk_box_pack_start (GTK_BOX (global.gui.sub_center_box), global.gui.disp_widgets[W_PROFILER], TRUE, TRUE, 1);
  gtk_box_pack_end (GTK_BOX (global.gui.sub_center_box), global.gui.disp_widgets[W_FORWARDL], TRUE, TRUE, 1);

  // gtk_cifro_area_control_set_scroll_mode (GTK_CIFRO_AREA_CONTROL (global.fl), GTK_CIFRO_AREA_SCROLL_MODE_COMBINED);
  //
  // /* Управление отображением. */
  // global.fl_player = hyscan_gtk_forward_look_get_player (global.fl);
  //
  // /* Полоска прокрутки. */
  // global.position_range = hyscan_gtk_forward_look_get_adjustment (global.fl);
  // gtk_range_set_adjustment (GTK_RANGE (global.position), global.position_range);
  global.gui.side_revealer = g_object_new (GTK_TYPE_REVEALER, "transition-type", GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT, "transition-duration", 100, NULL);
  gtk_container_add (GTK_CONTAINER (global.gui.side_revealer), global.gui.left_box);

  global.gui.bott_revealer = g_object_new (GTK_TYPE_REVEALER, "transition-type", GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP, "transition-duration", 100, NULL);
  gtk_container_add (GTK_CONTAINER (global.gui.bott_revealer), fl_play_control);

  gtk_revealer_set_reveal_child (GTK_REVEALER (global.gui.bott_revealer), FALSE);

  {
    GtkWidget *nav_indicators;
    nav_indicators = GTK_WIDGET (gtk_builder_get_object (common_builder, "nav_indicators"));

    GtkGrid *grid = GTK_GRID (global.gui.grid);
    global.gui.lstack = gtk_stack_new ();
    global.gui.rstack = gtk_stack_new ();

    build_all (common_pages, GTK_STACK (global.gui.lstack), GTK_STACK (global.gui.rstack));
    build_all (sonar_pages, GTK_STACK (global.gui.lstack), GTK_STACK (global.gui.rstack));

    gtk_grid_attach (GTK_GRID (grid), global.gui.lstack,          0, 0, 1, 3);
    gtk_grid_attach (GTK_GRID (grid), global.gui.rstack,          1, 0, 1, 3);
    gtk_grid_attach (GTK_GRID (grid), global.gui.side_revealer,   2, 0, 1, 3);
    gtk_grid_attach (GTK_GRID (grid), global.gui.center_box,      4, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), global.gui.bott_revealer,   4, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), nav_indicators,             4, 2, 1, 1);
  }

  gtk_container_add (GTK_CONTAINER (global.gui.window), global.gui.grid);
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
        panel->current.cur_distance =    keyfile_double_read_helper (config, panel->name, "sonar.cur_distance", 50);
        panel->current.cur_signal =      keyfile_double_read_helper (config, panel->name, "sonar.cur_signal", 1);
        panel->current.cur_gain0 =       keyfile_double_read_helper (config, panel->name, "sonar.cur_gain0", 0);
        panel->current.cur_gain_step =   keyfile_double_read_helper (config, panel->name, "sonar.cur_gain_step", 10);
        panel->current.cur_level =       keyfile_double_read_helper (config, panel->name, "sonar.cur_level", 0.5);
        panel->current.cur_sensitivity = keyfile_double_read_helper (config, panel->name, "sonar.cur_sensitivity", 0.6);

        panel->vis_current.brightness =  keyfile_double_read_helper (config, panel->name, "cur_brightness",   80.0);
        panel->vis_current.colormap =    keyfile_double_read_helper (config, panel->name, "cur_color_map",    0);
        panel->vis_current.black =       keyfile_double_read_helper (config, panel->name, "cur_black",        0);
        panel->vis_current.sensitivity = keyfile_double_read_helper (config, panel->name, "cur_sensitivity",  8.0);


        brightness_set (&global, panel->vis_current.brightness, panel->vis_current.black, panelx);
        if (panel->type == AME_PANEL_WATERFALL || panel->type == AME_PANEL_ECHO)
          color_map_set (&global, panel->vis_current.colormap, panelx);

        scale_set (&global, FALSE, NULL, panelx);

        /* Для локаторов мы ничего не задаем,
         * т.к. эти параметры будут
         * установлены в старт-стоп. */
      }
  }

  widget_swap (NULL, GINT_TO_POINTER (ALL));

  if (full_screen)
    gtk_window_fullscreen (GTK_WINDOW (global.gui.window));

  gtk_widget_show_all (global.gui.window);

  global.marks.sync = hyscan_mark_sync_new ("127.0.0.1", 10010);


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
  if (config != NULL)
    {
      GHashTableIter iter;
      gpointer k, v;
      g_hash_table_iter_init (&iter, global.panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        {
          AmePanel *panel = v;
          keyfile_double_write_helper (config, panel->name, "sonar.cur_distance", panel->current.cur_distance);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_signal", panel->current.cur_signal);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_gain0", panel->current.cur_gain0);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_gain_step", panel->current.cur_gain_step);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_level", panel->current.cur_level);
          keyfile_double_write_helper (config, panel->name, "sonar.cur_sensitivity", panel->current.cur_sensitivity);

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

  g_clear_object (&common_builder);

  g_clear_object (&global.cache);
  g_clear_object (&global.db_info);
  g_clear_object (&global.db);

  g_free (global.track_name);
  g_free (db_uri);
  g_free (project_name);
  g_free (config_file);

  // g_clear_object (&splash);

  return 0;
}
