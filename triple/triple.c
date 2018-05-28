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

static void
depth_writer (GObject *emitter)
{
  guint32 first, last, i;
  HyScanAntennaPosition antenna;
  HyScanGeoGeodetic coord;
  GString *string = NULL;
  gchar *words = NULL;
  GError *error = NULL;

  HyScanDB * db = global.db;
  gchar *project = global.project_name;
  gchar *track = global.track_name;

  HyScanNavData * dpt = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, project, track,
                                         1, HYSCAN_SOURCE_NMEA_DPT, HYSCAN_NMEA_FIELD_DEPTH));

  if (dpt == NULL)
    {
      g_warning ("Failed to open dpt parser.");
      return;
    }

  HyScanmLoc * mloc = hyscan_mloc_new (db, project, track);
  if (mloc == NULL)
    {
      g_warning ("Failed to mLocation.");
      g_clear_object (&dpt);
      return;
    }

  hyscan_mloc_set_cache (mloc, global.cache);

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

      status = hyscan_mloc_get (mloc, time, &antenna, 0, &coord);
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
                       words, strlen (words),
                       &error);

  if (error != NULL)
    {
      g_message ("Depth save failure: %s", error->message);
      g_error_free (error);
    }

  g_free (words);
}

static void
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

static void
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

static void
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
static void
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

static HyScanDataSchemaEnumValue *
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
static gboolean
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

static gboolean
idle_key (GdkEvent *event)
{
  gtk_main_do_event (event);
  gdk_event_free (event);
  g_message ("Event! %i %i", event->key.keyval, event->key.state);
  return G_SOURCE_REMOVE;
}

static void
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

static void
nav_del (GObject *emitter,
         gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Delete, GDK_CONTROL_MASK);
}

static void
nav_pg_up (GObject *emitter,
           gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Page_Up, GDK_CONTROL_MASK);
}

static void
nav_pg_down (GObject *emitter,
        gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Page_Down, GDK_CONTROL_MASK);
}

static void
nav_up (GObject *emitter,
        gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Up, GDK_CONTROL_MASK);
}
static void
nav_down (GObject *emitter,
        gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Down, GDK_CONTROL_MASK);
}

static void
nav_left (GObject *emitter,
        gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Left, GDK_CONTROL_MASK);
}
static void
nav_right (GObject *emitter,
        gpointer udata)
{
  gint selector = GPOINTER_TO_INT (udata);

  nav_common (global.gui.disp_widgets[selector], GDK_KEY_Right, GDK_CONTROL_MASK);
}

static void
fl_prev (GObject *emitter,
         gpointer udata)
{
  gdouble value;
  value = gtk_adjustment_get_value (global.GFL.position_range);
  gtk_adjustment_set_value (global.GFL.position_range, value - 1);
}

static void
fl_next (GObject *emitter,
         gpointer udata)
{
  gdouble value;

  value = gtk_adjustment_get_value (global.GFL.position_range);
  gtk_adjustment_set_value (global.GFL.position_range, value + 1);
}


static void
run_manager (GObject *emitter)
{
  HyScanDBInfo *info;
  GtkWidget *dialog;
  gint res;

  info = hyscan_db_info_new (global.db);
  dialog = hyscan_ame_project_new (global.db, info, global.gui.window);
  res = gtk_dialog_run (GTK_DIALOG (dialog));

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
      hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GFL.sonar.sonar_ctl), project);
      hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GPF.sonar.sonar_ctl), project);
      hyscan_mark_manager_set_project (global.mman, global.db, project);
      g_free (project);
    }

  g_object_unref (info);
  gtk_widget_destroy (dialog);
}


/* Функция вызывается при изменении списка проектов. */
static void
projects_changed (HyScanDBInfo *db_info,
                  Global       *global)
{
  GHashTable *projects = hyscan_db_info_get_projects (db_info);

  /* Если рабочий проект есть в списке, мониторим его. */
  if (g_hash_table_lookup (projects, global->project_name))
    hyscan_db_info_set_project (db_info, global->project_name);

  g_hash_table_unref (projects);
}

static GtkTreePath *
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

static GtkTreePath *
ame_gtk_tree_path_prev (GtkTreePath *path)
{
  gtk_tree_path_prev (path);
  return path;
}

static GtkTreePath *
ame_gtk_tree_path_next (GtkTreePath *path)
{
  gtk_tree_path_next (path);
  return path;
}

static void
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

static GtkTreeView *
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

static void
list_scroll_up (GObject *emitter,
                gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, TRUE, FALSE);
}

static void
list_scroll_down (GObject *emitter,
                  gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, FALSE, FALSE);
}

static void
list_scroll_start (GObject *emitter,
                   gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, TRUE, TRUE);
}
static void
list_scroll_end (GObject *emitter,
                 gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, FALSE, TRUE);
}

/* Функция вызывается при изменении списка галсов. */
static void
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

static void
active_mark_changed (HyScanGtkProjectViewer *marks_viewer,
                     Global                 *global)
{
  const gchar *mark_id;
  GHashTable *marks;
  HyScanMarkManagerMarkLoc *mark;

  mark_id = hyscan_gtk_project_viewer_get_selected_item (marks_viewer);

  if ((marks = hyscan_mark_manager_get_w_coords (global->mman)) == NULL)
    return;

  if ((mark = g_hash_table_lookup (marks, mark_id)) != NULL)
    {
      hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (global->gui.meditor),
                                       mark_id,
                                       mark->mark->name,
                                       mark->mark->operator_name,
                                       mark->mark->description,
                                       mark->lat,
                                       mark->lon);
    }

  g_hash_table_unref (marks);
}

static inline gboolean
ame_float_equal (gdouble a, gdouble b)
{
  return (ABS(a - b) < 1e-6);
}
static gboolean
marks_equal (HyScanMarkManagerMarkLoc *a,
             HyScanMarkManagerMarkLoc *b)
{
  gboolean coords;
  gboolean name;
  gboolean size;

  if (a == NULL || b == NULL)
    {
      g_warning ("Null mark. It's a mistake");
      return FALSE;
    }

  coords = ame_float_equal (a->lat,b->lat) && ame_float_equal (a->lon,b->lon);
  name = g_strcmp0 (a->mark->name, b->mark->name) == 0;
  size = (a->mark->width == b->mark->width) && (a->mark->height == b->mark->height);

  return (coords && name && size);
}

static void
mark_processor (gpointer        key,
                gpointer        value,
                GHashTable     *source,
                HyScanMarkSync *sync,
                gboolean        this_is_an_old_list)
{
  HyScanMarkManagerMarkLoc *our, *their;

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

static void
mark_sync_func (GHashTable *marks,
                Global     *global)
{
  GHashTableIter iter;
  gpointer key, value;

  if (global->marksync.previous == NULL)
    {
      global->marksync.previous = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                         (GDestroyNotify)hyscan_mark_manager_mark_loc_free);
      goto copy;
    }

  /* Надо пройтись сначала по старому, чтобы знать, что удалилось.
   * А потом по актуальному списку, посмотреть,
   * что появилось нового, а что поменялось */

  g_hash_table_iter_init (&iter, global->marksync.previous);
  while (g_hash_table_iter_next (&iter, &key, &value))
    mark_processor (key, value, marks, global->marksync.msync, TRUE);

  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    mark_processor (key, value, global->marksync.previous, global->marksync.msync, FALSE);

  g_hash_table_remove_all (global->marksync.previous);

  copy:
  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_hash_table_insert (global->marksync.previous, g_strdup (key), hyscan_mark_manager_mark_loc_copy (value));
}

static void
mark_manager_changed (HyScanMarkManager *mark_manager,
                      Global            *global)
{
  GtkTreeIter tree_iter;
  GHashTable *marks;
  GHashTableIter marks_iter;
  gpointer key, value;
  const gchar *mark_id;
  GtkListStore *ls;

  hyscan_gtk_project_viewer_clear (HYSCAN_GTK_PROJECT_VIEWER (global->gui.mark_view));

  if ((marks = hyscan_mark_manager_get_w_coords (mark_manager)) == NULL)
    return;

  /* Сравниваем с предыдущими метками. Если надо, отсылаем наружу. */
  mark_sync_func (marks, global);
  ls = hyscan_gtk_project_viewer_get_liststore (HYSCAN_GTK_PROJECT_VIEWER (global->gui.mark_view));

  g_hash_table_iter_init (&marks_iter, marks);
  while (g_hash_table_iter_next (&marks_iter, &key, &value))
    {
      mark_id = key;
      HyScanMarkManagerMarkLoc *mark = value;
      GDateTime *mtime;
      gchar *mtime_str;

      mtime = g_date_time_new_from_unix_local (mark->mark->modification_time / 1e6);
      mtime_str =  g_date_time_format (mtime, "%d.%m %H:%M");

      gtk_list_store_append (ls, &tree_iter);
      gtk_list_store_set (ls, &tree_iter,
                          0, mark_id,
                          1, mark->mark->name,
                          2, mtime_str,
                          3, mark->mark->modification_time,
                          -1);

      g_free (mtime_str);
      g_date_time_unref (mtime);
    }

  g_hash_table_unref (marks);
}

static void
mark_modified (HyScanGtkMarkEditor *med,
               Global              *global)
{
  gchar *mark_id = NULL;
  GHashTable *marks;
  HyScanWaterfallMark *mark;

  hyscan_gtk_mark_editor_get_mark (med, &mark_id, NULL, NULL, NULL);

  if ((marks = hyscan_mark_manager_get (global->mman)) == NULL)
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

      hyscan_mark_manager_modify_mark (global->mman, mark_id, modified_mark);

      hyscan_waterfall_mark_free (modified_mark);
    }

  g_free (mark_id);
  g_hash_table_unref (marks);
}

/* Функция прокручивает список галсов. */
static gboolean
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

static gchar *
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
      if (!current->active)
        continue;

      active = g_strdup (current->name);
      break;
    }

  g_hash_table_unref (tracks);
  return active;
}

static gboolean
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
      if (g_strcmp0 (current->name, name) != 0)
        continue;

      active = current->active;
    }

  g_hash_table_unref (tracks);
  return active;
}

/* Обработчик изменения галса. */
static void
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

  hyscan_forward_look_player_open (global->GFL.fl_player, global->db, global->project_name, track_name, TRUE);
  hyscan_fl_coords_set_project (global->GFL.fl_coords, global->db, global->project_name, track_name);
  hyscan_gtk_waterfall_state_set_track (HYSCAN_GTK_WATERFALL_STATE (global->GSS.wf),
                                        global->db, global->project_name, track_name, TRUE);

  pftrack = g_strdup_printf (PF_TRACK_PREFIX "%s", track_name);
  hyscan_gtk_waterfall_state_set_track (HYSCAN_GTK_WATERFALL_STATE (global->GPF.wf),
                                        global->db, global->project_name, pftrack, FALSE);
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
static void
position_changed (GtkAdjustment *range,
                  Global        *global)
{
}

/* Функция устанавливает яркость отображения. */
static gboolean
brightness_set (Global  *global,
                gdouble  cur_brightness,
                gdouble  cur_black,
                gint     selector)
{
  gchar *text_bright;
  gchar *text_black;
  gdouble b, g, w;

  if (cur_brightness < 1.0)
    return FALSE;
  if (cur_brightness > 100.0)
    return FALSE;
  if (cur_black < 0.0)
    return FALSE;
  if (cur_black > 100.0)
    return FALSE;

  text_bright = g_strdup_printf ("<small><b>%.0f%%</b></small>", cur_brightness);
  text_black = g_strdup_printf ("<small><b>%.0f%%</b></small>", cur_black);

  g_message ("Brightness_set: sonar#%i, white: %f, black: %f", selector, cur_brightness, cur_black);
  switch (selector)
    {
    case W_SIDESCAN:
      b = 0;
      w = 1.0 - (cur_brightness / 100.0) * 0.99;
      g = 1.25 - 0.5 * (cur_brightness / 100.0);
      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (global->GSS.wf), b, g, w);
      gtk_label_set_markup (global->GSS.gui.brightness_value, text_bright);
      break;
    case W_PROFILER:
      b = cur_black * 0.0000025;
      w = 1.0 - (cur_brightness / 100.0) * 0.99;
      w *= w;
      g = 1;
      if (b >= w)
        {
          g_message ("BBC error");
          return FALSE;
        }

      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (global->GPF.wf), b, g, w);
      gtk_label_set_markup (global->GPF.gui.brightness_value, text_bright);
      gtk_label_set_markup (global->GPF.gui.black_value, text_black);
      break;
    case W_FORWARDL:
      hyscan_gtk_forward_look_set_brightness (global->GFL.fl, cur_brightness);
      gtk_label_set_markup (global->GFL.gui.brightness_value, text_bright);
      break;
    }

  g_free (text_bright);
  g_free (text_black);

  return TRUE;
}

