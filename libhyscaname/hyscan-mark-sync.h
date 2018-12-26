#ifndef __HYSCAN_MARK_SYNC_H__
#define __HYSCAN_MARK_SYNC_H__

#include <glib-object.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_MARK_SYNC             (hyscan_mark_sync_get_type ())
#define HYSCAN_MARK_SYNC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_MARK_SYNC, HyScanMarkSync))
#define HYSCAN_IS_MARK_SYNC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_MARK_SYNC))
#define HYSCAN_MARK_SYNC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_MARK_SYNC, HyScanMarkSyncClass))
#define HYSCAN_IS_MARK_SYNC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_MARK_SYNC))
#define HYSCAN_MARK_SYNC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_MARK_SYNC, HyScanMarkSyncClass))

typedef struct _HyScanMarkSync HyScanMarkSync;
typedef struct _HyScanMarkSyncPrivate HyScanMarkSyncPrivate;
typedef struct _HyScanMarkSyncClass HyScanMarkSyncClass;

struct _HyScanMarkSync
{
  GObject parent_instance;

  HyScanMarkSyncPrivate *priv;
};

struct _HyScanMarkSyncClass
{
  GObjectClass parent_class;
};

GType                  hyscan_mark_sync_get_type         (void);

HYSCAN_API
HyScanMarkSync        *hyscan_mark_sync_new              (const gchar         *address,
                                                          guint16              port);

HYSCAN_API
gboolean               hyscan_mark_sync_set              (HyScanMarkSync      *sync,
                                                          const gchar         *id,
                                                          const gchar         *name,
                                                          gdouble              latitude,
                                                          gdouble              longitude,
                                                          gdouble              size);

HYSCAN_API
gboolean               hyscan_mark_sync_remove           (HyScanMarkSync      *sync,
                                                          const gchar         *id);

G_END_DECLS

#endif /* __HYSCAN_MARK_SYNC_H__ */
