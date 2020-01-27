#ifndef __HYSCAN_GTK_MAP_PRELOAD_H__
#define __HYSCAN_GTK_MAP_PRELOAD_H__

#include <hyscan-gtk-map.h>
#include <hyscan-gtk-map-base.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_MAP_PRELOAD             (hyscan_gtk_map_preload_get_type ())
#define HYSCAN_GTK_MAP_PRELOAD(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_MAP_PRELOAD, HyScanGtkMapPreload))
#define HYSCAN_IS_GTK_MAP_PRELOAD(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_MAP_PRELOAD))
#define HYSCAN_GTK_MAP_PRELOAD_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_MAP_PRELOAD, HyScanGtkMapPreloadClass))
#define HYSCAN_IS_GTK_MAP_PRELOAD_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_MAP_PRELOAD))
#define HYSCAN_GTK_MAP_PRELOAD_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_MAP_PRELOAD, HyScanGtkMapPreloadClass))

typedef struct _HyScanGtkMapPreload HyScanGtkMapPreload;
typedef struct _HyScanGtkMapPreloadPrivate HyScanGtkMapPreloadPrivate;
typedef struct _HyScanGtkMapPreloadClass HyScanGtkMapPreloadClass;

struct _HyScanGtkMapPreload
{
  GtkBox parent_instance;

  HyScanGtkMapPreloadPrivate *priv;
};

struct _HyScanGtkMapPreloadClass
{
  GtkBoxClass parent_class;
};

GType                  hyscan_gtk_map_preload_get_type         (void);

GtkWidget *            hyscan_gtk_map_preload_new              (HyScanGtkMap     *map,
                                                                HyScanGtkMapBase *base);

G_END_DECLS

#endif /* __HYSCAN_GTK_MAP_PRELOAD_H__ */
