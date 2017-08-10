#include <hyscan-gtk-area.h>
#include <hyscan-sonar-driver.h>
#include <hyscan-sonar-client.h>
#include <hyscan-sonar-control.h>
#include <hyscan-forward-look-data.h>
#include <hyscan-gtk-waterfall-grid.h>
#include <hyscan-tile-color.h>
#include <hyscan-db-info.h>
#include <hyscan-cached.h>
#include <math.h>

#include "sonar-configure.h"
#include "hyscan-gtk-forward-look.h"

#define hyscan_return_val_if_fail(expr,val) do {if (!(expr)) return (val);} while (FALSE)
#define hyscan_exit_if(expr,msg) do {if (!(expr)) break; g_message ((msg)); goto exit;} while (FALSE)
#define hyscan_exit_if_w_param(expr,msg,param) do {if (!(expr)) break; g_message ((msg),(param)); goto exit;} while (FALSE)
#define STARBOARD HYSCAN_SOURCE_SIDE_SCAN_STARBOARD
#define PORTSIDE HYSCAN_SOURCE_SIDE_SCAN_PORT
#define PROFILER HYSCAN_SOURCE_PROFILER
#define FORWARDLOOK HYSCAN_SOURCE_FORWARD_LOOK

#define FORWARD_LOOK_MAX_DISTANCE      75.0
#define SIDE_SCAN_MAX_DISTANCE         150.0
#define PROFILER_MAX_DISTANCE          50.0

#define MAX_COLOR_MAPS                 3
#define BLACK_BG                       0xff000000

enum
{
  DATE_SORT_COLUMN,
  TRACK_COLUMN,
  DATE_COLUMN,
  N_COLUMNS
};

enum
{
  W_SIDESCAN,
  W_PROFILER,
  W_FORWARDL,
  W_LAST
};

typedef struct
{
  HyScanSonarControl      *sonar;
  HyScanTVGControl        *tvg;
  HyScanGeneratorControl  *gen;

  gdouble                  cur_distance;
  guint                    cur_signal;
  gdouble                  cur_gain0;
  gdouble                  cur_gain_step;

} SonarCommon;

typedef struct
{
  GtkLabel                *distance_value;
  GtkLabel                *tvg0_value;
  GtkLabel                *tvg_value;
  GtkLabel                *signal_value;

  GtkLabel                *brightness_value;
  GtkLabel                *scale_value;
  GtkLabel                *third_value; /* Sensitivity or colormap. */

  //GtkBox                  *widget_container;
  //GtkBox                  *view_control;
  //GtkBox                  *sonar_control;
} SonarSpecificGui;

typedef struct
{
  //struct
  //{
  HyScanDB                            *db;
  HyScanDBInfo                        *db_info;

  gchar                               *project_name;
  gchar                               *track_prefix;
  gchar                               *track_name;

  HyScanCache                         *cache;

  gboolean                             full_screen;
  gdouble                              sound_velocity;

  gint                                 sonar_selector;
  //} common;

  struct
    {
      GtkWidget                           *window; // окно

      GtkWidget                           *gtk_area; // ареа
      GtkWidget                           *h_pane;
      GtkWidget                           *v_pane;

      GtkWidget                           *right_box; // бокс справа
      GtkWidget                           *selector; //
      GtkWidget                           *stack; // контейнер для виджетов настроек
      GtkWidget                           *record_control;

      GtkSwitch                           *start_stop_switch;

      GtkWidget                           *track_control;
      GtkTreeView                         *track_view;
      GtkTreeModel                        *track_list;
      GtkAdjustment                       *track_range;

      GtkWidget                           *ctrl_widgets[W_LAST];
      GtkWidget                           *disp_widgets[W_LAST];

  } gui;

  struct
  {
    gdouble                              cur_brightness;
    gdouble                              cur_sensitivity;

    SonarCommon                          sonar;
    HyScanDataSchemaEnumValue          **fl_signals;
    guint                                fl_n_signals;

    SonarSpecificGui                     gui;

    HyScanGtkForwardLook                *fl;
    HyScanForwardLookPlayer             *fl_player;
    // TODO: fl with player widget!
    GtkAdjustment                       *position_range;
    GtkScale                            *position;
    GtkSwitch                           *mode_target;
  } GFL;

  struct
  {
    guint32                             *color_maps[MAX_COLOR_MAPS];
    guint                                color_map_len[MAX_COLOR_MAPS];
    guint                                cur_color_map;
    gdouble                              cur_brightness;

    SonarCommon                          sonar;
    HyScanDataSchemaEnumValue          **pf_signals;
    guint                                pf_n_signals;

    SonarSpecificGui                     gui;

    HyScanGtkWaterfall                  *wf;
    GtkSwitch                           *live_view;
  } GPF;

  struct
  {
    guint32                             *color_maps[MAX_COLOR_MAPS];
    guint                                color_map_len[MAX_COLOR_MAPS];
    guint                                cur_color_map;
    gdouble                              cur_brightness;

    SonarCommon                          sonar;
    HyScanDataSchemaEnumValue          **sb_signals;
    guint                                sb_n_signals;
    HyScanDataSchemaEnumValue          **ps_signals;
    guint                                ps_n_signals;

    SonarSpecificGui                     gui;
    HyScanGtkWaterfall                  *wf;
    GtkSwitch                           *live_view;
  } GSS;

} NewGlobal;

NewGlobal global = { 0 };

/* Функция изменяет режим окна full screen. */
static gboolean
key_press (GtkWidget   *widget,
           GdkEventKey *event,
           NewGlobal      *global)
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

/* Функция вызывается при изменении списка проектов. */
static void
projects_changed (HyScanDBInfo *db_info,
                  NewGlobal       *global)
{
  GHashTable *projects = hyscan_db_info_get_projects (db_info);

  /* Если рабочий проект есть в списке, мониторим его. */
  if (g_hash_table_lookup (projects, global->project_name))
    hyscan_db_info_set_project (db_info, global->project_name);

  g_hash_table_unref (projects);
}

/* Функция вызывается при изменении списка галсов. */
static void
tracks_changed (HyScanDBInfo *db_info,
                NewGlobal       *global)
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
      HyScanTrackInfo *track_info;

      //HyScanSourceInfo *forward_look_info;
      /* Проверяем что галс содержит данные вперёдсмотрящего локатора. */
      // TODO: проверить при открытии, может показать заглушку.
      //track_info = value;
      //forward_look_info = g_hash_table_lookup (track_info->sources, GINT_TO_POINTER (HYSCAN_SOURCE_FORWARD_LOOK));
      //if (!forward_look_info || !forward_look_info->raw)
      //  continue;

      /* Добавляем в список галсов. */
      gtk_list_store_append (GTK_LIST_STORE (global->gui.track_list), &tree_iter);
      gtk_list_store_set (GTK_LIST_STORE (global->gui.track_list), &tree_iter,
                          DATE_SORT_COLUMN, g_date_time_to_unix (track_info->ctime),
                          TRACK_COLUMN, g_strdup (track_info->name),
                          DATE_COLUMN, g_date_time_format (track_info->ctime, "%d/%m/%Y %H:%M"),
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

/* Функция прокручивает список галсов. */
static gboolean
track_scroll (GtkWidget *widget,
              GdkEvent  *event,
              NewGlobal    *global)
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

/* Обработчик изменения галса. */
static void
track_changed (GtkTreeView *list,
               NewGlobal      *global)
{
  GValue value = G_VALUE_INIT;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  const gchar *track_name;

  /* Определяем название нового галса. */
  gtk_tree_view_get_cursor (list, &path, NULL);
  if (path == NULL)
    return;

  if (!gtk_tree_model_get_iter (global->gui.track_list, &iter, path))
    return;

  /* Открываем новый галс. */
  gtk_tree_model_get_value (global->gui.track_list, &iter, TRACK_COLUMN, &value);
  track_name = g_value_get_string (&value);

  hyscan_forward_look_player_open (global->GFL.fl_player, global->db, global->project_name, track_name, TRUE);
  hyscan_gtk_waterfall_open (global->GSS.wf, global->db, global->project_name, track_name, TRUE);
  hyscan_gtk_waterfall_open (global->GPF.wf, global->db, global->project_name, track_name, TRUE);

  g_value_unset (&value);

  /* Режим отображения. */
  if (gtk_switch_get_state (global->gui.start_stop_switch))
    {
      hyscan_forward_look_player_real_time (global->GFL.fl_player);
      hyscan_gtk_waterfall_drawer_automove (HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf), TRUE);
      hyscan_gtk_waterfall_drawer_automove (HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf), TRUE);
    }
  else
    {
      hyscan_forward_look_player_pause (global->GFL.fl_player);
      hyscan_gtk_waterfall_drawer_automove (HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf), FALSE);
      hyscan_gtk_waterfall_drawer_automove (HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf), FALSE);
    }
}

/* Функция обрабатывает данные для текущего индекса. */
static void
position_changed (GtkAdjustment *range,
                  NewGlobal        *global)
{
}

