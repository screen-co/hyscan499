/* hyscan-gtk-fnn-gliko-wrapper.h
 *
 * Copyright 2019 Screen LLC, Alexander Dmitriev <m1n7@yandex.ru>
 *
 * This file is part of HyScanFNN.
 *
 * HyScanFNN is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HyScanFNN is distributed in the hope that it will be useful,
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

/* HyScanFNN имеет двойную лицензию.
 *
 * Во-первых, вы можете распространять HyScanFNN на условиях Стандартной
 * Общественной Лицензии GNU версии 3, либо по любой более поздней версии
 * лицензии (по вашему выбору). Полные положения лицензии GNU приведены в
 * <http://www.gnu.org/licenses/>.
 *
 * Во-вторых, этот программный код можно использовать по коммерческой
 * лицензии. Для этого свяжитесь с ООО Экран - <info@screen-co.ru>.
 */

/**
 * SECTION: hyscan-gtk-fnn-gliko-wrapper
 * @Title HyScanGtkFnnGlikoWrapper
 * @Short_description
 *
 */

#include "hyscan-gtk-fnn-gliko-wrapper.h"

enum
{
  PROP_0,
  PROP_PLAYER,
  PROP_ADJUSTMENT
};

struct _HyScanGtkFnnGlikoWrapperPrivate
{
  HyScanDataPlayer *player;
  GtkAdjustment    *adjustment;

  GMutex            lock;

  gboolean          changed;
  gint64            left;
  gint64            right;
  gint64            current;

  gulong            signal_handler;
  guint             update_tag;
};

static void    hyscan_gtk_fnn_gliko_wrapper_set_property             (GObject               *object,
                                                                      guint                  prop_id,
                                                                      const GValue          *value,
                                                                      GParamSpec            *pspec);
static void    hyscan_gtk_fnn_gliko_wrapper_object_constructed       (GObject               *object);
static void    hyscan_gtk_fnn_gliko_wrapper_object_finalize          (GObject               *object);
static void    hyscan_gtk_fnn_gliko_wrapper_seek                     (GtkAdjustment            *adj,
                                                                      HyScanGtkFnnGlikoWrapper *self);
static void    hyscan_gtk_fnn_gliko_wrapper_range                    (HyScanDataPlayer         *player,
                                                                      gint64                    left,
                                                                      gint64                    right,
                                                                      HyScanGtkFnnGlikoWrapper *self);
static void    hyscan_gtk_fnn_gliko_wrapper_process                  (HyScanDataPlayer         *player,
                                                                      gint64                    time,
                                                                      HyScanGtkFnnGlikoWrapper *self);
static gboolean hyscan_gtk_fnn_gliko_wrapper_update                  (HyScanGtkFnnGlikoWrapper *self);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkFnnGlikoWrapper, hyscan_gtk_fnn_gliko_wrapper, G_TYPE_OBJECT);

