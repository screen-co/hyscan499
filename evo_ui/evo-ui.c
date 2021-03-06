#include <gmodule.h>
#include <hyscan-config.h>
#include <hyscan-geo.h>
#include <hyscan-gtk-area.h>
#include <hyscan-gtk-fnn-offsets.h>
#include <hyscan-gtk-actuator-control.h>
#include "hyscan-gtk-mark-export.h"
#include <hyscan-planner-export.h>
#include "evo-ui.h"
#include "hyscan-gtk-rec.h"
#include "hyscan-gtk-map-go.h"
#include "hyscan-gtk-map-preload.h"
#include <hyscan-gtk-fnn-export.h>
#define GETTEXT_PACKAGE "hyscanfnn-evoui"
#include <glib/gi18n-lib.h>
#include <proj_api.h>
#include <hyscan-gtk-layer-list.h>
#include <hyscan-gtk-map-mark-list.h>
#include <hyscan-gtk-mark-manager.h>

// #define EVO_EMPTY_PAGE "empty_page"
#define EVO_NOT_MAP "evo-not-map"
#define EVO_MAP "evo-map"
#define EVO_ENABLE_EXTRAS "HY_PARAM"

#define EVO_GRID_KEY "GRID"
#define EVO_MARK_KEY "MARK"
#define EVO_METR_KEY "METR"
#define EVO_SHAD_KEY "SHAD"
#define EVO_COOR_KEY "COOR"
#define EVO_MAGN_KEY "MAGN"

#define TVG_TEST(caps,concrete) ((caps & concrete) == concrete)
#define DEG2RAD(x) ((x) * G_PI / 180.0)

enum
{
  XYZ_TO_FILE,
  XYZ_BATCH,
  UTM_TO_FILE,
  MARKS_TO_CSV,
  MARKS_TO_CLIPBOARD,
  MARKS_TO_HTML,
  PLANNER_TO_XML,
  PLANNER_TO_KML,
  IMPORT_PLANNER_XML,
};

EvoUI global_ui = {0,};
Global *_global = NULL;
/* Виджет для Журнала Меток. */
GtkWidget  *mark_manager_window = NULL;

static void la_ruler_changed_cb (HyScanGtkGlikoView *view, gdouble az, gdouble ar, gdouble bz, gdouble br, gpointer user_data);
static void la_player_scale_cb (GtkRange *range, gpointer user_data);
static void la_pause_toggle_cb (GtkToggleButton *toggle_button, gpointer user_data);
static void la_x10_toggle_cb (GtkToggleButton *toggle_button, gpointer user_data);
static void la_player_range_cb (HyScanDataPlayer *player, gint64 min, gint64 max, gpointer user_data);
static void la_player_ready_cb (HyScanDataPlayer *player, gint64 time, gpointer user_data);

void
filesave_dialog (const gchar *extension,
                 const gchar *project,
                 const gchar *track,
                 const gchar *data)
{
  GtkWidget *dialog;
  gint res;
  gchar *folder = NULL;
  gchar *filename = NULL;
  GError *error = NULL;

  dialog = gtk_file_chooser_dialog_new (_("Save file"),
                                        GTK_WINDOW (_global->gui.window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("Cancel"), GTK_RESPONSE_CANCEL,
                                        _("Save"), GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

  folder = keyfile_string_read_helper (global_ui.settings, "EVO", "export_folder");

  filename = g_strdup_printf ("%s-%s.%s", project, track, extension);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), folder);
  g_free (filename);
  g_free (folder);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));

  keyfile_string_write_helper (global_ui.settings, "EVO", "export_folder", folder);

  g_file_set_contents (filename, data, strlen (data), &error);

  if (error != NULL)
    {
      g_message ("Depth save failure: %s", error->message);
      g_error_free (error);
    }

  gtk_widget_destroy (dialog);
  g_free (folder);
  g_free (filename);
}

void
depth_writer (GObject *emitter)
{
  guint32 first, last, i;
  HyScanAntennaOffset antenna;
  HyScanGeoGeodetic coord;
  GString *string = NULL;
  gchar *words = NULL;

  HyScanDB * db = _global->db;
  HyScanCache * cache = _global->cache;
  gchar *project = _global->project_name;
  gchar *track = _global->track_name;

  HyScanNavData * dpt;
  HyScanmLoc * mloc;

  GHashTable * track_infos;
  HyScanTrackInfo * info;
  HyScanDBInfoSensorInfo *sensor_info;
  GHashTableIter iter;
  gpointer key, value;

  track_infos = hyscan_db_info_get_tracks (_global->db_info);
  info = g_hash_table_lookup (track_infos, track);
  if (info == NULL)
    {
      g_warning ("Couldn't find track <%s> info.", track);
      return;
    }

  g_hash_table_iter_init (&iter, info->sensor_infos);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (NULL == g_strrstr ((gchar*)key, "echosounder"))
        continue;

      sensor_info = value;
      g_message ("Track <%s>: using <%s> as echosounder data (%i)", track, (gchar*)key, sensor_info->channel);
      break;
    }

  if (sensor_info == NULL)
    {
      g_warning ("Couldn't find echosounder for track <%s>.", track);
      return;
    }

  dpt = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, cache, project, track,
                                                 sensor_info->channel,
                                                 HYSCAN_NMEA_DATA_DPT,
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
  g_string_append_printf (string, "#%s;%s\n", project, track);
  g_string_append_printf (string, "#LAT,LON,DPT\n");

  hyscan_nav_data_get_range (dpt, &first, &last);
  antenna = hyscan_nav_data_get_offset (dpt);

  for (i = first; i <= last; ++i)
    {
      gdouble val;
      gint64 time;
      gboolean status;
      gchar lat_str[1024] = {'\0'};
      gchar lon_str[1024] = {'\0'};
      gchar dpt_str[1024] = {'\0'};

      status = hyscan_nav_data_get (dpt, NULL, i, &time, &val);
      if (!status)
        continue;

      status = hyscan_mloc_get (mloc, NULL, time, &antenna, 0, 0, 0, &coord);
      if (!status)
        continue;

      g_ascii_formatd (lat_str, 1024, "%12.9f", coord.lat);
      g_ascii_formatd (lon_str, 1024, "%12.9f", coord.lon);
      g_ascii_formatd (dpt_str, 1024, "%12.9f", val);
      g_string_append_printf (string, "%s;%s;%s\n", lat_str, lon_str, dpt_str);
    }

  words = g_string_free (string, FALSE);

  if (words == NULL)
    return;

  filesave_dialog ("xyz.txt", project, track, words);
  g_free (words);
}

void
depth_writer_batch (GObject *em)
{
  EvoUI *ui = &global_ui;

  HyScanMapTrackModel *tmodel = hyscan_gtk_map_builder_get_track_model (ui->map_builder);
  GtkWidget *w = hyscan_gtk_fnn_export_new (_global->db_info,
                                            _global->cache,
                                            tmodel);
  gtk_widget_show_all (w);
}

/* честно украдено у Пылаева */
static guint
convert_zone_number_by_pylaev (const gdouble  lat,
                               const gdouble  lon,
                               gint          *err)
{
  guint zone_number = 0;
  gint er = 0;

  if ((lat < -80.) || (lat > 84.))
    er = -1;

  if ((lon < -180.) || ( lon >= 180.))
    er = -2;

  if (er != 0)
    {
      if (err != NULL)
        *err = er;
      return zone_number;
    }

  zone_number = (gint)((lon + 180.) / 6.) + 1;

  if ((lat >= 56.0) && (lat < 64.0) && ( lon >= 3.0) && ( lon < 12.0))
    zone_number = 32;

  /* Special zones for Svalbard */
  if ((lat >= 72.0) && (lat < 84.0))
    {
      if ((lon >= 0.0) && (lon <  9.0))
        zone_number = 31;
      else if ((lon >= 9.0) && (lon < 21.0))
        zone_number = 33;
      else if ((lon >= 21.0) && (lon < 33.0))
        zone_number = 35;
      else if ((lon >= 33.0) && (lon < 42.0))
        zone_number = 37;
    }

  if (err != NULL)
    *err = er;

  return zone_number;
}