/* Функция устанавливает масштаб отображения. */
static gboolean
scale_set (Global   *global,
           gboolean  scale_up,
           gpointer  user_data,
           gint      selector)
{
  GtkCifroAreaZoomType zoom_dir;
  gdouble from_y, to_y;
  gdouble min_y, max_y;
  gdouble scale;
  gchar *text = NULL;

  const gdouble *scales;
  gint n_scales;
  gint i_scale;

  switch (selector)
    {
    case W_SIDESCAN:
      if (user_data == NULL)
        hyscan_gtk_waterfall_control_zoom (global->GSS.wf_ctrl, scale_up);

      i_scale = hyscan_gtk_waterfall_get_scale (HYSCAN_GTK_WATERFALL (global->GSS.wf),
                                                &scales, &n_scales);

      text = g_strdup_printf ("<small><b>1:%.0f</b></small>", scales[i_scale]);
      gtk_label_set_markup (global->GSS.gui.scale_value, text);
      break;

    case W_PROFILER:
      if (user_data == NULL)
        hyscan_gtk_waterfall_control_zoom (global->GPF.wf_ctrl, scale_up);

      i_scale = hyscan_gtk_waterfall_get_scale (HYSCAN_GTK_WATERFALL (global->GPF.wf),
                                                &scales, &n_scales);

      text = g_strdup_printf ("<small><b>1:%.0f</b></small>", scales[i_scale]);
      gtk_label_set_markup (global->GPF.gui.scale_value, text);
      break;

    case W_FORWARDL:
      gtk_cifro_area_get_view (GTK_CIFRO_AREA (global->GFL.fl), NULL, NULL, &from_y, &to_y);
      gtk_cifro_area_get_limits (GTK_CIFRO_AREA (global->GFL.fl), NULL, NULL, &min_y, &max_y);
      scale = 100.0 * (fabs (to_y - from_y) / fabs (max_y - min_y));

      if (scale_up && (scale < 5.0))
        return FALSE;

      if (!scale_up && (scale > 100.0))
        return FALSE;

      zoom_dir = (scale_up) ? GTK_CIFRO_AREA_ZOOM_IN : GTK_CIFRO_AREA_ZOOM_OUT;
      gtk_cifro_area_zoom (GTK_CIFRO_AREA (global->GFL.fl), zoom_dir, zoom_dir, 0.0, 0.0);

      gtk_cifro_area_get_view (GTK_CIFRO_AREA (global->GFL.fl), NULL, NULL, &from_y, &to_y);
      gtk_cifro_area_get_limits (GTK_CIFRO_AREA (global->GFL.fl), NULL, NULL, &min_y, &max_y);
      scale = 100.0 * (fabs (to_y - from_y) / fabs (max_y - min_y));

      text = g_strdup_printf ("<small><b>%.0f%%</b></small>", scale);
      gtk_label_set_markup (global->GFL.gui.scale_value, text);
      break;
    }


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
static guint32*
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
static gboolean
color_map_set (Global *global,
               guint   cur_color_map,
               gint    selector)
{
  HyScanGtkWaterfall *wfd = NULL;
  GtkLabel *label = NULL;
  guint32 **cmap = NULL;
  guint   *len = NULL;
  gchar   *text;
  guint32 bg;
  const gchar *color_map_name;

  if (cur_color_map >= MAX_COLOR_MAPS)
    return FALSE;

  g_message ("color_map_set: sonar#%i, cmap %i", selector, cur_color_map);
  switch (selector)
    {
    case W_SIDESCAN:
      wfd = HYSCAN_GTK_WATERFALL (global->GSS.wf);
      cmap = global->GSS.color_maps;
      len = global->GSS.color_map_len;
      label = global->GSS.gui.colormap_value;
      bg = BLACK_BG;
      break;
    case W_PROFILER:
      g_message ("Trying to set PF colormap!");
      wfd = HYSCAN_GTK_WATERFALL (global->GPF.wf);
      cmap = global->GPF.color_maps;
      len = global->GPF.color_map_len;
      bg = WHITE_BG;
      label = NULL;
      cur_color_map = 0;
      break;

    default:
      g_warning ("Wrong sonar selector!");
      return FALSE;
    }

  if (cur_color_map == 0)
    color_map_name = "Джонни Айв";
  else if (cur_color_map == 1)
    color_map_name = "Ретро-стиль";
  else if (cur_color_map == 2)
    color_map_name = "Серость";
  else if (cur_color_map == 3)
    color_map_name = "Киберпанк";

  hyscan_gtk_waterfall_set_colormap_for_all (wfd, cmap[cur_color_map], len[cur_color_map], bg);
  hyscan_gtk_waterfall_set_substrate (wfd, bg);

  text = g_strdup_printf ("<small><b>%s</b></small>", color_map_name);
  if (label != NULL)
    gtk_label_set_markup (label, text);
  g_free (text);

  return TRUE;
}

/* Функция устанавливает порог чувствительности. */
static gboolean
sensitivity_set (Global  *global,
                 gdouble  cur_sensitivity)
 {
   gchar *text;

   if (cur_sensitivity < 1.0)
     return FALSE;
   if (cur_sensitivity > 10.0)
     return FALSE;

   hyscan_gtk_forward_look_set_sensitivity (global->GFL.fl, cur_sensitivity);

   text = g_strdup_printf ("<small><b>%.0f</b></small>", cur_sensitivity);
   gtk_label_set_markup (global->GFL.gui.sensitivity_value, text);
   g_free (text);

   return TRUE;
 }


/* Функция устанавливает излучаемый сигнал. */
static gboolean
signal_set (Global *global,
            guint   cur_signal,
            gint    selector)
{
  gchar *text;
  gboolean status;

  GtkLabel *label;
  HyScanGeneratorControl *gen;
  HyScanSourceType src0, src1;
  HyScanDataSchemaEnumValue *sig0, *sig1;
  gboolean is_equal;

  g_message ("Signal_set: Sonar#%i, Signal %i", selector, cur_signal);
  if (cur_signal == 0)
    return FALSE;

  switch (selector)
    {
    case W_SIDESCAN:
      if (cur_signal >= global->GSS.sb_n_signals || cur_signal >= global->GSS.ps_n_signals)
        return FALSE;
      src0 = STARBOARD;
      src1 = PORTSIDE;
      gen = global->GSS.sonar.gen_ctl;
      sig0 = global->GSS.sb_signals[cur_signal];
      sig1 = global->GSS.ps_signals[cur_signal];
      label = global->GSS.gui.signal_value;
      break;
    case W_PROFILER:
      if (cur_signal >= global->GPF.pf_n_signals)
        return FALSE;
      src1 = src0 = PROFILER;
      gen = global->GPF.sonar.gen_ctl;
      sig0 = global->GPF.pf_signals[cur_signal];
      if (global->GPF.have_es)
        {
          src1 = ECHOSOUNDER;
          sig1 = find_signal_by_name (global->GPF.es_signals, sig0);
        }
      label = global->GPF.gui.signal_value;
      break;
    case W_FORWARDL:
      if (cur_signal >= global->GFL.fl_n_signals)
        return FALSE;
      src0 = src1 = FORWARDLOOK;
      gen = global->GFL.sonar.gen_ctl;
      sig0 = sig1 = global->GFL.fl_signals[cur_signal];
      label = global->GFL.gui.signal_value;
      break;
    default:
      g_warning ("signal_set: wrong sonar_ctl selector (%i@%i)!", selector, __LINE__);
      return FALSE;
    }

  status = hyscan_generator_control_set_preset (gen, src0, sig0->value);
  if (!status)
    return FALSE;

  if (src1 != src0 && sig1 == NULL)
    {
      g_warning ("Signal not found for board %i", src1);
      src1 = src0;
    }

  if (src1 != src0)
    {
      status = hyscan_generator_control_set_preset (gen, src1, sig1->value);
      if (!status)
        return FALSE;
    }

  if (src0 == src1)
    {
      text = g_strdup_printf ("<small><b>%s</b></small>", sig0->name);
    }
  else
    {
      is_equal = g_strcmp0 (sig0->name, sig1->name);
      if (is_equal == 0)
        text = g_strdup_printf ("<small><b>%s</b></small>", sig0->name);
      else
        text = g_strdup_printf ("<small><b>%s, %s</b></small>", sig0->name, sig1->name);
    }

  gtk_label_set_markup (label, text);
  g_free (text);

  return TRUE;
}

/* Функция устанавливает параметры ВАРУ. */
static gboolean
tvg_set (Global  *global,
         gdouble *gain0,
         gdouble  step,
         gint     selector)
{
  gchar *gain_text, *step_text;
  gboolean status;
  GtkLabel *tvg0_value, *tvg_value;
  gdouble min_gain;
  gdouble max_gain;

  g_message ("tvg_set: sonar#%i, gain0 %f, step %f", selector, *gain0, step);

  switch (selector)
    {
      case W_SIDESCAN:
        hyscan_tvg_control_get_gain_range (global->GSS.sonar.tvg_ctl, STARBOARD, &min_gain, &max_gain);
        *gain0 = CLAMP (*gain0, min_gain, max_gain);
        status = hyscan_tvg_control_set_linear_db (global->GSS.sonar.tvg_ctl, STARBOARD, *gain0, step);
        hyscan_return_val_if_fail (status, FALSE);

        hyscan_tvg_control_get_gain_range (global->GSS.sonar.tvg_ctl, PORTSIDE, &min_gain, &max_gain);
        *gain0 = CLAMP (*gain0, min_gain, max_gain);
        status = hyscan_tvg_control_set_linear_db (global->GSS.sonar.tvg_ctl, PORTSIDE, *gain0, step);
        hyscan_return_val_if_fail (status, FALSE);

        tvg_value = global->GSS.gui.tvg_value;
        tvg0_value = global->GSS.gui.tvg0_value;
        break;

      case W_PROFILER:
        hyscan_tvg_control_get_gain_range (global->GPF.sonar.tvg_ctl, PROFILER, &min_gain, &max_gain);
        *gain0 = CLAMP (*gain0, min_gain, max_gain);
        status = hyscan_tvg_control_set_linear_db (global->GPF.sonar.tvg_ctl, PROFILER, *gain0, step);
        if (global->GPF.have_es)
          {
            hyscan_tvg_control_get_gain_range (global->GPF.sonar.tvg_ctl, ECHOSOUNDER, &min_gain, &max_gain);
            status = hyscan_tvg_control_set_constant (global->GPF.sonar.tvg_ctl, ECHOSOUNDER, min_gain);
            hyscan_return_val_if_fail (status, FALSE);
          }

        tvg_value = global->GPF.gui.tvg_value;
        tvg0_value = global->GPF.gui.tvg0_value;
        break;

      case W_FORWARDL:
        hyscan_tvg_control_get_gain_range (global->GFL.sonar.tvg_ctl, FORWARDLOOK, &min_gain, &max_gain);
        *gain0 = CLAMP (*gain0, min_gain, max_gain);
        status = hyscan_tvg_control_set_linear_db (global->GFL.sonar.tvg_ctl, FORWARDLOOK, *gain0, step);
        hyscan_return_val_if_fail (status, FALSE);

        tvg_value = global->GFL.gui.tvg_value;
        tvg0_value = global->GFL.gui.tvg0_value;
        break;
      default:
        g_warning ("tvg_set: wrong sonar_ctl selector (%i@%i)!", selector, __LINE__);
        return FALSE;
    }

  gain_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", *gain0);
  gtk_label_set_markup (tvg0_value, gain_text);
  g_free (gain_text);
  step_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", step);
  gtk_label_set_markup (tvg_value, step_text);
  g_free (step_text);

  return TRUE;
}

/* Функция устанавливает параметры автоВАРУ. */
static gboolean
auto_tvg_set (Global  *global,
              gdouble  level,
              gdouble  sensitivity,
              gint     selector)
{
  gboolean status;
  GtkLabel *tvg_level_value, *tvg_sens_value;
  gchar *sens_text, *level_text;

  g_message ("auto_tvg_set: Sonar#%i, level %f, sens %f", selector, level, sensitivity);

  switch (selector)
    {
      case W_SIDESCAN:
        status = hyscan_tvg_control_set_auto (global->GSS.sonar.tvg_ctl,
                                              STARBOARD, level, sensitivity);
        hyscan_return_val_if_fail (status, FALSE);
        status = hyscan_tvg_control_set_auto (global->GSS.sonar.tvg_ctl,
                                              PORTSIDE, level, sensitivity);
        hyscan_return_val_if_fail (status, FALSE);

        tvg_sens_value = global->GSS.gui.tvg_sens_value;
        tvg_level_value = global->GSS.gui.tvg_level_value;
        break;

      case W_PROFILER:
        status = hyscan_tvg_control_set_auto (global->GPF.sonar.tvg_ctl,
                                              PROFILER, level, sensitivity);
        hyscan_return_val_if_fail (status, FALSE);

        tvg_sens_value = global->GPF.gui.tvg_sens_value;
        tvg_level_value = global->GPF.gui.tvg_level_value;
        break;

      case W_FORWARDL:
        status = hyscan_tvg_control_set_auto (global->GFL.sonar.tvg_ctl,
                                              FORWARDLOOK, level, sensitivity);
        hyscan_return_val_if_fail (status, FALSE);

        tvg_sens_value = global->GFL.gui.tvg_sens_value;
        tvg_level_value = global->GFL.gui.tvg_level_value;
        break;
      default:
        return FALSE;
    }

  sens_text = g_strdup_printf ("<small><b>%.1f</b></small>", sensitivity);
  level_text = g_strdup_printf ("<small><b>%.1f</b></small>", level);

  gtk_label_set_markup (tvg_sens_value, sens_text);
  gtk_label_set_markup (tvg_level_value, level_text);

  g_free (sens_text);
  g_free (level_text);

  return TRUE;
}

static void
distance_printer (GtkLabel *label,
                  gdouble   value)
{
  gchar *text;
  text = g_strdup_printf ("<small><b>%.0f м</b></small>", value);
  gtk_label_set_markup (label, text);
  g_free (text);
}


/* Функция устанавливает рабочую дистанцию. */
static gboolean
distance_set (Global  *global,
              gdouble  wanted_distance,
              gint     selector)
{
  gdouble real_distance;
  gboolean status;

  if (wanted_distance < 1.0)
    return FALSE;

  real_distance = wanted_distance / (global->sound_velocity / 2.0);

  if (selector == W_PROFILER && wanted_distance <= PROFILER_MAX_DISTANCE)
      {
        status = hyscan_sonar_control_set_receive_time (global->GPF.sonar.sonar_ctl, PROFILER, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        if (global->GPF.have_es)
          {
            status = hyscan_sonar_control_set_receive_time (global->GPF.sonar.sonar_ctl, ECHOSOUNDER, real_distance);
          }
        hyscan_return_val_if_fail (status, FALSE);
        distance_printer (global->GPF.gui.distance_value, wanted_distance);
        global->GPF.sonar.cur_distance = wanted_distance;
      }
  else if (selector == W_SIDESCAN && wanted_distance <= SIDE_SCAN_MAX_DISTANCE)
      {
        status = hyscan_sonar_control_set_receive_time (global->GSS.sonar.sonar_ctl, STARBOARD, real_distance);
        status &= hyscan_sonar_control_set_receive_time (global->GSS.sonar.sonar_ctl, PORTSIDE, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_printer (global->GSS.gui.distance_value, wanted_distance);
        global->GSS.sonar.cur_distance = wanted_distance;
      }
  else if (selector == W_FORWARDL && wanted_distance <= FORWARD_LOOK_MAX_DISTANCE)
      {
        status = hyscan_sonar_control_set_receive_time (global->GFL.sonar.sonar_ctl, FORWARDLOOK, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_printer (global->GFL.gui.distance_value, wanted_distance);
        global->GFL.sonar.cur_distance = wanted_distance;
      }
  else
      {
        return FALSE;
      }

  return TRUE;
}

static void
void_callback (gpointer data)
{
  g_message ("This is a void callback. ");
}

static void
brightness_up (GtkWidget *widget,
              gint        selector)
{
  gdouble new_brightness;

  switch (selector)
    {
      case W_SIDESCAN:
        new_brightness = global.GSS.cur_brightness;
        {
          if (new_brightness < 50.0)
            new_brightness = new_brightness + 10.0;
          else if (new_brightness < 90.0)
            new_brightness = new_brightness + 5.0;
          else
            new_brightness = new_brightness + 1.0;
        }
        break;
      case W_PROFILER:
        new_brightness = global.GPF.cur_brightness;
        {
          if (new_brightness < 50.0)
            new_brightness = new_brightness + 10.0;
          else if (new_brightness < 90.0)
            new_brightness = new_brightness + 5.0;
          else
            new_brightness = new_brightness + 1.0;
        }
        break;
      case W_FORWARDL:
        new_brightness = global.GFL.cur_brightness;
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
        g_warning ("brightness_up: wrong sonar_ctl selector (%i@%i)!", selector, __LINE__);
        return;
    }

  new_brightness = CLAMP (new_brightness, 1.0, 100.0);

  switch (selector)
    {
      case W_SIDESCAN:
        if (!brightness_set (&global, new_brightness, global.GSS.cur_black, W_SIDESCAN))
          return;
        global.GSS.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global.GSS.wf));
        break;
      case W_PROFILER:
        if (!brightness_set (&global, new_brightness, global.GPF.cur_black, W_PROFILER))
          return;
        global.GPF.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global.GPF.wf));
        break;
      case W_FORWARDL:
        if (!brightness_set (&global, new_brightness, 0, W_FORWARDL))
          return;
        global.GFL.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));
        break;
    }
}

