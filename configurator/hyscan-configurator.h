#ifndef __HYSCAN_CONFIGURATOR_H__
#define __HYSCAN_CONFIGURATOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_CONFIGURATOR             (hyscan_configurator_get_type ())
#define HYSCAN_CONFIGURATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_CONFIGURATOR, HyScanConfigurator))
#define HYSCAN_IS_CONFIGURATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_CONFIGURATOR))
#define HYSCAN_CONFIGURATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_CONFIGURATOR, HyScanConfiguratorClass))
#define HYSCAN_IS_CONFIGURATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_CONFIGURATOR))
#define HYSCAN_CONFIGURATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_CONFIGURATOR, HyScanConfiguratorClass))

typedef struct _HyScanConfigurator HyScanConfigurator;
typedef struct _HyScanConfiguratorPrivate HyScanConfiguratorPrivate;
typedef struct _HyScanConfiguratorClass HyScanConfiguratorClass;

struct _HyScanConfigurator
{
  GObject parent_instance;

  HyScanConfiguratorPrivate *priv;
};

struct _HyScanConfiguratorClass
{
  GObjectClass parent_class;
};

GType                  hyscan_configurator_get_type                (void);

HyScanConfigurator *   hyscan_configurator_new                     (const gchar         *path);

const gchar *          hyscan_configurator_get_db_profile_name     (HyScanConfigurator  *configurator);

const gchar *          hyscan_configurator_get_db_dir              (HyScanConfigurator  *configurator);

const gchar *          hyscan_configurator_get_map_dir             (HyScanConfigurator  *configurator);

void                   hyscan_configurator_set_db_profile_name     (HyScanConfigurator  *configurator,
                                                                    const gchar         *db_profile_name);

void                   hyscan_configurator_set_db_dir              (HyScanConfigurator  *configurator,
                                                                    const gchar         *db_dir);

void                   hyscan_configurator_set_map_dir             (HyScanConfigurator  *configurator,
                                                                    const gchar         *map_cache_dir);

gboolean               hyscan_configurator_is_valid                (HyScanConfigurator  *configurator);

gboolean               hyscan_configurator_write                   (HyScanConfigurator  *configurator);

G_END_DECLS

#endif /* __HYSCAN_CONFIGURATOR_H__ */
