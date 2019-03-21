#ifndef __FNN_PARAM_PROXY_H__
#define __FNN_PARAM_PROXY_H__

#include <hyscan-api.h>
#include <hyscan-param.h>

G_BEGIN_DECLS

#define FNN_TYPE_PARAM_PROXY             (fnn_param_proxy_get_type ())
#define FNN_PARAM_PROXY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), FNN_TYPE_PARAM_PROXY, FnnParamProxy))
#define FNN_IS_PARAM_PROXY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FNN_TYPE_PARAM_PROXY))
#define FNN_PARAM_PROXY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), FNN_TYPE_PARAM_PROXY, FnnParamProxyClass))
#define FNN_IS_PARAM_PROXY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), FNN_TYPE_PARAM_PROXY))
#define FNN_PARAM_PROXY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), FNN_TYPE_PARAM_PROXY, FnnParamProxyClass))

typedef struct _FnnParamProxy FnnParamProxy;
typedef struct _FnnParamProxyPrivate FnnParamProxyPrivate;
typedef struct _FnnParamProxyClass FnnParamProxyClass;

/* TODO: Change GObject to type of the base class. */
struct _FnnParamProxy
{
  GObject parent_instance;

  FnnParamProxyPrivate *priv;
};

/* TODO: Change GObjectClass to type of the base class. */
struct _FnnParamProxyClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType                  fnn_param_proxy_get_type         (void);

HYSCAN_API
FnnParamProxy *        fnn_param_proxy_new              (HyScanParam *backend,
                                                         HyScanDataSchema *schema,
                                                         GHashTable *mapping);

G_END_DECLS

#endif /* __FNN_PARAM_PROXY_H__ */