static void
brightness_down (GtkWidget *widget,
                 gint       selector)
{
  gdouble new_brightness;

  switch (selector)
    {
      case W_SIDESCAN:
        new_brightness = global.GSS.cur_brightness;
        {
          if (new_brightness > 90.0)
            new_brightness = new_brightness - 1.0;
          else if (new_brightness > 50.0)
            new_brightness = new_brightness - 5.0;
          else
            new_brightness = new_brightness - 10.0;
        }
        break;
      case W_PROFILER:
        new_brightness = global.GPF.cur_brightness;
        {
          if (new_brightness > 90.0)
            new_brightness = new_brightness - 1.0;
          else if (new_brightness > 50.0)
            new_brightness = new_brightness - 5.0;
          else
            new_brightness = new_brightness - 10.0;
        }
        break;
      case W_FORWARDL:
        new_brightness = global.GFL.cur_brightness;
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
        g_warning ("brightness_down: wrong sonar_ctl selector (%i@%i)!", selector, __LINE__);
        return;
    }

  new_brightness = CLAMP (new_brightness, 1.0, 100.0);

  switch (selector)
    {
      case W_SIDESCAN:
        if (!brightness_set (&global, new_brightness, global.GSS.cur_black, W_SIDESCAN))
          return;
        global.GSS.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global.GSS.wf));
        break;
      case W_PROFILER:
        if (!brightness_set (&global, new_brightness, global.GPF.cur_black, W_PROFILER))
          return;
        global.GPF.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global.GPF.wf));
        break;
      case W_FORWARDL:
        if (!brightness_set (&global, new_brightness, 0, W_FORWARDL))
          return;
        global.GFL.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));
        break;
    }
}

static void
black_up (GtkWidget *widget,
          gint       selector)
{
  gdouble new_black;

  switch (selector)
    {
      case W_SIDESCAN:
        new_black = global.GSS.cur_black;
        if (new_black < 50.0)
          new_black = new_black + 10.0;
        else if (new_black < 90.0)
          new_black = new_black + 5.0;
        else
          new_black = new_black + 1.0;
        break;
      case W_PROFILER:
        new_black = global.GPF.cur_black;
        if (new_black < 50.0)
          new_black = new_black + 10.0;
        else if (new_black < 90.0)
          new_black = new_black + 5.0;
        else
          new_black = new_black + 1.0;
        break;
        new_black = global.GPF.cur_black;
        break;
      case W_FORWARDL:
        new_black = global.GFL.cur_black;
        if (new_black < 10.0)
          new_black += 1.0;
        else if (new_black < 30.0)
          new_black += 2.0;
        else if (new_black < 50.0)
          new_black += 5.0;
        else
          new_black += 10.0;
        break;
      default:
        g_warning ("black_up: wrong sonar_ctl selector (%i@%i)!", selector, __LINE__);
        return;
    }

  new_black = CLAMP (new_black, 0.0, 100.0);

  switch (selector)
    {
      case W_SIDESCAN:
        if (!brightness_set (&global, global.GSS.cur_brightness, new_black, selector))
          return;
        global.GSS.cur_black = new_black;
        gtk_widget_queue_draw (GTK_WIDGET (global.GSS.wf));
        break;
      case W_PROFILER:
        if (!brightness_set (&global, global.GPF.cur_brightness, new_black, selector))
          return;
        global.GPF.cur_black = new_black;
        gtk_widget_queue_draw (GTK_WIDGET (global.GPF.wf));
        break;
      case W_FORWARDL:
        if (!brightness_set (&global, global.GFL.cur_brightness, 0, selector))
          return;
        global.GFL.cur_black = new_black;
        gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));
        break;
    }
}

static void
black_down (GtkWidget *widget,
            gint selector)
{
  gdouble new_black;

  switch (selector)
    {
      case W_SIDESCAN:
        new_black = global.GSS.cur_black;
          {
            if (new_black > 90.0)
              new_black = new_black - 1.0;
            else if (new_black > 50.0)
              new_black = new_black - 5.0;
            else
              new_black = new_black - 10.0;
          }

        break;
      case W_PROFILER:
        new_black = global.GPF.cur_black;
        {
          if (new_black > 90.0)
            new_black = new_black - 1.0;
          else if (new_black > 50.0)
            new_black = new_black - 5.0;
          else
            new_black = new_black - 10.0;
        }
        break;
      case W_FORWARDL:
        new_black = global.GFL.cur_black;
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
        g_warning ("black_down: wrong sonar_ctl selector (%i@%i)!", selector, __LINE__);
        return;
    }

  new_black = CLAMP (new_black, 1.0, 100.0);

  switch (selector)
    {
      case W_SIDESCAN:
        if (!brightness_set (&global, global.GSS.cur_brightness, new_black, selector))
          return;
        global.GSS.cur_black = new_black;
        gtk_widget_queue_draw (GTK_WIDGET (global.GSS.wf));
        break;
      case W_PROFILER:
        if (!brightness_set (&global, global.GPF.cur_brightness, new_black, selector))
          return;
        global.GPF.cur_black = new_black;
        gtk_widget_queue_draw (GTK_WIDGET (global.GPF.wf));
        break;
      case W_FORWARDL:
        if (!brightness_set (&global, global.GFL.cur_brightness, 0, selector))
          return;
        global.GFL.cur_black = new_black;
        gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));
        break;
    }
}

static void
scale_up (GtkWidget *widget,
          gint       selector)
{
  scale_set (&global, TRUE, NULL, selector);
}

static void
scale_down (GtkWidget *widget,
            gint       selector)
{
  scale_set (&global, FALSE, NULL, selector);
}

static void
sens_up (GtkWidget *widget,
         gint       selector)
{
  gdouble new_sensitivity;

  new_sensitivity = global.GFL.cur_sensitivity + 1;
  if (sensitivity_set (&global, new_sensitivity))
    global.GFL.cur_sensitivity = new_sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));
}

static void
sens_down (GtkWidget *widget,
           gint       selector)
{
  gdouble new_sensitivity;

  new_sensitivity = global.GFL.cur_sensitivity - 1;
  if (sensitivity_set (&global, new_sensitivity))
    global.GFL.cur_sensitivity = new_sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));
}

static void
color_map_up (GtkWidget *widget,
              gint       selector)
{
  guint cur_color_map;

  switch (selector)
    {
      case W_SIDESCAN:
        cur_color_map = global.GSS.cur_color_map + 1;
        cur_color_map %= MAX_COLOR_MAPS;
        if (color_map_set (&global, cur_color_map, selector))
          global.GSS.cur_color_map = cur_color_map;
        break;
      case W_PROFILER:
        cur_color_map = global.GPF.cur_color_map + 1;
        if (color_map_set (&global, cur_color_map, selector))
          global.GPF.cur_color_map = cur_color_map;
        break;
      default:
        g_message ("wrong color_map_up!"); //TODO: remove when debugged
        return;
    }
}

