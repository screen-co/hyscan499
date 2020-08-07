/* hyscan-gtk-navigator-app.h
 *
 * Copyright 2020 Screen LLC, Alexey Sakhnov <alexsakhnov@gmail.com>
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

#ifndef __HYSCAN_GTK_NAVIGATOR_APP_H__
#define __HYSCAN_GTK_NAVIGATOR_APP_H__

#include <gtk/gtk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_NAVIGATOR_APP             (hyscan_gtk_navigator_app_get_type ())
#define HYSCAN_GTK_NAVIGATOR_APP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_NAVIGATOR_APP, HyScanGtkNavigatorApp))
#define HYSCAN_IS_GTK_NAVIGATOR_APP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_NAVIGATOR_APP))
#define HYSCAN_GTK_NAVIGATOR_APP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_NAVIGATOR_APP, HyScanGtkNavigatorAppClass))
#define HYSCAN_IS_GTK_NAVIGATOR_APP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_NAVIGATOR_APP))
#define HYSCAN_GTK_NAVIGATOR_APP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_NAVIGATOR_APP, HyScanGtkNavigatorAppClass))

typedef struct _HyScanGtkNavigatorApp HyScanGtkNavigatorApp;
typedef struct _HyScanGtkNavigatorAppPrivate HyScanGtkNavigatorAppPrivate;
typedef struct _HyScanGtkNavigatorAppClass HyScanGtkNavigatorAppClass;

struct _HyScanGtkNavigatorApp
{
  GtkApplication parent_instance;

  HyScanGtkNavigatorAppPrivate *priv;
};

struct _HyScanGtkNavigatorAppClass
{
  GtkApplicationClass parent_class;
};

GType                  hyscan_gtk_navigator_app_get_type         (void);

GtkApplication *       hyscan_gtk_navigator_app_new              (const gchar  *db_uri,
                                                                  gchar       **drivers);

G_END_DECLS

#endif /* __HYSCAN_GTK_NAVIGATOR_APP_H__ */
