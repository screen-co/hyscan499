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
  PROP_FL = 1,
  PROP_CACHE
};

struct _HyScanFlCoordsPrivate
{
  HyScanGtkForwardLook *fl;
  HyScanCache          *cache;

  gdouble               val;
  gdouble               cur_val;
  gdouble               mouse_x;
  gdouble               mouse_y;
  gint64                time_for_val;

  HyScanForwardLookData *fl_data;
  HyScanAntennaPosition  apos;
  HyScanmLoc            *loc;

  HyScanGeoGeodetic     coords;
  gboolean              coords_status;

  GdkRGBA               cyan;
  GdkRGBA               red;

};

static void     hyscan_fl_coords_set_property           (GObject               *object,
                                                         guint                  prop_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     hyscan_fl_coords_object_constructed     (GObject               *object);
static void     hyscan_fl_coords_object_finalize        (GObject               *object);

static void     hyscan_fl_coords_adj_changed            (GtkAdjustment         *adjustment,
                                                         gpointer               udata);
static gboolean hyscan_fl_coords_button                 (GtkWidget             *widget,
                                                         GdkEventButton        *event,
                                                         HyScanFlCoords        *self);
static gboolean hyscan_fl_coords_visible_draw           (GtkWidget             *widget,
                                                         cairo_t               *surface,
                                                         HyScanFlCoords        *self);

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
    g_param_spec_object ("fl", "HyScanGtkForwardLook", "HyScanGtkForwardLook object",
                         HYSCAN_TYPE_GTK_FORWARD_LOOK,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CACHE,
    g_param_spec_object ("cache", "cache", "HyScanCache object",
                         HYSCAN_TYPE_CACHE,
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
  else if (prop_id == PROP_CACHE)
    self->priv->cache = g_value_dup_object (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
hyscan_fl_coords_object_constructed (GObject *object)
{
  GtkAdjustment *adjustment;
  HyScanFlCoords *self = HYSCAN_FL_COORDS (object);
  HyScanFlCoordsPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_fl_coords_parent_class)->constructed (object);

  adjustment = hyscan_gtk_forward_look_get_adjustment (priv->fl);
  g_signal_connect (priv->fl, "button-press-event", G_CALLBACK (hyscan_fl_coords_button), self);
  g_signal_connect (priv->fl, "visible-draw", G_CALLBACK (hyscan_fl_coords_visible_draw), self);
  g_signal_connect (adjustment, "value-changed", G_CALLBACK (hyscan_fl_coords_adj_changed), self);
  priv->cur_val = priv->val = gtk_adjustment_get_value (adjustment);

  gdk_rgba_parse (&priv->cyan, "cyan");
  gdk_rgba_parse (&priv->red, "red");
}

static void
hyscan_fl_coords_object_finalize (GObject *object)
{
  HyScanFlCoords *self = HYSCAN_FL_COORDS (object);
  HyScanFlCoordsPrivate *priv = self->priv;

  /* Отключаемся от всех сигналов. */
  g_signal_handlers_disconnect_by_data (priv->fl, self);

  g_object_unref (priv->fl);
  g_object_unref (priv->cache);
  g_object_unref (priv->fl_data);
  g_object_unref (priv->loc);

  G_OBJECT_CLASS (hyscan_fl_coords_parent_class)->finalize (object);
}

static void
hyscan_fl_coords_adj_changed (GtkAdjustment *adjustment,
                              gpointer       udata)
{
  HyScanFlCoords *self = udata;
  self->priv->cur_val = gtk_adjustment_get_value (adjustment);
}
/* Функция создает новый виджет HyScanFlCoords. */
HyScanFlCoords*
hyscan_fl_coords_new (HyScanGtkForwardLook *fl,
                      HyScanCache          *cache)
{
  return g_object_new (HYSCAN_TYPE_FL_COORDS,
                       "fl", fl,
                       "cache", cache,
                       NULL);
}

void
hyscan_fl_coords_set_project (HyScanFlCoords *self,
                              HyScanDB       *db,
                              const gchar    *project,
                              const gchar    *track)
{
  HyScanFlCoordsPrivate *priv;

  g_return_if_fail (HYSCAN_IS_FL_COORDS (self));
  priv = self->priv;

  g_clear_object (&priv->loc);
  g_clear_object (&priv->fl_data);

  priv->loc = hyscan_mloc_new (db, priv->cache, project, track);
  priv->fl_data = hyscan_forward_look_data_new (db, priv->cache, project, track);

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
  guint32 index;
  guint32 n_points;
  HyScanGeoGeodetic geo;

  HyScanFlCoordsPrivate *priv = self->priv;
  GtkCifroArea *carea = GTK_CIFRO_AREA (widget);

  gtk_cifro_area_visible_point_to_value (carea, event->x, event->y, &y_val, &x_val); /* SIC!*/

  priv->mouse_x = y_val;
  priv->mouse_y = x_val;

  if (priv->fl_data == NULL || priv->loc == NULL)
    {
      g_message ("No navigation data!");
      priv->coords_status = FALSE;
      goto exit;
    }

  if (priv->cur_val != priv->val)
    {
      index = priv->cur_val;
      priv->val = priv->cur_val;
      hyscan_forward_look_data_get_doa (priv->fl_data, index, &n_points, &priv->time_for_val);
    }

  status = hyscan_mloc_get (priv->loc, priv->time_for_val, &priv->apos,
                            x_val, y_val, 0, &geo);

  priv->coords = geo;
  priv->coords_status = status;

  g_signal_emit (self, hyscan_fl_coords_signals[SIGNAL_COORDS], 0);

exit:
  gtk_widget_queue_draw (widget);
  return FALSE;
}

static void
hyscan_fl_coords_cross (cairo_t *cairo,
                        gdouble  x,
                        gdouble  y)
{
  cairo_move_to (cairo, x - 10, y - 10);
  cairo_line_to (cairo, x + 10, y + 10);
  cairo_move_to (cairo, x + 10, y - 10);
  cairo_line_to (cairo, x - 10, y + 10);

}

static gboolean
hyscan_fl_coords_visible_draw (GtkWidget       *widget,
                               cairo_t         *cairo,
                               HyScanFlCoords  *self)
{
  gdouble x, y;
  HyScanFlCoordsPrivate *priv = self->priv;

  if (priv->cur_val != priv->val)
    return FALSE;

  /* draw cross in mouse click place */
  gtk_cifro_area_value_to_point (GTK_CIFRO_AREA (widget),
                                 &x, &y,
                                 priv->mouse_x,
                                 priv->mouse_y);

  cairo_save (cairo);

  gdk_cairo_set_source_rgba (cairo, priv->coords_status ? &priv->cyan :
                                                          &priv->red);

  hyscan_fl_coords_cross (cairo, x, y);

  cairo_stroke (cairo);
  cairo_restore (cairo);

  return FALSE;
}

gboolean
hyscan_fl_coords_get_coords (HyScanFlCoords *self,
                             gdouble        *lat,
                             gdouble        *lon)
{
  HyScanFlCoordsPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_FL_COORDS (self), FALSE);
  priv = self->priv;

  *lat = priv->coords.lat;
  *lon = priv->coords.lon;
  return priv->coords_status;
}
