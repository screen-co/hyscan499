/* hyscan-gtk-dev-indicator.c
 *
 * Copyright 2019 Screen LLC, Alexey Sakhnov <alexsakhnov@gmail.com>
 *
 * This file is part of HyScanFnn library.
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
 * SECTION: hyscan-gtk-dev-indicator
 * @Short_description: Виджет статуса подключения оборудования.
 * @Title: HyScanGtkDevIndicator
 *
 * Виджет периодически проверяет информацию о статусе оборудования и отображает
 * агрегированный статус по всем устройствам. Во всплывающей подсказке показывается
 * детальная информация по каждому из устройств.
 *
 * Предполагается использование виджета в статусной строке.
 *
 */

#define GETTEXT_PACKAGE "libhyscanfnn"
#include "hyscan-gtk-dev-indicator.h"
#include <glib/gi18n-lib.h>

#define STATUS_QUERY_INTERVAL 5000

enum
{
  PROP_O,
  PROP_CONTROL
};

struct _HyScanGtkDevIndicatorPrivate
{
  HyScanControl           *control;            /* Объект управления устройствами. */

  gchar                  **devices;            /* Идентификаторы устройств. */
  guint                    n_devices;          /* Число устройств. */
  HyScanDeviceStatusType  *statuses;           /* Статусы устройств. */
  HyScanDeviceStatusType   status;             /* Общий статус подключения. */

  GtkWidget               *tooltip_grid;       /* Виджет всплывающей подсказки. */
  GtkWidget              **status_labels;      /* Виджеты GtkLabel для отображения статусов. */
  guint                    query_tag;          /* Тэг таймаут-функции по запросу статуса. */
};

static void          hyscan_gtk_dev_indicator_set_property             (GObject                  *object,
                                                                        guint                     prop_id,
                                                                        const GValue             *value,
                                                                        GParamSpec               *pspec);
static void          hyscan_gtk_dev_indicator_object_constructed       (GObject                  *object);
static void          hyscan_gtk_dev_indicator_object_finalize          (GObject                  *object);
static gboolean      hyscan_gtk_dev_indicator_dev_status_query         (HyScanGtkDevIndicator    *indicator);
void                 hyscan_gtk_dev_indicator_dev_status               (GTask                    *task,
                                                                        HyScanGtkDevIndicator    *indicator,
                                                                        gpointer                  task_data,
                                                                        GCancellable             *cancellable);
static void          hyscan_gtk_dev_indicator_dev_status_ready         (HyScanGtkDevIndicator    *indicator,
                                                                        GTask                    *task,
                                                                        gpointer                  user_data);
static void          hyscan_gtk_dev_indicator_dev_set_status           (HyScanGtkDevIndicator    *indicator,
                                                                        HyScanDeviceStatusType    status);
static gboolean      hyscan_gtk_dev_indicator_query_tooltip            (GtkWidget                *widget,
                                                                        gint                      x,
                                                                        gint                      y,
                                                                        gboolean                  keyboard_tooltip,
                                                                        GtkTooltip               *tooltip);
static const gchar * hyscan_gtk_dev_indicator_get_status_name          (HyScanDeviceStatusType    status);
static const gchar * hyscan_gtk_dev_indicator_get_status_class         (HyScanDeviceStatusType    status);
static const gchar * hyscan_gtk_dev_indicator_get_status_color         (HyScanDeviceStatusType    status);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkDevIndicator, hyscan_gtk_dev_indicator, GTK_TYPE_LABEL)

static void
hyscan_gtk_dev_indicator_class_init (HyScanGtkDevIndicatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GdkScreen *screen;
  GtkCssProvider *provider;

  object_class->set_property = hyscan_gtk_dev_indicator_set_property;

  object_class->constructed = hyscan_gtk_dev_indicator_object_constructed;
  object_class->finalize = hyscan_gtk_dev_indicator_object_finalize;
  widget_class->query_tooltip = hyscan_gtk_dev_indicator_query_tooltip;

  g_object_class_install_property (object_class, PROP_CONTROL,
    g_param_spec_object ("control", "HyScanControl", "HyScanControl", HYSCAN_TYPE_CONTROL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));


  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/org/libhyscanfnn/gtk/hyscan-gtk-dev-indicator.css");

  screen = gdk_display_get_default_screen (gdk_display_get_default ());
  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  gtk_widget_class_set_css_name (widget_class, "devindicator");
}

static void
hyscan_gtk_dev_indicator_init (HyScanGtkDevIndicator *gtk_dev_indicator)
{
  gtk_dev_indicator->priv = hyscan_gtk_dev_indicator_get_instance_private (gtk_dev_indicator);
}

