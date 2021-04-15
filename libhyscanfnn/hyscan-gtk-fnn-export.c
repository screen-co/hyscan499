/* hyscan-gtk-fnn-export.h
 *
 * Copyright 2019 Screen LLC, Alexander Dmitriev <m1n7@yandex.ru>
 *
 * This file is part of HyScanFnn.
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
 * SECTION: hyscan-gtk-fnn-export
 * @Title HyScanGtkFnnExport
 * @Short_description
 *
 */

#define GETTEXT_PACKAGE "libhyscanfnn"
#include <glib/gi18n-lib.h>
#include <hyscan-mloc.h>
#include "hyscan-gtk-fnn-export.h"

enum
{
  PROP_0,
  PROP_DB_INFO,
  PROP_CACHE,
  PROP_PROJECT,
  PROP_TRACK_MODEL,
};

struct _HyScanGtkFnnExportPrivate
{
  HyScanDBInfo        *info;
  HyScanCache         *cache;
  gchar               *project;
  HyScanMapTrackModel *tmodel;

  GtkWidget           *chooser;
  GtkProgressBar      *progress_bar;
  GtkWidget           *progress_page;
  GtkLabel            *current;

  gchar              **tracks;
  GThread             *thread;
  gint                 current_index;
};

static void    hyscan_gtk_fnn_export_set_property             (GObject               *object,
                                                               guint                  prop_id,
                                                               const GValue          *value,
                                                               GParamSpec            *pspec);
static void    hyscan_gtk_fnn_export_object_constructed       (GObject               *object);
static void    hyscan_gtk_fnn_export_object_finalize          (GObject               *object);
static void    hyscan_gtk_fnn_export_prepare                  (GtkAssistant          *assistant,
                                                               GtkWidget             *page);
static void    hyscan_gtk_fnn_export_apply                    (GtkAssistant          *assistant);
static void    hyscan_gtk_fnn_export_close                    (GtkAssistant          *assistant);
static gpointer    hyscan_gtk_fnn_export_thread               (gpointer               udata);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkFnnExport, hyscan_gtk_fnn_export, GTK_TYPE_ASSISTANT);

