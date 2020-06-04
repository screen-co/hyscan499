#ifndef __HYSCAN_MIGRATE_CONFIG_H__
#define __HYSCAN_MIGRATE_CONFIG_H__

#include <glib-object.h>
#include <hyscan-cancellable.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_MIGRATE_CONFIG             (hyscan_migrate_config_get_type ())
#define HYSCAN_MIGRATE_CONFIG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_MIGRATE_CONFIG, HyScanMigrateConfig))
#define HYSCAN_IS_MIGRATE_CONFIG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_MIGRATE_CONFIG))
#define HYSCAN_MIGRATE_CONFIG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_MIGRATE_CONFIG, HyScanMigrateConfigClass))
#define HYSCAN_IS_MIGRATE_CONFIG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_MIGRATE_CONFIG))
#define HYSCAN_MIGRATE_CONFIG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_MIGRATE_CONFIG, HyScanMigrateConfigClass))

typedef struct _HyScanMigrateConfig HyScanMigrateConfig;
typedef struct _HyScanMigrateConfigPrivate HyScanMigrateConfigPrivate;
typedef struct _HyScanMigrateConfigClass HyScanMigrateConfigClass;

struct _HyScanMigrateConfig
{
  GObject parent_instance;

  HyScanMigrateConfigPrivate *priv;
};

struct _HyScanMigrateConfigClass
{
  GObjectClass parent_class;
};

enum
{
  HYSCAN_MIGRATE_CONFIG_STATUS_INVALID,
  HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY,
  HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED,
  HYSCAN_MIGRATE_CONFIG_STATUS_LATEST,
};

GType                  hyscan_migrate_config_get_type         (void);

HyScanMigrateConfig *  hyscan_migrate_config_new              (const gchar         *path);

gboolean               hyscan_migrate_config_run              (HyScanMigrateConfig *migrate_config,
                                                               HyScanCancellable   *cancellable);

gint                   hyscan_migrate_config_status           (HyScanMigrateConfig *migrate_config);

G_END_DECLS

#endif /* __HYSCAN_MIGRATE_CONFIG_H__ */
