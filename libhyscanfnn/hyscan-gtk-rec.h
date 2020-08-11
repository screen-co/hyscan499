#ifndef __HYSCAN_GTK_REC_H__
#define __HYSCAN_GTK_REC_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_REC             (hyscan_gtk_rec_get_type ())
#define HYSCAN_GTK_REC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_REC, HyScanGtkRec))
#define HYSCAN_IS_GTK_REC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_REC))
#define HYSCAN_GTK_REC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_REC, HyScanGtkRecClass))
#define HYSCAN_IS_GTK_REC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_REC))
#define HYSCAN_GTK_REC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_REC, HyScanGtkRecClass))

typedef struct _HyScanGtkRec HyScanGtkRec;
typedef struct _HyScanGtkRecPrivate HyScanGtkRecPrivate;
typedef struct _HyScanGtkRecClass HyScanGtkRecClass;

struct _HyScanGtkRec
{
  GtkSwitch parent_instance;

  HyScanGtkRecPrivate *priv;
};

struct _HyScanGtkRecClass
{
  GtkSwitchClass parent_class;
};

HYSCAN_API
GType                  hyscan_gtk_rec_get_type         (void);

HYSCAN_API
GtkWidget *            hyscan_gtk_rec_new              (HyScanControlModel *control_model);


G_END_DECLS

#endif /* __HYSCAN_GTK_REC_H__ */
