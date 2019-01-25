#ifndef __HYSCAN_AME_BUTTON_H__
#define __HYSCAN_AME_BUTTON_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_AME_BUTTON             (hyscan_ame_button_get_type ())
#define HYSCAN_AME_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_AME_BUTTON, HyScanAmeButton))
#define HYSCAN_IS_AME_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_AME_BUTTON))
#define HYSCAN_AME_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_AME_BUTTON, HyScanAmeButtonClass))
#define HYSCAN_IS_AME_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_AME_BUTTON))
#define HYSCAN_AME_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_AME_BUTTON, HyScanAmeButtonClass))

#define HYSCAN_AME_BUTTON_HEIGHT_REQUEST 150

typedef struct _HyScanAmeButton HyScanAmeButton;
typedef struct _HyScanAmeButtonPrivate HyScanAmeButtonPrivate;
typedef struct _HyScanAmeButtonClass HyScanAmeButtonClass;
struct _HyScanAmeButton
{
  GtkBox parent_instance;

  HyScanAmeButtonPrivate *priv;
};

struct _HyScanAmeButtonClass
{
  GtkBoxClass parent_class;
};

HYSCAN_API
GType                    hyscan_ame_button_get_type         (void);

HYSCAN_API
GtkWidget               *hyscan_ame_button_new              (const gchar     *icon_name,
                                                             const gchar     *label,
                                                             gboolean         is_toggle,
                                                             gboolean         state);

HYSCAN_API
GtkLabel *               hyscan_ame_button_create_value     (HyScanAmeButton *ab,
                                                             const gchar     *text);

HYSCAN_API
void                     hyscan_ame_button_activate         (HyScanAmeButton *ab);

HYSCAN_API
void                     hyscan_ame_button_set_state       (HyScanAmeButton *ab,
                                                             gboolean         state);

HYSCAN_API
void                     hyscan_ame_button_set_sensitive    (HyScanAmeButton *ab,
                                                             gboolean         state);

G_END_DECLS

#endif /* __HYSCAN_AME_BUTTON_H__ */