/* Функция устанавливает яркость отображения. */
static gboolean
brightness_set (NewGlobal  *global,
                gdouble  cur_brightness)
{
  gchar *text;
  gdouble b, g, w;

  if (cur_brightness < 1.0)
    return FALSE;
  if (cur_brightness > 100.0)
    return FALSE;

  b = 0.0;
  g = 1.25 - 0.5 * (cur_brightness / 100.0);
  w = 1.0 - (cur_brightness / 100.0) * 0.99;

  text = g_strdup_printf ("<small><b>%.0f%%</b></small>", cur_brightness);

  switch (global->sonar_selector)
    {
    case W_SIDESCAN:
      hyscan_gtk_waterfall_drawer_set_levels_for_all (HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf), b, g, w);
      gtk_label_set_markup (global->GSS.gui.brightness_value, text);
      break;
    case W_PROFILER:
      hyscan_gtk_waterfall_drawer_set_levels_for_all (HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf), b, g, w);
      gtk_label_set_markup (global->GPF.gui.brightness_value, text);
      break;
    case W_FORWARDL:
      hyscan_gtk_forward_look_set_brightness (global->GFL.fl, cur_brightness);
      gtk_label_set_markup (global->GFL.gui.brightness_value, text);
      break;
    }

  g_free (text);

  return TRUE;
}

/* Функция устанавливает масштаб отображения. */
static gboolean
scale_set (NewGlobal   *global,
           gboolean  scale_up)
{
  GtkCifroAreaZoomType zoom_dir;
  gdouble from_y, to_y;
  gdouble min_y, max_y;
  gdouble scale;
  gchar *text = NULL;

  const gdouble *scales;
  gint n_scales;
  gint i_scale;

  switch (global->sonar_selector)
    {
    case W_SIDESCAN:
      i_scale = hyscan_gtk_waterfall_drawer_get_scale (HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf),
                                                       &scales, &n_scales);

      text = g_strdup_printf ("<small><b>1:%.0f</b></small>", scales[i_scale]);
      gtk_label_set_markup (global->GSS.gui.scale_value, text);
      break;

    case W_PROFILER:
      i_scale = hyscan_gtk_waterfall_drawer_get_scale (HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf),
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

/* Функция устанавливает новую палитру. */
static gboolean
color_map_set (NewGlobal *global,
               guint   cur_color_map)
{
  HyScanGtkWaterfallDrawer *wfd = NULL;
  GtkLabel *label = NULL;
  guint32 **cmap = NULL;
  guint   *len = NULL;
  gchar   *text;
  const gchar *color_map_name;

  if (cur_color_map >= MAX_COLOR_MAPS)
    return FALSE;

  switch (global->sonar_selector)
    {
    case W_SIDESCAN:
      wfd = HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf);
      cmap = global->GSS.color_maps;
      len = global->GSS.color_map_len;
      label = global->GSS.gui.third_value;
      break;
    case W_PROFILER:
      wfd = HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf);
      cmap = global->GPF.color_maps;
      len = global->GPF.color_map_len;
      label = global->GPF.gui.third_value;
      break;

    default:
      g_warning ("Invalid sonar_selector!"); //TODO: remove when debugged;
      return FALSE;
    }

  switch (cur_color_map)
    {
    case 0:
      color_map_name = "БЕЛАЯ";
      break;
    case 1:
      color_map_name = "ЖЁЛТАЯ";
      break;
    case 2:
      color_map_name = "ЗЕЛЁНАЯ";
      break;
    default:
      return FALSE;
    }


  hyscan_gtk_waterfall_drawer_set_colormap_for_all (wfd, cmap[cur_color_map], len[cur_color_map], BLACK_BG);

  text = g_strdup_printf ("<small><b>%s</b></small>", color_map_name);
  gtk_label_set_markup (label, text);
  g_free (text);

  return TRUE;
}

/* Функция устанавливает порог чувствительности. */
static gboolean
sensitivity_set (NewGlobal  *global,
                 gdouble  cur_sensitivity)
 {
   gchar *text;

   if (cur_sensitivity < 1.0)
     return FALSE;
   if (cur_sensitivity > 10.0)
     return FALSE;

   hyscan_gtk_forward_look_set_sensitivity (global->GFL.fl, cur_sensitivity);

   text = g_strdup_printf ("<small><b>%.0f</b></small>", cur_sensitivity);
   gtk_label_set_markup (global->GFL.gui.third_value, text);
   g_free (text);

   return TRUE;
 }


/* Функция устанавливает излучаемый сигнал. */
static gboolean
signal_set (NewGlobal *global,
            guint   cur_signal)
{
  gchar *text;
  gboolean status;

  GtkLabel *label;
  HyScanGeneratorControl *gen;
  HyScanSourceType src0, src1;
  HyScanDataSchemaEnumValue **signals0, **signals1;
  gboolean is_equal;

  if (cur_signal == 0)
    return FALSE;

  switch (global->sonar_selector)
    {
    case W_SIDESCAN:
      if (cur_signal >= global->GSS.sb_n_signals || cur_signal >= global->GSS.ps_n_signals)
        return FALSE;
      src0 = STARBOARD;
      src1 = PORTSIDE;
      gen = global->GSS.sonar.gen;
      signals0 = global->GSS.sb_signals;
      signals1 = global->GSS.ps_signals;
      label = global->GSS.gui.signal_value;
      break;
    case W_PROFILER:
      if (cur_signal >= global->GPF.pf_n_signals)
        return FALSE;
      src0 = src1 = PROFILER;
      gen = global->GPF.sonar.gen;
      signals0 = signals1 = global->GPF.pf_signals;
      label = global->GPF.gui.signal_value;
      break;
    case W_FORWARDL:
      if (cur_signal >= global->GFL.fl_n_signals)
        return FALSE;
      src0 = src1 = FORWARDLOOK;
      gen = global->GFL.sonar.gen;
      signals0 = signals1 = global->GFL.fl_signals;
      label = global->GFL.gui.signal_value;
      break;
    default:
      g_warning ("signal_set: wrong sonar selector!");
      return;
    }

  status = hyscan_generator_control_set_preset (gen, src0, signals0[cur_signal]->value);
  if (!status)
    return FALSE;

  if (src1 != src0)
    {
      status = hyscan_generator_control_set_preset (gen, src1, signals1[cur_signal]->value);
      if (!status)
        return FALSE;
    }

  if (src0 == src1)
    {
      text = g_strdup_printf ("<small><b>%s</b></small>", signals0[cur_signal]->name);
    }
  else
    {
      is_equal = g_strcmp0 (signals0[cur_signal]->name, signals1[cur_signal]->name);
      if (is_equal == 0)
        {
          text = g_strdup_printf ("<small><b>%s</b></small>", signals0[cur_signal]->name);
        }
      else
        {
          text = g_strdup_printf ("<small><b>%s, %s</b></small>",
                                  signals0[cur_signal]->name,
                                  signals1[cur_signal]->name);
        }
    }

  gtk_label_set_markup (label, text);
  g_free (text);

  return TRUE;
}

/* Функция устанавливает параметры ВАРУ. */
static gboolean
tvg_set (NewGlobal  *global,
         gdouble  gain0,
         gdouble  step)
{
  gchar *gain_text, *step_text;
  gboolean status;
  GtkLabel *tvg0_value, *tvg_value;


  // TODO: SS has: hyscan_tvg_control_get_gain_range (global->sonar.tvg, STARBOARD,
  //                                   &min_gain, &max_gain); Maybe I should use it too?
  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        status = hyscan_tvg_control_set_linear_db (global->GSS.sonar.tvg, STARBOARD, gain0, step);
        hyscan_return_val_if_fail (status, FALSE);

        status = hyscan_tvg_control_set_linear_db (global->GSS.sonar.tvg, PORTSIDE, gain0, step);
        hyscan_return_val_if_fail (status, FALSE);

        tvg_value = global->GSS.gui.tvg_value;
        tvg0_value = global->GSS.gui.tvg0_value;
        break;

      case W_PROFILER:
        status = hyscan_tvg_control_set_linear_db (global->GPF.sonar.tvg, PROFILER, gain0, step);
        hyscan_return_val_if_fail (status, FALSE);
        tvg_value = global->GPF.gui.tvg_value;
        tvg0_value = global->GPF.gui.tvg0_value;
        break;

      case W_FORWARDL:
        status = hyscan_tvg_control_set_linear_db (global->GFL.sonar.tvg, FORWARDLOOK, gain0, step);
        hyscan_return_val_if_fail (status, FALSE);
        tvg_value = global->GFL.gui.tvg_value;
        tvg0_value = global->GFL.gui.tvg0_value;
        break;
      default:
        g_warning ("tvg_set: wrong sonar selector!");
        return FALSE;
    }

  gain_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", gain0);
  gtk_label_set_markup (tvg0_value, gain_text);
  g_free (gain_text);
  step_text = g_strdup_printf ("<small><b>%.1f dB</b></small>", step);
  gtk_label_set_markup (tvg_value, step_text);
  g_free (step_text);

  return TRUE;
}

