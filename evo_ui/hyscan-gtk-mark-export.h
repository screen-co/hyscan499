/*
 * hyscan-gtk-mark-export.h
 *
 *  Created on: 20 нояб. 2019 г.
 *      Author: Andrey Zakharov <zaharov@screen-co.ru>
 */

#ifndef __HYSCAN_GTK_MARK_EXPORT_H__
#define __HYSCAN_GTK_MARK_EXPORT_H__

#include <gtk/gtk.h>
#include <hyscan-mark-loc-model.h>
#include <hyscan-object-model.h>

void
hyscan_gtk_mark_export_save_as_csv       (GtkWindow          *window,
                                          HyScanMarkLocModel *ml_model,
                                          HyScanObjectModel  *mark_geo_model,
                                          gchar              *project_name);

void
hyscan_gtk_mark_export_copy_to_clipboard (HyScanMarkLocModel *ml_model,
                                          HyScanObjectModel  *mark_geo_model,
                                          gchar              *project_name);

gchar *
hyscan_gtk_mark_export_to_str (HyScanMarkLocModel *ml_model,
                               HyScanObjectModel  *mark_geo_model,
                               gchar              *project_name);

#endif /* __HYSCAN_GTK_MARK_EXPORT_H__ */