static void
color_map_down (GtkWidget *widget,
                gint       selector)
{
  guint cur_color_map;

  switch (selector)
    {
      case W_SIDESCAN:
        cur_color_map = global.GSS.cur_color_map - 1;
        if (color_map_set (&global, cur_color_map, selector))
          global.GSS.cur_color_map = cur_color_map;
        break;
      case W_PROFILER:
        cur_color_map = global.GPF.cur_color_map - 1;
        if (color_map_set (&global, cur_color_map, selector))
          global.GPF.cur_color_map = cur_color_map;
        break;
      default:
        g_message ("wrong color_map_down!"); //TODO: remove when debugged
        return;
    }
}

/* Функция переключает режим отображения виджета ВС. */
static void
mode_changed (GtkWidget *widget,
              gboolean   state,
              gint       selector)
{
  HyScanGtkForwardLookViewType type;
  if (selector != W_FORWARDL)
    {
      g_message ("wrong mode_changed!");
      return;
    }

  type = state ? HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET : HYSCAN_GTK_FORWARD_LOOK_VIEW_SURFACE;
  hyscan_gtk_forward_look_set_type (global.GFL.fl, type);
  gtk_widget_queue_draw (GTK_WIDGET (global.GFL.fl));

  return;
}

static gboolean
pf_special (GtkWidget  *widget,
            gboolean    state,
            gint        selector)
{
  HyScanTileFlags flags = 0;
  HyScanTileFlags pf_flag = HYSCAN_TILE_PROFILER;
  HyScanGtkWaterfallState *wf = HYSCAN_GTK_WATERFALL_STATE (global.GPF.wf);

  hyscan_gtk_waterfall_state_get_tile_flags (wf, &flags);

  if (state)
    flags |= pf_flag;
  else
    flags &= ~pf_flag;

  hyscan_gtk_waterfall_state_set_tile_flags (wf, flags);

  return TRUE;
}

/* Функция переключает автосдвижку. */
static gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           gint        selector)
{
  if (selector == W_SIDESCAN)
    {
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global.GSS.wf), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.GSS.live_view), state);
    }
  else if (selector == W_PROFILER)
    {
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global.GPF.wf), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.GPF.live_view), state);
    }
  else if (selector == W_FORWARDL)
    {
      if (state)
        hyscan_forward_look_player_real_time (global.GFL.fl_player);
      else
        hyscan_forward_look_player_pause (global.GFL.fl_player);

      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.GFL.live_view), state);
    }
  else if (selector == ALL)
    {
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global.GSS.wf), state);
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global.GPF.wf), state);

      if (state)
        hyscan_forward_look_player_real_time (global.GFL.fl_player);
      else
        hyscan_forward_look_player_pause (global.GFL.fl_player);

      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.GSS.live_view), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.GPF.live_view), state);
      hyscan_ame_button_set_state (HYSCAN_AME_BUTTON (global.GFL.live_view), state);
    }

  return TRUE;
}

static void
live_view_off (GtkWidget  *widget,
               gboolean    state,
               Global     *global)
{
  if (GTK_WIDGET (widget) == GTK_WIDGET (global->GSS.wf))
    {
      g_message ("Try to set live view!");
      // gtk_switch_set_active (global->GSS.live_view, state);
    }
  else if (GTK_WIDGET (widget) == GTK_WIDGET (global->GPF.wf))
    {
      g_message ("Try to set live view!");
      // gtk_switch_set_active (global->GPF.live_view, state);
    }
  else
    {
      g_message ("live_view_off: wrong calling instance!");
    }

  live_view (NULL, state, ALL);
}

static void
distance_up (GtkWidget *widget,
             gint       selector)
{
  gdouble cur_distance;

  switch (selector)
    {
      case W_FORWARDL:
        cur_distance = global.GFL.sonar.cur_distance + 5.0;
        if (distance_set (&global, cur_distance, selector))
          global.GFL.sonar.cur_distance = cur_distance;
        break;
      case W_SIDESCAN:
        cur_distance = global.GSS.sonar.cur_distance + 5.0;
        if (distance_set (&global, cur_distance, selector))
          global.GSS.sonar.cur_distance = cur_distance;
        break;
      case W_PROFILER:
        cur_distance = global.GPF.sonar.cur_distance + 5.0;
        if (distance_set (&global, cur_distance, selector))
          global.GPF.sonar.cur_distance = cur_distance;
        break;
      default:
        WRONG_SELECTOR;
    }
}

static void
distance_down (GtkWidget *widget,
               gint       selector)
{
  gdouble cur_distance;

  switch (selector)
    {
      case W_FORWARDL:
        cur_distance = global.GFL.sonar.cur_distance - 5.0;
        if (distance_set (&global, cur_distance, selector))
          global.GFL.sonar.cur_distance = cur_distance;
        break;
      case W_SIDESCAN:
        cur_distance = global.GSS.sonar.cur_distance - 5.0;
        if (distance_set (&global, cur_distance, selector))
          global.GSS.sonar.cur_distance = cur_distance;
        break;
      case W_PROFILER:
        cur_distance = global.GPF.sonar.cur_distance - 5.0;
        if (distance_set (&global, cur_distance, selector))
          global.GPF.sonar.cur_distance = cur_distance;
        break;
      default:
        WRONG_SELECTOR;
    }
}

static void
tvg0_up (GtkWidget *widget,
         gint       selector)
{
  gdouble cur_gain0;

  switch (selector)
    {
    case W_FORWARDL:
      cur_gain0 = global.GFL.sonar.cur_gain0 + 0.5;
      if (tvg_set (&global, &cur_gain0, global.GFL.sonar.cur_gain_step, selector))
        global.GFL.sonar.cur_gain0 = cur_gain0;
      break;

    case W_SIDESCAN:
      cur_gain0 = global.GSS.sonar.cur_gain0 + 0.5;
      if (tvg_set (&global, &cur_gain0, global.GSS.sonar.cur_gain_step, selector))
        global.GSS.sonar.cur_gain0 = cur_gain0;
      break;

    case W_PROFILER:
      cur_gain0 = global.GPF.sonar.cur_gain0 + 0.5;
      if (tvg_set (&global, &cur_gain0, global.GPF.sonar.cur_gain_step, selector))
        global.GPF.sonar.cur_gain0 = cur_gain0;
      break;
    default:
      WRONG_SELECTOR;
    }
}

static void
tvg0_down (GtkWidget *widget,
           gint       selector)
{
  gdouble cur_gain0;

  switch (selector)
    {
    case W_FORWARDL:
      cur_gain0 = global.GFL.sonar.cur_gain0 - 0.5;
      if (tvg_set (&global, &cur_gain0, global.GFL.sonar.cur_gain_step, selector))
        global.GFL.sonar.cur_gain0 = cur_gain0;
      break;

    case W_SIDESCAN:
      cur_gain0 = global.GSS.sonar.cur_gain0 - 0.5;
      if (tvg_set (&global, &cur_gain0, global.GSS.sonar.cur_gain_step, selector))
        global.GSS.sonar.cur_gain0 = cur_gain0;
      break;

    case W_PROFILER:
      cur_gain0 = global.GPF.sonar.cur_gain0 - 0.5;
      if (tvg_set (&global, &cur_gain0, global.GPF.sonar.cur_gain_step, selector))
        global.GPF.sonar.cur_gain0 = cur_gain0;
    }
}

static void
tvg_up (GtkWidget *widget,
        gint       selector)
{
  gdouble cur_gain_step;

  switch (selector)
    {
    case W_FORWARDL:
      cur_gain_step = global.GFL.sonar.cur_gain_step + 2.5;
      if (tvg_set (&global, &global.GFL.sonar.cur_gain0, cur_gain_step, selector))
        global.GFL.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_SIDESCAN:
      cur_gain_step = global.GSS.sonar.cur_gain_step + 2.5;
      if (tvg_set (&global, &global.GSS.sonar.cur_gain0, cur_gain_step, selector))
        global.GSS.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_PROFILER:
      cur_gain_step = global.GPF.sonar.cur_gain_step + 2.5;
      if (tvg_set (&global, &global.GPF.sonar.cur_gain0, cur_gain_step, selector))
        global.GPF.sonar.cur_gain_step = cur_gain_step;
    }
}

static void
tvg_down (GtkWidget *widget,
          gint       selector)
{
  gdouble cur_gain_step;

  switch (selector)
    {
    case W_FORWARDL:
      cur_gain_step = global.GFL.sonar.cur_gain_step - 2.5;
      if (tvg_set (&global, &global.GFL.sonar.cur_gain0, cur_gain_step, selector))
        global.GFL.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_SIDESCAN:
      cur_gain_step = global.GSS.sonar.cur_gain_step - 2.5;
      if (tvg_set (&global, &global.GSS.sonar.cur_gain0, cur_gain_step, selector))
        global.GSS.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_PROFILER:
      cur_gain_step = global.GPF.sonar.cur_gain_step - 2.5;
      if (tvg_set (&global, &global.GPF.sonar.cur_gain0, cur_gain_step, selector))
        global.GPF.sonar.cur_gain_step = cur_gain_step;
    }
}

static void
tvg_level_up (GtkWidget *widget,
              gint       selector)
{
  gdouble cur_level;

  switch (selector)
    {
    case W_FORWARDL:
      cur_level = global.GFL.sonar.cur_level + 0.1;
      cur_level = CLAMP (cur_level, 0.0, 1.0);
      if (auto_tvg_set (&global, cur_level, global.GFL.sonar.cur_sensitivity, W_FORWARDL))
        global.GFL.sonar.cur_level = cur_level;
      break;

    case W_SIDESCAN:
      cur_level = global.GSS.sonar.cur_level + 0.1;
      cur_level = CLAMP (cur_level, 0.0, 1.0);
      if (auto_tvg_set (&global, cur_level, global.GSS.sonar.cur_sensitivity, W_SIDESCAN))
        global.GSS.sonar.cur_level = cur_level;
      break;

    case W_PROFILER:
      cur_level = global.GPF.sonar.cur_level + 0.1;
      cur_level = CLAMP (cur_level, 0.0, 1.0);
      if (auto_tvg_set (&global, cur_level, global.GPF.sonar.cur_sensitivity, W_PROFILER))
        global.GPF.sonar.cur_level = cur_level;
    }
}

static void
tvg_level_down (GtkWidget *widget,
                gint       selector)
{
  gdouble cur_level;

    switch (selector)
      {
      case W_FORWARDL:
        cur_level = global.GFL.sonar.cur_level - 0.1;
        cur_level = CLAMP (cur_level, 0.0, 1.0);
        if (auto_tvg_set (&global, cur_level, global.GFL.sonar.cur_sensitivity, W_FORWARDL))
          global.GFL.sonar.cur_level = cur_level;
        break;

      case W_SIDESCAN:
        cur_level = global.GSS.sonar.cur_level - 0.1;
        cur_level = CLAMP (cur_level, 0.0, 1.0);
        if (auto_tvg_set (&global, cur_level, global.GSS.sonar.cur_sensitivity, W_SIDESCAN))
          global.GSS.sonar.cur_level = cur_level;
        break;

      case W_PROFILER:
        cur_level = global.GPF.sonar.cur_level - 0.1;
        cur_level = CLAMP (cur_level, 0.0, 1.0);
        if (auto_tvg_set (&global, cur_level, global.GPF.sonar.cur_sensitivity, W_PROFILER))
          global.GPF.sonar.cur_level = cur_level;
      }
}

static void
tvg_sens_up (GtkWidget *widget,
             gint       selector)
{
  gdouble cur_sensitivity;

    switch (selector)
      {
      case W_FORWARDL:
        cur_sensitivity = global.GFL.sonar.cur_sensitivity + 0.1;
        cur_sensitivity = CLAMP (cur_sensitivity, 0.0, 1.0);
        if (auto_tvg_set (&global, global.GFL.sonar.cur_level, cur_sensitivity, W_FORWARDL))
          global.GFL.sonar.cur_sensitivity = cur_sensitivity;
        break;

      case W_SIDESCAN:
        cur_sensitivity = global.GSS.sonar.cur_sensitivity + 0.1;
        cur_sensitivity = CLAMP (cur_sensitivity, 0.0, 1.0);
        if (auto_tvg_set (&global, global.GSS.sonar.cur_level, cur_sensitivity, W_SIDESCAN))
          global.GSS.sonar.cur_sensitivity = cur_sensitivity;
        break;

      case W_PROFILER:
        cur_sensitivity = global.GPF.sonar.cur_sensitivity + 0.1;
        cur_sensitivity = CLAMP (cur_sensitivity, 0.0, 1.0);
        if (auto_tvg_set (&global, global.GPF.sonar.cur_level, cur_sensitivity, W_PROFILER))
          global.GPF.sonar.cur_sensitivity = cur_sensitivity;
      }
}

