#ifndef __HYSCAN_GTK_FNN_BOX_H__
#define __HYSCAN_GTK_FNN_BOX_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_FNN_BOX             (hyscan_gtk_fnn_box_get_type ())
#define HYSCAN_GTK_FNN_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_FNN_BOX, HyScanGtkFnnBox))
#define HYSCAN_IS_GTK_FNN_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_FNN_BOX))
#define HYSCAN_GTK_FNN_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_FNN_BOX, HyScanGtkFnnBoxClass))
#define HYSCAN_IS_GTK_FNN_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_FNN_BOX))
#define HYSCAN_GTK_FNN_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_FNN_BOX, HyScanGtkFnnBoxClass))

typedef struct _HyScanGtkFnnBox HyScanGtkFnnBox;
typedef struct _HyScanGtkFnnBoxPrivate HyScanGtkFnnBoxPrivate;
typedef struct _HyScanGtkFnnBoxClass HyScanGtkFnnBoxClass;

/* TODO: Change GtkGrid to type of the base class. */
struct _HyScanGtkFnnBox
{
  GtkGrid parent_instance;

  HyScanGtkFnnBoxPrivate *priv;
};

/* TODO: Change GtkGridClass to type of the base class. */
struct _HyScanGtkFnnBoxClass
{
  GtkGridClass parent_class;
};

HYSCAN_API
GType                  hyscan_gtk_fnn_box_get_type         (void);

HYSCAN_API
GtkWidget *            hyscan_gtk_fnn_box_new              (void);

HYSCAN_API
void                   hyscan_gtk_fnn_box_pack             (HyScanGtkFnnBox *box,
                                                            GtkWidget       *widget,
                                                            gint             id,
                                                            gint             left,
                                                            gint             top,
                                                            gint             width,
                                                            gint             height);

HYSCAN_API
void                   hyscan_gtk_fnn_box_set_visible      (HyScanGtkFnnBox *box,
                                                            gint             id);

HYSCAN_API
gint                   hyscan_gtk_fnn_box_next_visible     (HyScanGtkFnnBox *box);

HYSCAN_API
void                   hyscan_gtk_fnn_box_show_all         (HyScanGtkFnnBox *box);


G_END_DECLS

#endif /* __HYSCAN_GTK_FNN_BOX_H__ */
