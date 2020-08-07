#ifndef __HYSCAN_GTK_MAP_GO_H__
#define __HYSCAN_GTK_MAP_GO_H__

#include <hyscan-gtk-map.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_MAP_GO             (hyscan_gtk_map_go_get_type ())
#define HYSCAN_GTK_MAP_GO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_MAP_GO, HyScanGtkMapGo))
#define HYSCAN_IS_GTK_MAP_GO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_MAP_GO))
#define HYSCAN_GTK_MAP_GO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_MAP_GO, HyScanGtkMapGoClass))
#define HYSCAN_IS_GTK_MAP_GO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_MAP_GO))
#define HYSCAN_GTK_MAP_GO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_MAP_GO, HyScanGtkMapGoClass))

typedef struct _HyScanGtkMapGo HyScanGtkMapGo;
typedef struct _HyScanGtkMapGoPrivate HyScanGtkMapGoPrivate;
typedef struct _HyScanGtkMapGoClass HyScanGtkMapGoClass;

struct _HyScanGtkMapGo
{
  GtkBox parent_instance;

  HyScanGtkMapGoPrivate *priv;
};

struct _HyScanGtkMapGoClass
{
  GtkBoxClass parent_class;
};

HYSCAN_API
GType                  hyscan_gtk_map_go_get_type         (void);

HYSCAN_API
GtkWidget *            hyscan_gtk_map_go_new             (HyScanGtkMap *map);

G_END_DECLS

#endif /* __HYSCAN_GTK_MAP_GO_H__ */