/* Функция устанавливает рабочую дистанцию. */
static gboolean
distance_set (NewGlobal  *global,
              gdouble  cur_distance)
{
  gchar *text;
  gdouble real_distance;
  gboolean status;
  GtkLabel *distance_value;

  if (cur_distance < 1.0)
    return FALSE;

  real_distance = cur_distance / (global->sound_velocity / 2.0);

  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        hyscan_return_val_if_fail (cur_distance <= SIDE_SCAN_MAX_DISTANCE, FALSE);
        status = hyscan_sonar_control_set_receive_time (global->GSS.sonar.sonar, STARBOARD, real_distance);
        hyscan_return_val_if_fail (status, FALSE);

        status = hyscan_sonar_control_set_receive_time (global->GSS.sonar.sonar, PORTSIDE, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_value = global->GSS.gui.distance_value;

      case W_PROFILER:
        hyscan_return_val_if_fail (cur_distance <= PROFILER_MAX_DISTANCE, FALSE);
        status = hyscan_sonar_control_set_receive_time (global->GPF.sonar.sonar, PROFILER, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_value = global->GPF.gui.distance_value;

      case W_FORWARDL:
        hyscan_return_val_if_fail (cur_distance <= FORWARD_LOOK_MAX_DISTANCE, FALSE);
        status = hyscan_sonar_control_set_receive_time (global->GFL.sonar.sonar, FORWARDLOOK, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_value = global->GFL.gui.distance_value;
      default:
        g_warning ("distance_set: wrong sonar selector!");
        return TRUE;

    }

  text = g_strdup_printf ("<small><b>%.0f м</b></small>", cur_distance);
  gtk_label_set_markup (distance_value, text);
  g_free (text);

  return TRUE;
}

static void
void_callback (gpointer data)
{
  g_message ("This callback does nothing and indicates, that you failed at connecting signals");
}

static void
brightness_up (GtkWidget *widget,
               NewGlobal    *global)
{
  gdouble new_brightness;

  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        new_brightness = global->GSS.cur_brightness;
        break;
      case W_PROFILER:
        new_brightness = global->GPF.cur_brightness;
        break;
      case W_FORWARDL:
        new_brightness = global->GFL.cur_brightness;
        break;
      default:
        g_warning ("brightness_up: wrong sonar selector!");
        return;
    }

  /* FL: */
  if (global->sonar_selector == W_FORWARDL)
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

  /* SS: */
  else
    {
      if (new_brightness < 50.0)
        new_brightness = new_brightness + 10.0;
      else if (new_brightness < 90.0)
        new_brightness = new_brightness + 5.0;
      else
        new_brightness = new_brightness + 1.0;
    }

  new_brightness = CLAMP (new_brightness, 1.0, 100.0);
  if (!brightness_set (global, new_brightness))
    return;

  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        global->GSS.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global->GSS.wf));
        break;
      case W_PROFILER:
        global->GPF.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global->GPF.wf));
        break;
      case W_FORWARDL:
        global->GFL.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));
        break;
    }
}

static void
brightness_down (GtkWidget *widget,
                 NewGlobal    *global)
{
  gdouble new_brightness;

  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        new_brightness = global->GSS.cur_brightness;
        break;
      case W_PROFILER:
        new_brightness = global->GPF.cur_brightness;
        break;
      case W_FORWARDL:
        new_brightness = global->GFL.cur_brightness;
        break;
      default:
        g_warning ("brightness_down: wrong sonar selector!");
        return;
    }

  /* FL: */
  if (global->sonar_selector == W_FORWARDL)
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

  /* SS: */
  else
    {
      if (new_brightness > 90.0)
        new_brightness = new_brightness - 1.0;
      else if (new_brightness > 50.0)
        new_brightness = new_brightness - 5.0;
      else
        new_brightness = new_brightness - 10.0;
    }

  new_brightness = CLAMP (new_brightness, 1.0, 100.0);
  if (!brightness_set (global, new_brightness))
    return;

  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        global->GSS.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global->GSS.wf));
        break;
      case W_PROFILER:
        global->GPF.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global->GPF.wf));
        break;
      case W_FORWARDL:
        global->GFL.cur_brightness = new_brightness;
        gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));
        break;
    }
}

static void
scale_up (GtkWidget *widget,
          NewGlobal    *global)
{
  scale_set (global, TRUE);
}

static void
scale_down (GtkWidget *widget,
            NewGlobal    *global)
{
  scale_set (global, FALSE);
}

static void
sensitivity_up (GtkWidget *widget,
                NewGlobal    *global)
{
  gdouble new_sensitivity;

  new_sensitivity = global->GFL.cur_sensitivity + 1;
  if (sensitivity_set (global, new_sensitivity))
    global->GFL.cur_sensitivity = new_sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));
}

static void
sensitivity_down (GtkWidget *widget,
                  NewGlobal    *global)
{
  gdouble new_sensitivity;

  new_sensitivity = global->GFL.cur_sensitivity - 1;
  if (sensitivity_set (global, new_sensitivity))
    global->GFL.cur_sensitivity = new_sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));
}

static void
color_map_up (GtkWidget *widget,
              NewGlobal    *global)
{
  guint cur_color_map;

  switch (global->sonar_selector)
    {
      case W_FORWARDL:
        g_message ("wrong color_map_up!"); //TODO: remove when debugged
        return;
      case W_SIDESCAN:
        cur_color_map = global->GSS.cur_color_map + 1;
        if (color_map_set (global, cur_color_map))
          global->GSS.cur_color_map = cur_color_map;
        break;
      case W_PROFILER:
        cur_color_map = global->GPF.cur_color_map + 1;
        if (color_map_set (global, cur_color_map))
          global->GPF.cur_color_map = cur_color_map;
        break;
    }
}

static void
color_map_down (GtkWidget *widget,
                NewGlobal    *global)
{
  guint cur_color_map;

  switch (global->sonar_selector)
    {
      case W_FORWARDL:
        g_message ("wrong color_map_down!"); //TODO: remove when debugged
        return;
      case W_SIDESCAN:
        cur_color_map = global->GSS.cur_color_map - 1;
        if (color_map_set (global, cur_color_map))
          global->GSS.cur_color_map = cur_color_map;
        break;
      case W_PROFILER:
        cur_color_map = global->GPF.cur_color_map - 1;
        if (color_map_set (global, cur_color_map))
          global->GPF.cur_color_map = cur_color_map;
        break;
    }
}

/* Функция переключает режим отображения виджета ВС. */
static gboolean
mode_changed (GtkWidget *widget,
              gboolean   state,
              NewGlobal    *global)
{
  if (global->sonar_selector == W_SIDESCAN || global->sonar_selector == W_PROFILER)
    {
      g_message ("wrong mode_changed!");
      return FALSE;
    }

  gtk_switch_set_state (GTK_SWITCH (widget), state);
  hyscan_gtk_forward_look_set_type (global->GFL.fl, state ?
                                                    HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET :
                                                    HYSCAN_GTK_FORWARD_LOOK_VIEW_SURFACE);
  gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));

  return TRUE;
}

/* Функция переключает автосдвижку. */
static gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           NewGlobal     *global)
{
  GtkSwitch *live_view;
  HyScanGtkWaterfallDrawer *wfd;

  g_message ("LIVE VIEW TRIGGERED");

  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        live_view = global->GSS.live_view;
        wfd = HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf);
        break;
      case W_PROFILER:
        live_view = global->GPF.live_view;
        wfd = HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf);
        break;
      case W_FORWARDL:
        g_message ("live_view: wrong sonar_selector");
        return TRUE;
    }

  // TODO: probably simplify.
  if (state)
    {
      if (!gtk_switch_get_state (live_view))
        hyscan_gtk_waterfall_drawer_automove (wfd, state);

      gtk_switch_set_state (live_view, TRUE);
    }
  else
    {
      if (gtk_switch_get_state (live_view))
        hyscan_gtk_waterfall_drawer_automove (wfd, state);

      gtk_switch_set_state (live_view, FALSE);
    }
  return TRUE;
}

static void
live_view_off (GtkWidget  *widget,
               gboolean    state,
               NewGlobal     *global)
{
  switch (global->sonar_selector)
    {
      case W_SIDESCAN:
        if (!state)
          gtk_switch_set_active (global->GSS.live_view, FALSE);
        break;
      case W_PROFILER:
        if (!state)
          gtk_switch_set_active (global->GPF.live_view, FALSE);
        break;
      case W_FORWARDL:
      default:
        g_message ("live_view_off: wrong sonar_selector!"); // TODO: removewhendebugged
        break;
    }
}

static void
distance_up (GtkWidget *widget,
             NewGlobal    *global)
{
  gdouble cur_distance;

  switch (global->sonar_selector)
    {
      case W_FORWARDL:
        cur_distance = global->GFL.sonar.cur_distance + 5.0;
        if (distance_set (global, cur_distance))
          global->GFL.sonar.cur_distance = cur_distance;
        break;
      case W_SIDESCAN:
        cur_distance = global->GSS.sonar.cur_distance + 5.0;
        if (distance_set (global, cur_distance))
          global->GSS.sonar.cur_distance = cur_distance;
        break;
      case W_PROFILER:
        cur_distance = global->GSS.sonar.cur_distance + 5.0;
        if (distance_set (global, cur_distance))
          global->GSS.sonar.cur_distance = cur_distance;
        break;
    }
}