static void
tvg_sens_down (GtkWidget *widget,
               gint       selector)
{
  gdouble cur_sensitivity;

    switch (selector)
      {
      case W_FORWARDL:
        cur_sensitivity = global.GFL.sonar.cur_sensitivity - 0.1;
        cur_sensitivity = CLAMP (cur_sensitivity, 0.0, 1.0);
        if (auto_tvg_set (&global, global.GFL.sonar.cur_level, cur_sensitivity, W_FORWARDL))
          global.GFL.sonar.cur_sensitivity = cur_sensitivity;
        break;

      case W_SIDESCAN:
        cur_sensitivity = global.GSS.sonar.cur_sensitivity - 0.1;
        cur_sensitivity = CLAMP (cur_sensitivity, 0.0, 1.0);
        if (auto_tvg_set (&global, global.GSS.sonar.cur_level, cur_sensitivity, W_SIDESCAN))
          global.GSS.sonar.cur_sensitivity = cur_sensitivity;
        break;

      case W_PROFILER:
        cur_sensitivity = global.GPF.sonar.cur_sensitivity - 0.1;
        cur_sensitivity = CLAMP (cur_sensitivity, 0.0, 1.0);
        if (auto_tvg_set (&global, global.GPF.sonar.cur_level, cur_sensitivity, W_PROFILER))
          global.GPF.sonar.cur_sensitivity = cur_sensitivity;
      }
}

static void
signal_up (GtkWidget *widget,
           gint      selector)
{
  guint cur_signal;

  switch (selector)
    {
    case W_FORWARDL:
      cur_signal = global.GFL.sonar.cur_signal + 1;
      if (signal_set (&global, cur_signal, selector))
        global.GFL.sonar.cur_signal = cur_signal;
      break;

    case W_SIDESCAN:
      cur_signal = global.GSS.sonar.cur_signal + 1;
      if (signal_set (&global, cur_signal, selector))
        global.GSS.sonar.cur_signal = cur_signal;
      break;

    case W_PROFILER:
      cur_signal = global.GPF.sonar.cur_signal + 1;
      if (signal_set (&global, cur_signal, selector))
        global.GPF.sonar.cur_signal = cur_signal;
      break;
    }
}

static void
signal_down (GtkWidget *widget,
             gint      selector)
{
  guint cur_signal;

  switch (selector)
    {
    case W_FORWARDL:
      cur_signal = global.GFL.sonar.cur_signal - 1;
      if (signal_set (&global, cur_signal, selector))
        global.GFL.sonar.cur_signal = cur_signal;
      break;

    case W_SIDESCAN:
      cur_signal = global.GSS.sonar.cur_signal - 1;
      if (signal_set (&global, cur_signal, selector))
        global.GSS.sonar.cur_signal = cur_signal;
      break;

    case W_PROFILER:
      cur_signal = global.GPF.sonar.cur_signal - 1;
      if (signal_set (&global, cur_signal, selector))
        global.GPF.sonar.cur_signal = cur_signal;
      break;
    }
}

static void
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


//static void
//widget_swap (GtkToggleButton *togglebutton,
//             gpointer         user_data)
//{
//  gint selector = global.sonar_selector;
//
//  selector = (selector + 1) % 3;
//
//  gtk_stack_set_visible_child_name (GTK_STACK (global.gui.main_stack),
//                                    global.gui.widget_names[selector]);
//
//  gchar *text = g_strdup_printf ("<small><b>%s</b></small>", global.gui.widget_names[selector]);
//  gtk_label_set_markup (GTK_LABEL (global.gui.current_view), text);
//  g_free (text);
//
//
//  global.sonar_selector = selector;
//}


/* Функция включает/выключает излучение. */
static void
start_stop (GtkWidget *widget,
            gboolean   state)
{
  HyScanAmeButton * button = HYSCAN_AME_BUTTON (widget);
  gchar *pftrack;

  /* Включаем излучение. */
  if (state)
    {
      g_message ("Start sonars. Dry is %s", global.dry ? "ON" : "OFF");
      static gint n_tracks = -1;
      gboolean status;

      /* Закрываем текущий открытый галс. */
      g_clear_pointer(&global.track_name, g_free);

      /* Задаем параметры гидролокаторов. */
      if (!global.dry && !signal_set (&global, global.GSS.sonar.cur_signal, W_SIDESCAN))
        {
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }
      if (!auto_tvg_set (&global, global.GSS.sonar.cur_level, global.GSS.sonar.cur_sensitivity, W_SIDESCAN) ||
          !distance_set (&global, global.GSS.sonar.cur_distance, W_SIDESCAN))
        {
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }

      if (!global.dry && !signal_set (&global, global.GPF.sonar.cur_signal, W_PROFILER))
         {
           hyscan_ame_button_set_state (button, FALSE);
           return;
         }

      if (!tvg_set      (&global, &global.GPF.sonar.cur_gain0, global.GPF.sonar.cur_gain_step, W_PROFILER) ||
          !distance_set (&global, global.GPF.sonar.cur_distance, W_PROFILER))
        {
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }
      if (!global.dry && !signal_set   (&global, global.GFL.sonar.cur_signal, W_FORWARDL))
          {
            hyscan_ame_button_set_state (button, FALSE);
            return;
          }
      if (!tvg_set      (&global, &global.GFL.sonar.cur_gain0, global.GFL.sonar.cur_gain_step, W_FORWARDL) ||
          !distance_set (&global, global.GFL.sonar.cur_distance, W_FORWARDL))
        {
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }

      /* Число галсов в проекте. */
      if (n_tracks < 0)
        {
          gint32 project_id;
          gchar **tracks;
          gchar **strs;

          project_id = hyscan_db_project_open (global.db, global.project_name);
          tracks = hyscan_db_track_list (global.db, project_id);

          // TODO: посчитать вручную, игнорируя галсы с префиксом пф.
          for (strs = tracks; strs != NULL && *strs != NULL; strs++)
            {
              if (!g_str_has_prefix (*strs, PF_TRACK_PREFIX))
                n_tracks++;
            }

          hyscan_db_close (global.db, project_id);
          g_free (tracks);
        }

      /* Включаем запись нового галса. */
      global.track_name = g_strdup_printf ("%d%s", ++n_tracks, global.dry ? DRY_TRACK_SUFFIX : "");

      status = hyscan_sonar_control_start (global.GSS.sonar.sonar_ctl, global.track_name, HYSCAN_TRACK_SURVEY);

      if (!status)
        {
          g_message ("GSS startup failed @ %i",__LINE__);
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }

      pftrack = g_strdup_printf (PF_TRACK_PREFIX "%s", global.track_name);
      status = hyscan_sonar_control_start (global.GPF.sonar.sonar_ctl, pftrack, HYSCAN_TRACK_SURVEY);
      g_free (pftrack);

      if (!status)
        {
          g_message ("GPF startup failed @ %i",__LINE__);
          hyscan_ame_button_set_state (button, FALSE);
          return;
        }

      /* Если локатор включён, переходим в режим онлайн. */
      gtk_widget_set_sensitive (GTK_WIDGET (global.gui.track_view), FALSE);

      live_view (NULL, TRUE, ALL);

      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global.GSS.wf), TRUE);
      hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (global.GPF.wf), TRUE);
    }

  /* Выключаем излучение и блокируем режим онлайн. */
  else
    {
      g_message ("Stop sonars");
      hyscan_sonar_control_stop (global.GSS.sonar.sonar_ctl);
      hyscan_sonar_control_stop (global.GPF.sonar.sonar_ctl);
      hyscan_forward_look_player_pause (global.GFL.fl_player);
      hyscan_ame_button_set_state(button, FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (global.gui.track_view), TRUE);
    }

  start_stop_disabler (widget, state);

  return;
}

static gboolean
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

static void
nav_printer (gchar       **target,
             GMutex       *lock,
             const gchar  *format,
             ...)
{
  va_list args;

  if (*target != NULL)
    g_clear_pointer (target, g_free);

  va_start (args, format);
  g_mutex_lock (lock);
  *target = g_strdup_vprintf (format, args);
  g_mutex_unlock (lock);
  va_end (args);
}

static void
sensor_cb (HyScanSensorControl      *src,
           const gchar              *name,
           HyScanSensorProtocolType  protocol,
           HyScanDataType            type,
           HyScanDataWriterData     *data,
           Global                   *global)
{
  HyScanDataWriter *dst = HYSCAN_DATA_WRITER (global->GFL.sonar.sonar_ctl);
  gchar **nmea;
  const gchar * rmc = NULL, *dpt = NULL;
  guint i;

  HyScanDataWriterData writeable;

  writeable.time = data->time;
  /* Отправка RMC, GGA и DPT. */
  nmea = g_strsplit_set (data->data, "\r\n", data->size);
  for (i = 0; (nmea != NULL) && (nmea[i] != NULL); i++)
    {
      HyScanSourceType type;
      writeable.size = strlen (nmea[i]) + 1;
      writeable.data = nmea[i];

      if (g_str_has_prefix (nmea[i]+3, "RMC"))
        {
          type = HYSCAN_SOURCE_NMEA_RMC;
          rmc = nmea[i];
        }
      else if (g_str_has_prefix (nmea[i]+3, "GGA"))
        {
          type = HYSCAN_SOURCE_NMEA_GGA;
        }
      else if (g_str_has_prefix (nmea[i]+3, "DPT"))
        {
          type = HYSCAN_SOURCE_NMEA_DPT;
          dpt = nmea[i];
        }
      else
        continue;

      hyscan_data_writer_sensor_add_data (dst, name, type, 1, &writeable);
    }

  // hyscan_data_writer_sensor_add_data (dst, name, HYSCAN_SOURCE_NMEA_ANY, 1, data);

  /* Обновляем данные виджета навигации. */
  {
    GMutex *mtx = &global->nmea.lock;
    gdouble time = 0, date, lat, lon, trk, spd, dpt_;

    if (hyscan_nmea_parser_from_string (global->nmea.time, rmc, &time) &&
        hyscan_nmea_parser_from_string (global->nmea.date, rmc, &date))
      {
        GTimeZone *tz;
        GDateTime *local, *utc;
        gchar *dtstr;

        tz = g_time_zone_new ("+03:00");
        utc = g_date_time_new_from_unix_utc (date + time);
        local = g_date_time_to_timezone (utc, tz);

        dtstr = g_date_time_format (local, "%d.%m.%y %H:%M:%S");

        nav_printer (&global->nmea.tmd_value, mtx, "%s", dtstr);

        g_free (dtstr);
        g_date_time_unref (local);
        g_date_time_unref (utc);
        g_time_zone_unref (tz);
      }

    if (hyscan_nmea_parser_from_string (global->nmea.lat, rmc, &lat))
      nav_printer (&global->nmea.lat_value, mtx, "%f °%s", lat, lat > 0 ? "С.Ш." : "Ю.Ш.");
    if (hyscan_nmea_parser_from_string (global->nmea.lon, rmc, &lon))
      nav_printer (&global->nmea.lon_value, mtx, "%f °%s", lon, lon > 0 ? "В.Д." : "З.Д.");
    if (hyscan_nmea_parser_from_string (global->nmea.trk, rmc, &trk))
      nav_printer (&global->nmea.trk_value, mtx, "%f °", trk);
    if (hyscan_nmea_parser_from_string (global->nmea.spd, rmc, &spd))
      nav_printer (&global->nmea.spd_value, mtx, "%f м/с", spd * 0.514);
    if (hyscan_nmea_parser_from_string (global->nmea.dpt, dpt, &dpt_))
      nav_printer (&global->nmea.dpt_value, mtx, "%f м", dpt_);
  }

  g_strfreev (nmea);

}

