#ifndef __HYSCAN_FNN_SPLASH_H__
#define __HYSCAN_FNN_SPLASH_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_FNN_SPLASH             (hyscan_fnn_splash_get_type ())
#define HYSCAN_FNN_SPLASH(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FNN_SPLASH, HyScanFnnSplash))
#define HYSCAN_IS_FNN_SPLASH(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FNN_SPLASH))
#define HYSCAN_FNN_SPLASH_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FNN_SPLASH, HyScanFnnSplashClass))
#define HYSCAN_IS_FNN_SPLASH_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FNN_SPLASH))
#define HYSCAN_FNN_SPLASH_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FNN_SPLASH, HyScanFnnSplashClass))

typedef struct _HyScanFnnSplash HyScanFnnSplash;
typedef struct _HyScanFnnSplashPrivate HyScanFnnSplashPrivate;
typedef struct _HyScanFnnSplashClass HyScanFnnSplashClass;

struct _HyScanFnnSplash
{
  GtkDialog parent_instance;

  HyScanFnnSplashPrivate *priv;
};

struct _HyScanFnnSplashClass
{
  GtkDialogClass parent_class;
};

HYSCAN_API
GType                  hyscan_fnn_splash_get_type         (void);

HYSCAN_API
HyScanFnnSplash       *hyscan_fnn_splash_new              (const gchar *message);

G_END_DECLS

#endif /* __HYSCAN_FNN_SPLASH_H__ */