static void
hyscan_gtk_fnn_export_class_init (HyScanGtkFnnExportClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);
  GtkAssistantClass *aclass = GTK_ASSISTANT_CLASS (klass);

  oclass->set_property = hyscan_gtk_fnn_export_set_property;
  oclass->constructed = hyscan_gtk_fnn_export_object_constructed;
  oclass->finalize = hyscan_gtk_fnn_export_object_finalize;

  aclass->prepare = hyscan_gtk_fnn_export_prepare;
  aclass->apply = hyscan_gtk_fnn_export_apply;
  aclass->close = hyscan_gtk_fnn_export_close;
  aclass->cancel = hyscan_gtk_fnn_export_close;

  gtk_widget_class_set_template_from_resource (wclass, "/org/libhyscanfnn/gtk/hyscan-gtk-fnn-export.ui");
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, chooser);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, progress_bar);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, progress_page);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, current);

  g_object_class_install_property (oclass, PROP_DB_INFO,
    g_param_spec_object ("dbinfo", "dbinfo", "HyScanDBInfo", HYSCAN_TYPE_DB_INFO,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (oclass, PROP_CACHE,
    g_param_spec_object ("cache", "cache", "HyScanCache", HYSCAN_TYPE_CACHE,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (oclass, PROP_TRACK_MODEL,
    g_param_spec_object ("track-model", "track-model", "HyScanMapTrackModel", HYSCAN_TYPE_MAP_TRACK_MODEL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_fnn_export_init (HyScanGtkFnnExport *self)
{
  self->priv = hyscan_gtk_fnn_export_get_instance_private (self);
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
hyscan_gtk_fnn_export_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (object);
  HyScanGtkFnnExportPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_DB_INFO:
      priv->info = g_value_dup_object (value);
      break;

    case PROP_CACHE:
      priv->cache = g_value_dup_object (value);
      break;

    case PROP_TRACK_MODEL:
      priv->tmodel = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_fnn_export_object_constructed (GObject *object)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (object);
  HyScanGtkFnnExportPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_gtk_fnn_export_parent_class)->constructed (object);

  priv->project = hyscan_db_info_get_project(priv->info);

  gtk_window_set_modal (GTK_WINDOW (self), TRUE);
}

static void
hyscan_gtk_fnn_export_object_finalize (GObject *object)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (object);
  HyScanGtkFnnExportPrivate *priv = self->priv;

  g_clear_object (&priv->info);
  g_clear_object (&priv->cache);
  g_clear_object (&priv->tmodel);

  G_OBJECT_CLASS (hyscan_gtk_fnn_export_parent_class)->finalize (object);
}

static void
hyscan_gtk_fnn_export_prepare (GtkAssistant *assistant,
                               GtkWidget    *page)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (assistant);
  HyScanGtkFnnExportPrivate *priv = self->priv;

  if (page != priv->progress_page)
    return;
}

static void
hyscan_gtk_fnn_export_apply (GtkAssistant *assistant)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (assistant);
  HyScanGtkFnnExportPrivate *priv = self->priv;

  priv->tracks = hyscan_map_track_model_get_tracks (priv->tmodel);

  priv->thread = g_thread_new ("fnn-xyz-export", hyscan_gtk_fnn_export_thread, self);
}

static void
hyscan_gtk_fnn_export_close (GtkAssistant *assistant)
{
  gtk_widget_destroy (GTK_WIDGET (assistant));
}

static gboolean
hyscan_gtk_fnn_export_update_label (gpointer data)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (data);
  HyScanGtkFnnExportPrivate *priv = self->priv;
  gint i, total;
  gdouble fraction;

  i = g_atomic_int_get (&priv->current_index);
  total = g_strv_length (priv->tracks);
  fraction = (gdouble)i / (gdouble)total;

  gtk_progress_bar_set_fraction (priv->progress_bar, fraction);
  if (i != total)
    {
      gtk_label_set_text (priv->current, priv->tracks[i]);
      return G_SOURCE_REMOVE;
    }

  gtk_assistant_set_page_complete (GTK_ASSISTANT (self), priv->progress_page, TRUE);
  gtk_label_set_text (priv->current, "All done!");
  if (priv->thread != NULL)
    g_thread_join (priv->thread);
  priv->thread = NULL;

  return G_SOURCE_REMOVE;
}

static void
hyscan_gtk_fnn_export_export_one (GHashTable                *track_infos,
                                  HyScanDB                  *db,
                                  HyScanGtkFnnExportPrivate *priv,
                                  const gchar               *track)
{
  guint32 first, last, i;
  HyScanAntennaOffset antenna;
  HyScanGeoGeodetic coord;
  GString *string = NULL;
  gchar *words = NULL;

  HyScanCache * cache = priv->cache;
  gchar *project = priv->project;

  HyScanNavData * dpt;
  HyScanmLoc * mloc;

  HyScanTrackInfo * info = NULL;
  HyScanDBInfoSensorInfo *sensor_info = NULL;
  GHashTableIter iter;
  gpointer key, value;

  gchar *filename = NULL, *uri = NULL, *full_path = NULL;
  GError *error = NULL;

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

  mloc = hyscan_mloc_new (db, priv->cache, priv->project, track);
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

  uri = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->chooser));
  filename = g_strdup_printf ("%s-%s.xyz.txt", priv->project, track);
  full_path = g_build_filename (uri, filename, NULL);
  // g_message ("%s, %s, %s", uri, filename, full_path);
  g_file_set_contents (full_path, words, strlen (words), &error);
  // if(error)
    // g_message("err: %s", error->message);
  g_free (words);
  g_free (uri);
  g_free (filename);
  g_free (full_path);
}

static gpointer
hyscan_gtk_fnn_export_thread (gpointer udata)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (udata);
  HyScanGtkFnnExportPrivate *priv = self->priv;
  gint i, n;
  GHashTable * track_infos;
  // HyScanTrackInfo * info;
  HyScanDB * db;

  db = hyscan_db_info_get_db (priv->info);
  track_infos = hyscan_db_info_get_tracks (priv->info);
  n = g_strv_length (priv->tracks);
  for (i = 0; i < n; ++i)
    {
      g_atomic_int_set (&priv->current_index, i);
      g_idle_add (hyscan_gtk_fnn_export_update_label, self);
      hyscan_gtk_fnn_export_export_one (track_infos, db, priv, priv->tracks[i]);
    }

  g_atomic_int_set (&priv->current_index, i);
  g_idle_add (hyscan_gtk_fnn_export_update_label, self);

  g_clear_object (&db);
  g_hash_table_unref (track_infos);

  return NULL;
}

HyScanGtkFnnExport *
hyscan_gtk_fnn_export_new (HyScanDBInfo        *info,
                           HyScanCache         *cache,
                           HyScanMapTrackModel *tmodel)
{
  return g_object_new (HYSCAN_TYPE_GTK_FNN_EXPORT,
                       "dbinfo", info,
                       "cache", cache,
                       "track-model", tmodel,
                       NULL);
}
