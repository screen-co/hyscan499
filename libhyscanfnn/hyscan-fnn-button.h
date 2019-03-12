#ifndef __HYSCAN_FNN_BUTTON_H__
#define __HYSCAN_FNN_BUTTON_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_FNN_BUTTON             (hyscan_fnn_button_get_type ())
#define HYSCAN_FNN_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FNN_BUTTON, HyScanFnnButton))
#define HYSCAN_IS_FNN_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FNN_BUTTON))
#define HYSCAN_FNN_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FNN_BUTTON, HyScanFnnButtonClass))
#define HYSCAN_IS_FNN_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FNN_BUTTON))
#define HYSCAN_FNN_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FNN_BUTTON, HyScanFnnButtonClass))

typedef struct _HyScanFnnButton HyScanFnnButton;
typedef struct _HyScanFnnButtonPrivate HyScanFnnButtonPrivate;
typedef struct _HyScanFnnButtonClass HyScanFnnButtonClass;
struct _HyScanFnnButton
{
  GtkBox parent_instance;

  HyScanFnnButtonPrivate *priv;
};

struct _HyScanFnnButtonClass
{
  GtkBoxClass parent_class;
};

HYSCAN_API
GType                    hyscan_fnn_button_get_type         (void);

HYSCAN_API
gint                     hyscan_fnn_button_get_size_request (void);

HYSCAN_API
GtkWidget               *hyscan_fnn_button_new              (const gchar     *icon_name,
                                                             const gchar     *label,
                                                             gboolean         is_toggle,
                                                             gboolean         state);

HYSCAN_API
GtkLabel *               hyscan_fnn_button_create_value     (HyScanFnnButton *ab,
                                                             const gchar     *text);

HYSCAN_API
void                     hyscan_fnn_button_activate         (HyScanFnnButton *ab);

HYSCAN_API
void                     hyscan_fnn_button_set_state       (HyScanFnnButton *ab,
                                                             gboolean         state);

HYSCAN_API
void                     hyscan_fnn_button_set_sensitive    (HyScanFnnButton *ab,
                                                             gboolean         state);

G_END_DECLS

#endif /* __HYSCAN_FNN_BUTTON_H__ */