void
utm_xyz (GObject *emitter)
{
  guint32 first, last, i;
  HyScanAntennaOffset antenna;
  HyScanGeoGeodetic coord;
  GString *string = NULL;
  gchar *words = NULL;

  HyScanDB * db = _global->db;
  HyScanCache * cache = _global->cache;
  gchar *project = _global->project_name;
  gchar *track = _global->track_name;

  HyScanNavData * dpt;
  HyScanNavData * alt;
  HyScanNavData * hog;
  HyScanmLoc * mloc;

  dpt = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, cache, project, track, 3, HYSCAN_NMEA_DATA_DPT, HYSCAN_NMEA_FIELD_DEPTH));
  alt = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, cache, project, track, 2, HYSCAN_NMEA_DATA_GGA, HYSCAN_NMEA_FIELD_ALTITUDE));
  hog = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, cache, project, track, 2, HYSCAN_NMEA_DATA_GGA, HYSCAN_NMEA_FIELD_HOG));

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
  g_string_append_printf (string, "#%s;%s\n", project, track);
  g_string_append_printf (string, "#LAT,LON,DPT\n");

  hyscan_nav_data_get_range (dpt, &first, &last);
  antenna = hyscan_nav_data_get_offset (dpt);

  for (i = first; i <= last; ++i)
    {
      gdouble depth, altitude, height;
      gint64 time;
      gboolean status;
      gchar lat_str[1024] = {'\0'};
      gchar lon_str[1024] = {'\0'};
      gchar dpt_str[1024] = {'\0'};

      status = hyscan_nav_data_get (dpt, NULL, i, &time, &depth);
      if (!status)
        continue;

      status = hyscan_mloc_get (mloc, NULL, time, &antenna, 0, 0, 0, &coord);
      if (!status)
        continue;

      {
        HyScanDBFindStatus fstatus;
        guint32 index;

        fstatus = hyscan_nav_data_find_data (alt, time, &index, NULL, NULL, NULL);
        if (fstatus != HYSCAN_DB_FIND_OK)
          continue;

        status  = hyscan_nav_data_get (alt, NULL, index, NULL, &altitude);
        status &= hyscan_nav_data_get (hog, NULL, index, NULL, &height);

        if (!status)
          continue;

        HyScanGeoGeodetic in = {.lat = coord.lat, .lon = coord.lon, .h = altitude + height};
        HyScanGeoGeodetic out;
        hyscan_geo_cs_transform (&out, in, HYSCAN_GEO_CS_WGS84, HYSCAN_GEO_CS_PZ90);
      }

      /* переносим дно*/
      depth = altitude + height - depth;

      {
        projPJ proj_src;
        projPJ proj_dst;
        gchar *init_str;

        init_str = g_strdup_printf ("+proj=utm +datum=WGS84 +zone=%i", convert_zone_number_by_pylaev (coord.lat, coord.lon, NULL));
        proj_src = pj_init_plus ("+proj=latlon +datum=WGS84");
        proj_dst = pj_init_plus (init_str);

        coord.lat = DEG2RAD (coord.lat);
        coord.lon = DEG2RAD (coord.lon);

        if (0 != pj_transform (proj_src, proj_dst, 1, 1, &coord.lon, &coord.lat, NULL))
          {
            g_free (init_str);
            pj_free (proj_dst);
            pj_free (proj_src);
            continue;
          }

        g_free (init_str);
        pj_free (proj_dst);
        pj_free (proj_src);
      }

      g_ascii_formatd (lat_str, 1024, "%12.9f", coord.lat);
      g_ascii_formatd (lon_str, 1024, "%12.9f", coord.lon);
      g_ascii_formatd (dpt_str, 1024, "%12.9f", depth);
      g_string_append_printf (string, "%s;%s;%s\n", lat_str, lon_str, dpt_str);
    }

  words = g_string_free (string, FALSE);

  if (words == NULL)
    return;

  filesave_dialog ("utm.txt", project, track, words);
  g_free (words);
}

void
mark_exporter (GObject  *emitter,
               gpointer  _selector)
{
  gint selector = GPOINTER_TO_INT (_selector);

  if (selector == XYZ_TO_FILE)
    {
      depth_writer (NULL);
    }
  else if (selector == XYZ_BATCH)
    {
      depth_writer_batch (NULL);
    }
  else if (selector == UTM_TO_FILE)
    {
      utm_xyz (NULL);
    }
  else if (selector == IMPORT_PLANNER_XML)
    {
      hyscan_gtk_map_builder_run_planner_import (global_ui.map_builder);
    }
  else if (selector == PLANNER_TO_XML || selector == PLANNER_TO_KML)
    {
      gchar *data;
      GHashTable *objects;
      HyScanPlannerModel *planner;

      planner = hyscan_gtk_model_manager_get_planner_model (_global->model_manager);
      if (planner == NULL)
        return;

      objects = hyscan_object_store_get_all (HYSCAN_OBJECT_STORE (planner), G_TYPE_BOXED);
      g_object_unref (planner);

      if (objects == NULL)
        return;

      if (selector == PLANNER_TO_XML)
        {
          data = hyscan_planner_export_xml_to_str (objects);
          filesave_dialog ("plan.xml", _global->project_name, NULL, data);
        }
      else
        {
          data = hyscan_planner_export_kml_to_str (objects);
          filesave_dialog ("plan.kml", _global->project_name, NULL, data);
        }

      g_hash_table_destroy (objects);
      g_free (data);
    }
  else
    {
      HyScanObjectModel  *geo = hyscan_gtk_model_manager_get_geo_mark_model (_global->model_manager);
      HyScanMarkLocModel *wf  = hyscan_gtk_model_manager_get_acoustic_mark_loc_model (_global->model_manager);

      switch (selector)
        {
          case MARKS_TO_CSV:
            {
              gchar *data = hyscan_gtk_mark_export_to_str (wf, geo, _global->project_name);
              filesave_dialog ("marks.txt", _global->project_name, _global->track_name, data);
            }
          break;
          case MARKS_TO_CLIPBOARD:
            {
              hyscan_gtk_mark_export_copy_to_clipboard (wf, geo, _global->project_name);
            }
          break;
          case MARKS_TO_HTML:
            {
              hyscan_gtk_mark_export_save_as_html (_global->model_manager,
                                                   GTK_WINDOW (_global->gui.window),
                                                   FALSE);
            }
          break;
          default: break;
        }
      g_object_unref (geo);
      g_object_unref (wf);
    }
}

/* Обработчик пункта меню "Журнал Меток". */
void run_mark_manager ()
{
  if (mark_manager_window == NULL)
    {
      GtkWindow *window;
      GtkWidget *mark_manager;
      /* Создаём виджет. */
      mark_manager_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      window = GTK_WINDOW (mark_manager_window);
      /* Помещаем виджет в центр. */
      gtk_window_set_position (window, GTK_WIN_POS_CENTER);
      /* Размер виджета. */
      gtk_window_set_default_size (window, 800, 600);
      /* Заголовок виджета. */
      gtk_window_set_title (window, _("Mark Manager"));
      /* Отступ от края в 5 пикселей. */
      gtk_container_set_border_width (GTK_CONTAINER (mark_manager_window), 5);
      /* Cкрыть виджет по сигналу закрытия виджета. */
      g_signal_connect (mark_manager_window,
                        "delete-event",
                        G_CALLBACK (gtk_widget_hide_on_delete),
                        NULL);
      /* Создаём Журнал Меток. */
      mark_manager = hyscan_gtk_mark_manager_new (_global->model_manager);
      /* Помещаем Журнал Меток в окно. */
      gtk_container_add (GTK_CONTAINER (mark_manager_window), mark_manager);
      /* Делаем все виджеты видимыми. */
      gtk_widget_show_all (mark_manager_window);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (mark_manager_window));
    }
}

/* OVERRIDES */
gboolean
evo_brightness_set_override (Global  *global,
                             gdouble  new_brightness,
                             gdouble  new_black,
                             gint     panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gchar *text_bright;
  gchar *text_black;
  gdouble b, g, w;
  FnnPanel *panel;
  gdouble new_contrast = new_brightness;
  gdouble new_shrink = new_black;

  panel = get_panel (global, panelx);

  if (new_brightness < 0.0)
    return FALSE;
  if (new_brightness > 100.0)
    return FALSE;
  if (new_black < -100.0)
    return FALSE;
  if (new_black > 100.0)
    return FALSE;

  text_bright = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_brightness);
  text_black = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_black);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
    case FNN_PANEL_ECHO:
      {
        HyScanSourceType lsource, rsource;

        new_contrast *= .99 * 0.01;
        new_shrink *= .99 * 0.01;
        wf = (VisualWF*)panel->vis_gui;
        hyscan_gtk_waterfall_state_get_sources (HYSCAN_GTK_WATERFALL_STATE (wf->wf), &lsource, &rsource);
        hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), new_shrink, 1.0, new_contrast);
        gtk_label_set_markup (wf->common.white_value, text_bright);
        gtk_label_set_markup (wf->common.black_value, text_black);
        break;
      }

    case FNN_PANEL_PROFILER:
      b = new_black / 250000;
      w = b + (1 - 0.99 * new_brightness / 100.0) * (1 - b);
      g = 1;
      if (b >= w)
        {
          g_message ("BBC error");
          return FALSE;
        }

      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w);

      gtk_label_set_markup (wf->common.white_value, text_bright);
      gtk_label_set_markup (wf->common.black_value, text_black);
      break;

    case FNN_PANEL_FORWARDLOOK:
      fl = (VisualFL*)panel->vis_gui;
      hyscan_gtk_forward_look_set_brightness (fl->fl, new_brightness);
      gtk_label_set_markup (fl->common.white_value, text_bright);
      break;

    default:
      g_warning ("brightness_set: wrong panel type!");
    }

  g_free (text_bright);
  g_free (text_black);

  return TRUE;
}

void
evo_project_changed_override (Global      *global,
                              const gchar *project)
{
  hyscan_gtk_model_manager_set_project_name (_global->model_manager, project);
}
/***
 *     #     # ######     #    ######  ######  ####### ######   #####
 *     #  #  # #     #   # #   #     # #     # #       #     # #     #
 *     #  #  # #     #  #   #  #     # #     # #       #     # #
 *     #  #  # ######  #     # ######  ######  #####   ######   #####
 *     #  #  # #   #   ####### #       #       #       #   #         #
 *     #  #  # #    #  #     # #       #       #       #    #  #     #
 *      ## ##  #     # #     # #       #       ####### #     #  #####
 *
 */

void
exit_or_restart (gpointer _restart)
{
  if (GPOINTER_TO_INT (_restart))
    _global->request_restart = TRUE;

  gtk_widget_destroy (_global->gui.window);
}

void
magnifier_x2 (GtkToggleButton             *button,
              HyScanGtkWaterfallMagnifier *magn)
{
  if (!gtk_toggle_button_get_active (button))
    return;

  hyscan_gtk_waterfall_magnifier_set_zoom (magn, 2);
}

void
magnifier_x3 (GtkToggleButton             *button,
              HyScanGtkWaterfallMagnifier *magn)
{
  if (!gtk_toggle_button_get_active (button))
    return;

  hyscan_gtk_waterfall_magnifier_set_zoom (magn, 3);
}

void
player_adj_value_changed (GtkAdjustment            *adj,
                          HyScanGtkWaterfallPlayer *player)
{
  hyscan_gtk_waterfall_player_set_speed (player, gtk_adjustment_get_value (adj));
}

void
player_adj_value_printer (GtkAdjustment *adj,
                          GtkLabel      *label)
{
  gchar *markup;

  markup = g_strdup_printf ("<small><b>%2.0f %s</b></small>", gtk_adjustment_get_value(adj), _("m/s"));
  gtk_label_set_markup (label, markup);
  g_free (markup);
}

void
player_stop (GtkAdjustment *adjustment)
{
  gtk_adjustment_set_value (adjustment, 0);
}

