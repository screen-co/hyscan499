#include <hyscan-gtk-area.h>
#include <hyscan-sonar-driver.h>
#include <hyscan-sonar-client.h>
#include <hyscan-sonar-control.h>
#include <hyscan-forward-look-data.h>
#include <hyscan-gtk-waterfall-grid.h>
#include <hyscan-tile-color.h>
#include <hyscan-db-info.h>
#include <hyscan-cached.h>
#include <urpc-server.h>
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

#define PF_TRACK_PREFIX                ".pf"

#define AME_KEY_RIGHT 39
#define AME_KEY_LEFT 37
#define AME_KEY_ENTER 10
#define AME_KEY_UP 38
#define AME_KEY_ESCAPE 27
#define AME_KEY_EXIT 1

#define URPC_KEYCODE_NO_ACTION -1


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
  HyScanSonarControl      *sonar_ctl;
  HyScanTVGControl        *tvg_ctl;
  HyScanGeneratorControl  *gen_ctl;

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

  //GtkBox                  *container_widgt;
  //GtkBox                  *view_control_widgt;
  //GtkBox                  *sonar_control_widgth;
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

  //struct
  //{
  uRpcServer *urpc_server;
  gint        urpc_keycode;
  //} urpc;

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


      GtkToggleButton                     *ss_selector;
      GtkToggleButton                     *pf_selector;
      GtkToggleButton                     *fl_selector;
      GtkToggleButton                     *single_window;

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

} Global;

Global global = { 0 };

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
      HyScanTrackInfo *track_info;

      //HyScanSourceInfo *forward_look_info;
      track_info = value;

      //forward_look_info = g_hash_table_lookup (track_info->sources, GINT_TO_POINTER (HYSCAN_SOURCE_FORWARD_LOOK));
      //if (!forward_look_info || !forward_look_info->raw)
      //  continue;
      if (g_str_has_prefix (track_info->name, PF_TRACK_PREFIX))
        continue;

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
  /* Открываем новый галс. */
  gtk_tree_model_get_value (global->gui.track_list, &iter, TRACK_COLUMN, &value);
  track_name = g_value_get_string (&value);

  hyscan_forward_look_player_open (global->GFL.fl_player, global->db, global->project_name, track_name, TRUE);
  hyscan_gtk_waterfall_open (global->GSS.wf, global->db, global->project_name, track_name, TRUE);

  pftrack = g_strdup_printf (PF_TRACK_PREFIX "%s", track_name);
  hyscan_gtk_waterfall_open (global->GPF.wf, global->db, global->project_name, pftrack, TRUE);
  g_free (pftrack);

  g_value_unset (&value);

  /* Режим отображения. */
  if (global->gui.start_stop_switch != NULL && gtk_switch_get_state (global->gui.start_stop_switch))
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
                  Global        *global)
{
}

/* Функция устанавливает яркость отображения. */
static gboolean
brightness_set (Global  *global,
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
scale_set (Global   *global,
           gboolean  scale_up,
           gpointer  user_data)
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
      if (user_data == NULL)
        hyscan_gtk_waterfall_zoom (global->GSS.wf, scale_up);

      i_scale = hyscan_gtk_waterfall_drawer_get_scale (HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf),
                                                       &scales, &n_scales);

      text = g_strdup_printf ("<small><b>1:%.0f</b></small>", scales[i_scale]);
      gtk_label_set_markup (global->GSS.gui.scale_value, text);
      break;

    case W_PROFILER:
      if (user_data == NULL)
        hyscan_gtk_waterfall_zoom (global->GPF.wf, scale_up);

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
color_map_set (Global *global,
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
      g_warning ("Wrong sonar selector!");
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
  hyscan_gtk_waterfall_drawer_set_substrate (wfd, BLACK_BG);

  text = g_strdup_printf ("<small><b>%s</b></small>", color_map_name);
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
   gtk_label_set_markup (global->GFL.gui.third_value, text);
   g_free (text);

   return TRUE;
 }


/* Функция устанавливает излучаемый сигнал. */
static gboolean
signal_set (Global *global,
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
      gen = global->GSS.sonar.gen_ctl;
      signals0 = global->GSS.sb_signals;
      signals1 = global->GSS.ps_signals;
      label = global->GSS.gui.signal_value;
      break;
    case W_PROFILER:
      if (cur_signal >= global->GPF.pf_n_signals)
        return FALSE;
      src0 = src1 = PROFILER;
      gen = global->GPF.sonar.gen_ctl;
      signals0 = signals1 = global->GPF.pf_signals;
      label = global->GPF.gui.signal_value;
      break;
    case W_FORWARDL:
      if (cur_signal >= global->GFL.fl_n_signals)
        return FALSE;
      src0 = src1 = FORWARDLOOK;
      gen = global->GFL.sonar.gen_ctl;
      signals0 = signals1 = global->GFL.fl_signals;
      label = global->GFL.gui.signal_value;
      break;
    default:
      g_warning ("signal_set: wrong sonar_ctl selector (%i@%i)!", global->sonar_selector, __LINE__);
      return FALSE;
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
tvg_set (Global  *global,
         gdouble *gain0,
         gdouble  step)
{
  gchar *gain_text, *step_text;
  gboolean status;
  GtkLabel *tvg0_value, *tvg_value;
  gdouble min_gain;
  gdouble max_gain;

  // TODO: SS has: hyscan_tvg_control_get_gain_range (global->sonar_ctl.tvg_ctl, STARBOARD,
  //                                   &min_gain, &max_gain); Maybe I should use it too?
  switch (global->sonar_selector)
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
        hyscan_return_val_if_fail (status, FALSE);

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
        g_warning ("tvg_set: wrong sonar_ctl selector (%i@%i)!", global->sonar_selector, __LINE__);
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

/* Функция устанавливает рабочую дистанцию. */
static gboolean
distance_set (Global  *global,
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
        status = hyscan_sonar_control_set_receive_time (global->GSS.sonar.sonar_ctl, STARBOARD, real_distance);
        hyscan_return_val_if_fail (status, FALSE);

        status = hyscan_sonar_control_set_receive_time (global->GSS.sonar.sonar_ctl, PORTSIDE, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_value = global->GSS.gui.distance_value;
        break;

      case W_PROFILER:
        hyscan_return_val_if_fail (cur_distance <= PROFILER_MAX_DISTANCE, FALSE);
        status = hyscan_sonar_control_set_receive_time (global->GPF.sonar.sonar_ctl, PROFILER, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_value = global->GPF.gui.distance_value;
        break;

      case W_FORWARDL:
        hyscan_return_val_if_fail (cur_distance <= FORWARD_LOOK_MAX_DISTANCE, FALSE);
        status = hyscan_sonar_control_set_receive_time (global->GFL.sonar.sonar_ctl, FORWARDLOOK, real_distance);
        hyscan_return_val_if_fail (status, FALSE);
        distance_value = global->GFL.gui.distance_value;
        break;
      default:
        g_warning ("distance_set: wrong sonar_ctl selector (%i@%i)!", global->sonar_selector, __LINE__);
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
               Global    *global)
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
        g_warning ("brightness_up: wrong sonar_ctl selector (%i@%i)!", global->sonar_selector, __LINE__);
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
                 Global    *global)
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
        g_warning ("brightness_down: wrong sonar_ctl selector (%i@%i)!", global->sonar_selector, __LINE__);
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
          Global    *global)
{
  scale_set (global, TRUE, NULL);
}

static void
scale_down (GtkWidget *widget,
            Global    *global)
{
  scale_set (global, FALSE, NULL);
}

static void
sensitivity_up (GtkWidget *widget,
                Global    *global)
{
  gdouble new_sensitivity;

  new_sensitivity = global->GFL.cur_sensitivity + 1;
  if (sensitivity_set (global, new_sensitivity))
    global->GFL.cur_sensitivity = new_sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));
}

