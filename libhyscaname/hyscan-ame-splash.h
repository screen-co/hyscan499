#ifndef __HYSCAN_AME_SPLASH_H__
#define __HYSCAN_AME_SPLASH_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_AME_SPLASH             (hyscan_ame_splash_get_type ())
#define HYSCAN_AME_SPLASH(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_AME_SPLASH, HyScanAmeSplash))
#define HYSCAN_IS_AME_SPLASH(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_AME_SPLASH))
#define HYSCAN_AME_SPLASH_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_AME_SPLASH, HyScanAmeSplashClass))
#define HYSCAN_IS_AME_SPLASH_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_AME_SPLASH))
#define HYSCAN_AME_SPLASH_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_AME_SPLASH, HyScanAmeSplashClass))

typedef struct _HyScanAmeSplash HyScanAmeSplash;
typedef struct _HyScanAmeSplashPrivate HyScanAmeSplashPrivate;
typedef struct _HyScanAmeSplashClass HyScanAmeSplashClass;

struct _HyScanAmeSplash
{
  GtkDialog parent_instance;

  HyScanAmeSplashPrivate *priv;
};

struct _HyScanAmeSplashClass
{
  GtkDialogClass parent_class;
};

HYSCAN_API
GType                  hyscan_ame_splash_get_type         (void);

HYSCAN_API
HyScanAmeSplash       *hyscan_ame_splash_new              (const gchar *message);

G_END_DECLS

#endif /* __HYSCAN_AME_SPLASH_H__ */