void
player_slower (GtkAdjustment *adjustment)
{
  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_value (adjustment) - 1.0);
}

void
player_faster (GtkAdjustment *adjustment)
{
  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_value (adjustment) + 1.0);
}

void
map_offline_wrapper (GObject *emitter,
                     HyScanGtkMapBuilder *map)
{
  hyscan_gtk_map_builder_set_offline (map,
                                      !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (emitter)));
}

void
menu_dry_wrapper (GObject *emitter)
{
  gboolean state = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (emitter));

  set_dry (_global, state);
}

static void
sensor_toggle_wrapper (GtkCheckMenuItem *mitem,
                       const gchar      *name)
{
  gboolean active = gtk_check_menu_item_get_active (mitem);

  g_signal_handlers_block_by_func (mitem, sensor_toggle_wrapper, (gpointer) name);
  if (hyscan_sensor_set_enable (HYSCAN_SENSOR (_global->control), name, active))
    {
      g_message ("Sensor %s is now %s", name, active ? "ON" : "OFF");
      gtk_check_menu_item_set_active (mitem, active);
    }
  else
    {
      g_message ("Couldn't turn sensor %s %s", name, active ? "ON" : "OFF");
      gtk_check_menu_item_set_active (mitem, !active);
    }
  g_signal_handlers_unblock_by_func (mitem, sensor_toggle_wrapper, (gpointer) name);
}

/* Обработчик сигнала о старте или остановке работы гидролокатора. */
void
ui_start_stop (HyScanControlModel *sonar_model)
{
  EvoUI *ui = &global_ui;
  gboolean state;

  state = hyscan_sonar_state_get_start (HYSCAN_SONAR_STATE (sonar_model), NULL, NULL, NULL, NULL);

  gtk_widget_set_sensitive (ui->starter.dry_switch, !state);
  gtk_widget_set_sensitive (GTK_WIDGET (ui->starter.dry_menu), !state);
}

void
balance_changed (GtkAdjustment *adj,
                 gpointer       udata)
{
  gint panelx = GPOINTER_TO_INT (udata);
  FnnPanel *panel = get_panel (_global, panelx);

  levels_set (_global, panel->vis_current.white, panel->vis_current.gamma, panel->vis_current.black, panelx);
}

void
remove_track_select (void)
{
  guint tag = global_ui.track_select_tag;

  global_ui.track_select_tag = 0;
  if (tag != 0)
    g_source_remove (tag);
}

gboolean
track_select (Global *global)
{
  EvoUI *ui = &global_ui;
  GtkTreePath *path = NULL, *path2 = NULL;
  GtkTreeIter iter;
  gchar *track_name = NULL;

  track_name = hyscan_gtk_map_builder_get_selected_track (ui->map_builder);

  if (track_name == NULL)
    goto exit;

  if (!gtk_tree_model_get_iter_first (global->gui.track.list, &iter))
    goto exit;

  do
    {
      gchar *track;
      gtk_tree_model_get (global->gui.track.list, &iter, FNN_TRACK_COLUMN, &track, -1);

      if (0 != g_strcmp0 (track, track_name))
        continue;


      path2 = gtk_tree_model_get_path (global->gui.track.list, &iter);
      gtk_tree_view_set_cursor (global->gui.track.tree, path2, NULL, FALSE);
    }
  while (gtk_tree_model_iter_next (global->gui.track.list, &iter));

exit:
  g_free (track_name);
  gtk_tree_path_free (path);
  gtk_tree_path_free (path2);
  global_ui.track_select_tag = 0;
  return G_SOURCE_REMOVE;
}

gboolean
add_track_select (Global *global)
{
  remove_track_select ();

  global_ui.track_select_tag = g_timeout_add (100, (GSourceFunc)track_select, global);

  return TRUE;
}
//  #####  ####### #     # ####### ######  ####### #
// #     # #     # ##    #    #    #     # #     # #
// #       #     # # #   #    #    #     # #     # #
// #       #     # #  #  #    #    ######  #     # #
// #       #     # #   # #    #    #   #   #     # #
// #     # #     # #    ##    #    #    #  #     # #
//  #####  ####### #     #    #    #     # ####### #######

/* функция переключает виджеты. */
void
widget_swap (GObject     *emitter,
             GParamSpec  *pspec,
             const gchar *child)
{
  EvoUI *ui = &global_ui;

  if (child == NULL)
    child = gtk_stack_get_visible_child_name (GTK_STACK (ui->acoustic_stack));
  if (child == NULL)
    return;

  if (g_str_equal (child, EVO_MAP))
    gtk_stack_set_visible_child_name (GTK_STACK (ui->nav_stack), EVO_MAP);
  else
    gtk_stack_set_visible_child_name (GTK_STACK (ui->nav_stack), EVO_NOT_MAP);

  gtk_stack_set_visible_child_name (GTK_STACK (ui->control_stack), child); // fixme!11;
  /* Теперь всякие специальные случаи. */
  {
    gboolean bottom_visible = FALSE;
    bottom_visible = g_str_equal (child, "ForwardLook") |
                     g_str_has_prefix (child, "LookAround");
    hyscan_gtk_area_set_bottom_visible (HYSCAN_GTK_AREA (ui->area), bottom_visible);
  }


}

void
run_offset_setup (GObject *emitter,
                  Global  *global)
{
  GtkWidget *dialog;

  dialog = hyscan_gtk_fnn_offsets_new (global->control, GTK_WINDOW (global->gui.window));
  gtk_widget_show_all (dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}

//   #     #    #    ### #     #
//   ##   ##   # #    #  ##    #
//   # # # #  #   #   #  # #   #
//   #  #  # #     #  #  #  #  #
//   #     # #######  #  #   # #
//   #     # #     #  #  #    ##
//   #     # #     # ### #     #

GObject *
get_from_builder (GtkBuilder * builder,
                  const gchar *name)
{
  GObject *obj = NULL;

  if (name == NULL)
    return NULL;

  obj = gtk_builder_get_object (builder, name);
  if (obj == NULL)
    {
      g_warning ("EvoUI: <%s> not found.", name);
      return NULL;
    }

  return obj;
}

GtkWidget *
get_widget_from_builder (GtkBuilder  * builder,
                         const gchar * name)
{
  return GTK_WIDGET (get_from_builder (builder, name));
}

GtkLabel *
get_label_from_builder (GtkBuilder  * builder,
                        const gchar * name)
{
  return GTK_LABEL (get_from_builder (builder, name));
}

GtkAdjustment *
get_adjust_from_builder (GtkBuilder * builder,
                        const gchar * name)
{
  return GTK_ADJUSTMENT (get_from_builder (builder, name));
}

GtkWidget *
make_stack (void)
{
  GtkWidget *stack;

  stack = gtk_stack_new ();
  gtk_stack_set_transition_type (GTK_STACK (stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 100);

  return stack;
}

GtkWidget *
make_layer_list (EvoUI *ui,
                 VisualWF *vwf)
{
  GtkWidget *layer_list;

  layer_list = hyscan_gtk_layer_list_new (HYSCAN_GTK_LAYER_CONTAINER (vwf->wf));

  hyscan_gtk_layer_set_visible (HYSCAN_GTK_LAYER (vwf->wf_magn), FALSE);

  /* Регистрируем слой в layer_store. */
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (layer_list), HYSCAN_GTK_LAYER (vwf->wf_grid), EVO_GRID_KEY, _("Grid"));
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (layer_list), HYSCAN_GTK_LAYER (vwf->wf_mark), EVO_MARK_KEY, _("Marks"));
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (layer_list), HYSCAN_GTK_LAYER (vwf->wf_metr), EVO_METR_KEY, _("Measurements"));
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (layer_list), HYSCAN_GTK_LAYER (vwf->wf_shad), EVO_SHAD_KEY, _("Shadow measure"));
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (layer_list), HYSCAN_GTK_LAYER (vwf->wf_coor), EVO_COOR_KEY, _("Coordinates"));
  hyscan_gtk_layer_list_add (HYSCAN_GTK_LAYER_LIST (layer_list), HYSCAN_GTK_LAYER (vwf->wf_magn), EVO_MAGN_KEY, _("Magnifier"));

  hyscan_gtk_layer_list_set_visible_fn (HYSCAN_GTK_LAYER_LIST (layer_list), EVO_GRID_KEY,
                                        (hyscan_gtk_layer_list_visible_fn) hyscan_gtk_waterfall_grid_show_grid);

  {
    GtkWidget *radio_x2, *radio_x3, *radio_box;

    radio_x2 = gtk_radio_button_new_with_label (NULL, "x2");
    radio_x3 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio_x2), "x3");

    g_object_set (radio_x2, "draw-indicator", FALSE, NULL);
    g_object_set (radio_x3, "draw-indicator", FALSE, NULL);

    g_signal_connect (radio_x2, "toggled", G_CALLBACK (magnifier_x2), vwf->wf_magn);
    g_signal_connect (radio_x3, "toggled", G_CALLBACK (magnifier_x3), vwf->wf_magn);

    radio_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class (gtk_widget_get_style_context (radio_box), "linked");
    gtk_box_pack_end (GTK_BOX (radio_box), radio_x3, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (radio_box), radio_x2, FALSE, FALSE, 0);

    hyscan_gtk_layer_list_set_tools (HYSCAN_GTK_LAYER_LIST (layer_list), EVO_MAGN_KEY, radio_box);
  }

  gtk_widget_show_all (layer_list);

  return layer_list;
}

