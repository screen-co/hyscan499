#ifndef __HYSCAN_GTK_CONFIGURATOR_H__
#define __HYSCAN_GTK_CONFIGURATOR_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_CONFIGURATOR             (hyscan_gtk_configurator_get_type ())
#define HYSCAN_GTK_CONFIGURATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_CONFIGURATOR, HyScanGtkConfigurator))
#define HYSCAN_IS_GTK_CONFIGURATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_CONFIGURATOR))
#define HYSCAN_GTK_CONFIGURATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_CONFIGURATOR, HyScanGtkConfiguratorClass))
#define HYSCAN_IS_GTK_CONFIGURATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_CONFIGURATOR))
#define HYSCAN_GTK_CONFIGURATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_CONFIGURATOR, HyScanGtkConfiguratorClass))

typedef struct _HyScanGtkConfigurator HyScanGtkConfigurator;
typedef struct _HyScanGtkConfiguratorPrivate HyScanGtkConfiguratorPrivate;
typedef struct _HyScanGtkConfiguratorClass HyScanGtkConfiguratorClass;

struct _HyScanGtkConfigurator
{
  GtkWindow parent_instance;

  HyScanGtkConfiguratorPrivate *priv;
};

struct _HyScanGtkConfiguratorClass
{
  GtkWindowClass parent_class;
};

HYSCAN_API
GType                  hyscan_gtk_configurator_get_type   (void);

HYSCAN_API
GtkWidget *            hyscan_gtk_configurator_new        (const gchar           *settings_ini);

HYSCAN_API
gboolean               hyscan_gtk_configurator_configured (HyScanGtkConfigurator *configurator);

G_END_DECLS

#endif /* __HYSCAN_GTK_CONFIGURATOR_H__ */
