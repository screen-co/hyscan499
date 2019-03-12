/* evo-gtk-datetime.c
 *
 * Copyright 2018 Screen LLC, Alexander Dmitriev <m1n7@yandex.ru>
 *
 * This file is part of EvoGui.
 *
 * EvoGui is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EvoGui is distributed in the hope that it will be useful,
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

/* EvoGui имеет двойную лицензию.
 *
 * Во-первых, вы можете распространять EvoGui на условиях Стандартной
 * Общественной Лицензии GNU версии 3, либо по любой более поздней версии
 * лицензии (по вашему выбору). Полные положения лицензии GNU приведены в
 * <http://www.gnu.org/licenses/>.
 *
 * Во-вторых, этот программный код можно использовать по коммерческой
 * лицензии. Для этого свяжитесь с ООО Экран - <info@screen-co.ru>.
 */

/**
 * SECTION: evo-gtk-datetime
 * @Short_description: виджет даты и времени
 * @Title: EvoGtkDatetime
 *
 * Виджет отображает заданное время и дату или что-то одно.
 * Временная зона всегда UTC.
 */
#include "evo-sensors.h"

#define EVO_SENSORS_STYLE "evo-gtk-datetime"

enum
{
  PROP_0,
  PROP_CONTROL,
};

enum
{
  COL_NFNN,
  COL_STATE,
  N_COLUMNS
};

struct _EvoSensorsPrivate
{
  HyScanControl *control;

  GtkTreeModel  *store;
};

static void        evo_sensors_object_set_property  (GObject           *object,
                                                     guint              prop_id,
                                                     const GValue      *value,
                                                     GParamSpec        *pspec);
static void        evo_sensors_object_constructed   (GObject           *object);
static void        evo_sensors_object_finalize      (GObject           *object);

static GtkTreeModel * evo_sensors_make_model        (EvoSensorsPrivate *priv);
static void        evo_sensors_make_tree            (EvoSensors *self);
static void        evo_sensors_fill_model           (EvoSensorsPrivate *priv);
static void        evo_sensors_toggled              (GtkCellRendererToggle *cell_renderer,
                                                     gchar                 *path,
                                                     gpointer               user_data);

G_DEFINE_TYPE_WITH_PRIVATE (EvoSensors, evo_sensors, GTK_TYPE_TREE_VIEW);

static void
evo_sensors_class_init (EvoSensorsClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = evo_sensors_object_set_property;
  oclass->constructed = evo_sensors_object_constructed;
  oclass->finalize = evo_sensors_object_finalize;

  g_object_class_install_property (oclass, PROP_CONTROL,
    g_param_spec_object("control", "control", "HyScanControl",
                        HYSCAN_TYPE_CONTROL,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
evo_sensors_init (EvoSensors *self)
{
  self->priv = evo_sensors_get_instance_private (self);
}

static void
evo_sensors_object_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  EvoSensors *self = EVO_SENSORS (object);

  switch (prop_id)
    {
    case PROP_CONTROL:
      self->priv->control = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
evo_sensors_object_constructed (GObject *object)
{
  EvoSensors *self = EVO_SENSORS (object);
  EvoSensorsPrivate *priv = self->priv;

  G_OBJECT_CLASS (evo_sensors_parent_class)->constructed (object);

  if (priv->control == NULL)
    return;

  priv->store = evo_sensors_make_model (priv);
  evo_sensors_make_tree (self);

  gtk_tree_view_set_model (GTK_TREE_VIEW (self), priv->store);

  evo_sensors_fill_model (priv);

  gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);
}

static void
evo_sensors_object_finalize (GObject *object)
{
  EvoSensors *self = EVO_SENSORS (object);
  EvoSensorsPrivate *priv = self->priv;

  g_object_unref (priv->control);

  G_OBJECT_CLASS (evo_sensors_parent_class)->finalize (object);
}

static GtkTreeModel *
evo_sensors_make_model (EvoSensorsPrivate *priv)
{
  GtkListStore *store = NULL;
  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN);
  return GTK_TREE_MODEL (store);
}

static void
evo_sensors_make_tree (EvoSensors *self)
{
  GtkTreeView *tself = GTK_TREE_VIEW (self);
  GtkCellRenderer *renderer = NULL;
  GtkTreeViewColumn *column = NULL;

  gtk_tree_view_set_enable_tree_lines (tself, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
                                                     "text", COL_NFNN,
                                                     NULL);
  gtk_tree_view_column_set_visible (column, TRUE);
  gtk_tree_view_append_column (tself, column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("State", renderer,
                                                     "active", COL_STATE,
                                                     NULL);
  g_signal_connect (renderer, "toggled", G_CALLBACK (evo_sensors_toggled), self);
  gtk_tree_view_append_column (tself, column);
}

static void
evo_sensors_fill_model (EvoSensorsPrivate *priv)
{
  const gchar * const * sensors;

  sensors = hyscan_control_sensors_list (priv->control);

  for (; *sensors != NULL; ++sensors)
    {
      GtkTreeIter iter;
      gtk_list_store_append (GTK_LIST_STORE (priv->store), &iter);
      gtk_list_store_set (GTK_LIST_STORE (priv->store), &iter,
                          COL_NFNN, *sensors,
                          COL_STATE, TRUE,
                          -1);
    }
}

static void
evo_sensors_toggled (GtkCellRendererToggle *cell_renderer,
                     gchar                 *path,
                     gpointer               user_data)
{
  EvoSensors *self = user_data;
  EvoSensorsPrivate *priv = self->priv;
  gboolean active;
  GtkTreeIter iter;
  gchar * name;

  active = !gtk_cell_renderer_toggle_get_active (cell_renderer);

  gtk_tree_model_get_iter_from_string (priv->store, &iter, path);
  gtk_tree_model_get (priv->store, &iter, COL_NFNN, &name, -1);


  if (hyscan_sensor_set_enable (HYSCAN_SENSOR (priv->control), name, active))
    {
      g_message ("Sensor %s is now %s", name, active ? "ON" : "OFF");
      gtk_list_store_set (GTK_LIST_STORE (priv->store), &iter, COL_STATE, active, -1);
    }
  else
    {
      g_message ("Couldn't turn sensor %s %s", name, active ? "ON" : "OFF");
    }

  g_free (name);
}

GtkWidget *
evo_sensors_new (HyScanControl *control)
{
  if (control == NULL)
    {
      g_warning ("No control");
      return NULL;
    }

  return g_object_new (EVO_TYPE_SENSORS, "control", control, NULL);
}