GtkWidget *
make_record_control (Global *global,
                     EvoUI   *ui)
{
  GtkBuilder *b;
  GtkWidget *w, *rec_switch;

  if (global->sonar_model == NULL)
    return NULL;

  /* Обработчик выполняет действия, при смене статуса записи. */
  g_signal_connect (global->sonar_model, "start-stop", G_CALLBACK (ui_start_stop), NULL);

  b = gtk_builder_new_from_resource ("/org/evo/gtk/record.ui");

  w = g_object_ref (get_widget_from_builder (b, "record_control"));
  ui->starter.dry_switch = g_object_ref (get_widget_from_builder (b, "start_stop_dry"));
  rec_switch = hyscan_gtk_rec_new (global->sonar_model);

  /* Помещаем виджет включения записи в GtkGrid. */
  gtk_widget_set_halign (rec_switch, GTK_ALIGN_CENTER);
  gtk_grid_attach (GTK_GRID (w), rec_switch, 1, 1, 1, 1);

  {
    gchar ** env;
    const gchar * param_set;

    env = g_get_environ ();
    param_set = g_environ_getenv (env, EVO_ENABLE_EXTRAS);
    if (param_set != NULL)
      {
        gtk_widget_set_visible (get_widget_from_builder (b, "dry_label"), TRUE);
        gtk_widget_set_visible (ui->starter.dry_switch, TRUE);
      }
    g_strfreev (env);
  }

  g_object_unref (b);

  return w;
}


GtkBuilder *
get_builder_for_panel (EvoUI * ui,
                       gint    panelx)
{
  GtkBuilder * b;
  gpointer k = GINT_TO_POINTER (panelx);

  b = g_hash_table_lookup (ui->builders, k);

  if (b != NULL)
    return b;

  b = gtk_builder_new_from_resource ("/org/evo/gtk/evo.ui");
  gtk_builder_set_translation_domain (b, GETTEXT_PACKAGE);
  g_hash_table_insert (ui->builders, k, b);

  return b;
}

void
add_to_sg (GtkSizeGroup *sg,
           GtkBuilder   *b,
           const gchar  *item)
{
  GtkWidget *w = get_widget_from_builder (b, item);
  gtk_size_group_add_widget (sg, w);
}

#define EVO_TVG_VALUE(value) panel->gui. value = get_label_from_builder  (b, #value); add_to_sg (sg, b, #value);

GtkWidget *
make_tvg_control (Global *global,
                  FnnPanel *panel,
                  GtkBuilder *b,
                  GtkSizeGroup *sg)
{
  const gchar * tvg_control_name;
  HyScanSonarTVGModeType caps, preferred = HYSCAN_SONAR_TVG_MODE_NONE;
  const HyScanSonarInfoSource * info;
  HyScanSourceType *sources = panel->sources;
  gboolean auto_ok = TRUE, log_ok = TRUE, lin_ok = TRUE, const_ok = TRUE;

  for (; sources != NULL && *sources != HYSCAN_SOURCE_INVALID; ++sources)
    {
      info = g_hash_table_lookup (global->sonar_infos, GINT_TO_POINTER (*sources));
      if (info->tvg == NULL)
        {
          source_informer ("No TVG info", *sources);
          continue;
        }

      caps = info->tvg->capabilities;
      auto_ok  &= TVG_TEST (caps, HYSCAN_SONAR_TVG_MODE_AUTO);
      lin_ok   &= TVG_TEST (caps, HYSCAN_SONAR_TVG_MODE_LINEAR_DB);
      log_ok   &= TVG_TEST (caps, HYSCAN_SONAR_TVG_MODE_LOGARITHMIC);
      const_ok &= TVG_TEST (caps, HYSCAN_SONAR_TVG_MODE_CONSTANT);
    }

  g_message ("Panel <%s>, TVG: AUTO %i; LIN %i; LOG %i CONST %i",
             panel->name, auto_ok, lin_ok, log_ok, const_ok);

  if (panel->panelx == X_ECHOSOUND || panel->panelx == X_ECHO_LOW || panel->panelx == X_ECHO_HIGH)
    {
      if (auto_ok)
        {
          tvg_control_name = "auto_tvg_control";
          panel->gui.tvg_level_value = get_label_from_builder  (b, "tvg_level_value");     add_to_sg (sg, b, "tvg_level_label");
          panel->gui.tvg_sens_value  = get_label_from_builder  (b, "tvg_sens_value");      add_to_sg (sg, b, "tvg_sensitivity_label");
          preferred = HYSCAN_SONAR_TVG_MODE_AUTO;
        }
      else if (log_ok)
        {
          tvg_control_name = "log_tvg_control";
          panel->gui.logtvg_gain0_value = get_label_from_builder  (b, "logtvg_gain0_value");     add_to_sg (sg, b, "logtvg_gain0_label");
          panel->gui.logtvg_beta_value  = get_label_from_builder  (b, "logtvg_beta_value");      add_to_sg (sg, b, "logtvg_beta_label");
          panel->gui.logtvg_alpha_value  = get_label_from_builder  (b, "logtvg_alpha_value");    add_to_sg (sg, b, "logtvg_alpha_label");
          preferred = HYSCAN_SONAR_TVG_MODE_LOGARITHMIC;
        }
      else if (lin_ok)
        {
          tvg_control_name = "lin_tvg_control";
          panel->gui.tvg_value  = get_label_from_builder  (b, "tvg_value");          add_to_sg (sg, b, "tvg_label");
          panel->gui.tvg0_value = get_label_from_builder  (b, "tvg0_value");         add_to_sg (sg, b, "tvg0_label");
          preferred = HYSCAN_SONAR_TVG_MODE_LINEAR_DB;
        }
      else if (const_ok)
        {
          tvg_control_name = "const_tvg_control";
          panel->gui.consttvg_value  = get_label_from_builder  (b, "consttvg_value");      add_to_sg (sg, b, "consttvg_label");
          preferred = HYSCAN_SONAR_TVG_MODE_CONSTANT;
        }
      else
        {
          tvg_control_name = NULL;
          return NULL;
        }
    }
  else
    {
      if (auto_ok)
        {
          tvg_control_name = "auto_tvg_control";
          panel->gui.tvg_level_value = get_label_from_builder  (b, "tvg_level_value");     add_to_sg (sg, b, "tvg_level_label");
          panel->gui.tvg_sens_value  = get_label_from_builder  (b, "tvg_sens_value");      add_to_sg (sg, b, "tvg_sensitivity_label");
          preferred = HYSCAN_SONAR_TVG_MODE_AUTO;
        }
      else if (lin_ok)
        {
          tvg_control_name = "lin_tvg_control";
          panel->gui.tvg_value  = get_label_from_builder  (b, "tvg_value");          add_to_sg (sg, b, "tvg_label");
          panel->gui.tvg0_value = get_label_from_builder  (b, "tvg0_value");         add_to_sg (sg, b, "tvg0_label");
          preferred = HYSCAN_SONAR_TVG_MODE_LINEAR_DB;
        }
      else if (log_ok)
        {
          tvg_control_name = "log_tvg_control";
          panel->gui.logtvg_gain0_value = get_label_from_builder  (b, "logtvg_gain0_value");     add_to_sg (sg, b, "logtvg_gain0_label");
          panel->gui.logtvg_beta_value  = get_label_from_builder  (b, "logtvg_beta_value");      add_to_sg (sg, b, "logtvg_beta_label");
          panel->gui.logtvg_alpha_value  = get_label_from_builder  (b, "logtvg_alpha_value");    add_to_sg (sg, b, "logtvg_alpha_label");
          preferred = HYSCAN_SONAR_TVG_MODE_LOGARITHMIC;
        }
      else if (const_ok)
        {
          tvg_control_name = "const_tvg_control";
          panel->gui.consttvg_value  = get_label_from_builder  (b, "consttvg_value");      add_to_sg (sg, b, "consttvg_label");
          preferred = HYSCAN_SONAR_TVG_MODE_CONSTANT;
        }
      else
        {
          tvg_control_name = NULL;
          return NULL;
        }
    }

  tvg_set_preferred_mode (global, preferred, panel->panelx);
  return get_widget_from_builder (b, tvg_control_name);
}