static void
hyscan_gtk_dev_indicator_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  HyScanGtkDevIndicator *gtk_dev_indicator = HYSCAN_GTK_DEV_INDICATOR (object);
  HyScanGtkDevIndicatorPrivate *priv = gtk_dev_indicator->priv;

  switch (prop_id)
    {
    case PROP_CONTROL:
      priv->control = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_dev_indicator_object_constructed (GObject *object)
{
  HyScanGtkDevIndicator *indicator = HYSCAN_GTK_DEV_INDICATOR (object);
  HyScanGtkDevIndicatorPrivate *priv = indicator->priv;

  G_OBJECT_CLASS (hyscan_gtk_dev_indicator_parent_class)->constructed (object);

  g_return_if_fail (priv->control != NULL);

  priv->devices = g_strdupv ((gchar **) hyscan_control_devices_list (priv->control));
  priv->n_devices = g_strv_length (priv->devices);

  gtk_widget_set_has_tooltip (GTK_WIDGET (indicator), TRUE);
  hyscan_gtk_dev_indicator_dev_set_status (indicator, HYSCAN_DEVICE_STATUS_ERROR);

  priv->query_tag = g_idle_add ((GSourceFunc) hyscan_gtk_dev_indicator_dev_status_query, indicator);
}

static void
hyscan_gtk_dev_indicator_object_finalize (GObject *object)
{
  HyScanGtkDevIndicator *gtk_dev_indicator = HYSCAN_GTK_DEV_INDICATOR (object);
  HyScanGtkDevIndicatorPrivate *priv = gtk_dev_indicator->priv;

  if (priv->query_tag != 0)
    g_source_remove (priv->query_tag);

  g_clear_object (&priv->tooltip_grid);
  g_clear_object (&priv->control);
  g_clear_pointer (&priv->devices, g_strfreev);
  g_free (priv->statuses);
  g_free (priv->status_labels);

  G_OBJECT_CLASS (hyscan_gtk_dev_indicator_parent_class)->finalize (object);
}

static gboolean
hyscan_gtk_dev_indicator_query_tooltip (GtkWidget  *widget,
                                        gint        x,
                                        gint        y,
                                        gboolean    keyboard_tooltip,
                                        GtkTooltip *tooltip)
{
  HyScanGtkDevIndicator *indicator = HYSCAN_GTK_DEV_INDICATOR (widget);
  HyScanGtkDevIndicatorPrivate *priv = indicator->priv;

  gint i;

  if (priv->tooltip_grid == NULL)
    {
      GtkWidget *grid;

      grid = gtk_grid_new ();
      gtk_grid_attach (GTK_GRID (grid), gtk_label_new (_("Device status")), 0, 0, 2, 1);

      priv->status_labels = g_new0 (GtkWidget *, priv->n_devices);
      for (i = 0; priv->devices[i] != NULL; i++)
        {
          GtkWidget *label;

          priv->status_labels[i] = gtk_label_new (NULL);

          label = gtk_label_new (priv->devices[i]);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);

          gtk_grid_attach (GTK_GRID (grid), label, 0, i + 1, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), priv->status_labels[i], 1, i + 1, 1, 1);
        }

      /* Увеличиваем счетчик, т.к. при скрытии подсказки ref_count уменьшается до 0. */
      priv->tooltip_grid = g_object_ref (grid);
    }

  gtk_widget_show_all (priv->tooltip_grid);
  gtk_tooltip_set_custom (tooltip, priv->tooltip_grid);

  for (i = 0; priv->devices[i] != NULL; i++)
    {
      HyScanDeviceStatusType dev_status;
      gchar *markup;

      dev_status = priv->statuses[i];
      markup = g_strdup_printf ("<span size=\"small\" color=\"%s\">%s</span>",
                                hyscan_gtk_dev_indicator_get_status_color (dev_status),
                                hyscan_gtk_dev_indicator_get_status_name (dev_status));
      gtk_label_set_markup (GTK_LABEL (priv->status_labels[i]), markup);

      g_free (markup);
    }

  return TRUE;
}

void
hyscan_gtk_dev_indicator_dev_status (GTask                 *task,
                                     HyScanGtkDevIndicator *indicator,
                                     gpointer               task_data,
                                     GCancellable          *cancellable)
{
  HyScanGtkDevIndicatorPrivate *priv = indicator->priv;
  HyScanDeviceStatusType *statuses;
  gint i;

  statuses = g_new0 (HyScanDeviceStatusType, priv->n_devices);

  for (i = 0; priv->devices[i] != NULL; i++)
    statuses[i] = hyscan_control_device_get_status (priv->control, priv->devices[i]);

  g_task_return_pointer (task, statuses, g_free);
}

