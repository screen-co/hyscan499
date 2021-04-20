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
  PROP_MODEL,
  PROP_NAME
};

struct _HyScanGtkActuatorControlPrivate
{
  HyScanControlModel *model;
  gchar              *name;

  const HyScanActuatorInfoActuator *ainfo;
  HyScanSourceType   *sources;
  gint                n_sources;

  GtkLabel           *direction_label;
  GtkLabel           *sector_label;

  GtkComboBoxText    *sector;
  GtkSpinButton      *direction;
  GtkAdjustment      *direction_adjustment;
  GtkSwitch          *precise;
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
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkActuatorControl, precise);

  g_object_class_install_property (oclass, PROP_MODEL,
    g_param_spec_object ("model", "model", "HyScanControlModel", HYSCAN_TYPE_CONTROL_MODEL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (oclass, PROP_NAME,
    g_param_spec_string ("name", "name", "Actuator name", NULL,
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

    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Ф-ия обрабатывает все входные данные и при необоходимости задает
 * новые параметры актуатора в режиме сканирования. */
static gboolean
hyscan_gtk_actuator_control_scan (HyScanGtkActuatorControl *self)
{
  HyScanGtkActuatorControlPrivate *priv = self->priv;
  gdouble from, to, speed, sector, direction;
  gdouble max_rec_time = 0;
  const gchar *txt;
  gint i;
  gboolean precise;

  /* Считываю данные с виджетов. */
  precise = gtk_switch_get_active (priv->precise);
  g_message ("precise: %i", precise);
  txt = gtk_combo_box_text_get_active_text (priv->sector);
  sector = g_ascii_strtod (txt, NULL);
  direction = gtk_spin_button_get_value (priv->direction);

  /* Начальный и конечный угол поворота. */
  from = direction - sector / 2.0;
  to = direction + sector / 2.0;
  from = CLAMP (from, priv->ainfo->min_range, priv->ainfo->max_range);
  to = CLAMP (to, priv->ainfo->min_range, priv->ainfo->max_range);

  /* Скорость вращения зависит от выбранных дальностей по бортам. */
  for (i = 0; i < priv->n_sources; ++i)
    {
      HyScanSourceType source = priv->sources[i];
      HyScanSonarReceiverModeType rmode;
      gdouble receive_time, wait_time;
      gboolean ok;

      rmode = hyscan_sonar_state_receiver_get_mode (HYSCAN_SONAR_STATE (priv->model),
                                                    source);
      if (rmode != HYSCAN_SONAR_RECEIVER_MODE_MANUAL)
        {
          g_message ("Can't scan with auto receiver");
          return FALSE;
        }

      ok = hyscan_sonar_state_receiver_get_time (HYSCAN_SONAR_STATE (priv->model),
                                                 source,
                                                 &receive_time, &wait_time);
      if (!ok)
        return FALSE;

      receive_time = receive_time + wait_time;
      max_rec_time = MAX (receive_time, max_rec_time);
    }

  speed = 0.25 / max_rec_time;
  if (!precise)
    speed *= 2.0;

  speed = CLAMP (speed, priv->ainfo->min_speed, priv->ainfo->max_speed);
  g_message ("Actuator control: [%f;%f]@%f", from, to, speed);
  hyscan_actuator_scan (HYSCAN_ACTUATOR (priv->model), priv->name, from, to, speed);

  return FALSE;
}

static void
hyscan_gtk_actuator_control_object_constructed (GObject *object)
{
  HyScanGtkActuatorControl *self = HYSCAN_GTK_ACTUATOR_CONTROL (object);
  HyScanGtkActuatorControlPrivate *priv = self->priv;
  HyScanControl *control;
  const HyScanSourceType *sources;
  guint32 n_sources, i, j;
  const HyScanSonarInfoSource *source_info;

  G_OBJECT_CLASS (hyscan_gtk_actuator_control_parent_class)->constructed (object);

  control = hyscan_control_model_get_control (priv->model);

  /* Информация о моём актуаторе. */
  priv->ainfo = hyscan_control_actuator_get_info (control, priv->name);

  /* Теперь ищу, к каким источникам он привязан, чтобы подсоединиться к сигналам
   * модели на смену параметров. */
  sources = hyscan_control_sources_list (control, &n_sources);
  for (i = 0; i < n_sources; ++i)
    {
      source_info = hyscan_control_source_get_info (control, sources[i]);
      if (0 == g_strcmp0 (priv->name, source_info->actuator))
        priv->n_sources += 1;
    }

  priv->sources = g_malloc0_n (sizeof(HyScanSourceType), priv->n_sources);

  for (j = 0, i = 0; i < n_sources; ++i)
    {
      HyScanSourceType src = sources[i];
      // gchar *signal_name;

      source_info = hyscan_control_source_get_info (control, src);
      if (0 != g_strcmp0 (priv->name, source_info->actuator))
        continue;

      priv->sources[j++] = src;
      // signal_name = g_strdup_printf ("sonar::%s", hyscan_source_get_id_by_type (src));
      // g_free (signal_name);
    }

  g_signal_connect_swapped (priv->model, "sonar",
                            G_CALLBACK (hyscan_gtk_actuator_control_scan), self);
  g_signal_connect_swapped (priv->sector, "changed",
                            G_CALLBACK (hyscan_gtk_actuator_control_scan), self);
  g_signal_connect_swapped (priv->direction, "value-changed",
                            G_CALLBACK (hyscan_gtk_actuator_control_scan), self);
  g_signal_connect_swapped (priv->precise, "state-set",
                            G_CALLBACK (hyscan_gtk_actuator_control_scan), self);

  g_clear_object (&control);
}

static void
hyscan_gtk_actuator_control_object_finalize (GObject *object)
{
  HyScanGtkActuatorControl *self = HYSCAN_GTK_ACTUATOR_CONTROL (object);
  HyScanGtkActuatorControlPrivate *priv = self->priv;

  g_clear_object (&priv->model);
  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->sources, g_free);

  G_OBJECT_CLASS (hyscan_gtk_actuator_control_parent_class)->finalize (object);
}

GtkWidget *
hyscan_gtk_actuator_control_new (HyScanControlModel *model,
                                 const gchar        *name)
{
  return g_object_new (HYSCAN_TYPE_GTK_ACTUATOR_CONTROL,
                       "model", model,
                       "name", name,
                       NULL);
}

const gchar *
hyscan_gtk_actuator_control_find_actuator_by_source (HyScanControlModel *model,
                                                     HyScanSourceType    source)
{
  HyScanControl *control;
  const HyScanSonarInfoSource *source_info;

  g_return_val_if_fail (HYSCAN_IS_CONTROL_MODEL(model), NULL);

  control = hyscan_control_model_get_control (model);
  if (control == NULL)
    return NULL;

  source_info = hyscan_control_source_get_info (control, source);
  g_clear_object (&control);

  if (source_info == NULL)
    return NULL;

  return source_info->actuator;
}