static gboolean
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
static gboolean
start_stop_dry (GtkWidget *widget,
                gboolean   state)
{
  global.dry = state;

  if (global.GPF.sonar.gen_ctl != NULL && global.GSS.sonar.gen_ctl != NULL)
    {
      hyscan_generator_control_set_enable (global.GPF.sonar.gen_ctl, PROFILER, !global.dry);
      hyscan_generator_control_set_enable (global.GPF.sonar.gen_ctl, ECHOSOUNDER, !global.dry);
      hyscan_generator_control_set_enable (global.GSS.sonar.gen_ctl, PORTSIDE, !global.dry);
      hyscan_generator_control_set_enable (global.GSS.sonar.gen_ctl, STARBOARD, !global.dry);
      hyscan_generator_control_set_enable (global.GFL.sonar.gen_ctl, FORWARDLOOK, !global.dry);
    }
  else
    {
      g_warning ("Should not reach here %i", __LINE__);
    }

  start_stop (widget, state);

  return TRUE;
}

static gint
urpc_cb (uint32_t  session,
         uRpcData *urpc_data,
         void     *proc_data,
         void     *key_data)
{
  gint keycode = GPOINTER_TO_INT (proc_data);
  g_atomic_int_set (&global.urpc_keycode, keycode);

  //g_idle_add_full (G_PRIORITY_DEFAULT, ame_key_func, NULL, NULL);

  return 0;
}