static void
sensitivity_down (GtkWidget *widget,
                  Global    *global)
{
  gdouble new_sensitivity;

  new_sensitivity = global->GFL.cur_sensitivity - 1;
  if (sensitivity_set (global, new_sensitivity))
    global->GFL.cur_sensitivity = new_sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (global->GFL.fl));
}

static void
color_map_up (GtkWidget *widget,
              Global    *global)
{
  guint cur_color_map;

  switch (global->sonar_selector)
    {
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
      case W_FORWARDL:
      default:
        g_message ("wrong color_map_up!"); //TODO: remove when debugged
        return;
    }
}

static void
color_map_down (GtkWidget *widget,
                Global    *global)
{
  guint cur_color_map;

  switch (global->sonar_selector)
    {
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
      case W_FORWARDL:
      default:
        g_message ("wrong color_map_down!"); //TODO: remove when debugged
        return;
    }
}

/* Функция переключает режим отображения виджета ВС. */
static gboolean
mode_changed (GtkWidget *widget,
              gboolean   state,
              Global    *global)
{
  if (global->sonar_selector != W_FORWARDL)
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
           Global     *global)
{
  GtkSwitch *live_view;
  HyScanGtkWaterfallDrawer *wfd;

  if (GTK_WIDGET (widget) == GTK_WIDGET (global->GSS.live_view))
    {
      live_view = global->GSS.live_view;
      wfd = HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf);
    }
  else if (GTK_WIDGET (widget) == GTK_WIDGET (global->GPF.live_view))
    {
      live_view = global->GPF.live_view;
      wfd = HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf);
    }
  else
    {
      g_message ("live_view triggerred by wrong widget");
      return TRUE;
    }

  if (state != gtk_switch_get_state (live_view))
    hyscan_gtk_waterfall_drawer_automove (wfd, state);
  gtk_switch_set_state (live_view, state);

  return TRUE;
}

static void
live_view_off (GtkWidget  *widget,
               gboolean    state,
               Global     *global)
{
  if (GTK_WIDGET (widget) == GTK_WIDGET (global->GSS.wf))
    {
      gtk_switch_set_active (global->GSS.live_view, state);
    }
  else if (GTK_WIDGET (widget) == GTK_WIDGET (global->GPF.wf))
    {
      gtk_switch_set_active (global->GPF.live_view, state);
    }
  else
    {
      g_message ("live_view_off: wrong calling instance!");
    }
}

static void
distance_up (GtkWidget *widget,
             Global    *global)
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
        cur_distance = global->GPF.sonar.cur_distance + 5.0;
        if (distance_set (global, cur_distance))
          global->GPF.sonar.cur_distance = cur_distance;
        break;
    }
}

static void
distance_down (GtkWidget *widget,
               Global    *global)
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
        cur_distance = global->GPF.sonar.cur_distance - 5.0;
        if (distance_set (global, cur_distance))
          global->GPF.sonar.cur_distance = cur_distance;
        break;
    }
}

static void
tvg0_up (GtkWidget *widget,
         Global    *global)
{
  gdouble cur_gain0;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain0 = global->GFL.sonar.cur_gain0 + 0.5;
      if (tvg_set (global, &cur_gain0, global->GFL.sonar.cur_gain_step))
        global->GFL.sonar.cur_gain0 = cur_gain0;
      break;

    case W_SIDESCAN:
      cur_gain0 = global->GSS.sonar.cur_gain0 + 0.5;
      if (tvg_set (global, &cur_gain0, global->GSS.sonar.cur_gain_step))
        global->GSS.sonar.cur_gain0 = cur_gain0;
      break;

    case W_PROFILER:
      cur_gain0 = global->GPF.sonar.cur_gain0 + 0.5;
      if (tvg_set (global, &cur_gain0, global->GPF.sonar.cur_gain_step))
        global->GPF.sonar.cur_gain0 = cur_gain0;
      break;
    }
}