static void
distance_down (GtkWidget *widget,
               NewGlobal    *global)
{
  gdouble cur_distance;

  switch (global->sonar_selector)
    {
      case W_FORWARDL:
        cur_distance = global->GFL.sonar.cur_distance - 5.0;
        if (distance_set (global, cur_distance))
          global->GFL.sonar.cur_distance = cur_distance;
        break;
      case W_SIDESCAN:
        cur_distance = global->GSS.sonar.cur_distance - 5.0;
        if (distance_set (global, cur_distance))
          global->GSS.sonar.cur_distance = cur_distance;
        break;
      case W_PROFILER:
        cur_distance = global->GSS.sonar.cur_distance - 5.0;
        if (distance_set (global, cur_distance))
          global->GSS.sonar.cur_distance = cur_distance;
        break;
    }
}

static void
tvg0_up (GtkWidget *widget,
         NewGlobal    *global)
{
  gdouble cur_gain0;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain0 = global->GFL.sonar.cur_gain0 + 0.5;
      if (tvg_set (global, cur_gain0, global->GFL.sonar.cur_gain_step))
        global->GFL.sonar.cur_gain0 = cur_gain0;
      break;

    case W_SIDESCAN:
      cur_gain0 = global->GSS.sonar.cur_gain0 + 0.5;
      if (tvg_set (global, cur_gain0, global->GSS.sonar.cur_gain_step))
        global->GSS.sonar.cur_gain0 = cur_gain0;
      break;

    case W_PROFILER:
      cur_gain0 = global->GPF.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, cur_gain0, global->GPF.sonar.cur_gain_step))
        global->GPF.sonar.cur_gain0 = cur_gain0;
      break;
    }
}

static void
tvg0_down (GtkWidget *widget,
           NewGlobal    *global)
{
  gdouble cur_gain0;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain0 = global->GFL.sonar.cur_gain0 - 0.5;
      if (tvg_set (global, cur_gain0, global->GFL.sonar.cur_gain_step))
        global->GFL.sonar.cur_gain0 = cur_gain0;
      break;

    case W_SIDESCAN:
      cur_gain0 = global->GSS.sonar.cur_gain0 - 0.5;
      if (tvg_set (global, cur_gain0, global->GSS.sonar.cur_gain_step))
        global->GSS.sonar.cur_gain0 = cur_gain0;
      break;

    case W_PROFILER:
      cur_gain0 = global->GPF.sonar.cur_gain0 - 2.5;
      if (tvg_set (global, cur_gain0, global->GPF.sonar.cur_gain_step))
        global->GPF.sonar.cur_gain0 = cur_gain0;
    }
}

static void
tvg_up (GtkWidget *widget,
        NewGlobal    *global)
{
  gdouble cur_gain_step;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain_step = global->GFL.sonar.cur_gain_step + 2.5;
      if (tvg_set (global, global->GFL.sonar.cur_gain0, cur_gain_step))
        global->GFL.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_SIDESCAN:
      cur_gain_step = global->GSS.sonar.cur_gain_step + 2.5;
      if (tvg_set (global, global->GSS.sonar.cur_gain0, cur_gain_step))
        global->GSS.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_PROFILER:
      cur_gain_step = global->GPF.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, global->GPF.sonar.cur_gain0, cur_gain_step))
        global->GPF.sonar.cur_gain_step = cur_gain_step;
    }
}

static void
tvg_down (GtkWidget *widget,
          NewGlobal    *global)
{
  gdouble cur_gain_step;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain_step = global->GFL.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, global->GFL.sonar.cur_gain0, cur_gain_step))
        global->GFL.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_SIDESCAN:
      cur_gain_step = global->GSS.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, global->GSS.sonar.cur_gain0, cur_gain_step))
        global->GSS.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_PROFILER:
      cur_gain_step = global->GPF.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, global->GPF.sonar.cur_gain0, cur_gain_step))
        global->GPF.sonar.cur_gain_step = cur_gain_step;
    }
}

static void
signal_up (GtkWidget *widget,
           NewGlobal    *global)
{
  guint cur_signal;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_signal = global->GFL.sonar.cur_signal + 1;
      if (signal_set (global, cur_signal))
        global->GFL.sonar.cur_signal = cur_signal;
      break;

    case W_SIDESCAN:
      cur_signal = global->GSS.sonar.cur_signal + 1;
      if (signal_set (global, cur_signal))
        global->GSS.sonar.cur_signal = cur_signal;
      break;

    case W_PROFILER:
      cur_signal = global->GPF.sonar.cur_signal + 1;
      if (signal_set (global, cur_signal))
        global->GPF.sonar.cur_signal = cur_signal;
      break;
    }
}

static void
signal_down (GtkWidget *widget,
             NewGlobal    *global)
{
  guint cur_signal;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_signal = global->GFL.sonar.cur_signal - 1;
      if (signal_set (global, cur_signal))
        global->GFL.sonar.cur_signal = cur_signal;
      break;

    case W_SIDESCAN:
      cur_signal = global->GSS.sonar.cur_signal - 1;
      if (signal_set (global, cur_signal))
        global->GSS.sonar.cur_signal = cur_signal;
      break;

    case W_PROFILER:
      cur_signal = global->GPF.sonar.cur_signal - 1;
      if (signal_set (global, cur_signal))
        global->GPF.sonar.cur_signal = cur_signal;
      break;
    }
}

void
widget_swap (GtkToggleButton *togglebutton,
             gpointer         user_data)
{
  g_message ("IMPLEMENT!");
  gint desired_widget = GPOINTER_TO_INT (user_data);

  gtk_stack_set_visible_child (GTK_STACK (global.gui.stack), global.gui.ctrl_widgets[desired_widget]);
}

void
one_window (GtkToggleButton *togglebutton,
            gpointer         user_data)
{
  g_message ("IMPLEMENT!");
  //gboolean active = gtk_toggle_button_get_active (togglebutton);
  //gtk_widget_set_visible (vpan, !active);
}

/* Функция включает/выключает излучение. */
static gboolean
start_stop (GtkWidget *widget,
            gboolean   state,
            NewGlobal    *global)
{
  return TRUE;
}
//   gint selector = global->sonar_selector;
//   /* Включаем излучение. */
//   if (state)
//     {
//       static gint n_tracks = -1;
//       gboolean status;
//
//       /* Закрываем текущий открытый галс. */
//       g_clear_pointer(&global->track_name, g_free);
//
//       /* Задаем параметры гидролокаторов. */
//
//       if (!signal_set   (global, global->sonar.cur_signal) ||
//           !tvg_set      (global, global->sonar.cur_gain0, global->sonar.cur_gain_step) ||
//           !distance_set (global, global->sonar.cur_distance))
//         {
//           gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
//           return TRUE;
//         }
//
//       /* Число галсов в проекте. */
//       if (n_tracks < 0)
//         {
//           gint32 project_id;
//           gchar **tracks;
//
//           project_id = hyscan_db_project_open (global->db, global->project_name);
//           tracks = hyscan_db_track_list (global->db, project_id);
//           n_tracks = (tracks == NULL) ? 0 : g_strv_length (tracks);
//
//           hyscan_db_close (global->db, project_id);
//           g_free (tracks);
//         }
//
//       /* Включаем запись нового галса. */
//       global->track_name = g_strdup_printf ("%s%d", global->track_prefix, ++n_tracks);
//       status = hyscan_sonar_control_start (global->sonar.sonar, global->track_name, HYSCAN_TRACK_SURVEY);
//
//       /* Если локатор включён, переходим в режим онлайн. */
//       if (status)
//         {
//           gtk_switch_set_state (GTK_SWITCH (widget), TRUE);
//           gtk_widget_set_sensitive (GTK_WIDGET (global->track_view), FALSE);
//         }
//       else
//         {
//           gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
//         }
//     }
//
//   /* Выключаем излучение и блокируем режим онлайн. */
//   else
//     {
//       if (gtk_switch_get_state (GTK_SWITCH (widget)))
//         {
//           hyscan_sonar_control_stop (global->sonar.sonar);
//           hyscan_forward_look_player_pause (global->fl_player);
//
//           gtk_switch_set_state (GTK_SWITCH (widget), FALSE);
//           gtk_widget_set_sensitive (GTK_WIDGET (global->track_view), TRUE);
//         }
//     }
//
//   return TRUE;
// }