/* Ядро всего уйца. Подключает сигналы, достает виджеты. */
GtkWidget *
make_page_for_panel (EvoUI     *ui,
                     FnnPanel *panel,
                     gint      panelx,
                     Global   *global)
{
  GtkBuilder *b;
  GtkWidget *view = NULL, *sonar = NULL, *tvg = NULL, *layers = NULL;
  GtkWidget *extra = NULL;
  GtkWidget *box, *scroll;
  VisualWF *wf;
  VisualLA *vla;
  GtkSizeGroup * sg;

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  b = get_builder_for_panel (ui, panelx);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  if (panel_sources_are_in_sonar (global, panel))
    {
      sonar = get_widget_from_builder (b, "sonar_control");
      tvg = make_tvg_control (global, panel, b, sg);
      panel->gui.distance_value         = get_label_from_builder  (b, "distance_value");      add_to_sg (sg, b, "distance_label");
      panel->gui.signal_value           = get_label_from_builder  (b, "signal_value");        add_to_sg (sg, b, "signal_label");
                                                                                              add_to_sg (sg, b, "disable_label");
    }

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:

      view = get_widget_from_builder (b, "ss_view_control");
      panel->vis_gui->white_value       = get_label_from_builder (b, "ss_white_value");  add_to_sg (sg, b, "ss_white_label");
      // panel->vis_gui->black_value       = get_label_from_builder (b, "ss_black_value");       add_to_sg (sg, b, "ss_black_label");
      panel->vis_gui->gamma_value       = get_label_from_builder (b, "ss_gamma_value");       add_to_sg (sg, b, "ss_gamma_label");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "ss_scale_value");       add_to_sg (sg, b, "ss_scale_label");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "ss_color_map_value");   add_to_sg (sg, b, "ss_color_map_label");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "ss_live_view");         add_to_sg (sg, b, "ss_live_view_label");

      wf = (VisualWF*)panel->vis_gui;
      layers = make_layer_list (ui, wf);

      {
        GtkLabel *label = get_label_from_builder (b, "ss_player_value");
        GtkAdjustment *adj = get_adjust_from_builder (b, "ss_player_adjustment");             add_to_sg (sg, b, "ss_player_label");
        g_signal_connect (adj, "value-changed", G_CALLBACK (player_adj_value_changed), wf->wf_play);
        g_signal_connect (adj, "value-changed", G_CALLBACK (player_adj_value_printer), label);
        g_signal_connect_swapped (wf->wf_play, "player-stop", G_CALLBACK (player_stop), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "ss_player_stop"), "clicked", G_CALLBACK (player_stop), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "ss_player_slower"), "clicked", G_CALLBACK (player_slower), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "ss_player_faster"), "clicked", G_CALLBACK (player_faster), adj);

        player_adj_value_printer (adj, label);
      }

      break;

    case FNN_PANEL_PROFILER:

      view = get_widget_from_builder (b, "pf_view_control");
      panel->vis_gui->white_value  = get_label_from_builder (b, "pf_white_value");  add_to_sg (sg, b, "pf_white_label");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "pf_scale_value");       add_to_sg (sg, b, "pf_scale_label");
      panel->vis_gui->black_value       = get_label_from_builder (b, "pf_black_value");       add_to_sg (sg, b, "pf_black_label");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "pf_live_view");         add_to_sg (sg, b, "pf_live_view_label");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "pf_color_map_value");   add_to_sg (sg, b, "pf_color_map_label");


      wf = (VisualWF*)panel->vis_gui;
      layers =  make_layer_list (ui, wf);

      {
        GtkLabel *label = get_label_from_builder (b, "pf_player_value");
        GtkAdjustment *adj = get_adjust_from_builder (b, "pf_player_adjustment");             add_to_sg (sg, b, "pf_player_label");
        g_signal_connect (adj, "value-changed", G_CALLBACK (player_adj_value_changed), wf->wf_play);
        g_signal_connect (adj, "value-changed", G_CALLBACK (player_adj_value_printer), label);
        g_signal_connect_swapped (wf->wf_play, "player-stop", G_CALLBACK (player_stop), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "pf_player_stop"), "clicked", G_CALLBACK (player_stop), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "pf_player_slower"), "clicked", G_CALLBACK (player_slower), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "pf_player_faster"), "clicked", G_CALLBACK (player_faster), adj);

        player_adj_value_printer (adj, label);
      }

      break;

    case FNN_PANEL_ECHO:

      view = get_widget_from_builder (b, "ss_view_control");
      panel->vis_gui->white_value       = get_label_from_builder (b, "ss_white_value");  add_to_sg (sg, b, "ss_white_label");
      // panel->vis_gui->black_value       = get_label_from_builder (b, "ss_black_value");       add_to_sg (sg, b, "ss_black_label");
      panel->vis_gui->gamma_value       = get_label_from_builder (b, "ss_gamma_value");       add_to_sg (sg, b, "ss_gamma_label");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "ss_scale_value");       add_to_sg (sg, b, "ss_scale_label");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "ss_color_map_value");   add_to_sg (sg, b, "ss_color_map_label");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "ss_live_view");         add_to_sg (sg, b, "ss_live_view_label");

      wf = (VisualWF*)panel->vis_gui;
      layers =  make_layer_list (ui, wf);

      {
        GtkLabel *label = get_label_from_builder (b, "ss_player_value");
        GtkAdjustment *adj = get_adjust_from_builder (b, "ss_player_adjustment");             add_to_sg (sg, b, "ss_player_label");
        g_signal_connect (adj, "value-changed", G_CALLBACK (player_adj_value_changed), wf->wf_play);
        g_signal_connect (adj, "value-changed", G_CALLBACK (player_adj_value_printer), label);
        g_signal_connect_swapped (wf->wf_play, "player-stop", G_CALLBACK (player_stop), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "ss_player_stop"), "clicked", G_CALLBACK (player_stop), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "ss_player_slower"), "clicked", G_CALLBACK (player_slower), adj);
        g_signal_connect_swapped (get_widget_from_builder(b, "ss_player_faster"), "clicked", G_CALLBACK (player_faster), adj);
        player_adj_value_printer (adj, label);
      }

      break;

    case FNN_PANEL_FORWARDLOOK:

      view = get_widget_from_builder (b, "fl_view_control");
      panel->vis_gui->white_value       = get_label_from_builder (b, "fl_brightness_value");  add_to_sg (sg, b, "fl_brightness_label");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "fl_scale_value");       add_to_sg (sg, b, "fl_scale_label");
      panel->vis_gui->sensitivity_value = get_label_from_builder (b, "fl_sensitivity_value"); add_to_sg (sg, b, "fl_sensitivity_label");

      // if (!panel_sources_are_in_sonar (global, panel))
      //   break;

      // sonar = get_widget_from_builder (b, "sonar_control");
      // tvg = make_tvg_control (global, panel, b, sg);
      // panel->gui.distance_value         = get_label_from_builder  (b, "distance_value");       add_to_sg (sg, b, "distance_label");
      // panel->gui.signal_value           = get_label_from_builder  (b, "signal_value");         add_to_sg (sg, b, "signal_label");

      break;

    case FNN_PANEL_LOOKAROUND:
      view = get_widget_from_builder (b, "la_view_control");
      // panel->vis_gui->black_value       = get_label_from_builder (b, "la_black_value");  add_to_sg (sg, b, "la_black_value");
      panel->vis_gui->white_value       = get_label_from_builder (b, "la_white_value");  add_to_sg (sg, b, "la_white_label");
      panel->vis_gui->gamma_value       = get_label_from_builder (b, "la_gamma_value");  add_to_sg (sg, b, "la_gamma_label");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "la_scale_value");  add_to_sg (sg, b, "la_scale_label");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "la_color_map_value");  add_to_sg (sg, b, "la_color_map_label");
                                                                                             add_to_sg (sg, b, "la_axis_rot_label");
                                                                                             add_to_sg (sg, b, "la_image_rot_label");
                                                                                             add_to_sg (sg, b, "la_depth_label");


      vla = (VisualLA *)panel->vis_gui;
      vla->la_a_degrees = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_a_degrees")));
      vla->la_a_meters = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_a_meters")));
      vla->la_b_degrees = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_b_degrees")));
      vla->la_b_meters = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_b_meters")));
      vla->la_ab_degrees = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_ab_degrees")));
      vla->la_ab_meters = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_ab_meters")));
      vla->la_ba_degrees = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_ba_degrees")));

      g_signal_connect (vla->glikoview, "ruler-changed", G_CALLBACK (la_ruler_changed_cb), vla);

      vla->la_player_scale = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_player_scale")));
      vla->la_pause_toggle = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_pause_toggle")));
      vla->la_x10_toggle = GTK_WIDGET (g_object_ref (gtk_builder_get_object (b, "la_x10_toggle")));

      g_signal_connect (vla->la_player_scale, "value-changed", G_CALLBACK (la_player_scale_cb), vla);
      g_signal_connect (vla->la_pause_toggle, "toggled", G_CALLBACK (la_pause_toggle_cb), vla);
      g_signal_connect (vla->la_x10_toggle, "toggled", G_CALLBACK (la_x10_toggle_cb), vla);

      g_signal_connect (vla->player, "range", G_CALLBACK (la_player_range_cb), vla);
      g_signal_connect (vla->player, "ready", G_CALLBACK (la_player_ready_cb), vla);

      vla->speed = 1.0;
      vla->player_min = 0;
      vla->player_position_changed = 0;

      if (panel_sources_are_in_sonar (global, panel))
        {
          const gchar *a1, *a2;
          a1 = hyscan_gtk_actuator_control_find_actuator_by_source (global->sonar_model, panel->sources[0]);
          a2 = hyscan_gtk_actuator_control_find_actuator_by_source (global->sonar_model, panel->sources[1]);
          if (0 && ((a1 == NULL && a2 == NULL) || 0 != g_strcmp0 (a1, a2)))
            {
              g_message ("Different actuators :(");
            }
          else
            {
              extra = hyscan_gtk_actuator_control_new (global->sonar_model, a1);
            }
          hyscan_data_player_play (vla->player, vla->speed);
        }
      break;

    default:
      g_warning ("EvoUI: wrong panel type!");
    }

  gtk_builder_connect_signals (b, GINT_TO_POINTER (panelx));

  gtk_box_pack_start (GTK_BOX (box), view, FALSE, FALSE, 0);
  if (layers != NULL)
    gtk_box_pack_start (GTK_BOX (box), layers, FALSE, FALSE, 0);
  if (extra != NULL)
    gtk_box_pack_end (GTK_BOX (box), extra, FALSE, FALSE, 0);
  if (tvg != NULL)
    gtk_box_pack_end (GTK_BOX (box), tvg, FALSE, FALSE, 0);
  if (sonar != NULL)
    gtk_box_pack_end (GTK_BOX (box), sonar, FALSE, TRUE, 0);


  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroll), box);

  gtk_widget_show_all (GTK_WIDGET (scroll));

  return scroll;
}

/* В списке меток на карте поменялась выбранная метка. */
static void
mark_selected (HyScanGtkMarkEditor *mark_editor)
{
  HyScanMark mark = {0};
  HyScanGeoPoint location = {0};
  gchar *id;
  GtkWidget *mark_list;

  mark_list = hyscan_gtk_map_builder_get_mark_list (global_ui.map_builder);
  id = hyscan_gtk_map_mark_list_get_selected (HYSCAN_GTK_MAP_MARK_LIST (mark_list), &mark, &location);
  hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (mark_editor), id,
                                   mark.name, mark.operator_name, mark.description,
                                   location.lat, location.lon);
  g_free (id);
}