static void
hyscan_gtk_fnn_gliko_wrapper_class_init (HyScanGtkFnnGlikoWrapperClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = hyscan_gtk_fnn_gliko_wrapper_set_property;
  oclass->constructed = hyscan_gtk_fnn_gliko_wrapper_object_constructed;
  oclass->finalize = hyscan_gtk_fnn_gliko_wrapper_object_finalize;

  g_object_class_install_property (oclass, PROP_PLAYER,
    g_param_spec_object ("player", "player", "player", HYSCAN_TYPE_DATA_PLAYER,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  // g_object_class_install_property (oclass, PROP_ADJUSTMENT,
    // g_param_spec_object ("adjustment", "adjustment", "adjustment", GTK_TYPE_ADJUSTMENT,
                         // G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_fnn_gliko_wrapper_init (HyScanGtkFnnGlikoWrapper *self)
{
  HyScanGtkFnnGlikoWrapperPrivate *priv;

  self->priv = hyscan_gtk_fnn_gliko_wrapper_get_instance_private (self);
  priv = self->priv;

  priv->adjustment = g_object_ref_sink (gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 1.0));

  g_mutex_init (&priv->lock);
}

static void
hyscan_gtk_fnn_gliko_wrapper_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  HyScanGtkFnnGlikoWrapper *self = HYSCAN_GTK_FNN_GLIKO_WRAPPER (object);
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_PLAYER:
      priv->player = g_value_dup_object (value);
      break;

    // case PROP_ADJUSTMENT:
      // priv->adjustment = g_value_dup_object (value);
      // break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_fnn_gliko_wrapper_object_constructed (GObject *object)
{
  HyScanGtkFnnGlikoWrapper *self = HYSCAN_GTK_FNN_GLIKO_WRAPPER (object);
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;
  gint64 ltime, rtime;

  G_OBJECT_CLASS (hyscan_gtk_fnn_gliko_wrapper_parent_class)->constructed (object);

  priv->signal_handler = g_signal_connect (priv->adjustment, "value-changed",
                    G_CALLBACK (hyscan_gtk_fnn_gliko_wrapper_seek), self);
  g_signal_connect (priv->player, "range",
                    G_CALLBACK (hyscan_gtk_fnn_gliko_wrapper_range), self);
  g_signal_connect (priv->player, "process",
                    G_CALLBACK (hyscan_gtk_fnn_gliko_wrapper_process), self);

  //hyscan_data_player_get_range (priv->player, &ltime, &rtime);
  //hyscan_gtk_fnn_gliko_wrapper_range (priv->player, ltime, rtime, self);

  priv->update_tag = g_timeout_add (100, (GSourceFunc)hyscan_gtk_fnn_gliko_wrapper_update, self);
}

static void
hyscan_gtk_fnn_gliko_wrapper_object_finalize (GObject *object)
{
  HyScanGtkFnnGlikoWrapper *self = HYSCAN_GTK_FNN_GLIKO_WRAPPER (object);
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;

  g_source_remove (priv->update_tag);

  g_clear_object (&priv->player);
  g_clear_object (&priv->adjustment);

  g_mutex_clear (&priv->lock);


  G_OBJECT_CLASS (hyscan_gtk_fnn_gliko_wrapper_parent_class)->finalize (object);
}

static void
hyscan_gtk_fnn_gliko_wrapper_seek (GtkAdjustment            *adj,
                                   HyScanGtkFnnGlikoWrapper *self)
{
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;
  gdouble current = gtk_adjustment_get_value (priv->adjustment);

  hyscan_data_player_seek (priv->player, current);
}

static void
hyscan_gtk_fnn_gliko_wrapper_range (HyScanDataPlayer         *player,
                                    gint64                    left,
                                    gint64                    right,
                                    HyScanGtkFnnGlikoWrapper *self)
{
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;

  g_mutex_lock (&priv->lock);
  priv->left = left;
  priv->right = right;
  priv->changed = TRUE;
  g_mutex_unlock (&priv->lock);

  //g_idle_add ((GSourceFunc)hyscan_gtk_fnn_gliko_wrapper_update, self);
}

static void
hyscan_gtk_fnn_gliko_wrapper_process (HyScanDataPlayer         *player,
                                      gint64                    time,
                                      HyScanGtkFnnGlikoWrapper *self)
{
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;

  g_mutex_lock (&priv->lock);
  priv->current = time;
  priv->changed = TRUE;
  g_mutex_unlock (&priv->lock);

  //g_idle_add ((GSourceFunc)hyscan_gtk_fnn_gliko_wrapper_update, self);
}

static gboolean
hyscan_gtk_fnn_gliko_wrapper_update (HyScanGtkFnnGlikoWrapper *self)
{
  HyScanGtkFnnGlikoWrapperPrivate *priv = self->priv;
  gdouble current, left, right;
  gboolean changed;

  g_mutex_lock (&priv->lock);
  current = priv->current;
  left = priv->left;
  right = priv->right;
  changed = priv->changed;
  priv->changed = FALSE; 
  g_mutex_unlock (&priv->lock);

  if (!changed)
    return G_SOURCE_CONTINUE;

  g_signal_handler_block (priv->adjustment, priv->signal_handler);
  gtk_adjustment_configure (self->priv->adjustment, current,
                              left, right,
                              1.0, 1.0, 1.0);
  g_signal_handler_unblock (priv->adjustment, priv->signal_handler);

  return G_SOURCE_CONTINUE;
}


HyScanGtkFnnGlikoWrapper *
hyscan_gtk_fnn_gliko_wrapper_new (HyScanDataPlayer *player)
{
  return g_object_new (HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER,
                       "player", player, NULL);
}

GtkAdjustment *
hyscan_gtk_fnn_gliko_wrapper_get_adjustment (HyScanGtkFnnGlikoWrapper *self)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_FNN_GLIKO_WRAPPER (self), NULL);

  return self->priv->adjustment;
}
