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

#ifndef __HYSCAN_GTK_FNN_GLIKO_WRAPPER_H__
#define __HYSCAN_GTK_FNN_GLIKO_WRAPPER_H__

#include <hyscan-data-player.h>
#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER             (hyscan_gtk_fnn_gliko_wrapper_get_type ())
#define HYSCAN_GTK_FNN_GLIKO_WRAPPER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER, HyScanGtkFnnGlikoWrapper))
#define HYSCAN_IS_GTK_FNN_GLIKO_WRAPPER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER))
#define HYSCAN_GTK_FNN_GLIKO_WRAPPER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER, HyScanGtkFnnGlikoWrapperClass))
#define HYSCAN_IS_GTK_FNN_GLIKO_WRAPPER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER))
#define HYSCAN_GTK_FNN_GLIKO_WRAPPER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_FNN_GLIKO_WRAPPER, HyScanGtkFnnGlikoWrapperClass))

typedef struct _HyScanGtkFnnGlikoWrapper HyScanGtkFnnGlikoWrapper;
typedef struct _HyScanGtkFnnGlikoWrapperPrivate HyScanGtkFnnGlikoWrapperPrivate;
typedef struct _HyScanGtkFnnGlikoWrapperClass HyScanGtkFnnGlikoWrapperClass;

struct _HyScanGtkFnnGlikoWrapper
{
  GObject parent_instance;

  HyScanGtkFnnGlikoWrapperPrivate *priv;
};

struct _HyScanGtkFnnGlikoWrapperClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType                       hyscan_gtk_fnn_gliko_wrapper_get_type         (void);

HYSCAN_API
HyScanGtkFnnGlikoWrapper *  hyscan_gtk_fnn_gliko_wrapper_new              (HyScanDataPlayer         *player);
HYSCAN_API
GtkAdjustment *             hyscan_gtk_fnn_gliko_wrapper_get_adjustment   (HyScanGtkFnnGlikoWrapper *self);

G_END_DECLS

#endif /* __HYSCAN_GTK_FNN_GLIKO_WRAPPER_H__ */
