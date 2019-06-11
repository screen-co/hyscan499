#include "hyscan-gtk-nav-indicator.h"
#include <hyscan-nmea-parser.h>
// #include <fnn-types-common.h>
#define GETTEXT_PACKAGE "libhyscanfnn"
#include <glib/gi18n-lib.h>



struct _HyScanGtkNavIndicatorPrivate
{
  struct
  {
    HyScanNMEAParser * lat;
    HyScanNMEAParser * lon;
    HyScanNMEAParser * trk;
    HyScanNMEAParser * spd;
    HyScanNMEAParser * dpt;
    HyScanNMEAParser * time;
    HyScanNMEAParser * date;
  } parser;

  struct
  {
    gchar            * lat;
    gchar            * lon;
    gchar            * trk;
    gchar            * spd;
    gchar            * dpt;
    gchar            * tmd;
  } string;

  GtkLabel           * lat;
  GtkLabel           * lon;
  GtkLabel           * trk;
  GtkLabel           * spd;
  GtkLabel           * dpt;
  GtkLabel           * tmd;

  GMutex  lock;
  guint   update_tag;
};

static void  hyscan_gtk_nav_indicator_constructed (GObject               *object);
static void  hyscan_gtk_nav_indicator_finalize    (GObject               *object);
static gboolean hyscan_gtk_nav_indicator_update   (HyScanGtkNavIndicator *self);


G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkNavIndicator, hyscan_gtk_nav_indicator, GTK_TYPE_BOX)

static void
hyscan_gtk_nav_indicator_class_init (HyScanGtkNavIndicatorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->constructed = hyscan_gtk_nav_indicator_constructed;
  G_OBJECT_CLASS (klass)->finalize = hyscan_gtk_nav_indicator_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/libhyscanfnn/gtk/hyscan-gtk-nav-indicator.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkNavIndicator, lat);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkNavIndicator, lon);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkNavIndicator, trk);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkNavIndicator, spd);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkNavIndicator, dpt);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkNavIndicator, tmd);
}

static void
hyscan_gtk_nav_indicator_init (HyScanGtkNavIndicator *me)
{
  me->priv = hyscan_gtk_nav_indicator_get_instance_private (me);
  gtk_widget_init_template (GTK_WIDGET (me));
}

static void
hyscan_gtk_nav_indicator_constructed (GObject *object)
{
  HyScanGtkNavIndicator *self = HYSCAN_GTK_NAV_INDICATOR (object);
  HyScanGtkNavIndicatorPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_gtk_nav_indicator_parent_class)->constructed (object);

  priv->parser.time = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_RMC, HYSCAN_NMEA_FIELD_TIME);
  priv->parser.date = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_RMC, HYSCAN_NMEA_FIELD_DATE);
  priv->parser.lat = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_RMC, HYSCAN_NMEA_FIELD_LAT);
  priv->parser.lon = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_RMC, HYSCAN_NMEA_FIELD_LON);
  priv->parser.trk = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_RMC, HYSCAN_NMEA_FIELD_TRACK);
  priv->parser.spd = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_RMC, HYSCAN_NMEA_FIELD_SPEED);
  priv->parser.dpt = hyscan_nmea_parser_new_empty (HYSCAN_NMEA_DATA_DPT, HYSCAN_NMEA_FIELD_DEPTH);

  priv->update_tag = g_timeout_add (1000, (GSourceFunc)(hyscan_gtk_nav_indicator_update), self);
}

