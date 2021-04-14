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
  GtkWidget           *progress_bar;
  GtkWidget           *progress_page;
  GtkWidget           *page12124, page1, page12, page1212, ty_pidor;
  GtkWidget           *current;
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
  aclass->close = gtk_widget_destroy;

  gtk_widget_class_set_template_from_resource (wclass, "/org/libhyscanfnn/gtk/hyscan-gtk-fnn-export.ui");
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, chooser);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, progress_bar);
  gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, progress_page);
  // gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, page12124);
  // gtk_widget_class_bind_template_child_private (wclass, HyScanGtkFnnExport, page1);
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

  /*
  GError *err = NULL;
  GBytes * bts =  g_resources_lookup_data ("/org/libhyscanfnn/gtk/hyscan-gtk-fnn-export.ui",
                           G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
  if (err)
    g_message ("fuck: %s", err->message);
  else
    g_message ("unfuck: %s", (gchar*)g_bytes_get_data (bts, NULL));
  */
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

  /* Запускаем процесс. */
  g_message("start export, pidorok");

}

static void
hyscan_gtk_fnn_export_apply (GtkAssistant *assistant)
{
  HyScanGtkFnnExport *self = HYSCAN_GTK_FNN_EXPORT (assistant);
  HyScanGtkFnnExportPrivate *priv = self->priv;

  hyscan_map_track_model_get_tracks (priv->tmodel);
  g_message ("Tracks: %s", g_strjoinv("\n\t", hyscan_map_track_model_get_tracks (priv->tmodel)));
  g_message("apply для пидоров");

  gtk_assistant_set_page_complete (assistant, priv->progress_page, TRUE);
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