static gboolean
ame_key_func (gpointer user_data)
{
  Global *global = user_data;
  gint keycode;

  keycode = g_atomic_int_get (&global->urpc_keycode);

  if (keycode == URPC_KEYCODE_NO_ACTION)
    return G_SOURCE_CONTINUE;

  switch (keycode)
    {
    default:
      g_message ("This key is not supported yet. ");
    }

  /* Сбрасываем значение кнопки, если его больше никто не перетер. */
  g_atomic_int_compare_and_exchange (&global->urpc_keycode, keycode, URPC_KEYCODE_NO_ACTION);

  return G_SOURCE_CONTINUE;
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

static void
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

static GtkWidget *
make_overlay (HyScanGtkWaterfall          *wf,
              HyScanGtkWaterfallGrid     **_grid,
              HyScanGtkWaterfallControl  **_ctrl,
              HyScanGtkWaterfallMark     **_mark,
              HyScanGtkWaterfallMeter    **_meter)
{
  GtkWidget * container = gtk_grid_new ();

  HyScanGtkWaterfallGrid *grid = hyscan_gtk_waterfall_grid_new (wf);
  HyScanGtkWaterfallControl *ctrl = hyscan_gtk_waterfall_control_new (wf);
  HyScanGtkWaterfallMark *mark = hyscan_gtk_waterfall_mark_new (wf);
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

static HyScanParam *
connect_to_sonar (const gchar *driver_path,
                  const gchar *driver_name,
                  const gchar *uri)
{
  HyScanParam * sonar = NULL;
  HyScanSonarClient *client = NULL;
  HyScanSonarDriver *driver = NULL;

  /* Подключение к гидролокатору с помощью HyScanSonarClient */
  if (driver_name == NULL)
    {
      client = hyscan_sonar_client_new (uri);
      if (client == NULL)
        {
          g_message ("can't connect to sonar '%s'", uri);
          return NULL;
        }

      if (!hyscan_sonar_client_set_master (client))
        {
          g_message ("can't set master mode on sonar '%s'", uri);
          g_object_unref (client);
          return NULL;
        }

      sonar = HYSCAN_PARAM (client);
    }
  /* Подключение с помощью драйвера гидролокатора. */
  else
    {
      driver = hyscan_sonar_driver_new (driver_path, driver_name);

      if (driver == NULL)
        {
          g_message ("can't load sonar driver '%s'", driver_name);
          return NULL;
        }

      sonar = hyscan_sonar_discover_connect (HYSCAN_SONAR_DISCOVER (driver), uri, NULL);
      if (sonar == NULL)
        {
          g_message ("can't connect to sonar '%s', continue in sonarless mode", uri);
        }
    }

  return sonar;
}

int
main (int argc, char **argv)
{
  gint               cache_size = 2048;        /* Размер кэша по умолчанию. */

  gchar             *driver_path = NULL;       /* Путь к драйверам гидролокатора. */
  gchar             *driver_name = NULL;       /* Название драйвера гидролокатора. */

  gchar             *prof_uri = NULL;          /* Адрес гидролокатора. */
  gchar             *flss_uri = NULL;          /* Адрес гидролокатора. */

  gchar             *db_uri = NULL;            /* Адрес базы данных. */
  gchar             *project_name = NULL;      /* Название проекта. */
  gdouble            sound_velocity = 1500.0;  /* Скорость звука по умолчанию. */
  HyScanSoundVelocity svp_val;
  GArray            *svp;                      /* Профиль скорости звука. */
  gdouble            ship_speed = 1.8;         /* Скорость движения судна. */
  gboolean           full_screen = FALSE;      /* Признак полноэкранного режима. */

  gchar             *config_file = NULL;       /* Название файла конфигурации. */
  GKeyFile          *config = NULL;

  // HyScanSonarDriver *prof_driver = NULL;       /* Драйвер профилографа. */
  // HyScanSonarDriver *flss_driver = NULL;       /* Драйвер вс+гбо. */
  HyScanParam       *prof_sonar = NULL;        /* Интерфейс управления локатором. */
  HyScanParam       *flss_sonar = NULL;        /* Интерфейс управления локатором. */

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

        { "driver-path",     'a',   0, G_OPTION_ARG_STRING,  &driver_path,     "Path to sonar_ctl drivers", NULL },
        { "driver-name",     'n',   0, G_OPTION_ARG_STRING,  &driver_name,     "Sonar driver name", NULL },

        { "prof-uri",    '1',   0, G_OPTION_ARG_STRING,  &prof_uri,     "Profiler uri", NULL },
        { "flss-uri",    '2',   0, G_OPTION_ARG_STRING,  &flss_uri,     "ForwardLook+SideScan uri", NULL },

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

  /* Путь к драйверам по умолчанию. */
  if ((driver_path == NULL) && (driver_name != NULL))
    driver_path = g_strdup (SONAR_DRIVERS_PATH);


  if (config_file != NULL)
    {
      config = g_key_file_new ();
      status = g_key_file_load_from_file (config, config_file,
                                          G_KEY_FILE_NONE, NULL);

      if (!status)
        g_message ("failed to load config file <%s>", config_file);

    }

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
      const gchar *project;
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
  if (flss_uri == NULL && prof_uri == NULL)
    goto no_sonar;

  flss_sonar = connect_to_sonar (driver_path, driver_name, flss_uri);
  prof_sonar = connect_to_sonar (driver_path, driver_name, prof_uri);

  if (flss_sonar == NULL || prof_sonar == NULL)
    {
      g_clear_object (&flss_sonar);
      g_clear_object (&prof_sonar);
      goto no_sonar;
    }

  if (flss_sonar != NULL)
    {
      HyScanGeneratorModeType gen_cap;
      HyScanTVGModeType tvg_cap;
      gboolean status;
      guint i;

      /* Управление локатором. */
      global.GFL.sonar.sonar_ctl = hyscan_sonar_control_new (flss_sonar, 4, 4, global.db);
      hyscan_exit_if_w_param (global.GFL.sonar.sonar_ctl == NULL, "unsupported sonar_ctl '%s'", flss_uri);
      global.GSS.sonar.sonar_ctl = g_object_ref (global.GFL.sonar.sonar_ctl);

      global.GFL.sonar.gen_ctl = HYSCAN_GENERATOR_CONTROL (global.GFL.sonar.sonar_ctl);
      global.GFL.sonar.tvg_ctl = HYSCAN_TVG_CONTROL (global.GFL.sonar.sonar_ctl);
      global.GSS.sonar.gen_ctl = HYSCAN_GENERATOR_CONTROL (global.GSS.sonar.sonar_ctl);
      global.GSS.sonar.tvg_ctl = HYSCAN_TVG_CONTROL (global.GSS.sonar.sonar_ctl);

      /* Параметры генераторов. */
      gen_cap = hyscan_generator_control_get_capabilities (global.GFL.sonar.gen_ctl, FORWARDLOOK);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "forward-look: unsupported generator mode");
      gen_cap = hyscan_generator_control_get_capabilities (global.GSS.sonar.gen_ctl, STARBOARD);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "starboard: unsupported generator mode");
      gen_cap = hyscan_generator_control_get_capabilities (global.GSS.sonar.gen_ctl, PORTSIDE);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "portside: unsupported generator mode");


      status = hyscan_generator_control_set_enable (global.GFL.sonar.gen_ctl, FORWARDLOOK, TRUE);
      hyscan_exit_if (!status, "forward-look: can't enable generator");
      status = hyscan_generator_control_set_enable (global.GSS.sonar.gen_ctl, STARBOARD, TRUE);
      hyscan_exit_if (!status, "starboard: can't enable generator");
      status = hyscan_generator_control_set_enable (global.GSS.sonar.gen_ctl, PORTSIDE, TRUE);
      hyscan_exit_if (!status, "portside: can't enable generator");

      /* Параметры ВАРУ. */
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GFL.sonar.tvg_ctl, FORWARDLOOK);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "forward-look: unsupported tvg_ctl mode");
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GSS.sonar.tvg_ctl, STARBOARD);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "starboard: unsupported tvg_ctl mode");
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GSS.sonar.tvg_ctl, PORTSIDE);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "portside: unsupported tvg_ctl mode");

      status = hyscan_tvg_control_set_enable (global.GFL.sonar.tvg_ctl, FORWARDLOOK, TRUE);
      hyscan_exit_if (!status, "forward-look: can't enable tvg_ctl");
      status = hyscan_tvg_control_set_enable (global.GSS.sonar.tvg_ctl, STARBOARD, TRUE);
      hyscan_exit_if (!status, "starboard: can't enable tvg_ctl");
      status = hyscan_tvg_control_set_enable (global.GSS.sonar.tvg_ctl, PORTSIDE, TRUE);
      hyscan_exit_if (!status, "portside: can't enable tvg_ctl");

      /* Сигналы зондирования. */
      global.GFL.fl_signals = hyscan_generator_control_list_presets (global.GFL.sonar.gen_ctl, FORWARDLOOK);
      hyscan_exit_if (global.GFL.fl_signals == NULL, "forward-look: can't load signal presets");
      global.GSS.sb_signals = hyscan_generator_control_list_presets (global.GSS.sonar.gen_ctl, STARBOARD);
      hyscan_exit_if (global.GSS.sb_signals == NULL, "starboard: can't load signal presets");
      global.GSS.ps_signals = hyscan_generator_control_list_presets (global.GSS.sonar.gen_ctl, PORTSIDE);
      hyscan_exit_if (global.GSS.ps_signals == NULL, "portside: can't load signal presets");

      for (i = 0; global.GFL.fl_signals[i] != NULL; i++);
      global.GFL.fl_n_signals = i;

      for (i = 0; global.GSS.sb_signals[i] != NULL; i++);
      global.GSS.sb_n_signals = i;

      for (i = 0; global.GSS.ps_signals[i] != NULL; i++);
      global.GSS.ps_n_signals = i;

      /* Настройка датчиков и антенн. */
      if (config != NULL)
        {
          status = TRUE;
          status &= setup_sensors (HYSCAN_SENSOR_CONTROL (global.GFL.sonar.sonar_ctl), config);
          status &= setup_sonar_antenna (global.GFL.sonar.sonar_ctl, FORWARDLOOK, config);
          status &= setup_sonar_antenna (global.GSS.sonar.sonar_ctl, STARBOARD, config);
          status &= setup_sonar_antenna (global.GSS.sonar.sonar_ctl, PORTSIDE, config);

          g_signal_connect (HYSCAN_SENSOR_CONTROL (global.GFL.sonar.sonar_ctl), "sensor-data", G_CALLBACK (sensor_cb), &global);
          sensor_label_writer_tag = g_idle_add ((GSourceFunc)sensor_label_writer, &global);

          hyscan_exit_if (!status, "Failed to parse configuration file. ");
        }

      /* Рабочий проект. */
      status = hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GFL.sonar.sonar_ctl), project_name);
      hyscan_exit_if (!status, "forward-look: can't set working project");
    }

  if (prof_sonar != NULL)
    {
      HyScanGeneratorModeType gen_cap;
      HyScanTVGModeType tvg_cap;
      gboolean status;
      guint i;

      /* Управление локатором. */
      global.GPF.sonar.sonar_ctl = hyscan_sonar_control_new (prof_sonar, 4, 4, global.db);
      hyscan_exit_if_w_param (global.GPF.sonar.sonar_ctl == NULL, "unsupported sonar_ctl '%s'", prof_uri);

      global.GPF.sonar.gen_ctl = HYSCAN_GENERATOR_CONTROL (global.GPF.sonar.sonar_ctl);
      global.GPF.sonar.tvg_ctl = HYSCAN_TVG_CONTROL (global.GPF.sonar.sonar_ctl);

      HyScanSourceType *pf_sources, *src_iter;
      pf_sources = hyscan_sonar_control_source_list (global.GPF.sonar.sonar_ctl);
      for (src_iter = pf_sources; *src_iter != HYSCAN_SOURCE_INVALID; ++src_iter)
        {
          /* Не-эхолоты нам не интересны. Увы. */
          if (*src_iter != ECHOSOUNDER)
            continue;

          global.GPF.have_es = TRUE;
          g_message ("This profiler has echosounder channel");

          gen_cap = hyscan_generator_control_get_capabilities (global.GPF.sonar.gen_ctl, ECHOSOUNDER);
          hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "profiler-echo: unsupported generator mode");

          status = hyscan_generator_control_set_enable (global.GPF.sonar.gen_ctl, ECHOSOUNDER, TRUE);
          hyscan_exit_if (!status, "profiler-echo: can't enable generator");

          tvg_cap = hyscan_tvg_control_get_capabilities (global.GPF.sonar.tvg_ctl, ECHOSOUNDER);
          hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "profiler-echo: unsupported tvg_ctl mode");

          global.GPF.es_signals = hyscan_generator_control_list_presets (global.GPF.sonar.gen_ctl, ECHOSOUNDER);
          hyscan_exit_if (global.GPF.es_signals == NULL, "profiler-echo: can't load signal presets");

          status = hyscan_tvg_control_set_enable (global.GPF.sonar.tvg_ctl, ECHOSOUNDER, TRUE);
          hyscan_exit_if (!status, "profiler-echo: can't enable tvg_ctl");
        }

      /* Параметры генераторов. */
      gen_cap = hyscan_generator_control_get_capabilities (global.GPF.sonar.gen_ctl, PROFILER);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "profiler: unsupported generator mode");

      status = hyscan_generator_control_set_enable (global.GPF.sonar.gen_ctl, PROFILER, TRUE);
      hyscan_exit_if (!status, "profiler: can't enable generator");

      /* Параметры ВАРУ. */
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GPF.sonar.tvg_ctl, PROFILER);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "profiler: unsupported tvg_ctl mode");

      global.GPF.pf_signals = hyscan_generator_control_list_presets (global.GPF.sonar.gen_ctl, PROFILER);
      hyscan_exit_if (global.GPF.pf_signals == NULL, "profiler-echo: can't load signal presets");

      status = hyscan_tvg_control_set_enable (global.GPF.sonar.tvg_ctl, PROFILER, TRUE);
      hyscan_exit_if (!status, "profiler: can't enable tvg_ctl");

      /* Сигналы зондирования. */
      for (i = 0; global.GPF.pf_signals[i] != NULL; i++);
      global.GPF.pf_n_signals = i;

      /* Настройка датчиков и антенн. */
      if (config != NULL)
        {
          status = setup_sonar_antenna (global.GPF.sonar.sonar_ctl, PROFILER, config);
          status &= setup_sonar_antenna (global.GPF.sonar.sonar_ctl, ECHOSOUNDER, config);

          hyscan_exit_if (!status, "Failed to parse configuration file. ");

          if (!status)
            goto exit;
        }

      /* Рабочий проект. */
      status = hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GPF.sonar.sonar_ctl), project_name);
      hyscan_exit_if (!status, "profiler: can't set working project");
    }

  /* Задаем синхронизацию. */
  g_message ("Master: flss, slave: profiler");
  hyscan_param_set_enum (HYSCAN_PARAM (flss_sonar), "/parameters/sync-type", 1 /*HYSCAN_SONAR_HYDRA_SYNC_INTERNAL_OUT*/);
  hyscan_param_set_enum (HYSCAN_PARAM (prof_sonar), "/parameters/sync-type", 2 /*HYSCAN_SONAR_HYDRA_SYNC_EXTERNAL*/ );

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
  common_builder = gtk_builder_new_from_resource ("/org/hyscan/gtk/common.ui");

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
    global.mman = hyscan_mark_manager_new ();
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
    hyscan_mark_manager_set_project (global.mman, global.db, global.project_name);

    g_signal_connect (global.mman, "changed", G_CALLBACK (mark_manager_changed), &global);
    g_signal_connect (mark_editor, "mark-modified", G_CALLBACK (mark_modified), &global);
    g_signal_connect (mark_list, "item-changed", G_CALLBACK (active_mark_changed), &global);
  }

  /* Ништяк: скелет гуя сделан. Теперь подцепим тут нужные сигналы. */
  gtk_builder_add_callback_symbol (common_builder, "track_scroll", G_CALLBACK (track_scroll));
  gtk_builder_add_callback_symbol (common_builder, "track_changed", G_CALLBACK (track_changed));
  gtk_builder_add_callback_symbol (common_builder, "position_changed", G_CALLBACK (position_changed));
  gtk_builder_connect_signals (common_builder, &global);

  // hyscan_ame_splash_stop (splash);

  /***
   *     ___   ___   ___   ___         ___                     ___   ___   ___   ___   ___   ___
   *      | |   |   |     |   | |     |   |  \ /        |   |   |     | | |     |       |   |
   *      + |   +    -+-  |-+-  |     |-+-|   +         | + |   +     + | | +-  |-+-    +    -+-
   *      | |   |       | |     |     |   |   |         |/ \|   |     | | |   | |       |       |
   *     ---   ---   ---         ---                           ---   ---   ---   ---         ---
   *
   * Создаем виджеты просмотра.
   */
  global.GSS.wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new ());
  hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (global.GSS.wf), ship_speed);

  GtkWidget *ss_ol = make_overlay (global.GSS.wf,
                                 &global.GSS.wf_grid,
                                 &global.GSS.wf_ctrl,
                                 &global.GSS.wf_mark,
                                 &global.GSS.wf_metr);



  global.GPF.wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new ());
  hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (global.GPF.wf), ship_speed / 10);
  GtkWidget *pf_ol = make_overlay (global.GPF.wf,
                                 &global.GPF.wf_grid,
                                 &global.GPF.wf_ctrl,
                                 &global.GPF.wf_mark,
                                 &global.GPF.wf_metr);
  hyscan_gtk_waterfall_grid_set_condence (global.GPF.wf_grid, 10.0);
  hyscan_gtk_waterfall_grid_set_grid_color (global.GPF.wf_grid, hyscan_tile_color_converter_c2i (32, 32, 32, 255));
  hyscan_gtk_waterfall_state_echosounder (HYSCAN_GTK_WATERFALL_STATE (global.GPF.wf), PROFILER);


  global.GFL.fl = HYSCAN_GTK_FORWARD_LOOK (hyscan_gtk_forward_look_new ());

  global.GFL.fl_player = hyscan_gtk_forward_look_get_player (global.GFL.fl);
  global.GFL.fl_coords = hyscan_fl_coords_new (global.GFL.fl);


  /* Управление воспроизведением FL. */
  fl_play_control = GTK_WIDGET (gtk_builder_get_object (common_builder, "fl_play_control"));
  hyscan_exit_if (fl_play_control == NULL, "can't load play control ui");
  global.GFL.position = GTK_SCALE (gtk_builder_get_object (common_builder, "position"));
  global.GFL.coords_label = GTK_LABEL (gtk_builder_get_object (common_builder, "fl_latlong"));
  g_signal_connect (global.GFL.fl_coords, "coords", G_CALLBACK (fl_coords_callback), global.GFL.coords_label);
  hyscan_exit_if (global.GFL.position == NULL, "incorrect play control ui");
  global.GFL.position_range = hyscan_gtk_forward_look_get_adjustment (global.GFL.fl);
  gtk_range_set_adjustment (GTK_RANGE (global.GFL.position), global.GFL.position_range);

  gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (global.GSS.wf), FALSE);
  gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (global.GPF.wf), FALSE);
  gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (global.GFL.fl), TRUE);

  g_signal_connect (G_OBJECT (global.GSS.wf), "automove-state", G_CALLBACK (live_view_off), &global);
  g_signal_connect_swapped (G_OBJECT (global.GSS.wf), "waterfall-zoom", G_CALLBACK (scale_set), &global);
  g_signal_connect (G_OBJECT (global.GPF.wf), "automove-state", G_CALLBACK (live_view_off), &global);
  g_signal_connect_swapped (G_OBJECT (global.GPF.wf), "waterfall-zoom", G_CALLBACK (scale_set), &global);

  /*  */
  global.gui.widget_names[W_SIDESCAN] = "ГБО";
  global.gui.widget_names[W_PROFILER] = "Профилограф";
  global.gui.widget_names[W_FORWARDL] = "Курсовой";

  /* Центральная зона: два box'a (наполним позже). */
  global.gui.sub_center_box     = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  global.gui.center_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);


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


  /* Cache */
  hyscan_forward_look_player_set_cache (global.GFL.fl_player, global.cache, NULL);
  hyscan_fl_coords_set_cache (global.GFL.fl_coords, global.cache);
  hyscan_gtk_waterfall_state_set_cache (HYSCAN_GTK_WATERFALL_STATE (global.GSS.wf),
                                        global.cache, global.cache, NULL);
  hyscan_gtk_waterfall_state_set_cache (HYSCAN_GTK_WATERFALL_STATE (global.GPF.wf),
                                        global.cache, global.cache, NULL);

  /* SV. */
  global.sound_velocity = sound_velocity;
  svp = g_array_new (FALSE, FALSE, sizeof (HyScanSoundVelocity));
  svp_val.depth = 0.0;
  svp_val.velocity = global.sound_velocity;
  g_array_insert_val (svp, 0, svp_val);

  hyscan_forward_look_player_set_sv (global.GFL.fl_player, global.sound_velocity);
  hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (global.GSS.wf), svp);
  hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (global.GPF.wf), svp);

  hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (global.GSS.wf), 100000);
  hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (global.GPF.wf), 100000);
  hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (global.GSS.wf), 500000);
  hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (global.GPF.wf), 500000);

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
  /* Делаем цветовые схемы. */
  {
    guint32 kolors[2];
    #include "colormaps.h"
    global.GSS.color_maps[0] = g_memdup (orange, 256 * sizeof (guint32));
    global.GSS.color_maps[1] = g_memdup (sepia, 256 * sizeof (guint32));
    global.GSS.color_map_len[0] = 256;
    global.GSS.color_map_len[1] = 256;
    kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
    kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0); /* Белый. */
    global.GSS.color_maps[2] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GSS.color_map_len[2]);
    kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.11, 1.0, 1.0);
    kolors[1] = hyscan_tile_color_converter_d2i (0.83, 0.0, 1.0, 1.0);
    global.GSS.color_maps[3] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GSS.color_map_len[3]);
  }

  global.GPF.color_maps[0] = hyscan_tile_color_compose_colormap_pf (&global.GPF.color_map_len[0]);