static void
tvg0_down (GtkWidget *widget,
           Global    *global)
{
  gdouble cur_gain0;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain0 = global->GFL.sonar.cur_gain0 - 0.5;
      if (tvg_set (global, &cur_gain0, global->GFL.sonar.cur_gain_step))
        global->GFL.sonar.cur_gain0 = cur_gain0;
      break;

    case W_SIDESCAN:
      cur_gain0 = global->GSS.sonar.cur_gain0 - 0.5;
      if (tvg_set (global, &cur_gain0, global->GSS.sonar.cur_gain_step))
        global->GSS.sonar.cur_gain0 = cur_gain0;
      break;

    case W_PROFILER:
      cur_gain0 = global->GPF.sonar.cur_gain0 - 0.5;
      if (tvg_set (global, &cur_gain0, global->GPF.sonar.cur_gain_step))
        global->GPF.sonar.cur_gain0 = cur_gain0;
    }
}

static void
tvg_up (GtkWidget *widget,
        Global    *global)
{
  gdouble cur_gain_step;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain_step = global->GFL.sonar.cur_gain_step + 2.5;
      if (tvg_set (global, &global->GFL.sonar.cur_gain0, cur_gain_step))
        global->GFL.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_SIDESCAN:
      cur_gain_step = global->GSS.sonar.cur_gain_step + 2.5;
      if (tvg_set (global, &global->GSS.sonar.cur_gain0, cur_gain_step))
        global->GSS.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_PROFILER:
      cur_gain_step = global->GPF.sonar.cur_gain_step + 2.5;
      if (tvg_set (global, &global->GPF.sonar.cur_gain0, cur_gain_step))
        global->GPF.sonar.cur_gain_step = cur_gain_step;
    }
}

static void
tvg_down (GtkWidget *widget,
          Global    *global)
{
  gdouble cur_gain_step;

  switch (global->sonar_selector)
    {
    case W_FORWARDL:
      cur_gain_step = global->GFL.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, &global->GFL.sonar.cur_gain0, cur_gain_step))
        global->GFL.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_SIDESCAN:
      cur_gain_step = global->GSS.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, &global->GSS.sonar.cur_gain0, cur_gain_step))
        global->GSS.sonar.cur_gain_step = cur_gain_step;
      break;

    case W_PROFILER:
      cur_gain_step = global->GPF.sonar.cur_gain_step - 2.5;
      if (tvg_set (global, &global->GPF.sonar.cur_gain0, cur_gain_step))
        global->GPF.sonar.cur_gain_step = cur_gain_step;
    }
}

static void
signal_up (GtkWidget *widget,
           Global    *global)
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
             Global    *global)
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
  gint selector = GPOINTER_TO_INT (user_data);
  GtkWidget *will_go_right = NULL; /* Виджет, который сейчас слева. */
  GtkWidget *will_go_left = NULL; /* Требуемый виджет. */
  GList *v_kids, *h_kids;
  void (*pack_func)(GtkBox*, GtkWidget*, gboolean, gboolean, guint) = NULL; /* Функция упаковки виджета в бокс справа. */

  if (!gtk_toggle_button_get_active (togglebutton))
    return;

  global.sonar_selector = selector;

  will_go_left = GTK_WIDGET (global.gui.disp_widgets[selector]);

  v_kids = gtk_container_get_children (GTK_CONTAINER (global.gui.v_pane));
  h_kids = gtk_container_get_children (GTK_CONTAINER (global.gui.h_pane));

  will_go_right = GTK_WIDGET (h_kids->data);

  if (v_kids == NULL || v_kids->next == NULL || v_kids->next->next != NULL)
    {
      g_warning ("v_pane children error: ");
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
    return;

  gtk_container_remove (GTK_CONTAINER (global.gui.h_pane), will_go_right);
  gtk_container_remove (GTK_CONTAINER (global.gui.v_pane), will_go_left);

  gtk_box_pack_start (GTK_BOX (global.gui.h_pane), will_go_left, TRUE, TRUE, 0);
  pack_func (GTK_BOX (global.gui.v_pane), will_go_right, TRUE, TRUE, 0);

  gtk_stack_set_visible_child (GTK_STACK (global.gui.stack), global.gui.ctrl_widgets[selector]);
}

void
one_window (GtkToggleButton *togglebutton,
            Global          *global)
{
  gboolean active = gtk_toggle_button_get_active (togglebutton);
  gtk_widget_set_visible (global->gui.v_pane, !active);
}