/* Самая крутая функция, строит весь уй. */
G_MODULE_EXPORT gboolean
build_interface (Global *global)
{
  EvoUI *ui = &global_ui;
  _global = global;

  GtkWidget *settings;
  GtkWidget *rbox;

  // global->override.brightness_set = evo_brightness_set_override;
  global->override.project_changed = evo_project_changed_override;

  /* EvoUi это грид, вверху стек-свитчер с кнопками панелей,
   * внизу HyScanGtkArea. Внутри арии:
   * Справа у нас управление локаторами и отображением,
   * слева выезжает контроль над галсами и метками
   * (причем там тоже стек, т.к. у карты упр-е своё),
   * снизу выезжает управление ВСЛ.
   * под арией навиндикатор. */
  ui->grid = gtk_grid_new ();

  ui->area = hyscan_gtk_area_new ();

  /* Центральная зона: виджет с виджетами. Да. */
  ui->nav_stack = make_stack ();
  ui->acoustic_stack = make_stack ();
  g_object_ref (ui->acoustic_stack);

  /* Связываем стеки. */
  g_signal_connect (ui->acoustic_stack, "notify::visible-child-name", G_CALLBACK (widget_swap), NULL);

  /* Cтек для управления локаторами. */
  rbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  ui->control_stack = make_stack ();

  /* Особенности реализации: хеш-таблица билдеров.*/
  ui->builders = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

  /* На данный момент все контейнеры готовы. Можно наполнять их. */

  /* Карта всегда в наличии. */
  {
    GtkWidget *box;
    GtkTreeView * tv;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    ui->map_builder = hyscan_gtk_map_builder_new (global->model_manager);
    hyscan_gtk_map_builder_add_grid (ui->map_builder);
    hyscan_gtk_map_builder_add_tracks (ui->map_builder);
    hyscan_gtk_map_builder_add_marks (ui->map_builder);
    hyscan_gtk_map_builder_add_pin (ui->map_builder);
    hyscan_gtk_map_builder_add_ruler (ui->map_builder);
    hyscan_gtk_map_builder_add_planner (ui->map_builder, TRUE);
    hyscan_gtk_map_builder_add_planner_list (ui->map_builder);

    /* Добавляем инструменты управления картой. */
    {
      GtkWidget *preload, *go;
      GtkWidget *notebook;
      GtkWidget *map = GTK_WIDGET (hyscan_gtk_map_builder_get_map (ui->map_builder));
      const gchar *base_id = "base";

      notebook = gtk_notebook_new ();

      go = hyscan_gtk_map_go_new (HYSCAN_GTK_MAP (map));
      g_object_set (go, "margin", 6, NULL);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), go, gtk_label_new (_("Go to")));

      preload = hyscan_gtk_map_preload_new (HYSCAN_GTK_MAP (map), base_id);
      g_object_set (preload, "margin", 6, NULL);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), preload, gtk_label_new (_("Download")));

      hyscan_gtk_map_builder_layer_set_tools (ui->map_builder, base_id, notebook);
    }

    /* Добавляем редактор меток. */
    {
      GtkWidget *mark_editor;
      mark_editor = hyscan_gtk_mark_editor_new (global->units);
      g_signal_connect (mark_editor, "mark-modified", G_CALLBACK (mark_modified), global);
      g_signal_connect_swapped (hyscan_gtk_map_builder_get_mark_list (ui->map_builder),
                                "changed", G_CALLBACK (mark_selected), mark_editor);
      gtk_box_pack_start (GTK_BOX (hyscan_gtk_map_builder_get_left_col (ui->map_builder)), mark_editor, FALSE, FALSE, 0);
    }

    gtk_stack_add_named (GTK_STACK (ui->control_stack),
                         hyscan_gtk_map_builder_get_right_col (ui->map_builder), EVO_MAP);
    gtk_stack_add_named (GTK_STACK (ui->nav_stack),
                         hyscan_gtk_map_builder_get_left_col (ui->map_builder), EVO_MAP);
    gtk_box_pack_start (GTK_BOX (box),
                        hyscan_gtk_map_builder_get_central (ui->map_builder), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (box),
                        hyscan_gtk_map_builder_get_bar (ui->map_builder),
                        FALSE, TRUE, 0);
    gtk_stack_add_titled (GTK_STACK (ui->acoustic_stack), box, EVO_MAP, _("Map"));
    gtk_stack_set_visible_child_name(GTK_STACK (ui->acoustic_stack), EVO_MAP);

    tv = GTK_TREE_VIEW (hyscan_gtk_map_builder_get_track_list (ui->map_builder));
    g_signal_connect_swapped (tv, "cursor-changed", G_CALLBACK (add_track_select), global);
  }


  /* Stack switcher */
  {
    ui->switcher = gtk_stack_switcher_new ();
    g_object_set (ui->switcher, "margin", 6,
                  "hexpand", TRUE, "halign", GTK_ALIGN_CENTER, NULL);
    gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (ui->switcher), GTK_STACK (ui->acoustic_stack));
  }

  /* Левая панель содержит список галсов, меток и редактор меток. */
  {
    GtkWidget *lbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6),
              *tracks = GTK_WIDGET (global->gui.track.view),
              *mlist = GTK_WIDGET (global->gui.mark_view),
              /**mark_manager = hyscan_gtk_mark_manager_new (global->model_manager),*/
              *meditor = GTK_WIDGET (global->gui.meditor);

    gtk_widget_set_margin_end (lbox, 6);
    gtk_widget_set_margin_top (lbox, 0);
    gtk_widget_set_margin_bottom (lbox, 0);

    g_object_set (tracks, "vexpand", FALSE,  "valign", GTK_ALIGN_FILL,
                          "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mlist, "vexpand", FALSE,  "valign", GTK_ALIGN_FILL,
                         "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (meditor, "vexpand", FALSE, "valign", GTK_ALIGN_END,
                           "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);

    gtk_box_pack_start (GTK_BOX (lbox), tracks, TRUE, TRUE, 0);
    /*gtk_box_pack_start (GTK_BOX (lbox), tracks, FALSE, TRUE, 0);*/
    gtk_box_pack_start (GTK_BOX (lbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), mlist, TRUE, TRUE, 0);
    /*gtk_box_pack_start (GTK_BOX (lbox), mlist, FALSE, TRUE, 0);*/
    /* Журнал меток. */
    /*gtk_box_pack_start (GTK_BOX (lbox), mark_manager, FALSE, TRUE, 0);*/
    /*gtk_box_pack_start (GTK_BOX (lbox), mark_manager, TRUE, TRUE, 0);*/
    gtk_box_pack_start (GTK_BOX (lbox), meditor, FALSE, FALSE, 0);
    // gtk_box_pack_start (GTK_BOX (lbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

    gtk_stack_add_named (GTK_STACK (ui->nav_stack), lbox, EVO_NOT_MAP);
  }

  /* Правая панель -- бокс с упр. записью и упр. локаторами. */
  /* Кстати, мои личные виджеты используются только в этом месте.  */
  ui->balance_table = g_hash_table_new (g_direct_hash, g_direct_equal);
  {
    GtkWidget * record;

    gtk_widget_set_margin_start (rbox, 6);
    gtk_widget_set_margin_top (rbox, 0);
    gtk_widget_set_margin_bottom (rbox, 0);

    record = make_record_control (global, ui);

    gtk_box_pack_start (GTK_BOX (rbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (rbox), ui->control_stack, TRUE, TRUE, 0);
    if (record != NULL)
      gtk_box_pack_start (GTK_BOX (rbox), record, FALSE, FALSE, 0);
  }

  /* Настройки 2.0. */
  {
    gint t = 0;
    GtkWidget *mitem;
    GtkWidget *menu = gtk_menu_new ();
    settings = gtk_menu_button_new ();

    gtk_container_add (GTK_CONTAINER (settings),
                       gtk_image_new_from_icon_name ("open-menu-symbolic",
                                                     GTK_ICON_SIZE_MENU));
    gtk_menu_button_set_popup (GTK_MENU_BUTTON (settings), menu);
    g_object_set (settings, "margin", 6, NULL);

    /* менеджер проектов */
    mitem = gtk_menu_item_new_with_label (_("Project Manager"));
    g_signal_connect (mitem, "activate", G_CALLBACK (run_manager), NULL);
    gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

    /* Журнал Меток. */
    mitem = gtk_menu_item_new_with_label (_("Mark Manager"));
    g_signal_connect (mitem, "activate", G_CALLBACK (run_mark_manager), NULL);
    gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

    /* офлайн-карта */
    mitem = gtk_check_menu_item_new_with_label (_("Online Map"));
    ui->map_offline = GTK_CHECK_MENU_ITEM (g_object_ref (mitem));
    g_signal_connect (mitem, "toggled", G_CALLBACK (map_offline_wrapper), ui->map_builder);
    gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

    /* Exports */
    {
      gint subt = 0;
      GtkWidget *submenu;
      submenu = gtk_menu_new ();
      subt = 0;

      mitem = gtk_menu_item_new_with_label (_("Marks as CSV"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (MARKS_TO_CSV));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("Marks as HTML"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (MARKS_TO_HTML));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("Marks to clipboard"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (MARKS_TO_CLIPBOARD));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("Track plan as XML"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (PLANNER_TO_XML));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("Track plan as KML"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (PLANNER_TO_KML));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("XYZ"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (XYZ_TO_FILE));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("XYZ batch"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (XYZ_BATCH));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("UTM"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (UTM_TO_FILE));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("HSX"));
      g_signal_connect (mitem, "activate", G_CALLBACK (run_export_data), _global);
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_separator_menu_item_new ();
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("Import track plan"));
      g_signal_connect (mitem, "activate", G_CALLBACK (mark_exporter), GINT_TO_POINTER (IMPORT_PLANNER_XML));
      gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

      mitem = gtk_menu_item_new_with_label (_("Export"));
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), submenu);
      gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;
    }


      if (global->control != NULL)
        {
          /* SENSORS */
          {
            gint subt = 0;
            GtkWidget *submenu = gtk_menu_new ();
            const gchar * const * sensors;

            sensors = hyscan_control_sensors_list (global->control);

            for (; *sensors != NULL; ++sensors)
              {
                mitem = gtk_check_menu_item_new_with_label (*sensors);
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mitem), TRUE);
                // g_signal_connect (mitem, "toggled", G_CALLBACK (sensor_toggle_wrapper), );

                g_signal_connect_data (mitem, "toggled", G_CALLBACK (sensor_toggle_wrapper),
                                       g_strdup (*sensors), (GClosureNotify)g_free, 0);


                gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;
              }

            mitem = gtk_menu_item_new_with_label (_("Sensors"));
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), submenu);
            gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;
          }

          /* SONAR */
          {
            gint subt = 0;
            GtkWidget *submenu = gtk_menu_new ();
            submenu = gtk_menu_new ();
            subt = 0;

            mitem = gtk_menu_item_new_with_label (_("Info"));
            g_signal_connect (mitem, "activate", G_CALLBACK (run_show_sonar_info), "/info");
            gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

            mitem = gtk_menu_item_new_with_label (_("State"));
            g_signal_connect (mitem, "activate", G_CALLBACK (run_show_sonar_info), "/state");
            gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

            mitem = gtk_menu_item_new_with_label (_("Params"));
            g_signal_connect (mitem, "activate", G_CALLBACK (run_param), "/params");
            gtk_menu_attach (GTK_MENU (submenu), mitem, 0, 1, subt, subt+1); ++subt;

            mitem = gtk_menu_item_new_with_label (_("Sonar"));
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), submenu);
            gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;
          }

          /* OFFSETS */
          mitem = gtk_menu_item_new_with_label (_("Antennas offsets"));
          g_signal_connect (mitem, "activate", G_CALLBACK (run_offset_setup), global);
          gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

          /* офлайн-карта */
          mitem = gtk_check_menu_item_new_with_label (_("No beaming"));
          ui->starter.dry_menu = GTK_CHECK_MENU_ITEM (g_object_ref (mitem));
          g_signal_connect (mitem, "toggled", G_CALLBACK (menu_dry_wrapper), NULL);
          g_object_bind_property (ui->starter.dry_menu, "active",
                                  ui->starter.dry_switch, "active",
                                  G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
          gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

          {
            gchar ** env;
            const gchar * param_set;

            env = g_get_environ ();
            param_set = g_environ_getenv (env, EVO_ENABLE_EXTRAS);
            if (param_set != NULL)
              {
                mitem = gtk_menu_item_new_with_label (_("Hardware info"));
                g_signal_connect (mitem, "activate", G_CALLBACK (run_param), "/");
                gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;
              }
            g_strfreev (env);
          }

        }
    /* exit */
    mitem = gtk_menu_item_new_with_label (_("Exit"));
    g_signal_connect_swapped (mitem, "activate", G_CALLBACK (exit_or_restart), GINT_TO_POINTER (FALSE));
    gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

    // mitem = gtk_menu_item_new_with_label (_("Restart"));
    // g_signal_connect_swapped (mitem, "activate", G_CALLBACK (exit_or_restart), GINT_TO_POINTER (TRUE));
    // gtk_menu_attach (GTK_MENU (menu), mitem, 0, 1, t, t+1); ++t;

    gtk_widget_show_all (settings);
    gtk_widget_show_all (menu);
  }

  /* Пакуем всё. */
  hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (ui->area), ui->acoustic_stack);
  hyscan_gtk_area_set_left (HYSCAN_GTK_AREA (ui->area), ui->nav_stack);
  hyscan_gtk_area_set_right (HYSCAN_GTK_AREA (ui->area), rbox);

  gtk_container_add (GTK_CONTAINER (global->gui.window), ui->grid);
  gtk_grid_attach (GTK_GRID (ui->grid), ui->switcher, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (ui->grid), settings,     1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (ui->grid), ui->area,     0, 1, 2, 1);

  if (global->gui.nav != NULL)
    {
      GtkWidget *separator;

      separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_widget_set_margin_top (separator, 6);

      gtk_orientable_set_orientation (GTK_ORIENTABLE (global->gui.nav), GTK_ORIENTATION_HORIZONTAL);
      gtk_widget_set_halign (global->gui.nav, GTK_ALIGN_END);
      gtk_box_set_spacing (GTK_BOX (global->gui.nav), 12);
      g_object_set (global->gui.nav, "margin", 3, NULL);

      gtk_grid_attach (GTK_GRID (ui->grid), separator,       0, 2, 2, 1);
      gtk_grid_attach (GTK_GRID (ui->grid), global->gui.nav, 0, 3, 2, 1);
    }

  gtk_stack_set_visible_child_name (GTK_STACK (ui->acoustic_stack), EVO_MAP);
  widget_swap (NULL, NULL, EVO_MAP);

  /* offsets */
  if (global->control != NULL)
    {
      gchar * file = g_build_filename (g_get_user_config_dir(), "hyscan", "offsets.ini", NULL);
      HyScanFnnOffsets * o =  hyscan_fnn_offsets_new (global->control);

      if (!hyscan_fnn_offsets_read (o, file))
        g_message ("didn't read offsets");
      else if (!hyscan_fnn_offsets_execute (o))
        g_message ("didn't apply offsets");

      /* Включаем на карте навигацию. */
      hyscan_gtk_map_builder_add_nav (ui->map_builder, global->sonar_model, TRUE);

      g_object_unref (o);
      g_free (file);
    }

  return TRUE;
}

