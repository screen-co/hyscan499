/*
 * \file hyscan-fl-coords.c
 *
 * \brief Исходный файл сетки для виджета водопад
 * \author Dmitriev Alexander (m1n7@yandex.ru)
 * \date 2017
 * \license Проприетарная лицензия ООО "Экран"
 *
 */

#include "hyscan-fl-coords.h"
#include <hyscan-tile-color.h>
#include <hyscan-mloc.h>
#include <hyscan-forward-look-data.h>

enum
{
  SIGNAL_COORDS,
  SIGNAL_LAST
};

enum
{
  PROP_FL = 1
};

struct _HyScanFlCoordsPrivate
{
  HyScanGtkForwardLook *fl;
  GtkAdjustment        *adjustment;
  gdouble               val;
  gint64                time_for_val;

  HyScanDB              *db;
  gchar                 *project;
  gchar                 *track;

  HyScanForwardLookData *fl_data;
  HyScanAntennaPosition  apos;
  HyScanmLoc            *loc;

  HyScanGeoGeodetic     coords;
  gboolean              coords_status;
};

static void     hyscan_fl_coords_set_property           (GObject               *object,
                                                         guint                  prop_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     hyscan_fl_coords_object_constructed     (GObject               *object);
static void     hyscan_fl_coords_object_finalize        (GObject               *object);

static gboolean hyscan_fl_coords_button                 (GtkWidget             *widget,
                                                         GdkEventButton        *event,
                                                        HyScanFlCoords *self);
static void     adjustment_changed                      (GtkAdjustment  *adj,
                                                         gpointer        udata);

static guint    hyscan_fl_coords_signals[SIGNAL_LAST] = {0};
G_DEFINE_TYPE_WITH_CODE (HyScanFlCoords, hyscan_fl_coords, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (HyScanFlCoords));

static void
hyscan_fl_coords_class_init (HyScanFlCoordsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_fl_coords_set_property;
  object_class->constructed = hyscan_fl_coords_object_constructed;
  object_class->finalize = hyscan_fl_coords_object_finalize;

  g_object_class_install_property (object_class, PROP_FL,
    g_param_spec_object ("fl", "Waterfall", "GtkWaterfall object",
                         HYSCAN_TYPE_GTK_FORWARD_LOOK,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  hyscan_fl_coords_signals[SIGNAL_COORDS] =
    g_signal_new ("coords", HYSCAN_TYPE_FL_COORDS, G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

}

static void
hyscan_fl_coords_init (HyScanFlCoords *self)
{
  self->priv = hyscan_fl_coords_get_instance_private (self);
}

static void
hyscan_fl_coords_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  HyScanFlCoords *self = HYSCAN_FL_COORDS (object);

  if (prop_id == PROP_FL)
    self->priv->fl = g_value_dup_object (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
hyscan_fl_coords_object_constructed (GObject *object)
{
  HyScanFlCoords *self = HYSCAN_FL_COORDS (object);
  HyScanFlCoordsPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_fl_coords_parent_class)->constructed (object);

  priv->adjustment = hyscan_gtk_forward_look_get_adjustment (priv->fl);
  g_signal_connect (priv->fl, "button-press-event", G_CALLBACK (hyscan_fl_coords_button), self);
}

static void
hyscan_fl_coords_object_finalize (GObject *object)
{
  HyScanFlCoords *self = HYSCAN_FL_COORDS (object);
  HyScanFlCoordsPrivate *priv = self->priv;

  /* Отключаемся от всех сигналов. */
  g_signal_handlers_disconnect_by_data (priv->fl, self);

  G_OBJECT_CLASS (hyscan_fl_coords_parent_class)->finalize (object);
}

/* Функция создает новый виджет HyScanFlCoords. */
HyScanFlCoords*
hyscan_fl_coords_new (HyScanGtkForwardLook *fl)
{
  return g_object_new (HYSCAN_TYPE_FL_COORDS,
                       "fl", fl,
                       NULL);
}

void
hyscan_fl_coords_set_project (HyScanFlCoords *self,
                              HyScanDB       *db,
                              gchar          *project,
                              gchar          *track)
{
  HyScanFlCoordsPrivate *priv;

  g_return_if_fail (HYSCAN_IS_FL_COORDS (self));
  priv = self->priv;

  g_clear_object (&priv->loc);
  g_clear_object (&priv->fl_data);
  g_clear_object (&priv->db);
  g_clear_pointer (&priv->project, g_free);
  g_clear_pointer (&priv->track, g_free);

  priv->db = g_object_ref (db);
  priv->project = g_strdup (project);
  priv->track = g_strdup (track);

  priv->loc = hyscan_mloc_new (db, project, track);
  priv->fl_data = hyscan_forward_look_data_new (db, project, track, TRUE);

  if (priv->loc == NULL || priv->fl_data == NULL)
    return;

  priv->apos = hyscan_forward_look_data_get_position (priv->fl_data);
}


static gboolean
hyscan_fl_coords_button (GtkWidget      *widget,
                         GdkEventButton *event,
                         HyScanFlCoords *self)
{
  gdouble x_val;
  gdouble y_val;
  gboolean status;
  gdouble cur_val;
  guint32 index;
  guint32 n_points;
  HyScanGeoGeodetic geo;

  HyScanFlCoordsPrivate *priv = self->priv;
  GtkCifroArea *carea = GTK_CIFRO_AREA (widget);

  gtk_cifro_area_visible_point_to_value (carea, event->x, event->y, &y_val, &x_val); /* SIC!*/

  if (priv->fl_data == NULL || priv->loc == NULL)
    {
      g_message ("No navigation data!");
      return FALSE;
    }

  cur_val = gtk_adjustment_get_value (priv->adjustment);
  if (cur_val != priv->val)
    {
      index = cur_val;
      priv->val = cur_val;
      hyscan_forward_look_data_get_doa_values (priv->fl_data, index, &n_points, &priv->time_for_val);
    }

  status = hyscan_mloc_get_fl (priv->loc,
                               priv->time_for_val,
                               &priv->apos,
                               x_val,
                               y_val,
                               &geo);

  priv->coords = geo;
  priv->coords_status = status;

  g_signal_emit (self, hyscan_fl_coords_signals[SIGNAL_COORDS], 0);

  return FALSE;
}


gboolean hyscan_fl_coords_get_coords (HyScanFlCoords *self, gdouble *lat, gdouble *lon)
{
  HyScanFlCoordsPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_FL_COORDS (self), FALSE);
  priv = self->priv;

  *lat = priv->coords.lat;
  *lon = priv->coords.lon;
  return priv->coords_status;
}