static void
hyscan_gtk_nav_indicator_finalize (GObject *object)
{
  HyScanGtkNavIndicator *self = HYSCAN_GTK_NAV_INDICATOR (object);
  HyScanGtkNavIndicatorPrivate *priv = self->priv;

  g_object_unref (priv->parser.time);
  g_object_unref (priv->parser.date);
  g_object_unref (priv->parser.lat);
  g_object_unref (priv->parser.lon);
  g_object_unref (priv->parser.trk);
  g_object_unref (priv->parser.spd);
  g_object_unref (priv->parser.dpt);

  if (priv->update_tag > 0)
    g_source_remove (priv->update_tag);

  g_mutex_clear (&priv->lock);

  G_OBJECT_CLASS (hyscan_gtk_nav_indicator_parent_class)->finalize (object);
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

static gboolean
hyscan_gtk_nav_indicator_update (HyScanGtkNavIndicator *self)
{
  HyScanGtkNavIndicatorPrivate *priv = self->priv;

  g_mutex_lock (&priv->lock);

  if (priv->string.lat != NULL)
    gtk_label_set_text (priv->lat, priv->string.lat);
  if (priv->string.lon != NULL)
    gtk_label_set_text (priv->lon, priv->string.lon);
  if (priv->string.trk != NULL)
    gtk_label_set_text (priv->trk, priv->string.trk);
  if (priv->string.spd != NULL)
    gtk_label_set_text (priv->spd, priv->string.spd);
  if (priv->string.dpt != NULL)
    gtk_label_set_text (priv->dpt, priv->string.dpt);
  if (priv->string.tmd != NULL)
    gtk_label_set_text (priv->tmd, priv->string.tmd);

  g_mutex_unlock (&priv->lock);

  return G_SOURCE_CONTINUE;
}

GtkWidget *
hyscan_gtk_nav_indicator_new (void)
{
  return GTK_WIDGET (g_object_new (HYSCAN_TYPE_GTK_NAV_INDICATOR, NULL));
}

void
hyscan_gtk_nav_indicator_push (HyScanGtkNavIndicator *self,
                               HyScanBuffer          *data)
{
  HyScanGtkNavIndicatorPrivate *priv;
  const gchar * rmcs = NULL, *dpts = NULL, *raw_data;
  gchar **nmea;
  guint32 size;
  guint i;
  gdouble time, date, lat, lon, trk, spd, dpt;
  gboolean td_ok, ll_ok, trk_ok, spd_ok, dpt_ok;

  g_return_if_fail (HYSCAN_IS_GTK_NAV_INDICATOR (self));
  priv = self->priv;

  /* Вытаскиваем данные. */
  raw_data = hyscan_buffer_get (data, NULL, &size);
  nmea = g_strsplit_set (raw_data, "\r\n", -1);

  /* Ищем строки РМЦ и ДПТ. */
  for (i = 0; (nmea != NULL) && (nmea[i] != NULL); i++)
    {
      if (g_str_has_prefix (nmea[i]+3, "RMC"))
        rmcs = nmea[i];
      else if (g_str_has_prefix (nmea[i]+3, "DPT"))
        dpts = nmea[i];
    }

  /* Парсим строки. */
  if (rmcs != NULL)
    {
      td_ok = hyscan_nmea_parser_parse_string (priv->parser.time, rmcs, &time) &&
              hyscan_nmea_parser_parse_string (priv->parser.date, rmcs, &date);

      ll_ok = hyscan_nmea_parser_parse_string (priv->parser.lat, rmcs, &lat) &&
              hyscan_nmea_parser_parse_string (priv->parser.lon, rmcs, &lon);

      trk_ok = hyscan_nmea_parser_parse_string (priv->parser.trk, rmcs, &trk);
      spd_ok = hyscan_nmea_parser_parse_string (priv->parser.spd, rmcs, &spd);

    }
  else
    {
      td_ok = ll_ok = trk_ok = spd_ok = FALSE;
    }
  if (dpts != NULL)
    {
      dpt_ok = hyscan_nmea_parser_parse_string (priv->parser.dpt, dpts, &dpt);
    }
  else
    {
      dpt_ok = FALSE;
    }

  /* Обновляем данные виджетов. */
  g_mutex_lock (&priv->lock);

  if (td_ok)
    {
      GTimeZone *tz;
      GDateTime *local, *utc;
      gchar *dtstr;

      tz = g_time_zone_new ("+03:00");
      utc = g_date_time_new_from_unix_utc (date + time);
      local = g_date_time_to_timezone (utc, tz);

      dtstr = g_date_time_format (local, "%d.%m.%y %H:%M:%S");

      nav_printer (&priv->string.tmd, "%s", dtstr);

      g_free (dtstr);
      g_date_time_unref (local);
      g_date_time_unref (utc);
      g_time_zone_unref (tz);
    }
  if (ll_ok)
    {
      nav_printer (&priv->string.lat, "%f °%s", lat, lat > 0 ? "С.Ш." : "Ю.Ш.");
      nav_printer (&priv->string.lon, "%f °%s", lon, lon > 0 ? "В.Д." : "З.Д.");
    }
  if (trk_ok)
    {
      nav_printer (&priv->string.trk, "%f °", trk);
    }
  if (spd_ok)
    {
      nav_printer (&priv->string.spd, "%f м/с", spd * 0.514);
    }
  if (dpt_ok)
    {
      nav_printer (&priv->string.dpt, "%f м", dpt);
    }

  g_mutex_unlock (&priv->lock);
  g_strfreev (nmea);
}