/***
 *     ___   ___   ___   ___               ___               ___               ___   ___
 *      | | |     |     |   | |   | |       |         |  /  |   | |     |   | |     |
 *      + | |-+-  |-+-  |-+-| |   | |       +         | +   |-+-| |     |   | |-+-   -+-
 *      | | |     |     |   | |   | |       |         |/    |   | |     |   | |         |
 *     ---   ---               ---   ---                           ---   ---   ---   ---
 *
 * Начальные значения.
 */
  global.GSS.cur_brightness =          keyfile_double_read_helper (config, "GSS", "cur_brightness",          80.0);
  global.GSS.cur_color_map =           keyfile_double_read_helper (config, "GSS", "cur_color_map",           0);
  global.GSS.cur_black =               keyfile_double_read_helper (config, "GSS", "cur_black",               0);
  global.GSS.sonar.cur_distance =      keyfile_double_read_helper (config, "GSS", "sonar.cur_distance",      SIDE_SCAN_MAX_DISTANCE);
  global.GSS.sonar.cur_signal =        keyfile_double_read_helper (config, "GSS", "sonar.cur_signal",        1);
  global.GSS.sonar.cur_gain0 =         keyfile_double_read_helper (config, "GSS", "sonar.cur_gain0",         0.0);
  global.GSS.sonar.cur_gain_step =     keyfile_double_read_helper (config, "GSS", "sonar.cur_gain_step",     10.0);
  global.GSS.sonar.cur_level =         keyfile_double_read_helper (config, "GSS", "sonar.cur_level",         0.5);
  global.GSS.sonar.cur_sensitivity =   keyfile_double_read_helper (config, "GSS", "sonar.cur_sensitivity",   0.6);

  global.GPF.cur_brightness =          keyfile_double_read_helper (config, "GPF", "cur_brightness",          80.0);
  global.GPF.cur_color_map =           keyfile_double_read_helper (config, "GPF", "cur_color_map",           0);
  global.GPF.cur_black =               keyfile_double_read_helper (config, "GPF", "cur_black",               0);
  global.GPF.sonar.cur_distance =      keyfile_double_read_helper (config, "GPF", "sonar.cur_distance",      PROFILER_MAX_DISTANCE);
  global.GPF.sonar.cur_signal =        keyfile_double_read_helper (config, "GPF", "sonar.cur_signal",        1);
  global.GPF.sonar.cur_gain0 =         keyfile_double_read_helper (config, "GPF", "sonar.cur_gain0",         0.0);
  global.GPF.sonar.cur_gain_step =     keyfile_double_read_helper (config, "GPF", "sonar.cur_gain_step",     10.0);

  global.GFL.cur_brightness =          keyfile_double_read_helper (config, "GFL", "cur_brightness",          80.0);
  global.GFL.cur_sensitivity =         keyfile_double_read_helper (config, "GFL", "cur_sensitivity",         8.0);
  global.GFL.sonar.cur_distance =      keyfile_double_read_helper (config, "GFL", "sonar.cur_distance",      FORWARD_LOOK_MAX_DISTANCE);

  global.GFL.sonar.cur_signal =        keyfile_double_read_helper (config, "GFL", "sonar.cur_signal",        1);
  global.GFL.sonar.cur_gain0 =         keyfile_double_read_helper (config, "GFL", "sonar.cur_gain0",         0.0);
  global.GFL.sonar.cur_gain_step =     keyfile_double_read_helper (config, "GFL", "sonar.cur_gain_step",     20.0);


  color_map_set (&global, global.GSS.cur_color_map, W_SIDESCAN);
  brightness_set (&global, global.GSS.cur_brightness, global.GSS.cur_black, W_SIDESCAN);
  scale_set (&global, FALSE, NULL, W_SIDESCAN);
  if (global.GSS.sonar.sonar_ctl != NULL)
    {
      distance_set (&global, global.GSS.sonar.cur_distance, W_SIDESCAN);
      auto_tvg_set (&global, global.GSS.sonar.cur_level, global.GSS.sonar.cur_sensitivity, W_SIDESCAN);
      signal_set (&global, global.GSS.sonar.cur_signal, W_SIDESCAN);
    }

  color_map_set (&global, global.GPF.cur_color_map, W_PROFILER);
  brightness_set (&global, global.GPF.cur_brightness, global.GPF.cur_black, W_PROFILER);
  scale_set (&global, FALSE, NULL, W_PROFILER);
  if (global.GPF.sonar.sonar_ctl != NULL)
    {
      distance_set (&global, global.GPF.sonar.cur_distance, W_PROFILER);
      tvg_set (&global, &global.GPF.sonar.cur_gain0, global.GPF.sonar.cur_gain_step, W_PROFILER);
      signal_set (&global, global.GPF.sonar.cur_signal, W_PROFILER);
    }

  brightness_set (&global, global.GFL.cur_brightness, 0, W_FORWARDL);
  sensitivity_set (&global, global.GFL.cur_sensitivity);
  scale_set (&global, FALSE, NULL, W_FORWARDL);
  if (global.GFL.sonar.sonar_ctl != NULL)
    {
      distance_set (&global, global.GFL.sonar.cur_distance, W_FORWARDL);
      tvg_set (&global, &global.GFL.sonar.cur_gain0, global.GFL.sonar.cur_gain_step, W_FORWARDL);
      signal_set (&global, global.GPF.sonar.cur_signal, W_FORWARDL);
    }

  widget_swap (NULL, GINT_TO_POINTER (ALL));

  /***
   *           ___   ___   ___
   *    |   | |   | |   | |
   *    |   | |-+-  |-+-  |
   *    |   | |  \  |     |
   *     ---               ---
   *
   */
  gchar *urpc_uri = "udp://127.0.0.3:3301";
  gint urpc_status = 0;
  global.urpc_keycode = URPC_KEYCODE_NO_ACTION;

  global.urpc_server = urpc_server_create (urpc_uri,
                                           1,
                                           10,
                                           URPC_DEFAULT_SESSION_TIMEOUT,
                                           URPC_DEFAULT_DATA_SIZE,
                                           URPC_DEFAULT_DATA_TIMEOUT);

  hyscan_exit_if_w_param (global.urpc_server == NULL, "Unable to create uRpc server at %s", urpc_uri);

  urpc_status = urpc_server_add_proc (global.urpc_server, AME_KEY_RIGHT  + URPC_PROC_USER, urpc_cb, GINT_TO_POINTER (AME_KEY_RIGHT));
  hyscan_exit_if (urpc_status != 0, "Failed to register AME_KEY_RIGHT callback. ");
  urpc_status = urpc_server_add_proc (global.urpc_server, AME_KEY_LEFT   + URPC_PROC_USER, urpc_cb, GINT_TO_POINTER (AME_KEY_LEFT));
  hyscan_exit_if (urpc_status != 0, "Failed to register AME_KEY_LEFT callback. ");
  urpc_status = urpc_server_add_proc (global.urpc_server, AME_KEY_ENTER  + URPC_PROC_USER, urpc_cb, GINT_TO_POINTER (AME_KEY_ENTER));
  hyscan_exit_if (urpc_status != 0, "Failed to register AME_KEY_ENTER callback. ");
  urpc_status = urpc_server_add_proc (global.urpc_server, AME_KEY_UP     + URPC_PROC_USER, urpc_cb, GINT_TO_POINTER (AME_KEY_UP));
  hyscan_exit_if (urpc_status != 0, "Failed to register AME_KEY_UP callback. ");
  urpc_status = urpc_server_add_proc (global.urpc_server, AME_KEY_ESCAPE + URPC_PROC_USER, urpc_cb, GINT_TO_POINTER (AME_KEY_ESCAPE));
  hyscan_exit_if (urpc_status != 0, "Failed to register AME_KEY_ESCAPE callback. ");
  urpc_status = urpc_server_add_proc (global.urpc_server, AME_KEY_EXIT   + URPC_PROC_USER, urpc_cb, GINT_TO_POINTER (AME_KEY_EXIT));
  hyscan_exit_if (urpc_status != 0, "Failed to register AME_KEY_EXIT callback. ");

  urpc_status = urpc_server_bind (global.urpc_server);
  hyscan_exit_if (urpc_status != 0, "Unable to start uRpc server. ");
  g_timeout_add (100, ame_key_func, &global);

  if (full_screen)
    gtk_window_fullscreen (GTK_WINDOW (global.gui.window));

  gtk_widget_show_all (global.gui.window);

  global.marksync.msync = hyscan_mark_sync_new ("127.0.0.1", 10010);


  gtk_main ();
  if (sensor_label_writer_tag > 0)
    g_source_remove (sensor_label_writer_tag);
  // hyscan_ame_splash_start (splash, "Отключение");

  if (global.GSS.sonar.sonar_ctl != NULL)
    hyscan_sonar_control_stop (global.GSS.sonar.sonar_ctl);
  if (global.GPF.sonar.sonar_ctl != NULL)
    hyscan_sonar_control_stop (global.GPF.sonar.sonar_ctl);
  if (global.GFL.sonar.sonar_ctl != NULL)
    hyscan_sonar_control_stop (global.GFL.sonar.sonar_ctl);

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
      keyfile_double_write_helper (config, "GSS", "cur_brightness",          global.GSS.cur_brightness);
      keyfile_double_write_helper (config, "GSS", "cur_color_map",           global.GSS.cur_color_map);
      keyfile_double_write_helper (config, "GSS", "cur_black",               global.GSS.cur_black);
      keyfile_double_write_helper (config, "GSS", "sonar.cur_signal",        global.GSS.sonar.cur_signal);
      keyfile_double_write_helper (config, "GSS", "sonar.cur_distance",      global.GSS.sonar.cur_distance);
      keyfile_double_write_helper (config, "GSS", "sonar.cur_gain0",         global.GSS.sonar.cur_gain0);
      keyfile_double_write_helper (config, "GSS", "sonar.cur_gain_step",     global.GSS.sonar.cur_gain_step);
      keyfile_double_write_helper (config, "GSS", "sonar.cur_level",         global.GSS.sonar.cur_level);
      keyfile_double_write_helper (config, "GSS", "sonar.cur_sensitivity",   global.GSS.sonar.cur_sensitivity);
      keyfile_double_write_helper (config, "GPF", "cur_brightness",          global.GPF.cur_brightness);
      keyfile_double_write_helper (config, "GPF", "cur_color_map",           global.GPF.cur_color_map);
      keyfile_double_write_helper (config, "GPF", "cur_black",               global.GPF.cur_black);
      keyfile_double_write_helper (config, "GPF", "sonar.cur_signal",        global.GPF.sonar.cur_signal);
      keyfile_double_write_helper (config, "GPF", "sonar.cur_distance",      global.GPF.sonar.cur_distance);
      keyfile_double_write_helper (config, "GPF", "sonar.cur_gain0",         global.GPF.sonar.cur_gain0);
      keyfile_double_write_helper (config, "GPF", "sonar.cur_gain_step",     global.GPF.sonar.cur_gain_step);
      keyfile_double_write_helper (config, "GFL", "cur_brightness",          global.GFL.cur_brightness);
      keyfile_double_write_helper (config, "GFL", "cur_sensitivity",         global.GFL.cur_sensitivity);
      keyfile_double_write_helper (config, "GFL", "sonar.cur_signal",        global.GFL.sonar.cur_signal);
      keyfile_double_write_helper (config, "GFL", "sonar.cur_distance",      global.GFL.sonar.cur_distance);
      keyfile_double_write_helper (config, "GFL", "sonar.cur_gain0",         global.GFL.sonar.cur_gain0);
      keyfile_double_write_helper (config, "GFL", "sonar.cur_gain_step",     global.GFL.sonar.cur_gain_step);

      keyfile_string_write_helper (config, "common",  "project",                 global.project_name);

      g_key_file_save_to_file (config, config_file, NULL);
      g_key_file_unref (config);
    }

exit:

  g_clear_object (&common_builder);

  g_clear_object (&global.cache);
  g_clear_object (&global.db_info);
  g_clear_object (&global.db);

  g_clear_pointer (&global.GFL.fl_signals, hyscan_data_schema_free_enum_values);
  g_clear_pointer (&global.GSS.sb_signals, hyscan_data_schema_free_enum_values);
  g_clear_pointer (&global.GSS.ps_signals, hyscan_data_schema_free_enum_values);
  g_clear_pointer (&global.GPF.pf_signals, hyscan_data_schema_free_enum_values);
  g_clear_object (&global.GFL.sonar.sonar_ctl);
  g_clear_object (&global.GPF.sonar.sonar_ctl);
  g_clear_object (&global.GSS.sonar.sonar_ctl);
  g_clear_object (&flss_sonar);
  g_clear_object (&prof_sonar);

  g_free (global.GSS.color_maps[0]);
  g_free (global.GSS.color_maps[1]);
  g_free (global.GSS.color_maps[2]);
  g_free (global.GPF.color_maps[0]);
  g_free (global.GPF.color_maps[1]);
  g_free (global.GPF.color_maps[2]);

  g_clear_object (&global.GSS.wf);
  g_clear_object (&global.GPF.wf);
  g_clear_object (&global.GFL.fl);

  g_free (global.track_name);
  g_free (driver_path);
  g_free (driver_name);
  g_free (db_uri);
  g_free (project_name);
  g_free (config_file);

  // g_clear_object (&splash);

  return 0;
}
