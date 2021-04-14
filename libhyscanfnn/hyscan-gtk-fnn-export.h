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

#ifndef __HYSCAN_GTK_FNN_EXPORT_H__
#define __HYSCAN_GTK_FNN_EXPORT_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>
#include <hyscan-db-info.h>
#include <hyscan-cache.h>
#include <hyscan-map-track-model.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_FNN_EXPORT             (hyscan_gtk_fnn_export_get_type ())
#define HYSCAN_GTK_FNN_EXPORT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_FNN_EXPORT, HyScanGtkFnnExport))
#define HYSCAN_IS_GTK_FNN_EXPORT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_FNN_EXPORT))
#define HYSCAN_GTK_FNN_EXPORT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_FNN_EXPORT, HyScanGtkFnnExportClass))
#define HYSCAN_IS_GTK_FNN_EXPORT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_FNN_EXPORT))
#define HYSCAN_GTK_FNN_EXPORT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_FNN_EXPORT, HyScanGtkFnnExportClass))

typedef struct _HyScanGtkFnnExport HyScanGtkFnnExport;
typedef struct _HyScanGtkFnnExportPrivate HyScanGtkFnnExportPrivate;
typedef struct _HyScanGtkFnnExportClass HyScanGtkFnnExportClass;

struct _HyScanGtkFnnExport
{
  GtkAssistant parent_instance;

  HyScanGtkFnnExportPrivate *priv;
};

struct _HyScanGtkFnnExportClass
{
  GtkAssistantClass parent_class;
};

HYSCAN_API
GType                  hyscan_gtk_fnn_export_get_type         (void);

HYSCAN_API
HyScanGtkFnnExport *        hyscan_gtk_fnn_export_new        (HyScanDBInfo        *info,
                                                              HyScanCache         *cache,
                                                              HyScanMapTrackModel *tmodel);

G_END_DECLS

#endif /* __HYSCAN_GTK_FNN_EXPORT_H__ */
