/**
 * \file hyscan-gtk-project-viewer.h
 *
 * \brief Заголовочный файл виджета просмотра проекта.
 * \author Vladimir Maximov (vmakxs@gmail.com)
 * \date 2018
 * \license Проприетарная лицензия ООО "Экран"
 *
 * \defgroup HyScanGtkProjectViewer HyScanGtkProjectViewer - виджет просмотра проекта
 *
 */

#ifndef __HYSCAN_GTK_PROJECT_VIEWER_H__
#define __HYSCAN_GTK_PROJECT_VIEWER_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_PROJECT_VIEWER            \
        (hyscan_gtk_project_viewer_get_type ())

#define HYSCAN_GTK_PROJECT_VIEWER(obj)            \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_PROJECT_VIEWER, HyScanGtkProjectViewer))

#define HYSCAN_IS_GTK_PROJECT_VIEWER(obj)         \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_PROJECT_VIEWER))

#define HYSCAN_GTK_PROJECT_VIEWER_CLASS(klass)    \
        (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_PROJECT_VIEWER, HyScanGtkProjectViewerClass))

#define HYSCAN_IS_GTK_PROJECT_VIEWER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_PROJECT_VIEWER))

#define HYSCAN_GTK_PROJECT_VIEWER_GET_CLASS(obj)  \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_PROJECT_VIEWER, HyScanGtkProjectViewerClass))

typedef struct _HyScanGtkProjectViewer HyScanGtkProjectViewer;
typedef struct _HyScanGtkProjectViewerPrivate HyScanGtkProjectViewerPrivate;
typedef struct _HyScanGtkProjectViewerClass HyScanGtkProjectViewerClass;

struct _HyScanGtkProjectViewer
{
  GtkGrid                          parent_instance;
  HyScanGtkProjectViewerPrivate   *priv;
};

struct _HyScanGtkProjectViewerClass
{
  GtkGridClass   parent_class;
};

HYSCAN_API
GType           hyscan_gtk_project_viewer_get_type            (void);

HYSCAN_API
GtkWidget*      hyscan_gtk_project_viewer_new                 (void);

HYSCAN_API
void            hyscan_gtk_project_viewer_clear               (HyScanGtkProjectViewer   *project_viewer);

HYSCAN_API
void            hyscan_gtk_project_viewer_set_selected_item   (HyScanGtkProjectViewer   *project_viewer,
                                                               const gchar              *selected_item);

HYSCAN_API
const gchar*    hyscan_gtk_project_viewer_get_selected_item   (HyScanGtkProjectViewer   *project_viewer);

HYSCAN_API
GtkListStore*   hyscan_gtk_project_viewer_get_liststore       (HyScanGtkProjectViewer   *project_viewer);

HYSCAN_API
GtkTreeView*    hyscan_gtk_project_viewer_get_tree_view       (HyScanGtkProjectViewer   *project_viewer);

G_END_DECLS

#endif /* __HYSCAN_GTK_PROJECT_VIEWER_H__ */