int
main (int argc, char **argv)
{
  gint               cache_size = 256;         /* Размер кэша по умолчанию. */

  gchar             *driver_path = NULL;       /* Путь к драйверам гидролокатора. */
  gchar             *driver_name = NULL;       /* Название драйвера гидролокатора. */

  gchar             *prof_uri = NULL;         /* Адрес гидролокатора. */
  gchar             *flss_uri = NULL;         /* Адрес гидролокатора. */

  gchar             *db_uri = NULL;            /* Адрес базы данных. */
  gchar             *project_name = NULL;      /* Название проекта. */
  gchar             *track_prefix = NULL;      /* Префикс названия галсов. */
  gdouble            sound_velocity = 1500.0;  /* Скорость звука по умолчанию. */
  gdouble            ship_speed = 1.8;         /* Скорость движения судна. */
  gboolean           full_screen = FALSE;      /* Признак полноэкранного режима. */
  gchar             *config_file = NULL;       /* Название файла конфигурации. */

  HyScanSonarDriver *prof_driver = NULL;            /* Драйвер гидролокатора. */
  HyScanSonarDriver *flss_driver = NULL;            /* Драйвер гидролокатора. */
  HyScanParam       *sonar = NULL;             /* Интерфейс управления локатором. */

  GtkBuilder        *fl_builder = NULL;
  GtkBuilder        *pf_builder = NULL;
  GtkBuilder        *ss_builder = NULL;
  GtkBuilder        *common_builder = NULL;

  GtkWidget         *header = NULL;
  //GtkWidget         *control = NULL;
  //GtkWidget         *view_control = NULL;
  //GtkWidget         *sonar_control = NULL;
  //GtkWidget         *track_control = NULL;
  //GtkWidget         *play_control = NULL;

  GtkWidget         *fl_control_box  = NULL;
  GtkWidget         *fl_play_control = NULL;
  GtkWidget         *fl_view_control = NULL;
  GtkWidget         *fl_sonar_control = NULL;

  GtkWidget         *ss_control_box  = NULL;
  GtkWidget         *ss_view_control = NULL;
  GtkWidget         *ss_sonar_control = NULL;

  GtkWidget         *pf_control_box  = NULL;
  GtkWidget         *pf_view_control = NULL;
  GtkWidget         *pf_sonar_control = NULL;

  GtkWidget *wdgt;

  gtk_init (&argc, &argv);

  /* Разбор командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionContext *context;

    GOptionEntry common_entries[] =
      {
        { "cache-size",      'c',   0, G_OPTION_ARG_INT,     &cache_size,      "Cache size, Mb", NULL },

        { "driver-path",     'a',   0, G_OPTION_ARG_STRING,  &driver_path,     "Path to sonar drivers", NULL },
        { "driver-name",     'n',   0, G_OPTION_ARG_STRING,  &driver_name,     "Sonar driver name", NULL },
        { "prof-uri",    '1',   0, G_OPTION_ARG_STRING,  &prof_uri,       "Profiler uri", NULL },
        { "flss-uri",    '2',   0, G_OPTION_ARG_STRING,  &flss_uri,     "ForwardLook+SideScan uri", NULL },

        { "db-uri",          'd',   0, G_OPTION_ARG_STRING,  &db_uri,          "HyScan DB uri", NULL },
        { "project-name",    'p',   0, G_OPTION_ARG_STRING,  &project_name,    "Project name", NULL },
        { "track-prefix",    't',   0, G_OPTION_ARG_STRING,  &track_prefix,    "Track name prefix", NULL },

        { "sound-velocity",  'v',   0, G_OPTION_ARG_DOUBLE,  &sound_velocity,  "Sound velocity, m/s", NULL },
        { "ship-speed",      'e',   0, G_OPTION_ARG_DOUBLE,  &ship_speed,      "Ship speed, m/s", NULL },
        { "full-screen",     'f',   0, G_OPTION_ARG_NONE,    &full_screen,     "Full screen mode", NULL },
        { NULL }
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

    if ((db_uri == NULL) || (project_name == NULL))
      {
        g_print ("%s", g_option_context_get_help (context, FALSE, NULL));
        return 0;
      }

    //if (args[1] != NULL)
      //config_file = g_strdup (args[1]); //// TODO: what?!

    g_option_context_free (context);
    g_strfreev (args);
  }

  /* Путь к драйверам по умолчанию. */
  if ((driver_path == NULL) && (driver_name != NULL))
    driver_path = g_strdup (SONAR_DRIVERS_PATH);

  /* Префикс имени галса по умолчанию. */
  if (track_prefix == NULL)
    track_prefix = g_strdup ("");

  /* Конфигурация. */
  global.sound_velocity = sound_velocity;
  global.full_screen = full_screen;
  global.project_name = project_name;
  global.track_prefix = track_prefix;

  /* Кэш. */
  cache_size = (cache_size <= 64) ? 256 : cache_size;
  global.cache = HYSCAN_CACHE (hyscan_cached_new (cache_size));

  /* Подключение к базе данных. */
  global.db = hyscan_db_new (db_uri);
  hyscan_exit_if_w_param (global.db == NULL, "can't connect to db '%s'", db_uri);

  /* Монитор базы данных. */
  global.db_info = hyscan_db_info_new (global.db);
  g_signal_connect (G_OBJECT (global.db_info), "projects-changed", G_CALLBACK (projects_changed), &global);
  g_signal_connect (G_OBJECT (global.db_info), "tracks-changed", G_CALLBACK (tracks_changed), &global);

  /* Подключение к гидролокатору. */
  if (flss_uri != NULL)
    {
      HyScanGeneratorModeType gen_cap;
      HyScanTVGModeType tvg_cap;
      HyScanSonarClient *client;
      GKeyFile *config;
      gboolean status;
      guint i;

      /* Подключение к гидролокатору с помощью HyScanSonarClient */
      if (driver_name == NULL)
        {
          // client = hyscan_sonar_client_new (flss_uri);
          // if (client == NULL)
          //   {
          //     g_message ("can't connect to sonar '%s'", flss_uri);
          //     goto exit;
          //   }
          //
          // if (!hyscan_sonar_client_set_master (client))
          //   {
          //     g_message ("can't set master mode on sonar '%s'", flss_uri);
          //     g_object_unref (client);
          //     goto exit;
          //   }
          //
          // sonar = HYSCAN_PARAM (client);
        }

      /* Подключение с помощью драйвера гидролокатора. */
      else
        {
          flss_driver = hyscan_sonar_driver_new (driver_path, driver_name);
          hyscan_exit_if_w_param (flss_driver == NULL, "can't load sonar driver '%s'", driver_name);

          sonar = hyscan_sonar_discover_connect (HYSCAN_SONAR_DISCOVER (flss_driver), flss_uri, NULL);
          hyscan_exit_if_w_param (sonar == NULL, "can't connect to sonar '%s'", flss_uri);
        }

      /* Управление локатором. */
      global.GFL.sonar.sonar = hyscan_sonar_control_new (sonar, 4, 4, global.db);
      hyscan_exit_if_w_param (global.GFL.sonar.sonar == NULL, "unsupported sonar '%s'", flss_uri);
      global.GSS.sonar.sonar = g_object_ref (global.GFL.sonar.sonar);

      global.GFL.sonar.gen = HYSCAN_GENERATOR_CONTROL (global.GFL.sonar.sonar);
      global.GFL.sonar.tvg = HYSCAN_TVG_CONTROL (global.GFL.sonar.sonar);
      global.GSS.sonar.gen = HYSCAN_GENERATOR_CONTROL (global.GSS.sonar.sonar);
      global.GSS.sonar.tvg = HYSCAN_TVG_CONTROL (global.GSS.sonar.sonar);

      /* Параметры генераторов. */
      gen_cap = hyscan_generator_control_get_capabilities (global.GFL.sonar.gen, FORWARDLOOK);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "forward-look: unsupported generator mode");
      gen_cap = hyscan_generator_control_get_capabilities (global.GSS.sonar.gen, STARBOARD);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "starboard: unsupported generator mode");
      gen_cap = hyscan_generator_control_get_capabilities (global.GSS.sonar.gen, PORTSIDE);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "portside: unsupported generator mode");

      status = hyscan_generator_control_set_enable (global.GFL.sonar.gen, FORWARDLOOK, TRUE);
      hyscan_exit_if (!status, "forward-look: can't enable generator");
      status = hyscan_generator_control_set_enable (global.GSS.sonar.gen, STARBOARD, TRUE);
      hyscan_exit_if (!status, "starboard: can't enable generator");
      status = hyscan_generator_control_set_enable (global.GSS.sonar.gen, PORTSIDE, TRUE);
      hyscan_exit_if (!status, "portside: can't enable generator");

      /* Параметры ВАРУ. */
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GFL.sonar.tvg, FORWARDLOOK);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "forward-look: unsupported tvg mode");
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GSS.sonar.tvg, STARBOARD);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "starboard: unsupported tvg mode");
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GSS.sonar.tvg, PORTSIDE);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "portside: unsupported tvg mode");

      status = hyscan_tvg_control_set_enable (global.GFL.sonar.tvg, FORWARDLOOK, TRUE);
      hyscan_exit_if (!status, "forward-look: can't enable tvg");
      status = hyscan_tvg_control_set_enable (global.GSS.sonar.tvg, STARBOARD, TRUE);
      hyscan_exit_if (!status, "starboard: can't enable tvg");
      status = hyscan_tvg_control_set_enable (global.GSS.sonar.tvg, PORTSIDE, TRUE);
      hyscan_exit_if (!status, "portside: can't enable tvg");

      /* Сигналы зондирования. */
      global.GFL.fl_signals = hyscan_generator_control_list_presets (global.GFL.sonar.gen, FORWARDLOOK);
      hyscan_exit_if (global.GFL.fl_signals == NULL, "forward-look: can't load signal presets");
      global.GSS.sb_signals = hyscan_generator_control_list_presets (global.GSS.sonar.gen, STARBOARD);
      hyscan_exit_if (global.GSS.sb_signals == NULL, "starboard: can't load signal presets");
      global.GSS.ps_signals = hyscan_generator_control_list_presets (global.GSS.sonar.gen, PORTSIDE);
      hyscan_exit_if (global.GSS.ps_signals == NULL, "portside: can't load signal presets");

      for (i = 0; global.GFL.fl_signals[i] != NULL; i++);
      global.GFL.fl_n_signals = i;

      for (i = 0; global.GSS.sb_signals[i] != NULL; i++);
      global.GSS.sb_n_signals = i;

      for (i = 0; global.GSS.ps_signals[i] != NULL; i++);
      global.GSS.ps_n_signals = i;

      /* Настройка датчиков и антенн. */
      // if (config_file != NULL)
      //   {
      //     config = g_key_file_new ();
      //     g_key_file_load_from_file (config, config_file, G_KEY_FILE_NONE, NULL);

      //     if (!setup_sensors (HYSCAN_SENSOR_CONTROL (global.GFL.sonar.sonar), config) ||
      //         !setup_sonar_antenna (global.GFL.sonar.sonar, FORWARDLOOK, config))
      //       {
      //         status = FALSE;
      //       }
      //     else
      //       {
      //         status = TRUE;
      //       }

      //     g_key_file_unref (config);

      //     if (!status)
      //       goto exit;
      //   }

      /* Рабочий проект. */
      status = hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GFL.sonar.sonar), project_name);
      hyscan_exit_if (!status, "forward-look: can't set working project");
      status = hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GSS.sonar.sonar), project_name);
      hyscan_exit_if (!status, "side-scan: can't set working project");
    }
  // TODO: same for Profiler.
  /* Закончили подключение к гидролокатору. */

  /* Билдеры. */
  common_builder = gtk_builder_new_from_resource ("/org/hyscan/gtk/common.ui");
  fl_builder = gtk_builder_new_from_resource ("/org/hyscan/gtk/triple.ui");
  pf_builder = gtk_builder_new_from_resource ("/org/hyscan/gtk/triple.ui");
  ss_builder = gtk_builder_new_from_resource ("/org/hyscan/gtk/triple.ui");
  /* Настраиваем основные элементы. */
  /* Основное окно программы. */
  global.gui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (global.gui.window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (global.gui.window), "key-press-event", G_CALLBACK (key_press), &global);
  gtk_window_set_title (GTK_WINDOW (global.gui.window), "ГБОГКПФ");
  gtk_window_set_default_size (GTK_WINDOW (global.gui.window), 1024, 768);

  /* Основная раскладка окна. */
  global.gui.gtk_area = hyscan_gtk_area_new ();

  /* Слева у нас список галсов. */
  global.gui.track_control = GTK_WIDGET (gtk_builder_get_object (common_builder, "track_control"));
  hyscan_exit_if (global.gui.track_control == NULL, "can't load track control ui");

  global.gui.track_view = GTK_TREE_VIEW (gtk_builder_get_object (common_builder, "track_view"));
  global.gui.track_list = GTK_TREE_MODEL (gtk_builder_get_object (common_builder, "track_list"));
  global.gui.track_range = GTK_ADJUSTMENT (gtk_builder_get_object (common_builder, "track_range"));
  hyscan_exit_if ((global.gui.track_view == NULL) ||
                  (global.gui.track_list == NULL) ||
                  (global.gui.track_range == NULL),
                  "incorrect track control ui");
  /* Сортировка списка галсов. */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (global.gui.track_list), 0, GTK_SORT_DESCENDING);
  /* Список галсов кладем слева. */
  hyscan_gtk_area_set_left (HYSCAN_GTK_AREA (global.gui.gtk_area), global.gui.track_control);

  /* Справа у нас виджет управления просмотром и локаторами.
   * А в нём:
   * - виджет управления отображением
   * - stack с тремя виджетами
   * - управление записью */

  global.gui.right_box      = GTK_WIDGET (gtk_box_new (GTK_ORIENTATION_VERTICAL, 2));
  global.gui.selector       = GTK_WIDGET (gtk_builder_get_object (common_builder, "sonar_selector"));
  global.gui.stack          = GTK_WIDGET (gtk_stack_new ());
  global.gui.record_control = GTK_WIDGET (gtk_builder_get_object (common_builder, "record_control"));

  /* Запихиваем всё в бокс: */
  gtk_box_pack_start (GTK_BOX (global.gui.right_box), global.gui.selector, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (global.gui.right_box), global.gui.stack, TRUE, TRUE, 2);
  gtk_box_pack_end (GTK_BOX (global.gui.right_box), global.gui.record_control, FALSE, FALSE, 2);

  /* Бокс запихиваем в правую панель арии: */
  hyscan_gtk_area_set_right (HYSCAN_GTK_AREA (global.gui.gtk_area), global.gui.right_box);

  /* Центральная зона: два box'a (наполним позже). */
  global.gui.v_pane = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  global.gui.h_pane = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_end (GTK_BOX (global.gui.h_pane), global.gui.v_pane, TRUE, TRUE, 1);
  /* If you prefer paned...
   * global.gui.v_pane = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
   * global.gui.h_pane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
   * gtk_paned_add2 (GTK_PANED (global.gui.h_pane), global.gui.v_pane, TRUE, TRUE, 1);
   */

  hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (global.gui.gtk_area), global.gui.h_pane);

  /* Ништяк: скелет гуя сделан. Теперь подцепим тут нужные сигналы. */
  gtk_builder_add_callback_symbol (common_builder, "track_scroll", G_CALLBACK (track_scroll));
  gtk_builder_add_callback_symbol (common_builder, "track_changed", G_CALLBACK (track_changed));
  gtk_builder_add_callback_symbol (common_builder, "position_changed", G_CALLBACK (position_changed));
  gtk_builder_add_callback_symbol (common_builder, "start_stop", G_CALLBACK (start_stop));

  /* Сигналы sonar_selector'a. */
  wdgt = GTK_WIDGET (gtk_builder_get_object (common_builder, "ss_selector"));
  g_signal_connect (wdgt, "toggled", G_CALLBACK (widget_swap), GINT_TO_POINTER (W_SIDESCAN));
  wdgt = GTK_WIDGET (gtk_builder_get_object (common_builder, "pf_selector"));
  g_signal_connect (wdgt, "toggled", G_CALLBACK (widget_swap), GINT_TO_POINTER (W_PROFILER));
  wdgt = GTK_WIDGET (gtk_builder_get_object (common_builder, "fl_selector"));
  g_signal_connect (wdgt, "toggled", G_CALLBACK (widget_swap), GINT_TO_POINTER (W_FORWARDL));
  wdgt = GTK_WIDGET (gtk_builder_get_object (common_builder, "single_window"));
  g_signal_connect (wdgt, "toggled", G_CALLBACK (one_window), NULL);

  /* Управление гидролокаторами и картинками. */
  fl_control_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  global.gui.ctrl_widgets[W_FORWARDL] = fl_control_box;
  fl_view_control = GTK_WIDGET (gtk_builder_get_object (fl_builder, "fl_view_control")); hyscan_exit_if (fl_view_control == NULL, "fl_view_control load failure");
  /////if (global.GFL.sonar.sonar != NULL) {
      fl_sonar_control = GTK_WIDGET (gtk_builder_get_object (fl_builder, "sonar_control"));
      hyscan_exit_if (fl_sonar_control == NULL, "fl_sonar_control load failure");
    /////}
  gtk_box_pack_start (GTK_BOX (fl_control_box), fl_view_control, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (fl_control_box), fl_sonar_control, FALSE, FALSE, 0);

  pf_control_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  global.gui.ctrl_widgets[W_PROFILER] = pf_control_box;
  pf_view_control = GTK_WIDGET (gtk_builder_get_object (pf_builder, "sonar_view_control")); hyscan_exit_if (pf_view_control == NULL, "pf_view_control load failure");
  /////if (global.GPF.sonar.sonar != NULL) {
      pf_sonar_control = GTK_WIDGET (gtk_builder_get_object (pf_builder, "sonar_control"));
      hyscan_exit_if (pf_sonar_control == NULL, "pf_sonar_control load failure");
    /////}
  gtk_box_pack_start (GTK_BOX (pf_control_box), pf_view_control, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (pf_control_box), pf_sonar_control, FALSE, FALSE, 0);

  ss_control_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  global.gui.ctrl_widgets[W_SIDESCAN] = ss_control_box;
  ss_view_control = GTK_WIDGET (gtk_builder_get_object (ss_builder, "sonar_view_control")); hyscan_exit_if (ss_view_control == NULL, "ss_view_control load failure");
  /////if (global.GSS.sonar.sonar != NULL) {
      ss_sonar_control = GTK_WIDGET (gtk_builder_get_object (ss_builder, "sonar_control"));
      hyscan_exit_if (ss_sonar_control == NULL, "ss_sonar_control load failure");
    /////}
  gtk_box_pack_start (GTK_BOX (ss_control_box), ss_view_control, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (ss_control_box), ss_sonar_control, FALSE, FALSE, 0);

  /* Теперь получаем лейблы для *_view_control и *_sonar_control. */
  global.GFL.gui.brightness_value = GTK_LABEL (gtk_builder_get_object (pf_builder, "fl_brightness_value")); hyscan_exit_if (global.GFL.gui.brightness_value == NULL, "global.GFL.gui.brightness_value failure");
  global.GFL.gui.scale_value      = GTK_LABEL (gtk_builder_get_object (pf_builder, "fl_scale_value")); hyscan_exit_if (global.GFL.gui.scale_value == NULL, "global.GFL.gui.scale_value failure");
  global.GFL.gui.third_value      = GTK_LABEL (gtk_builder_get_object (pf_builder, "fl_sensitivity_value")); hyscan_exit_if (global.GFL.gui.third_value == NULL, "global.GFL.gui.third_value failure");
  global.GFL.gui.distance_value   = GTK_LABEL (gtk_builder_get_object (pf_builder, "distance_value")); hyscan_exit_if (global.GFL.gui.distance_value == NULL, "global.GFL.gui.distance_value failure");
  global.GFL.gui.tvg0_value       = GTK_LABEL (gtk_builder_get_object (pf_builder, "tvg0_value")); hyscan_exit_if (global.GFL.gui.tvg0_value == NULL, "global.GFL.gui.tvg0_value failure");
  global.GFL.gui.tvg_value        = GTK_LABEL (gtk_builder_get_object (pf_builder, "tvg_value")); hyscan_exit_if (global.GFL.gui.tvg_value == NULL, "global.GFL.gui.tvg_value failure");
  global.GFL.gui.signal_value     = GTK_LABEL (gtk_builder_get_object (pf_builder, "signal_value")); hyscan_exit_if (global.GFL.gui.signal_value == NULL, "global.GFL.gui.signal_value failure");

  global.GSS.gui.brightness_value = GTK_LABEL (gtk_builder_get_object (ss_builder, "ss_brightness_value")); hyscan_exit_if (global.GSS.gui.brightness_value == NULL, "global.GSS.gui.brightness_value failure");
  global.GSS.gui.scale_value      = GTK_LABEL (gtk_builder_get_object (ss_builder, "ss_scale_value")); hyscan_exit_if (global.GSS.gui.scale_value == NULL, "global.GSS.gui.scale_value failure");
  global.GSS.gui.third_value      = GTK_LABEL (gtk_builder_get_object (ss_builder, "ss_color_map_value")); hyscan_exit_if (global.GSS.gui.third_value == NULL, "global.GSS.gui.third_value failure");
  global.GSS.gui.distance_value   = GTK_LABEL (gtk_builder_get_object (ss_builder, "distance_value")); hyscan_exit_if (global.GSS.gui.distance_value == NULL, "global.GSS.gui.distance_value failure");
  global.GSS.gui.tvg0_value       = GTK_LABEL (gtk_builder_get_object (ss_builder, "tvg0_value")); hyscan_exit_if (global.GSS.gui.tvg0_value == NULL, "global.GSS.gui.tvg0_value failure");
  global.GSS.gui.tvg_value        = GTK_LABEL (gtk_builder_get_object (ss_builder, "tvg_value")); hyscan_exit_if (global.GSS.gui.tvg_value == NULL, "global.GSS.gui.tvg_value failure");
  global.GSS.gui.signal_value     = GTK_LABEL (gtk_builder_get_object (ss_builder, "signal_value")); hyscan_exit_if (global.GSS.gui.signal_value == NULL, "global.GSS.gui.signal_value failure");

  global.GPF.gui.brightness_value = GTK_LABEL (gtk_builder_get_object (pf_builder, "ss_brightness_value")); hyscan_exit_if (global.GPF.gui.brightness_value == NULL, "global.GPF.gui.brightness_value failure");
  global.GPF.gui.scale_value      = GTK_LABEL (gtk_builder_get_object (pf_builder, "ss_scale_value")); hyscan_exit_if (global.GPF.gui.scale_value == NULL, "global.GPF.gui.scale_value failure");
  global.GPF.gui.third_value      = GTK_LABEL (gtk_builder_get_object (pf_builder, "ss_color_map_value")); hyscan_exit_if (global.GPF.gui.third_value == NULL, "global.GPF.gui.third_value failure");
  global.GPF.gui.distance_value   = GTK_LABEL (gtk_builder_get_object (pf_builder, "distance_value")); hyscan_exit_if (global.GPF.gui.distance_value == NULL, "global.GPF.gui.distance_value failure");
  global.GPF.gui.tvg0_value       = GTK_LABEL (gtk_builder_get_object (pf_builder, "tvg0_value")); hyscan_exit_if (global.GPF.gui.tvg0_value == NULL, "global.GPF.gui.tvg0_value failure");
  global.GPF.gui.tvg_value        = GTK_LABEL (gtk_builder_get_object (pf_builder, "tvg_value")); hyscan_exit_if (global.GPF.gui.tvg_value == NULL, "global.GPF.gui.tvg_value failure");
  global.GPF.gui.signal_value     = GTK_LABEL (gtk_builder_get_object (pf_builder, "signal_value")); hyscan_exit_if (global.GPF.gui.signal_value == NULL, "global.GPF.gui.signal_value failure");

  /* Теперь немного гемора: цепляемся к сигналам кнопок. Их много, а потому увы. */
  gtk_builder_add_callback_symbol (fl_builder, "brightness_up", G_CALLBACK (brightness_up));
  gtk_builder_add_callback_symbol (fl_builder, "brightness_down", G_CALLBACK (brightness_down));
  gtk_builder_add_callback_symbol (fl_builder, "scale_up", G_CALLBACK (scale_up));
  gtk_builder_add_callback_symbol (fl_builder, "scale_down", G_CALLBACK (scale_down));
  gtk_builder_add_callback_symbol (fl_builder, "sensitivity_up", G_CALLBACK (sensitivity_up));
  gtk_builder_add_callback_symbol (fl_builder, "sensitivity_down", G_CALLBACK (sensitivity_down));
  gtk_builder_add_callback_symbol (fl_builder, "mode_changed", G_CALLBACK (mode_changed));

  gtk_builder_add_callback_symbol (fl_builder, "distance_up", G_CALLBACK (distance_up));
  gtk_builder_add_callback_symbol (fl_builder, "distance_down", G_CALLBACK (distance_down));
  gtk_builder_add_callback_symbol (fl_builder, "tvg0_up", G_CALLBACK (tvg0_up));
  gtk_builder_add_callback_symbol (fl_builder, "tvg0_down", G_CALLBACK (tvg0_down));
  gtk_builder_add_callback_symbol (fl_builder, "tvg_up", G_CALLBACK (tvg_up));
  gtk_builder_add_callback_symbol (fl_builder, "tvg_down", G_CALLBACK (tvg_down));
  gtk_builder_add_callback_symbol (fl_builder, "signal_up", G_CALLBACK (signal_up));
  gtk_builder_add_callback_symbol (fl_builder, "signal_down", G_CALLBACK (signal_down));

  gtk_builder_add_callback_symbol (fl_builder, "color_map_up", G_CALLBACK (void_callback));
  gtk_builder_add_callback_symbol (fl_builder, "color_map_down", G_CALLBACK (void_callback));
  gtk_builder_add_callback_symbol (fl_builder, "live_view", G_CALLBACK (void_callback));

  gtk_builder_connect_signals (fl_builder, &global);

  gtk_builder_add_callback_symbol (ss_builder, "brightness_up", G_CALLBACK (brightness_up));
  gtk_builder_add_callback_symbol (ss_builder, "brightness_down", G_CALLBACK (brightness_down));
  gtk_builder_add_callback_symbol (ss_builder, "scale_up", G_CALLBACK (scale_up));
  gtk_builder_add_callback_symbol (ss_builder, "scale_down", G_CALLBACK (scale_down));
  gtk_builder_add_callback_symbol (ss_builder, "color_map_up", G_CALLBACK (color_map_up));
  gtk_builder_add_callback_symbol (ss_builder, "color_map_down", G_CALLBACK (color_map_down));
  gtk_builder_add_callback_symbol (ss_builder, "live_view", G_CALLBACK (live_view));

  gtk_builder_add_callback_symbol (ss_builder, "distance_up", G_CALLBACK (distance_up));
  gtk_builder_add_callback_symbol (ss_builder, "distance_down", G_CALLBACK (distance_down));
  gtk_builder_add_callback_symbol (ss_builder, "tvg0_up", G_CALLBACK (tvg0_up));
  gtk_builder_add_callback_symbol (ss_builder, "tvg0_down", G_CALLBACK (tvg0_down));
  gtk_builder_add_callback_symbol (ss_builder, "tvg_up", G_CALLBACK (tvg_up));
  gtk_builder_add_callback_symbol (ss_builder, "tvg_down", G_CALLBACK (tvg_down));
  gtk_builder_add_callback_symbol (ss_builder, "signal_up", G_CALLBACK (signal_up));
  gtk_builder_add_callback_symbol (ss_builder, "signal_down", G_CALLBACK (signal_down));

  gtk_builder_add_callback_symbol (ss_builder, "sensitivity_up", G_CALLBACK (void_callback));
  gtk_builder_add_callback_symbol (ss_builder, "sensitivity_down", G_CALLBACK (void_callback));
  gtk_builder_add_callback_symbol (ss_builder, "mode_changed", G_CALLBACK (void_callback));
  gtk_builder_connect_signals (ss_builder, &global);

  gtk_builder_add_callback_symbol (pf_builder, "brightness_up", G_CALLBACK (brightness_up));
  gtk_builder_add_callback_symbol (pf_builder, "brightness_down", G_CALLBACK (brightness_down));
  gtk_builder_add_callback_symbol (pf_builder, "scale_up", G_CALLBACK (scale_up));
  gtk_builder_add_callback_symbol (pf_builder, "scale_down", G_CALLBACK (scale_down));
  gtk_builder_add_callback_symbol (pf_builder, "color_map_up", G_CALLBACK (color_map_up));
  gtk_builder_add_callback_symbol (pf_builder, "color_map_down", G_CALLBACK (color_map_down));
  gtk_builder_add_callback_symbol (pf_builder, "live_view", G_CALLBACK (live_view));

  gtk_builder_add_callback_symbol (pf_builder, "distance_up", G_CALLBACK (distance_up));
  gtk_builder_add_callback_symbol (pf_builder, "distance_down", G_CALLBACK (distance_down));
  gtk_builder_add_callback_symbol (pf_builder, "tvg0_up", G_CALLBACK (tvg0_up));
  gtk_builder_add_callback_symbol (pf_builder, "tvg0_down", G_CALLBACK (tvg0_down));
  gtk_builder_add_callback_symbol (pf_builder, "tvg_up", G_CALLBACK (tvg_up));
  gtk_builder_add_callback_symbol (pf_builder, "tvg_down", G_CALLBACK (tvg_down));
  gtk_builder_add_callback_symbol (pf_builder, "signal_up", G_CALLBACK (signal_up));
  gtk_builder_add_callback_symbol (pf_builder, "signal_down", G_CALLBACK (signal_down));

  gtk_builder_add_callback_symbol (pf_builder, "sensitivity_up", G_CALLBACK (void_callback));
  gtk_builder_add_callback_symbol (pf_builder, "sensitivity_down", G_CALLBACK (void_callback));
  gtk_builder_add_callback_symbol (pf_builder, "mode_changed", G_CALLBACK (void_callback));
  gtk_builder_connect_signals (pf_builder, &global);

  /* Управление воспроизведением. */
  // play_control = GTK_WIDGET (gtk_builder_get_object (builder, "play_control"));
  // if (play_control == NULL)
  //   {
  //     g_message ("can't load play control ui");
  //     goto exit;
  //   }
  //
  // global.position = GTK_SCALE (gtk_builder_get_object (builder, "position"));
  // if (global.position == NULL)
  //   {
  //     g_message ("incorrect play control ui");
  //     goto exit;
  //   }
  /* Кладем гидролокаторские боксы в гткстэк. */
  gtk_stack_add_titled (GTK_STACK (global.gui.stack), ss_control_box, "ss_control_box", "ss");
  gtk_stack_add_titled (GTK_STACK (global.gui.stack), pf_control_box, "pf_control_box", "pf");
  gtk_stack_add_titled (GTK_STACK (global.gui.stack), fl_control_box, "fl_control_box", "fl");

  //GtkWidget *ssw = gtk_stack_switcher_new (); //todo: remove when debugged
  //gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (ssw), GTK_STACK (global.gui.stack)); //todo: remove when debugged
  //gtk_box_pack_end (GTK_BOX (global.gui.right_box), ssw, FALSE, FALSE, 2); //todo: remove when debugged

  /* Создаем виджеты просмотра. */
  // /* Секторный обзор. */
  global.GSS.wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_grid_new ());
  global.GPF.wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_grid_new ());
  global.GFL.fl = HYSCAN_GTK_FORWARD_LOOK (hyscan_gtk_forward_look_new ());

  gtk_box_pack_start (GTK_BOX (global.gui.v_pane), GTK_WIDGET (global.GFL.fl), TRUE, TRUE, 1);
  gtk_box_pack_end (GTK_BOX (global.gui.v_pane), GTK_WIDGET (global.GPF.wf), TRUE, TRUE, 1);
  gtk_box_pack_start (GTK_BOX (global.gui.h_pane), GTK_WIDGET (global.GSS.wf), TRUE, TRUE, 1);
  /* if you prefer paned...
   * gtk_paned_add1 (GTK_PANED (global.gui.v_pane), global.GFL.fl);
   * gtk_paned_add2 (GTK_PANED (global.gui.v_pane), global.GPF.wf);
   * gtk_paned_add1 (GTK_PANED (global.gui.h_pane), global.GSS.wf);
   */

  // gtk_widget_set_hexpand (GTK_WIDGET (global.fl), TRUE);
  // gtk_widget_set_vexpand (GTK_WIDGET (global.fl), TRUE);
  // gtk_widget_set_margin_top (GTK_WIDGET (global.fl), 12);
  //
  // gtk_cifro_area_control_set_scroll_mode (GTK_CIFRO_AREA_CONTROL (global.fl), GTK_CIFRO_AREA_SCROLL_MODE_COMBINED);
  //
  // /* Управление отображением. */
  // global.fl_player = hyscan_gtk_forward_look_get_player (global.fl);
  // hyscan_forward_look_player_set_cache (global.fl_player, global.cache, NULL);
  // hyscan_forward_look_player_set_sv (global.fl_player, global.sound_velocity);
  //
  // /* Полоска прокрутки. */
  // global.position_range = hyscan_gtk_forward_look_get_adjustment (global.fl);
  // gtk_range_set_adjustment (GTK_RANGE (global.position), global.position_range);

  /* Заголовок окна. */
  //header = gtk_header_bar_new ();
  //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
  //gtk_header_bar_set_title (GTK_HEADER_BAR (header), "uHyScan");
  //gtk_window_set_titlebar (GTK_WINDOW (global.gui.window), header);

  // hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (gtk_area), GTK_WIDGET (global.fl));

  // hyscan_gtk_area_set_bottom (HYSCAN_GTK_AREA (gtk_area), play_control);
  gtk_container_add (GTK_CONTAINER (global.gui.window), global.gui.gtk_area);

  /* Делаем цветовые схемы. */
  guint32 kolors[2];
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0); /* Белый. */
  global.GSS.color_maps[0] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GSS.color_map_len[0]);
  global.GPF.color_maps[0] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GPF.color_map_len[0]);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 0.0, 1.0); /* Желтый. */
  global.GSS.color_maps[1] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GSS.color_map_len[1]);
  global.GPF.color_maps[1] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GPF.color_map_len[1]);
  kolors[1] = hyscan_tile_color_converter_d2i (0.0, 1.0, 1.0, 1.0); /* Зеленый. */
  global.GSS.color_maps[2] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GSS.color_map_len[2]);
  global.GPF.color_maps[2] = hyscan_tile_color_compose_colormap (kolors, 2, &global.GPF.color_map_len[2]);

  /* Начальные значения. */
  global.GSS.cur_brightness = 80.0;
  global.GSS.cur_color_map = 0;
  global.GSS.sonar.cur_signal = 1;
  global.GSS.sonar.cur_gain0 = 0.0;
  global.GSS.sonar.cur_gain_step = 10.0;
  global.GSS.sonar.cur_distance = SIDE_SCAN_MAX_DISTANCE;

  global.GPF.cur_brightness = 80.0;
  global.GPF.cur_color_map = 0;
  global.GPF.sonar.cur_signal = 1;
  global.GPF.sonar.cur_gain0 = 0.0;
  global.GPF.sonar.cur_gain_step = 10.0;
  global.GPF.sonar.cur_distance = PROFILER_MAX_DISTANCE;

  global.GFL.cur_brightness = 80.0;
  global.GFL.cur_sensitivity = 8.0;
  global.GFL.sonar.cur_signal = 1;
  global.GFL.sonar.cur_gain0 = 0.0;
  global.GFL.sonar.cur_gain_step = 10.0;
  global.GFL.sonar.cur_distance = PROFILER_MAX_DISTANCE;