/* Функция включает/выключает излучение. */
static gboolean
start_stop (GtkWidget *widget,
            gboolean   state,
            Global    *global)
{
  gchar *pftrack;
  gint old_selector = global->sonar_selector;

  /* Включаем излучение. */
  if (state)
    {
      static gint n_tracks = -1;
      gboolean status;

      /* Закрываем текущий открытый галс. */
      g_clear_pointer(&global->track_name, g_free);

      /* Задаем параметры гидролокаторов. */
      global->sonar_selector = W_SIDESCAN;
      if (!signal_set   (global, global->GSS.sonar.cur_signal) ||
          !tvg_set      (global, &global->GSS.sonar.cur_gain0, global->GSS.sonar.cur_gain_step) ||
          !distance_set (global, global->GSS.sonar.cur_distance))
        {
          gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
          global->sonar_selector = old_selector;
          return TRUE;
        }
      global->sonar_selector = W_PROFILER;
      if (!signal_set   (global, global->GPF.sonar.cur_signal) ||
          !tvg_set      (global, &global->GPF.sonar.cur_gain0, global->GPF.sonar.cur_gain_step) ||
          !distance_set (global, global->GPF.sonar.cur_distance))
        {
          gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
          global->sonar_selector = old_selector;
          return TRUE;
        }
      global->sonar_selector = W_FORWARDL;
      if (!signal_set   (global, global->GFL.sonar.cur_signal) ||
          !tvg_set      (global, &global->GFL.sonar.cur_gain0, global->GFL.sonar.cur_gain_step) ||
          !distance_set (global, global->GFL.sonar.cur_distance))
        {
          gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
          global->sonar_selector = old_selector;
          return TRUE;
        }

      global->sonar_selector = old_selector;
      /* Число галсов в проекте. */
      if (n_tracks < 0)
        {
          gint32 project_id;
          gchar **tracks;
          gchar **strs;

          project_id = hyscan_db_project_open (global->db, global->project_name);
          tracks = hyscan_db_track_list (global->db, project_id);

          // TODO: посчитать вручную, игнорируя галсы с префиксом пф.
          for (strs = tracks; strs != NULL && *strs != NULL; strs++)
            {
              if (!g_str_has_prefix (*strs, PF_TRACK_PREFIX))
                n_tracks++;
            }

          hyscan_db_close (global->db, project_id);
          g_free (tracks);
        }

      /* Включаем запись нового галса. */
      global->track_name = g_strdup_printf ("%s%d", global->track_prefix, ++n_tracks);

      status = hyscan_sonar_control_start (global->GSS.sonar.sonar_ctl, global->track_name, HYSCAN_TRACK_SURVEY);

      if (!status)
        {
          gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
          g_message ("GSS startup failed @ %i",__LINE__);
          return TRUE;
        }

      pftrack = g_strdup_printf (PF_TRACK_PREFIX "%s", global->track_name);
      status = hyscan_sonar_control_start (global->GPF.sonar.sonar_ctl, pftrack, HYSCAN_TRACK_SURVEY);
      g_free (pftrack);

      if (!status)
        {
          gtk_switch_set_active (GTK_SWITCH (widget), FALSE);
          g_message ("GPF startup failed @ %i",__LINE__);
          return TRUE;
        }

      /* Если локатор включён, переходим в режим онлайн. */
      gtk_switch_set_state (GTK_SWITCH (widget), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (global->gui.track_view), FALSE);

      gtk_switch_set_active (global->GSS.live_view, TRUE);
      gtk_switch_set_active (global->GPF.live_view, TRUE);
      gtk_switch_set_state (global->GSS.live_view, TRUE);
      gtk_switch_set_state (global->GPF.live_view, TRUE);
      hyscan_gtk_waterfall_drawer_automove (HYSCAN_GTK_WATERFALL_DRAWER (global->GSS.wf), TRUE);
      hyscan_gtk_waterfall_drawer_automove (HYSCAN_GTK_WATERFALL_DRAWER (global->GPF.wf), TRUE);
    }

  /* Выключаем излучение и блокируем режим онлайн. */
  else if (gtk_switch_get_state (GTK_SWITCH (widget)))
    {
      hyscan_sonar_control_stop (global->GSS.sonar.sonar_ctl);
      hyscan_sonar_control_stop (global->GPF.sonar.sonar_ctl);
      hyscan_forward_look_player_pause (global->GFL.fl_player);
      gtk_switch_set_state (GTK_SWITCH (widget), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (global->gui.track_view), TRUE);
    }

  return TRUE;
}

static GObject*
get_from_builder (GtkBuilder  *builder,
                  const gchar *name)
{
  GObject *obj = NULL;

  obj = gtk_builder_get_object (builder, name);

  if (obj == NULL)
    g_message ("Failed to load <%s>", name);

  return obj;
}

static GtkWidget*
create_sonar_box (GtkBuilder         *builder,
                  const gchar        *view_ctl_name,
                  const gchar        *sonr_ctl_name,
                  HyScanSonarControl *sonar_control)
{
  GtkWidget *box = NULL;
  GtkWidget *view_wdgt = NULL;
  GtkWidget *sonar_wdgt = NULL;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  view_wdgt = GTK_WIDGET (get_from_builder (builder, view_ctl_name));

  if (view_wdgt == NULL)
    {
      g_clear_object (&box);
      return NULL;
    }

  if (sonar_control != NULL)
    {
      sonar_wdgt = GTK_WIDGET (get_from_builder (builder, sonr_ctl_name));
      if (sonar_wdgt == NULL)
        {
          g_clear_object (&box);
          return NULL;
        }
    }

  if (view_wdgt != NULL)
    gtk_box_pack_start (GTK_BOX (box), view_wdgt, FALSE, FALSE, 0);
  if (sonar_wdgt != NULL)
    gtk_box_pack_end (GTK_BOX (box), sonar_wdgt, FALSE, FALSE, 0);

  return box;
}


static gboolean
check_sonar_specific_gui (SonarSpecificGui *gui,
                          const gchar      *sonar)
{
  if (gui->distance_value == NULL ||
      gui->tvg0_value == NULL ||
      gui->tvg_value == NULL ||
      gui->signal_value == NULL ||
      gui->brightness_value == NULL ||
      gui->scale_value == NULL ||
      gui->third_value == NULL)
    {
      g_message ("Failure during loading %s widgets", sonar);
      return FALSE;
    }
  return TRUE;
}

static gboolean
get_sonar_specific_gui (SonarSpecificGui *gui,
                        GtkBuilder       *builder,
                        const gchar      *brightness_value,
                        const gchar      *scale_value,
                        const gchar      *third_value,
                        const gchar      *sonar)
{
  gui->brightness_value = GTK_LABEL (get_from_builder (builder, brightness_value));
  gui->scale_value      = GTK_LABEL (get_from_builder (builder, scale_value));
  gui->third_value      = GTK_LABEL (get_from_builder (builder, third_value));
  gui->distance_value   = GTK_LABEL (get_from_builder (builder, "distance_value"));
  gui->tvg0_value       = GTK_LABEL (get_from_builder (builder, "tvg0_value"));
  gui->tvg_value        = GTK_LABEL (get_from_builder (builder, "tvg_value"));
  gui->signal_value     = GTK_LABEL (get_from_builder (builder, "signal_value"));

  return check_sonar_specific_gui (gui, sonar);
}

static gint
urpc_cb (uint32_t  session,
         uRpcData *urpc_data,
         void     *proc_data,
         void     *key_data)
{
  gint keycode = GPOINTER_TO_INT (proc_data);
  g_atomic_int_set (&global.urpc_keycode, keycode);

  return 0;
}

