#ifndef __HYSCAN_GTK_FORWARD_LOOK_H__
#define __HYSCAN_GTK_FORWARD_LOOK_H__

#include <hyscan-forward-look-player.h>
#include <gtk-cifro-area-control.h>

G_BEGIN_DECLS

typedef enum
{
  HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET,
  HYSCAN_GTK_FORWARD_LOOK_VIEW_SURFACE
} HyScanGtkForwardLookViewType;

#define HYSCAN_TYPE_GTK_FORWARD_LOOK             (hyscan_gtk_forward_look_get_type ())
#define HYSCAN_GTK_FORWARD_LOOK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_FORWARD_LOOK, HyScanGtkForwardLook))
#define HYSCAN_IS_GTK_FORWARD_LOOK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_FORWARD_LOOK))
#define HYSCAN_GTK_FORWARD_LOOK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_FORWARD_LOOK, HyScanGtkForwardLookClass))
#define HYSCAN_IS_GTK_FORWARD_LOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_FORWARD_LOOK))
#define HYSCAN_GTK_FORWARD_LOOK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_FORWARD_LOOK, HyScanGtkForwardLookClass))

typedef struct _HyScanGtkForwardLook HyScanGtkForwardLook;
typedef struct _HyScanGtkForwardLookPrivate HyScanGtkForwardLookPrivate;
typedef struct _HyScanGtkForwardLookClass HyScanGtkForwardLookClass;

struct _HyScanGtkForwardLook
{
  GtkCifroAreaControl parent_instance;

  HyScanGtkForwardLookPrivate *priv;
};

struct _HyScanGtkForwardLookClass
{
  GtkCifroAreaControlClass parent_class;
};

HYSCAN_API
GType                          hyscan_gtk_forward_look_get_type        (void);

HYSCAN_API
GtkWidget                     *hyscan_gtk_forward_look_new             (void);

HYSCAN_API
HyScanForwardLookPlayer       *hyscan_gtk_forward_look_get_player      (HyScanGtkForwardLook          *fl);

HYSCAN_API
GtkAdjustment                 *hyscan_gtk_forward_look_get_adjustment  (HyScanGtkForwardLook          *fl);

HYSCAN_API
void                           hyscan_gtk_forward_look_set_type        (HyScanGtkForwardLook          *fl,
                                                                        HyScanGtkForwardLookViewType   type);

HYSCAN_API
void                           hyscan_gtk_forward_look_set_color       (HyScanGtkForwardLook          *fl,
                                                                        GdkRGBA                        color);

HYSCAN_API
void                           hyscan_gtk_forward_look_set_brightness  (HyScanGtkForwardLook          *fl,
                                                                        gdouble                        brightness);

HYSCAN_API
void                           hyscan_gtk_forward_look_set_sensitivity (HyScanGtkForwardLook          *fl,
                                                                        gdouble                        sensitivity);

G_END_DECLS

#endif /* __HYSCAN_GTK_FORWARD_LOOK_H__ */
