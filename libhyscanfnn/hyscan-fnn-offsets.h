#ifndef __HYSCAN_FNN_OFFSETS_H__
#define __HYSCAN_FNN_OFFSETS_H__

#include <hyscan-api.h>
#include <hyscan-control.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_FNN_OFFSETS             (hyscan_fnn_offsets_get_type ())
#define HYSCAN_FNN_OFFSETS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FNN_OFFSETS, HyScanFnnOffsets))
#define HYSCAN_IS_FNN_OFFSETS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FNN_OFFSETS))
#define HYSCAN_FNN_OFFSETS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FNN_OFFSETS, HyScanFnnOffsetsClass))
#define HYSCAN_IS_FNN_OFFSETS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FNN_OFFSETS))
#define HYSCAN_FNN_OFFSETS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FNN_OFFSETS, HyScanFnnOffsetsClass))

typedef struct _HyScanFnnOffsets HyScanFnnOffsets;
typedef struct _HyScanFnnOffsetsPrivate HyScanFnnOffsetsPrivate;
typedef struct _HyScanFnnOffsetsClass HyScanFnnOffsetsClass;

struct _HyScanFnnOffsets
{
  GObject parent_instance;

  HyScanFnnOffsetsPrivate *priv;
};

struct _HyScanFnnOffsetsClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType                  hyscan_fnn_offsets_get_type         (void);

HYSCAN_API
HyScanFnnOffsets *     hyscan_fnn_offsets_new              (HyScanControl    *control);

HYSCAN_API
GList *                hyscan_fnn_offsets_get_keys         (HyScanFnnOffsets *self);

HYSCAN_API
const gchar *          hyscan_fnn_offsets_get_master       (HyScanFnnOffsets *self,
                                                            const gchar      *key);

HYSCAN_API
HyScanAntennaOffset    hyscan_fnn_offsets_get_offset       (HyScanFnnOffsets *self,
                                                            const gchar      *key);

HYSCAN_API
gboolean               hyscan_fnn_offsets_set_master       (HyScanFnnOffsets *self,
                                                            const gchar      *key,
                                                            const gchar      *master);

HYSCAN_API
gboolean               hyscan_fnn_offsets_set_offset       (HyScanFnnOffsets    *self,
                                                            const gchar         *key,
                                                            HyScanAntennaOffset  offset);

HYSCAN_API
gboolean                hyscan_fnn_offsets_read             (HyScanFnnOffsets  *self,
                                                             const gchar       *file);
HYSCAN_API
gboolean                hyscan_fnn_offsets_write            (HyScanFnnOffsets *self,
                                                             const gchar      *file);
HYSCAN_API
gboolean                hyscan_fnn_offsets_execute          (HyScanFnnOffsets *self);

G_END_DECLS

#endif /* __HYSCAN_FNN_OFFSETS_H__ */
