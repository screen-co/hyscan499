#ifndef __HYSCAN_GTK_NAV_INDICATOR_H__
#define __HYSCAN_GTK_NAV_INDICATOR_H__

#include <gtk/gtk.h>
#include <hyscan-buffer.h>
#include <hyscan-sensor.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_NAV_INDICATOR (hyscan_gtk_nav_indicator_get_type ())
#define HYSCAN_GTK_NAV_INDICATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_NAV_INDICATOR, HyScanGtkNavIndicator))
#define HYSCAN_IS_GTK_NAV_INDICATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_NAV_INDICATOR))
#define HYSCAN_GTK_NAV_INDICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_NAV_INDICATOR, HyScanGtkNavIndicatorClass))
#define HYSCAN_IS_GTK_NAV_INDICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_NAV_INDICATOR))
#define HYSCAN_GTK_NAV_INDICATOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_NAV_INDICATOR, HyScanGtkNavIndicatorClass))

typedef struct _HyScanGtkNavIndicator HyScanGtkNavIndicator;
typedef struct _HyScanGtkNavIndicatorPrivate HyScanGtkNavIndicatorPrivate;
typedef struct _HyScanGtkNavIndicatorClass HyScanGtkNavIndicatorClass;

struct _HyScanGtkNavIndicator
{
  GtkBox                         parent_instance;
  HyScanGtkNavIndicatorPrivate  *priv;
};

struct _HyScanGtkNavIndicatorClass
{
  GtkGridClass  parent_class;
};

HYSCAN_API
GType       hyscan_gtk_nav_indicator_get_type  (void);

HYSCAN_API
GtkWidget*  hyscan_gtk_nav_indicator_new       (HyScanSensor            *sensor);

G_END_DECLS

#endif /* __HYSCAN_GTK_NAV_INDICATOR_H__ */