static gboolean
ame_key_func (gpointer user_data)
{
  Global *global = user_data;
  gint keycode;
  gboolean st;


  keycode = g_atomic_int_get (&global->urpc_keycode);

  if (keycode == URPC_KEYCODE_NO_ACTION)
    return G_SOURCE_CONTINUE;

  switch (keycode)
    {
    case AME_KEY_RIGHT:
      gtk_toggle_button_set_active (global->gui.ss_selector, TRUE);
      break;

    case AME_KEY_LEFT:
      gtk_toggle_button_set_active (global->gui.pf_selector, TRUE);
      break;

    case AME_KEY_ENTER:
      gtk_toggle_button_set_active (global->gui.fl_selector, TRUE);
      break;

    case AME_KEY_UP:
      st = gtk_toggle_button_get_active (global->gui.single_window);
      gtk_toggle_button_set_active (global->gui.single_window, !st);
      break;

    case AME_KEY_EXIT:
      gtk_main_quit ();
      return G_SOURCE_REMOVE;

    case AME_KEY_ESCAPE:
      if (global->full_screen)
        gtk_window_unfullscreen (GTK_WINDOW (global->gui.window));
      else
        gtk_window_fullscreen (GTK_WINDOW (global->gui.window));
      global->full_screen = !global->full_screen;
      break;

    default:
      g_message ("This key is not supported yet. ");
    }

  /* Сбрасываем значение кнопки, если его больше никто не перетер. */
  g_atomic_int_compare_and_exchange (&global->urpc_keycode, keycode, URPC_KEYCODE_NO_ACTION);

  return G_SOURCE_CONTINUE;
}

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
  HyScanSoundVelocity svp_val;
  GArray            *svp;                      /* Профиль скорости звука. */
  gdouble            ship_speed = 1.8;         /* Скорость движения судна. */
  gboolean           full_screen = FALSE;      /* Признак полноэкранного режима. */
  gchar             *config_file = NULL;       /* Название файла конфигурации. */

  HyScanSonarDriver *prof_driver = NULL;       /* Драйвер профилографа. */
  HyScanSonarDriver *flss_driver = NULL;       /* Драйвер вс+гбо. */
  HyScanParam       *prof_sonar = NULL;             /* Интерфейс управления локатором. */
  HyScanParam       *flss_sonar = NULL;             /* Интерфейс управления локатором. */

  GtkBuilder        *fl_builder = NULL;
  GtkBuilder        *pf_builder = NULL;
  GtkBuilder        *ss_builder = NULL;
  GtkBuilder        *common_builder = NULL;

  GtkWidget         *fl_control_box  = NULL;
  GtkWidget         *ss_control_box  = NULL;
  GtkWidget         *pf_control_box  = NULL;

  GtkWidget         *fl_play_control = NULL;

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
        { "project-name",    'p',   0, G_OPTION_ARG_STRING,  &project_name,    "Project name", NULL },
        //{ "track-prefix",    't',   0, G_OPTION_ARG_STRING,  &track_prefix,    "Track name prefix", NULL },

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

    if (args[1] != NULL)
      config_file = g_strdup (args[1]); //// TODO: what?!

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

  /***
   *     ___   ___         ___   ___         ___   ___               ___   ___   ___   ___   ___
   *    |     |   | |\  | |   | |   |       |     |   | |\  | |\  | |     |       |     |   |   | |\  |
   *     -+-  |   | | + | |-+-| |-+-        |     |   | | + | | + | |-+-  |       +     +   |   | | + |
   *        | |   | |  \| |   | |  \        |     |   | |  \| |  \| |     |       |     |   |   | |  \|
   *     ---   ---                           ---   ---               ---   ---         ---   ---
   *
   * Подключение к гидролокатору.
   */
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
          client = hyscan_sonar_client_new (flss_uri);
          if (client == NULL)
            {
              g_message ("can't connect to flss_sonar '%s'", flss_uri);
              goto exit;
            }

          if (!hyscan_sonar_client_set_master (client))
            {
              g_message ("can't set master mode on flss_sonar '%s'", flss_uri);
              g_object_unref (client);
              goto exit;
            }

          flss_sonar = HYSCAN_PARAM (client);
        }

      /* Подключение с помощью драйвера гидролокатора. */
      else
        {
          flss_driver = hyscan_sonar_driver_new (driver_path, driver_name);
          hyscan_exit_if_w_param (flss_driver == NULL, "can't load flss_sonar driver '%s'", driver_name);

          flss_sonar = hyscan_sonar_discover_connect (HYSCAN_SONAR_DISCOVER (flss_driver), flss_uri, NULL);
          hyscan_exit_if_w_param (flss_sonar == NULL, "can't connect to flss_sonar '%s'", flss_uri);
        }

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
      if (config_file != NULL)
        {
          config = g_key_file_new ();
          g_key_file_load_from_file (config, config_file, G_KEY_FILE_NONE, NULL);

          status = TRUE;
          status &= setup_sensors (HYSCAN_SENSOR_CONTROL (global.GFL.sonar.sonar_ctl), config);
          status &= setup_sonar_antenna (global.GFL.sonar.sonar_ctl, FORWARDLOOK, config);
          status &= setup_sonar_antenna (global.GSS.sonar.sonar_ctl, STARBOARD, config);
          status &= setup_sonar_antenna (global.GSS.sonar.sonar_ctl, PORTSIDE, config);

          g_key_file_unref (config);

          hyscan_exit_if (!status, "Failed to parse configuration file. ");
        }

      /* Рабочий проект. */
      status = hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GFL.sonar.sonar_ctl), project_name);
      hyscan_exit_if (!status, "forward-look: can't set working project");
    }

  if (prof_uri != NULL)
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
          client = hyscan_sonar_client_new (prof_uri);
          if (client == NULL)
            {
              g_message ("can't connect to sonar_ctl '%s'", prof_uri);
              goto exit;
            }

          if (!hyscan_sonar_client_set_master (client))
            {
              g_message ("can't set master mode on sonar_ctl '%s'", prof_uri);
              g_object_unref (client);
              goto exit;
            }

          prof_sonar = HYSCAN_PARAM (client);
        }

      /* Подключение с помощью драйвера гидролокатора. */
      else
        {
          prof_driver = hyscan_sonar_driver_new (driver_path, driver_name);
          hyscan_exit_if_w_param (prof_driver == NULL, "can't load prof_sonar driver '%s'", driver_name);

          prof_sonar = hyscan_sonar_discover_connect (HYSCAN_SONAR_DISCOVER (prof_driver), prof_uri, NULL);
          hyscan_exit_if_w_param (prof_sonar == NULL, "can't connect to prof_sonar '%s'", prof_uri);
        }

      /* Управление локатором. */
      global.GPF.sonar.sonar_ctl = hyscan_sonar_control_new (prof_sonar, 4, 4, global.db);
      hyscan_exit_if_w_param (global.GPF.sonar.sonar_ctl == NULL, "unsupported sonar_ctl '%s'", prof_uri);

      global.GPF.sonar.gen_ctl = HYSCAN_GENERATOR_CONTROL (global.GPF.sonar.sonar_ctl);
      global.GPF.sonar.tvg_ctl = HYSCAN_TVG_CONTROL (global.GPF.sonar.sonar_ctl);

      /* Параметры генераторов. */
      gen_cap = hyscan_generator_control_get_capabilities (global.GPF.sonar.gen_ctl, PROFILER);
      hyscan_exit_if (!(gen_cap | HYSCAN_GENERATOR_MODE_PRESET), "profiler: unsupported generator mode");

      status = hyscan_generator_control_set_enable (global.GPF.sonar.gen_ctl, PROFILER, TRUE);
      hyscan_exit_if (!status, "profiler: can't enable generator");

      /* Параметры ВАРУ. */
      tvg_cap = hyscan_tvg_control_get_capabilities (global.GPF.sonar.tvg_ctl, PROFILER);
      hyscan_exit_if (!(tvg_cap | HYSCAN_TVG_MODE_LINEAR_DB), "profiler: unsupported tvg_ctl mode");

      status = hyscan_tvg_control_set_enable (global.GPF.sonar.tvg_ctl, PROFILER, TRUE);
      hyscan_exit_if (!status, "profiler: can't enable tvg_ctl");

      /* Сигналы зондирования. */
      global.GPF.pf_signals = hyscan_generator_control_list_presets (global.GPF.sonar.gen_ctl, PROFILER);
      hyscan_exit_if (global.GPF.pf_signals == NULL, "profiler: can't load signal presets");

      for (i = 0; global.GPF.pf_signals[i] != NULL; i++);
      global.GPF.pf_n_signals = i;

      /* Настройка датчиков и антенн. */
      if (config_file != NULL)
        {
          config = g_key_file_new ();
          g_key_file_load_from_file (config, config_file, G_KEY_FILE_NONE, NULL);

          status = setup_sonar_antenna (global.GPF.sonar.sonar_ctl, PROFILER, config);
          g_key_file_unref (config);

          hyscan_exit_if (!status, "Failed to parse configuration file. ");

          if (!status)
            goto exit;
        }

      /* Рабочий проект. */
      status = hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.GPF.sonar.sonar_ctl), project_name);
      hyscan_exit_if (!status, "profiler: can't set working project");
    }
  /* Закончили подключение к гидролокатору. */

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

  global.gui.track_view = GTK_TREE_VIEW (get_from_builder (common_builder, "track_view"));
  global.gui.track_list = GTK_TREE_MODEL (get_from_builder (common_builder, "track_list"));
  global.gui.track_range = GTK_ADJUSTMENT (get_from_builder (common_builder, "track_range"));
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
  global.gui.selector       = GTK_WIDGET (get_from_builder (common_builder, "sonar_selector"));
  global.gui.stack          = GTK_WIDGET (gtk_stack_new ());

  if (global.GFL.sonar.sonar_ctl != NULL && global.GPF.sonar.sonar_ctl != NULL)
    {
      global.gui.record_control = GTK_WIDGET (get_from_builder (common_builder, "record_control"));
      global.gui.start_stop_switch = GTK_SWITCH (get_from_builder (common_builder, "start_stop"));
    }
  else /* (global.GFL.sonar.sonar_ctl == NULL || global.GPF.sonar.sonar_ctl == NULL) */
    {
      *(void**)&global.gui.record_control = *(void**)&global.gui.start_stop_switch = NULL;
    }

  /* Запихиваем всё в бокс: */
  gtk_box_pack_start (GTK_BOX (global.gui.right_box), global.gui.selector, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (global.gui.right_box), global.gui.stack, TRUE, TRUE, 2);

  if (global.gui.record_control != NULL)

    gtk_box_pack_end (GTK_BOX (global.gui.right_box), global.gui.record_control, FALSE, FALSE, 2);

  /* Бокс запихиваем в правую панель арии: */
  hyscan_gtk_area_set_right (HYSCAN_GTK_AREA (global.gui.gtk_area), global.gui.right_box);

  /* Центральная зона: два box'a (наполним позже). */
  global.gui.v_pane = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  global.gui.h_pane = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_end (GTK_BOX (global.gui.h_pane), global.gui.v_pane, TRUE, TRUE, 0);

  hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (global.gui.gtk_area), global.gui.h_pane);

  /* Ништяк: скелет гуя сделан. Теперь подцепим тут нужные сигналы. */
  gtk_builder_add_callback_symbol (common_builder, "track_scroll", G_CALLBACK (track_scroll));
  gtk_builder_add_callback_symbol (common_builder, "track_changed", G_CALLBACK (track_changed));
  gtk_builder_add_callback_symbol (common_builder, "position_changed", G_CALLBACK (position_changed));

  if (global.GFL.sonar.sonar_ctl != NULL && global.GPF.sonar.sonar_ctl != NULL)
    gtk_builder_add_callback_symbol (common_builder, "start_stop", G_CALLBACK (start_stop));
  else /* if (global.GFL.sonar.sonar_ctl == NULL || global.GPF.sonar.sonar_ctl == NULL) */
    gtk_builder_add_callback_symbol (common_builder, "start_stop", G_CALLBACK (void_callback));

  gtk_builder_connect_signals (common_builder, &global);


  /* Сигналы sonar_selector'a. */
  global.gui.ss_selector = g_object_ref (get_from_builder (common_builder, "ss_selector"));
  global.gui.pf_selector = g_object_ref (get_from_builder (common_builder, "pf_selector"));
  global.gui.fl_selector = g_object_ref (get_from_builder (common_builder, "fl_selector"));
  global.gui.single_window = g_object_ref (get_from_builder (common_builder, "single_window"));
  g_signal_connect (global.gui.ss_selector, "toggled", G_CALLBACK (widget_swap), GINT_TO_POINTER (W_SIDESCAN));
  g_signal_connect (global.gui.pf_selector, "toggled", G_CALLBACK (widget_swap), GINT_TO_POINTER (W_PROFILER));
  g_signal_connect (global.gui.fl_selector, "toggled", G_CALLBACK (widget_swap), GINT_TO_POINTER (W_FORWARDL));
  g_signal_connect (global.gui.single_window, "toggled", G_CALLBACK (one_window), &global);

  /* Управление гидролокаторами и отображением. */
  fl_control_box = create_sonar_box (fl_builder, "fl_view_control", "sonar_control", global.GFL.sonar.sonar_ctl);
  global.gui.ctrl_widgets[W_FORWARDL] = fl_control_box;

  pf_control_box = create_sonar_box (pf_builder, "sonar_view_control", "sonar_control", global.GPF.sonar.sonar_ctl);
  global.gui.ctrl_widgets[W_PROFILER] = pf_control_box;

  ss_control_box = create_sonar_box (ss_builder, "sonar_view_control", "sonar_control", global.GSS.sonar.sonar_ctl);
  global.gui.ctrl_widgets[W_SIDESCAN] = ss_control_box;

  /* Теперь получаем лейблы для *_view_control и *_sonar_control. */
  status = get_sonar_specific_gui (&global.GFL.gui, fl_builder, "fl_brightness_value", "fl_scale_value", "fl_sensitivity_value", "FL");
  hyscan_exit_if (!status, "failed to get FL-specific GUI");
  status = get_sonar_specific_gui (&global.GSS.gui, ss_builder, "ss_brightness_value", "ss_scale_value", "ss_color_map_value", "SS");
  hyscan_exit_if (!status, "failed to get SS-specific GUI");
  status = get_sonar_specific_gui (&global.GPF.gui, pf_builder, "ss_brightness_value", "ss_scale_value", "ss_color_map_value", "PF");
  hyscan_exit_if (!status, "failed to get PF-specific GUI");

  /***
   *     ___         ___   ___   ___               ___   ___   ___         ___         ___
   *      | | |   |   |     |   |   | |\  |       |       |   |     |\  | |   | |     |
   *      +-  |   |   +     +   |   | | + |        -+-    +   | +-  | + | |-+-| |      -+-
   *      | | |   |   |     |   |   | |  \|           |   |   |   | |  \| |   | |         |
   *     ---   ---               ---               ---   ---   ---               ---   ---
   *
   * Теперь немного гемора: цепляемся к сигналам кнопок. Их много, а потому увы.
   */
  {
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
  }

  /* Кладем гидролокаторские боксы в гткстэк. */
  gtk_stack_add_titled (GTK_STACK (global.gui.stack), ss_control_box, "ss_control_box", "ss");
  gtk_stack_add_titled (GTK_STACK (global.gui.stack), pf_control_box, "pf_control_box", "pf");
  gtk_stack_add_titled (GTK_STACK (global.gui.stack), fl_control_box, "fl_control_box", "fl");

  /***
   *     ___   ___   ___   ___         ___                     ___   ___   ___   ___   ___   ___
   *      | |   |   |     |   | |     |   |  \ /        |   |   |     | | |     |       |   |
   *      + |   +    -+-  |-+-  |     |-+-|   +         | + |   +     + | | +-  |-+-    +    -+-
   *      | |   |       | |     |     |   |   |         |/ \|   |     | | |   | |       |       |
   *     ---   ---   ---         ---                           ---   ---   ---   ---         ---
   *
   * Создаем виджеты просмотра.
   */
  global.GSS.wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_grid_new ());
  global.gui.disp_widgets[W_SIDESCAN] = g_object_ref (global.GSS.wf);

  global.GPF.wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_grid_new ());
  hyscan_gtk_waterfall_echosounder (global.GPF.wf, PROFILER);
  global.gui.disp_widgets[W_PROFILER] = g_object_ref (global.GPF.wf);

  global.GFL.fl = HYSCAN_GTK_FORWARD_LOOK (hyscan_gtk_forward_look_new ());
  global.gui.disp_widgets[W_FORWARDL] = g_object_ref (global.GFL.fl);
  global.GFL.fl_player = hyscan_gtk_forward_look_get_player (global.GFL.fl);

  /* Управление воспроизведением FL. */
  fl_play_control = GTK_WIDGET (get_from_builder (fl_builder, "fl_play_control"));
  hyscan_exit_if (fl_play_control == NULL, "can't load play control ui");
  global.GFL.position = GTK_SCALE (get_from_builder (fl_builder, "position"));
  hyscan_exit_if (global.GFL.position == NULL, "incorrect play control ui");
  global.GFL.position_range = hyscan_gtk_forward_look_get_adjustment (global.GFL.fl);
  gtk_range_set_adjustment (GTK_RANGE (global.GFL.position), global.GFL.position_range);

  hyscan_gtk_area_set_bottom (HYSCAN_GTK_AREA (global.gui.gtk_area), fl_play_control);

  gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (global.GSS.wf), FALSE);
  gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (global.GPF.wf), FALSE);
  gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (global.GFL.fl), TRUE);

  global.GSS.live_view = GTK_SWITCH (get_from_builder (ss_builder, "ss_live_view"));
  global.GPF.live_view = GTK_SWITCH (get_from_builder (pf_builder, "ss_live_view"));
  g_signal_connect (G_OBJECT (global.GSS.wf), "automove-state", G_CALLBACK (live_view_off), &global);
  g_signal_connect_swapped (G_OBJECT (global.GSS.wf), "waterfall-zoom", G_CALLBACK (scale_set), &global);
  g_signal_connect (G_OBJECT (global.GPF.wf), "automove-state", G_CALLBACK (live_view_off), &global);
  g_signal_connect_swapped (G_OBJECT (global.GPF.wf), "waterfall-zoom", G_CALLBACK (scale_set), &global);

  gtk_box_pack_start (GTK_BOX (global.gui.v_pane), GTK_WIDGET (global.GFL.fl), TRUE, TRUE, 1);
  gtk_box_pack_end (GTK_BOX (global.gui.v_pane), GTK_WIDGET (global.GPF.wf), TRUE, TRUE, 1);
  gtk_box_pack_start (GTK_BOX (global.gui.h_pane), GTK_WIDGET (global.GSS.wf), TRUE, TRUE, 1);

  /* Cache */
  hyscan_forward_look_player_set_cache (global.GFL.fl_player, global.cache, NULL);
  hyscan_gtk_waterfall_set_cache (global.GSS.wf, global.cache, global.cache, NULL);
  hyscan_gtk_waterfall_set_cache (global.GPF.wf, global.cache, global.cache, NULL);

  /* SV. */
  global.sound_velocity = sound_velocity;
  svp = g_array_new (FALSE, FALSE, sizeof (HyScanSoundVelocity));
  svp_val.depth = 0.0;
  svp_val.velocity = global.sound_velocity;
  g_array_insert_val (svp, 0, svp_val);

  hyscan_forward_look_player_set_sv (global.GFL.fl_player, global.sound_velocity);
  hyscan_gtk_waterfall_set_sound_velocity (global.GSS.wf, svp);
  hyscan_gtk_waterfall_set_sound_velocity (global.GPF.wf, svp);

  hyscan_gtk_waterfall_drawer_set_automove_period (HYSCAN_GTK_WATERFALL_DRAWER (global.GSS.wf), 100000);
  hyscan_gtk_waterfall_drawer_set_automove_period (HYSCAN_GTK_WATERFALL_DRAWER (global.GPF.wf), 100000);
  hyscan_gtk_waterfall_drawer_set_regeneration_period (HYSCAN_GTK_WATERFALL_DRAWER (global.GSS.wf), 500000);
  hyscan_gtk_waterfall_drawer_set_regeneration_period (HYSCAN_GTK_WATERFALL_DRAWER (global.GPF.wf), 500000);

  // gtk_cifro_area_control_set_scroll_mode (GTK_CIFRO_AREA_CONTROL (global.fl), GTK_CIFRO_AREA_SCROLL_MODE_COMBINED);
  //
  // /* Управление отображением. */
  // global.fl_player = hyscan_gtk_forward_look_get_player (global.fl);
  //
  // /* Полоска прокрутки. */
  // global.position_range = hyscan_gtk_forward_look_get_adjustment (global.fl);
  // gtk_range_set_adjustment (GTK_RANGE (global.position), global.position_range);

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