G_MODULE_EXPORT
void
destroy_interface (void)
{
  EvoUI *ui = &global_ui;

  g_signal_handlers_disconnect_by_func(ui->acoustic_stack, G_CALLBACK(widget_swap), NULL);
  g_clear_object (&ui->acoustic_stack);
  g_clear_pointer (&ui->builders, g_hash_table_unref);

  g_clear_object (&ui->map_builder);


}


G_MODULE_EXPORT
gboolean
kf_config (GKeyFile *kf)
{
  return TRUE;
}

G_MODULE_EXPORT
gboolean
kf_setting (GKeyFile *kf)
{
  EvoUI *ui = &global_ui;
  ui->settings = g_key_file_ref (kf);

  hyscan_gtk_map_builder_file_read (ui->map_builder, kf);
  gtk_check_menu_item_set_active (ui->map_offline,
                                  !hyscan_gtk_map_builder_get_offline (ui->map_builder));

  return TRUE;
}

G_MODULE_EXPORT
gboolean
kf_desetup (GKeyFile *kf)
{
  EvoUI *ui = &global_ui;
  Global *global = _global;
  GHashTableIter iter;
  gdouble balance;
  gpointer k, v;

  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, &k, &v)) /* panelx, fnnpanel */
    {
      FnnPanel *panel = v;
      GtkAdjustment *adj;

      if (panel->type != FNN_PANEL_WATERFALL)
        continue;

      adj = g_hash_table_lookup (ui->balance_table, k);
      if (adj == NULL)
        continue;

      balance = gtk_adjustment_get_value (adj);
      keyfile_double_write_helper (ui->settings, panel->name, "evo.balance", balance);
    }

  /* Запоминаем карту. */
  hyscan_gtk_map_builder_file_write (ui->map_builder, kf);

  g_clear_pointer (&ui->settings, g_key_file_unref);
  return TRUE;
}

void
panel_show (gint     panelx,
                  gboolean show)
{
  gboolean show_in_stack;
  GtkWidget *w;
  FnnPanel *panel = get_panel_quiet (_global, panelx);

  if (panel == NULL)
    {
      if (show)
        g_warning ("EvoUI: %i %i", __LINE__, panelx);
      return;
    }

  show_in_stack = (0 == g_strcmp0 (panel->name, global_ui.restoreable));

  w = gtk_stack_get_child_by_name (GTK_STACK (global_ui.acoustic_stack), panel->name);
  gtk_widget_set_visible (w, show);
  if (show_in_stack)
    gtk_stack_set_visible_child (GTK_STACK (global_ui.acoustic_stack), w);

  w = gtk_stack_get_child_by_name (GTK_STACK (global_ui.control_stack), panel->name);
  if (w != NULL)
    {
      gtk_widget_set_visible (w, show);
        if (show_in_stack)
      gtk_stack_set_visible_child (GTK_STACK (global_ui.control_stack), w);
    }
}


G_MODULE_EXPORT
gboolean
panel_pack (FnnPanel *panel,
            gint      panelx)
{
  EvoUI *ui = &global_ui;
  Global *global = _global;
  GtkWidget *control;
  GtkWidget *visual;
  const gchar *child_name;

  visual = panel->vis_gui->main;
  g_object_set (visual, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                        "hexpand", TRUE, "halign", GTK_ALIGN_FILL, NULL);

  control = make_page_for_panel (ui, panel, GPOINTER_TO_INT (panelx), global);

  child_name = gtk_stack_get_visible_child_name (GTK_STACK (ui->acoustic_stack));
  gtk_stack_add_titled (GTK_STACK (ui->control_stack), control, panel->name, panel->name_local);
  gtk_stack_add_titled (GTK_STACK (ui->acoustic_stack), visual, panel->name, panel->name_local);
  widget_swap (NULL, NULL, child_name);

  /* Нижняя панель содержит виджет управления впередсмотрящим и круговым. */
  if (panelx == X_FORWARDL)
  {
    VisualFL *fl = (VisualFL*)panel->vis_gui;
    hyscan_gtk_area_set_bottom (HYSCAN_GTK_AREA (ui->area), fl->play_control);
    gtk_widget_show_all(ui->area);
  }
  /*
  else if (panelx == X_LOOKARND || panelx == X_LOOK_LOW || panelx == X_LOOK_HI)
  {
    VisualLA *la = (VisualLA*)panel->vis_gui;
    hyscan_gtk_area_set_bottom (HYSCAN_GTK_AREA (ui->area), la->play_control);
    gtk_widget_show_all(ui->area);
  }
  */

  if (panel->type == FNN_PANEL_WATERFALL)
    {
      GtkAdjustment *adj;
      gdouble balance;

      balance = keyfile_double_read_helper (ui->settings, panel->name, "evo.balance", 0.0);
      adj = g_hash_table_lookup (ui->balance_table, GINT_TO_POINTER (panelx));
      if (adj != NULL)
        gtk_adjustment_set_value (adj, balance);
    }


  return TRUE;
}