//
  //brightness_set (&global, global.cur_brightness);
  //scale_set (&global, FALSE);
  //sensitivity_set (&global, global.cur_sensitivity);
  //if (sonar != NULL)
  //  {
  //    distance_set (&global, global.sonar.cur_distance);
  //    tvg_set (&global, global.sonar.cur_gain0, global.sonar.cur_gain_step);
  //    signal_set (&global, 1);
  //  }

  if (full_screen)
    gtk_window_fullscreen (GTK_WINDOW (global.gui.window));

  gtk_widget_show_all (global.gui.window);

  /* If you prefer paned...
   * gtk_paned_set_position (global.gui.v_pane, gtk_widget_get_allocated_height (global.gui.v_pane)/2);
   * gtk_paned_set_position (global.gui.h_pane, gtk_widget_get_allocated_width (global.gui.h_pane)/2);
   * g_object_set (global.gui.v_pane, "resize", TRUE);
   * g_object_set (global.gui.h_pane, "resize", TRUE);
   */

  gtk_main ();

  //if (sonar != NULL)
  //  hyscan_sonar_control_stop (global.sonar.sonar);

exit:
  /*
  g_clear_object (&builder);

  g_clear_object (&global.cache);
  g_clear_object (&global.db_info);
  g_clear_object (&global.db);

  g_clear_pointer (&global.sonar.signals, hyscan_data_schema_free_enum_values);
  g_clear_object (&global.sonar.sonar);
  g_clear_object (&sonar);
  g_clear_object (&driver);

  g_free (global.track_name);
  g_free (driver_path);
  g_free (driver_name);
  g_free (sonar_uri);
  g_free (db_uri);
  g_free (project_name);
  g_free (track_prefix);
  g_free (config_file);
*/

  return 0;
}
