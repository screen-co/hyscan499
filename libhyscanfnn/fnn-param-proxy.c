#include "fnn-param-proxy.h"

enum
{
  PROP_0,
  PROP_PARAM,
  PROP_SCHEMA,
  PROP_MAPPING
};

struct _FnnParamProxyPrivate
{
  HyScanParam      *backend;
  GHashTable       *mapping; /* k: new_path, v: old_path */
  HyScanDataSchema *schema;
};

static void    fnn_param_proxy_interface_init           (HyScanParamInterface  *iface);
static void    fnn_param_proxy_set_property             (GObject               *object,
                                                          guint                  prop_id,
                                                          const GValue          *value,
                                                          GParamSpec            *pspec);
static void    fnn_param_proxy_object_constructed       (GObject               *object);
static void    fnn_param_proxy_object_finalize          (GObject               *object);

G_DEFINE_TYPE_WITH_CODE (FnnParamProxy, fnn_param_proxy, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (FnnParamProxy)
                         G_IMPLEMENT_INTERFACE (HYSCAN_TYPE_PARAM, fnn_param_proxy_interface_init));


static void
fnn_param_proxy_class_init (FnnParamProxyClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = fnn_param_proxy_set_property;
  oclass->constructed = fnn_param_proxy_object_constructed;
  oclass->finalize = fnn_param_proxy_object_finalize;

  g_object_class_install_property (oclass, PROP_PARAM,
    g_param_spec_object ("param", "Param", "HyScanParam", HYSCAN_TYPE_PARAM,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (oclass, PROP_SCHEMA,
    g_param_spec_object ("schema", "Schema", "HyScanDataSchema", HYSCAN_TYPE_DATA_SCHEMA,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (oclass, PROP_SCHEMA,
    g_param_spec_pointer ("mapping", "Mapping", "Key mapping",
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
fnn_param_proxy_init (FnnParamProxy *param_proxy)
{
  param_proxy->priv = fnn_param_proxy_get_instance_private (param_proxy);
}

static void
fnn_param_proxy_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  FnnParamProxy *self = FNN_PARAM_PROXY (object);
  FnnParamProxyPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_PARAM:
      priv->backend = g_value_dup_object (value);
      break;
    case PROP_SCHEMA:
      priv->schema = g_value_dup_object (value);
      break;
    case PROP_MAPPING:
      priv->mapping = g_hash_table_ref (g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
fnn_param_proxy_object_constructed (GObject *object)
{
  G_OBJECT_CLASS (fnn_param_proxy_parent_class)->constructed (object);
}

static void
fnn_param_proxy_object_finalize (GObject *object)
{
  FnnParamProxy *self = FNN_PARAM_PROXY (object);
  FnnParamProxyPrivate *priv = self->priv;

  g_clear_object (&priv->backend);
  g_clear_object (&priv->schema);
  g_clear_pointer (&priv->mapping, g_hash_table_unref);

  G_OBJECT_CLASS (fnn_param_proxy_parent_class)->finalize (object);
}

FnnParamProxy *
fnn_param_proxy_new (HyScanParam *param,
                     HyScanDataSchema *schema,
                     GHashTable *mapping)
{
  return g_object_new (FNN_TYPE_PARAM_PROXY,
                       "param", param,
                       "schema", schema,
                       "mapping", mapping,
                       NULL);
}

/* Функция устанавливает значения параметров. */
static gboolean
fnn_param_proxy_set (HyScanParam     *param,
                     HyScanParamList *list)
{
  FnnParamProxy *self = FNN_PARAM_PROXY (param);
  FnnParamProxyPrivate *priv = self->priv;
  HyScanParamList *real_list;
  const gchar * const * keys;
  gboolean status;

  real_list = hyscan_param_list_new ();
  keys = hyscan_param_list_params (list);

  for (; *keys != NULL; ++keys)
    {
      GVariant *value;
      const gchar * real_key;

      real_key = g_hash_table_lookup (priv->mapping, *keys);

      if (real_key == NULL)
        continue;

      value = hyscan_param_list_get (list, *keys);
      hyscan_param_list_set (real_list, real_key, value);
      g_variant_unref (value);
    }

  status = hyscan_param_set (priv->backend, real_list);
  g_object_unref (real_list);
  return status;
}

static gboolean
fnn_param_proxy_get (HyScanParam     *param,
                     HyScanParamList *list)
{
  FnnParamProxy *self = FNN_PARAM_PROXY (param);
  FnnParamProxyPrivate *priv = self->priv;
  HyScanParamList *real_list;
  const gchar * const * keys;
  const gchar *real_key;
  gboolean status;

  real_list = hyscan_param_list_new ();
  keys = hyscan_param_list_params (list);

  /* stage 1, remap plist */
  for (; *keys != NULL; ++keys)
    {
      real_key = g_hash_table_lookup (priv->mapping, *keys);

      if (real_key == NULL)
        {
          g_warning( "knf %s", *keys);
          g_object_unref (real_list);
          return FALSE;
        }

      hyscan_param_list_add (real_list, real_key);
    }

  /* stage 2, real get */
  status = hyscan_param_get (priv->backend, real_list);

  if (!status)
    {
      g_object_unref (real_list);
      return FALSE;
    }

  /* stage 3, rewrite*/
  keys = hyscan_param_list_params (list);
  for (; *keys != NULL; ++keys)
    {
      GVariant *value;
      real_key = g_hash_table_lookup (priv->mapping, *keys);

      if (real_key == NULL)
        continue;

      value = hyscan_param_list_get (real_list, real_key);
      hyscan_param_list_set (list, *keys, value);
      g_variant_unref (value);
    }

  g_object_unref (real_list);
  return TRUE;
}

/* Функция возвращает схему параметров. */
static HyScanDataSchema *
fnn_param_proxy_schema (HyScanParam *param)
{
  FnnParamProxy *self = FNN_PARAM_PROXY (param);
  FnnParamProxyPrivate *priv = self->priv;

  return g_object_ref (priv->schema);
}

static void
fnn_param_proxy_interface_init (HyScanParamInterface *iface)
{
  iface->schema = fnn_param_proxy_schema;
  iface->set = fnn_param_proxy_set;
  iface->get = fnn_param_proxy_get;
}
