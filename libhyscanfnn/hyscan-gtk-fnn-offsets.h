#ifndef __HYSCAN_GTK_FNN_OFFSETS_H__
#define __HYSCAN_GTK_FNN_OFFSETS_H__

#include <hyscan-fnn-offsets.h>
#include <hyscan-api.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_FNN_OFFSETS             (hyscan_gtk_fnn_offsets_get_type ())
#define HYSCAN_GTK_FNN_OFFSETS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_FNN_OFFSETS, HyScanGtkFnnOffsets))
#define HYSCAN_IS_GTK_FNN_OFFSETS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_FNN_OFFSETS))
#define HYSCAN_GTK_FNN_OFFSETS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_FNN_OFFSETS, HyScanGtkFnnOffsetsClass))
#define HYSCAN_IS_GTK_FNN_OFFSETS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_FNN_OFFSETS))
#define HYSCAN_GTK_FNN_OFFSETS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_FNN_OFFSETS, HyScanGtkFnnOffsetsClass))

typedef struct _HyScanGtkFnnOffsets HyScanGtkFnnOffsets;
typedef struct _HyScanGtkFnnOffsetsPrivate HyScanGtkFnnOffsetsPrivate;
typedef struct _HyScanGtkFnnOffsetsClass HyScanGtkFnnOffsetsClass;

struct _HyScanGtkFnnOffsets
{
  GtkDialog parent_instance;

  HyScanGtkFnnOffsetsPrivate *priv;
};

struct _HyScanGtkFnnOffsetsClass
{
  GtkDialogClass parent_class;
};

HYSCAN_API
GType                   hyscan_gtk_fnn_offsets_get_type     (void);

HYSCAN_API
GtkWidget              *hyscan_gtk_fnn_offsets_new          (HyScanControl *control,
                                                             GtkWindow     *parent);

G_END_DECLS

#endif /* __HYSCAN_GTK_FNN_OFFSETS_H__ */