static const gchar *
hyscan_gtk_dev_indicator_get_status_name (HyScanDeviceStatusType status)
{
    switch (status)
    {
      case HYSCAN_DEVICE_STATUS_OK:
        return _("OK");

      case HYSCAN_DEVICE_STATUS_WARNING:
        return _("Warning");

      case HYSCAN_DEVICE_STATUS_CRITICAL:
        return _("Critical");

      case HYSCAN_DEVICE_STATUS_ERROR:
        return _("Error");

      default:
        return _("Unknown");
    }
}

static const gchar *
hyscan_gtk_dev_indicator_get_status_color (HyScanDeviceStatusType status)
{
    switch (status)
    {
      case HYSCAN_DEVICE_STATUS_OK:
        return "#73d216";

      case HYSCAN_DEVICE_STATUS_WARNING:
        return "#b4a60b";

      case HYSCAN_DEVICE_STATUS_CRITICAL:
        return "#f57900";

      case HYSCAN_DEVICE_STATUS_ERROR:
      default:
        return "#cc0000";
    }
}

static const gchar *
hyscan_gtk_dev_indicator_get_status_class (HyScanDeviceStatusType status)
{
    switch (status)
    {
      case HYSCAN_DEVICE_STATUS_OK:
        return "status-ok";

      case HYSCAN_DEVICE_STATUS_WARNING:
        return "status-warning";

      case HYSCAN_DEVICE_STATUS_CRITICAL:
        return "status-critical";

      case HYSCAN_DEVICE_STATUS_ERROR:
        return "status-error";

      default:
        return NULL;
    }
}

static void
hyscan_gtk_dev_indicator_dev_set_status (HyScanGtkDevIndicator  *indicator,
                                         HyScanDeviceStatusType  status)
{
  HyScanGtkDevIndicatorPrivate *priv = indicator->priv;
  GtkStyleContext *context;
  const gchar *rm_class, *add_class;

  context = gtk_widget_get_style_context (GTK_WIDGET (indicator));
  add_class = hyscan_gtk_dev_indicator_get_status_class (status);
  rm_class = hyscan_gtk_dev_indicator_get_status_class (priv->status);

  gtk_style_context_remove_class (context, rm_class);
  gtk_style_context_add_class (context, add_class);
  gtk_label_set_text (GTK_LABEL (indicator), hyscan_gtk_dev_indicator_get_status_name (status));
  priv->status = status;
}

static void
hyscan_gtk_dev_indicator_dev_status_ready (HyScanGtkDevIndicator *indicator,
                                           GTask                 *task,
                                           gpointer               user_data)
{
  HyScanGtkDevIndicatorPrivate *priv = indicator->priv;
  HyScanDeviceStatusType status;

  g_free (priv->statuses);
  priv->statuses = g_task_propagate_pointer (task, NULL);

  if (priv->statuses != NULL)
    {
      guint i;

      status = HYSCAN_DEVICE_STATUS_OK;
      for (i = 0; i < priv->n_devices; i++)
        status = MIN (status, priv->statuses[i]);
    }
  else
    {
      status = HYSCAN_DEVICE_STATUS_ERROR;
    }

  if (status != priv->status)
    hyscan_gtk_dev_indicator_dev_set_status (indicator, status);

  /* Запрашиваем статус ещё раз. */
  priv->query_tag = g_timeout_add (STATUS_QUERY_INTERVAL,
                                   (GSourceFunc) hyscan_gtk_dev_indicator_dev_status_query, indicator);
}

static gboolean
hyscan_gtk_dev_indicator_dev_status_query (HyScanGtkDevIndicator *indicator)
{
  HyScanGtkDevIndicatorPrivate *priv = indicator->priv;
  GTask *task;

  task = g_task_new (indicator, NULL, (GAsyncReadyCallback) hyscan_gtk_dev_indicator_dev_status_ready, NULL);
  g_task_run_in_thread (task, (GTaskThreadFunc) hyscan_gtk_dev_indicator_dev_status);
  g_object_unref (task);

  priv->query_tag = 0;

  return G_SOURCE_REMOVE;
}

GtkWidget *
hyscan_gtk_dev_indicator_new (HyScanControl *control)
{
  return g_object_new (HYSCAN_TYPE_GTK_DEV_INDICATOR,
                       "control", control, NULL);
}
