/* hyscan-gtk-actuator-control.h
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
 * SECTION: hyscan-gtk-actuator-control
 * @Title HyScanGtkActuatorControl
 * @Short_description
 *
 */

#include "hyscan-gtk-actuator-control.h"
#define GETTEXT_PACKAGE "libhyscanfnn"
#include <glib/gi18n-lib.h>

enum
{
  PROP_0,
  PROP_MODEL
};

struct _HyScanGtkActuatorControlPrivate
{
  HyScanControlModel *model;

  GtkLabel           *direction_label;
  GtkLabel           *sector_label;

  GtkComboBoxText    *sector;
  GtkSpinButton      *direction;
  GtkAdjustment      *direction_adjustment;
};

static void    hyscan_gtk_actuator_control_set_property             (GObject               *object,
                                                                     guint                  prop_id,
                                                                     const GValue          *value,
                                                                     GParamSpec            *pspec);
static void    hyscan_gtk_actuator_control_object_constructed       (GObject               *object);
static void    hyscan_gtk_actuator_control_object_finalize          (GObject               *object);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkActuatorControl, hyscan_gtk_actuator_control, GTK_TYPE_GRID);

static void
hyscan_gtk_actuator_control_class_init (HyScanGtkActuatorControlClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);

  oclass->set_property = hyscan_gtk_actuator_control_set_property;
  oclass->constructed = hyscan_gtk_actuator_control_object_constructed;
  oclass->finalize = hyscan_gtk_actuator_control_object_finalize;

  gtk_widget_class_set_template_from_resource (wclass, "/org/libhyscanfnn/gtk/hyscan-gtk-actuator-control.ui");
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkActuatorControl, direction_label);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkActuatorControl, sector_label);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkActuatorControl, sector);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkActuatorControl, direction);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkActuatorControl, direction_adjustment);

  g_object_class_install_property (oclass, PROP_MODEL,
    g_param_spec_object("model", "model", "HyScanControlModel", HYSCAN_TYPE_CONTROL_MODEL,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
hyscan_gtk_actuator_control_init (HyScanGtkActuatorControl *self)
{
  self->priv = hyscan_gtk_actuator_control_get_instance_private (self);
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
hyscan_gtk_actuator_control_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  HyScanGtkActuatorControl *self = HYSCAN_GTK_ACTUATOR_CONTROL (object);
  HyScanGtkActuatorControlPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_MODEL:
      priv->model = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
sector_changed (GtkComboBoxText          *combo,
                HyScanGtkActuatorControl *self)
{
  const gchar *txt = gtk_combo_box_text_get_active_text (combo);
  g_message ("Sector %s %s", txt, gtk_combo_box_get_active_id(combo));
}

static void
spin_change (GtkSpinButton            *spin,
             HyScanGtkActuatorControl *self)
{

  g_message ("direction %f", gtk_spin_button_get_value(spin));
}

static void
hyscan_gtk_actuator_control_object_constructed (GObject *object)
{
  HyScanGtkActuatorControl *self = HYSCAN_GTK_ACTUATOR_CONTROL (object);
  HyScanGtkActuatorControlPrivate *priv = self->priv;
  HyScanControl *control;

  G_OBJECT_CLASS (hyscan_gtk_actuator_control_parent_class)->constructed (object);

  control = hyscan_control_model_get_control (priv->model);


  g_signal_connect (priv->sector, "changed", G_CALLBACK (sector_changed), self);
  g_signal_connect (priv->direction, "value-changed", G_CALLBACK (spin_change), self);

  g_clear_object (&control);
}

static void
hyscan_gtk_actuator_control_object_finalize (GObject *object)
{
  HyScanGtkActuatorControl *self = HYSCAN_GTK_ACTUATOR_CONTROL (object);
  HyScanGtkActuatorControlPrivate *priv = self->priv;

  g_clear_object (&priv->model);

  G_OBJECT_CLASS (hyscan_gtk_actuator_control_parent_class)->finalize (object);
}

GtkWidget *
hyscan_gtk_actuator_control_new (HyScanControlModel *model)
{
  return g_object_new (HYSCAN_TYPE_GTK_ACTUATOR_CONTROL,
                       "model", model,
                       NULL);
}