/***
 *     ___   ___   ___   ___               ___               ___               ___   ___
 *      | | |     |     |   | |   | |       |         |  /  |   | |     |   | |     |
 *      + | |-+-  |-+-  |-+-| |   | |       +         | +   |-+-| |     |   | |-+-   -+-
 *      | | |     |     |   | |   | |       |         |/    |   | |     |   | |         |
 *     ---   ---               ---   ---                           ---   ---   ---   ---
 *
 * Начальные значения.
 */
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
  global.GFL.sonar.cur_gain_step = 20.0;
  global.GFL.sonar.cur_distance = PROFILER_MAX_DISTANCE;

  global.sonar_selector = W_SIDESCAN;
  brightness_set (&global, global.GSS.cur_brightness);
  color_map_set (&global, global.GSS.cur_color_map);
  scale_set (&global, FALSE, NULL);
  if (global.GSS.sonar.sonar_ctl != NULL)
    {
      distance_set (&global, global.GSS.sonar.cur_distance);
      tvg_set (&global, &global.GSS.sonar.cur_gain0, global.GSS.sonar.cur_gain_step);
      signal_set (&global, 1);
    }

  global.sonar_selector = W_PROFILER;
  brightness_set (&global, global.GPF.cur_brightness);
  color_map_set (&global, global.GPF.cur_color_map);
  scale_set (&global, FALSE, NULL);
  if (global.GPF.sonar.sonar_ctl != NULL)
    {
      distance_set (&global, global.GPF.sonar.cur_distance);
      tvg_set (&global, &global.GPF.sonar.cur_gain0, global.GPF.sonar.cur_gain_step);
      signal_set (&global, 1);
    }

  global.sonar_selector = W_FORWARDL;
  brightness_set (&global, global.GFL.cur_brightness);
  sensitivity_set (&global, global.GFL.cur_sensitivity);
  scale_set (&global, FALSE, NULL);
  if (global.GFL.sonar.sonar_ctl != NULL)
    {
      distance_set (&global, global.GFL.sonar.cur_distance);
      tvg_set (&global, &global.GFL.sonar.cur_gain0, global.GPF.sonar.cur_gain_step);
      signal_set (&global, 1);
    }

  global.sonar_selector = W_SIDESCAN;

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
  gtk_main ();

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
exit:

  g_clear_object (&common_builder);
  g_clear_object (&fl_builder);
  g_clear_object (&ss_builder);
  g_clear_object (&pf_builder);

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
  g_clear_object (&flss_driver);
  g_clear_object (&prof_driver);

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
  g_free (track_prefix);
  g_free (config_file);

  return 0;
}
