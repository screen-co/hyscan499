#ifndef __HYSCAN_GTK_AME_BOX_H__
#define __HYSCAN_GTK_AME_BOX_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_AME_BOX             (hyscan_gtk_ame_box_get_type ())
#define HYSCAN_GTK_AME_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_AME_BOX, HyScanGtkAmeBox))
#define HYSCAN_IS_GTK_AME_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_AME_BOX))
#define HYSCAN_GTK_AME_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_AME_BOX, HyScanGtkAmeBoxClass))
#define HYSCAN_IS_GTK_AME_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_AME_BOX))
#define HYSCAN_GTK_AME_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_AME_BOX, HyScanGtkAmeBoxClass))

typedef struct _HyScanGtkAmeBox HyScanGtkAmeBox;
typedef struct _HyScanGtkAmeBoxPrivate HyScanGtkAmeBoxPrivate;
typedef struct _HyScanGtkAmeBoxClass HyScanGtkAmeBoxClass;

/* TODO: Change GtkGrid to type of the base class. */
struct _HyScanGtkAmeBox
{
  GtkGrid parent_instance;

  HyScanGtkAmeBoxPrivate *priv;
};

/* TODO: Change GtkGridClass to type of the base class. */
struct _HyScanGtkAmeBoxClass
{
  GtkGridClass parent_class;
};

HYSCAN_API
GType                  hyscan_gtk_ame_box_get_type         (void);

HYSCAN_API
GtkWidget *            hyscan_gtk_ame_box_new              (void);

HYSCAN_API
void                   hyscan_gtk_ame_box_pack             (HyScanGtkAmeBox *box,
                                                            GtkWidget       *widget,
                                                            gint             id,
                                                            gint             left,
                                                            gint             top,
                                                            gint             width,
                                                            gint             height);

HYSCAN_API
void                   hyscan_gtk_ame_box_set_visible      (HyScanGtkAmeBox *box,
                                                            gint             id);

HYSCAN_API
gint                   hyscan_gtk_ame_box_next_visible     (HyScanGtkAmeBox *box);

HYSCAN_API
void                   hyscan_gtk_ame_box_show_all         (HyScanGtkAmeBox *box);


G_END_DECLS

#endif /* __HYSCAN_GTK_AME_BOX_H__ */