G_MODULE_EXPORT
void
panel_adjust_visibility (HyScanTrackInfo *track_info)
{
  gboolean sidescan_v = FALSE;
  gboolean side_low_v = FALSE;
  gboolean side_high_v = FALSE;
  gboolean profiler_v = FALSE;
  gboolean echosound_v = FALSE;
  gboolean forwardl_v = FALSE;
  gboolean lookaround_v = FALSE;
  gboolean lookaround_low_v = FALSE;
  gboolean lookaround_high_v = FALSE;
  guint i;

  for (i = 0; i < HYSCAN_SOURCE_LAST; ++i)
    {
      gboolean in_track_info = track_info != NULL && track_info->sources[i];
      gboolean in_sonar = _global->control != NULL && g_hash_table_lookup (_global->sonar_infos, GINT_TO_POINTER (i));

      if (!in_track_info && !in_sonar)
        continue;

      switch (i)
      {
        case HYSCAN_SOURCE_SIDE_SCAN_STARBOARD:      sidescan_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_PORT:           sidescan_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_LOW:  side_low_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_PORT_LOW:       side_low_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_HI:   side_high_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_PORT_HI:        side_high_v = TRUE; break;

        case HYSCAN_SOURCE_PROFILER:                 profiler_v = TRUE; break;
        case HYSCAN_SOURCE_PROFILER_ECHO:            profiler_v = TRUE; break;
        case HYSCAN_SOURCE_ECHOSOUNDER:              echosound_v = TRUE; break;

        case HYSCAN_SOURCE_FORWARD_LOOK:             forwardl_v = TRUE; break;

        case HYSCAN_SOURCE_LOOK_AROUND_STARBOARD:     lookaround_v = TRUE; break;
        case HYSCAN_SOURCE_LOOK_AROUND_PORT:          lookaround_v = TRUE; break;
        case HYSCAN_SOURCE_LOOK_AROUND_STARBOARD_LOW: lookaround_low_v = TRUE; break;
        case HYSCAN_SOURCE_LOOK_AROUND_PORT_LOW:      lookaround_low_v = TRUE; break;
        case HYSCAN_SOURCE_LOOK_AROUND_STARBOARD_HI:  lookaround_high_v = TRUE; break;
        case HYSCAN_SOURCE_LOOK_AROUND_PORT_HI:       lookaround_high_v = TRUE; break;
      }
    }

  /* Показываем активированные страницы. */
  if (sidescan_v)
    panel_show (X_SIDESCAN, TRUE);
  if (side_low_v)
    panel_show (X_SIDE_LOW, TRUE);
  if (side_high_v)
    panel_show (X_SIDE_HIGH, TRUE);
  if (profiler_v)
    panel_show (X_PROFILER, TRUE);
  if (echosound_v)
    panel_show (X_ECHOSOUND, TRUE);
  if (forwardl_v)
    panel_show (X_FORWARDL, TRUE);
  if (lookaround_v)
    panel_show (X_LOOKARND, TRUE);
  if (lookaround_low_v)
    panel_show (X_LOOK_LOW, TRUE);
  if (lookaround_high_v)
    panel_show (X_LOOK_HI, TRUE);

  /* Прячем неактивированные страницы. */
  if (!sidescan_v)
    panel_show (X_SIDESCAN, FALSE);
  if (!side_low_v)
    panel_show (X_SIDE_LOW, FALSE);
  if (!side_high_v)
    panel_show (X_SIDE_HIGH, FALSE);
  if (!profiler_v)
    panel_show (X_PROFILER, FALSE);
  if (!echosound_v)
    panel_show (X_ECHOSOUND, FALSE);
  if (!forwardl_v)
    panel_show (X_FORWARDL, FALSE);
  if (!lookaround_v)
    panel_show (X_LOOKARND, FALSE);
  if (!lookaround_low_v)
    panel_show (X_LOOK_LOW, FALSE);
  if (!lookaround_high_v)
    panel_show (X_LOOK_HI, FALSE);

}

static gdouble
range360 (const gdouble a)
{
  guint i;
  gdouble d;

  d = a * 65536.0 / 360.0;
  i = (int) d;
  d = (i & 0xFFFF);
  return d * 360.0 / 65536.0;
}

static void
la_ruler_changed_cb (HyScanGtkGlikoView *view, gdouble point_a_z, gdouble point_a_r, gdouble point_b_z, gdouble point_b_r, gpointer user_data)
{
  VisualLA *vla = (VisualLA *)user_data;
  gchar tmp[64];
  gdouble x1, y1, x2, y2, d;
  gdouble point_ab_z;
  gdouble point_ba_z;
  const gdouble radians_to_degrees = 180.0 / G_PI;
  const gdouble degrees_to_radians = G_PI / 180.0;
  gdouble a = hyscan_gtk_gliko_get_full_rotation (HYSCAN_GTK_GLIKO (view));

  hyscan_gtk_gliko_polar2pixel (HYSCAN_GTK_GLIKO (view), point_a_z, point_a_r, &x1, &y1);
  hyscan_gtk_gliko_polar2pixel (HYSCAN_GTK_GLIKO (view), point_b_z, point_b_r, &x2, &y2);

  point_ab_z = range360 (atan2 (x2 - x1, y1 - y2) * radians_to_degrees - a);
  point_ba_z = range360 (atan2 (x1 - x2, y2 - y1) * radians_to_degrees - a);

  x1 = point_a_r * sin (point_a_z * degrees_to_radians);
  y1 = point_a_r * cos (point_a_z * degrees_to_radians);

  x2 = point_b_r * sin (point_b_z * degrees_to_radians);
  y2 = point_b_r * cos (point_b_z * degrees_to_radians);

  d = sqrt ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));

  g_ascii_formatd (tmp, sizeof (tmp), "%.0f\302\260", point_a_z);
  gtk_label_set_text (GTK_LABEL( vla->la_a_degrees ), tmp);
  g_ascii_formatd (tmp, sizeof (tmp), "%.1f m", point_a_r);
  gtk_label_set_text (GTK_LABEL( vla->la_a_meters ), tmp);

  g_ascii_formatd (tmp, sizeof (tmp), "%.0f\302\260", point_b_z);
  gtk_label_set_text (GTK_LABEL( vla->la_b_degrees ), tmp);
  g_ascii_formatd (tmp, sizeof (tmp), "%.1f m", point_b_r);
  gtk_label_set_text (GTK_LABEL( vla->la_b_meters ), tmp);

  g_ascii_formatd (tmp, sizeof (tmp), "%.0f\302\260", point_ab_z);
  gtk_label_set_text (GTK_LABEL( vla->la_ab_degrees ), tmp);
  g_ascii_formatd (tmp, sizeof (tmp), "%.1f m", d);
  gtk_label_set_text (GTK_LABEL( vla->la_ab_meters ), tmp);

  g_ascii_formatd (tmp, sizeof (tmp), "%.0f\302\260", point_ba_z);
  gtk_label_set_text (GTK_LABEL( vla->la_ba_degrees ), tmp );

  /*
  g_snprintf (tmp, sizeof (tmp), "%s\302\260/%s м", tmpz, tmpr);
  gtk_label_set_text (GTK_LABEL (label_a), tmp);

  g_ascii_formatd (tmpz, sizeof (tmpz), "%.0f", point_b_z);
  g_ascii_formatd (tmpr, sizeof (tmpr), "%.1f", point_b_r);
  g_snprintf (tmp, sizeof (tmp), "%s\302\260/%s м", tmpz, tmpr);
  gtk_label_set_text (GTK_LABEL (label_b), tmp);

  g_ascii_formatd (tmpz, sizeof (tmpz), "%.0f", point_ab_z);
  g_ascii_formatd (tmpr, sizeof (tmpr), "%.1f", d);
  g_snprintf (tmp, sizeof (tmp), "%s\302\260/%s м", tmpz, tmpr);
  gtk_label_set_text (GTK_LABEL (label_ab), tmp);

  g_ascii_formatd (tmpz, sizeof (tmpz), "%.0f", point_ba_z);
  g_snprintf (tmp, sizeof (tmp), "%s\302\260/%s м", tmpz, tmpr);
  gtk_label_set_text (GTK_LABEL (label_ba), tmp);
  */
}

static void
la_player_scale_cb (GtkRange *range, gpointer user_data)
{
  VisualLA *vla = (VisualLA *)user_data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (vla->la_pause_toggle)))
    {
      hyscan_data_player_seek (vla->player, vla->player_min + (gint64) (1000000 * gtk_range_get_value (range)));
      vla->player_position_changed = 1;
    }
}

static void
la_pause_toggle_cb (GtkToggleButton *toggle_button, gpointer user_data)
{
  VisualLA *vla = (VisualLA *)user_data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_button)))
    {
      hyscan_data_player_pause (vla->player);
      gtk_widget_set_sensitive (GTK_WIDGET (vla->la_player_scale), TRUE);
      hyscan_gtk_gliko_set_playback (HYSCAN_GTK_GLIKO (vla->glikoview), 0);
      vla->player_position_changed = 0;
    }
  else
    {
      hyscan_gtk_gliko_set_playback (HYSCAN_GTK_GLIKO (vla->glikoview), vla->player_position_changed ? 2 : 1);
      vla->player_position_changed = 0;
      hyscan_data_player_play (vla->player, vla->speed);
      gtk_widget_set_sensitive (GTK_WIDGET (vla->la_player_scale), FALSE);
    }
}

static void
la_x10_toggle_cb (GtkToggleButton *toggle_button, gpointer user_data)
{
  VisualLA *vla = (VisualLA *)user_data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_button)))
    {
      vla->speed = 10.0;
    }
  else
    {
      vla->speed = 1.0;
    }
  hyscan_data_player_play (vla->player, vla->speed);
}

static void
la_player_range_cb (HyScanDataPlayer *player, gint64 min, gint64 max, gpointer user_data)
{
  VisualLA *vla = (VisualLA *)user_data;

  vla->player_min = min;
  gtk_range_set_range (GTK_RANGE (vla->la_player_scale), 0.0, 0.000001 * (max - min));
}

static void
la_player_ready_cb (HyScanDataPlayer *player, gint64 time, gpointer user_data)
{
  VisualLA *vla = (VisualLA *)user_data;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (vla->la_pause_toggle)))
    {
      gtk_range_set_value (GTK_RANGE (vla->la_player_scale), 0.000001 * (time - vla->player_min));
    }
}

