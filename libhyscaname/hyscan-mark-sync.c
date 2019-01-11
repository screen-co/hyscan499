#include "hyscan-mark-sync.h"
#include <gio/gio.h>
#include <string.h>

enum
{
  PROP_O,
  PROP_ADDRESS,
  PROP_PORT
};

struct _HyScanMarkSyncPrivate
{
  gchar               *address;
  guint16              port;
  GSocket             *socket;
};


static void    hyscan_mark_sync_set_property           (GObject               *object,
                                                        guint                  prop_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
static void    hyscan_mark_sync_object_constructed     (GObject               *object);
static void    hyscan_mark_sync_object_finalize        (GObject               *object);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanMarkSync, hyscan_mark_sync, G_TYPE_OBJECT)

static void
hyscan_mark_sync_class_init (HyScanMarkSyncClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_mark_sync_set_property;

  object_class->constructed = hyscan_mark_sync_object_constructed;
  object_class->finalize = hyscan_mark_sync_object_finalize;

  g_object_class_install_property (object_class, PROP_ADDRESS,
    g_param_spec_string ("address", "Address", "UDP address", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PORT,
    g_param_spec_uint ("port", "Port", "UDP port", 1024, 65535, 10000,
                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_mark_sync_init (HyScanMarkSync *sync)
{
  sync->priv = hyscan_mark_sync_get_instance_private (sync);
}

static void
hyscan_mark_sync_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  HyScanMarkSync *mark_sync = HYSCAN_MARK_SYNC (object);
  HyScanMarkSyncPrivate *priv = mark_sync->priv;

  switch (prop_id)
    {
    case PROP_ADDRESS:
      priv->address = g_value_dup_string (value);
      break;

    case PROP_PORT:
      priv->port = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_mark_sync_object_constructed (GObject *object)
{
  HyScanMarkSync *sync = HYSCAN_MARK_SYNC (object);
  HyScanMarkSyncPrivate *priv = sync->priv;

  GSocketAddress *address;
  GSocketFamily family;

  if (priv->address == NULL)
    return;

  address = g_inet_socket_address_new_from_string (priv->address, priv->port);
  if (address == NULL)
    {
      g_warning ("HyScanMarkSync: can't parse address '%s'", priv->address);
      return;
    }

  family = g_socket_address_get_family (address);
  priv->socket = g_socket_new (family, G_SOCKET_TYPE_DATAGRAM, 0, NULL);
  if (priv->socket == NULL)
    {
      g_warning ("HyScanMarkSync: can't create socket");
      return;
    }

  if (!g_socket_connect (priv->socket, address, NULL, NULL))
    {
      g_warning ("HyScanMarkSync: can't connect to '%s'", priv->address);
      g_clear_object (&priv->socket);
    }

  g_object_unref (address);
}

static void
hyscan_mark_sync_object_finalize (GObject *object)
{
  HyScanMarkSync *sync = HYSCAN_MARK_SYNC (object);
  HyScanMarkSyncPrivate *priv = sync->priv;

  g_clear_object (&priv->socket);
  g_free (priv->address);

  G_OBJECT_CLASS (hyscan_mark_sync_parent_class)->finalize (object);
}

HyScanMarkSync *
hyscan_mark_sync_new (const gchar *address,
                      guint16      port)
{
  return g_object_new (HYSCAN_TYPE_MARK_SYNC,
                       "address", address,
                       "port", port,
                       NULL);
}

gboolean
hyscan_mark_sync_set (HyScanMarkSync *sync,
                      const gchar    *id,
                      const gchar    *name,
                      gdouble         latitude,
                      gdouble         longitude,
                      gdouble         size)
{
  HyScanMarkSyncPrivate *priv;
  guint sentence_length;
  guchar sentence_crc;
  gchar sentence[1024];
  guint i;

  g_return_val_if_fail (HYSCAN_IS_MARK_SYNC (sync), FALSE);

  priv = sync->priv;

  if (priv->socket == NULL)
    return FALSE;

  g_snprintf (sentence, sizeof (sentence),
              "$SET,%s,%s,", id, name);

  sentence_length = strlen (sentence);
  g_ascii_formatd (sentence + sentence_length,
                   sizeof (sentence) - sentence_length,
                   "%f,", latitude);

  sentence_length = strlen (sentence);
  g_ascii_formatd (sentence + sentence_length,
                   sizeof (sentence) - sentence_length,
                   "%f,", longitude);

  sentence_length = strlen (sentence);
  g_ascii_formatd (sentence + sentence_length,
                   sizeof (sentence) - sentence_length,
                   "%f", size);

  sentence_crc = 0;
  sentence_length = strlen (sentence);
  for (i = 1; i < sentence_length; i++)
    sentence_crc ^= sentence[i];
  g_snprintf (sentence + sentence_length,
              sizeof (sentence) - sentence_length,
              "*%02X\r\n", sentence_crc);

  sentence_length = strlen (sentence);

  return (g_socket_send (priv->socket, sentence, sentence_length, NULL, NULL) == sentence_length);
}

gboolean
hyscan_mark_sync_remove (HyScanMarkSync *sync,
                         const gchar    *id)
{
  HyScanMarkSyncPrivate *priv;
  guint sentence_length;
  guchar sentence_crc;
  gchar sentence[1024];
  guint i;

  g_return_val_if_fail (HYSCAN_IS_MARK_SYNC (sync), FALSE);

  priv = sync->priv;

  if (priv->socket == NULL)
    return FALSE;

  g_snprintf (sentence, sizeof (sentence), "$DEL,%s", id);

  sentence_crc = 0;
  sentence_length = strlen (sentence);
  for (i = 1; i < sentence_length; i++)
    sentence_crc ^= sentence[i];
  g_snprintf (sentence + sentence_length,
              sizeof (sentence) - sentence_length,
              "*%02X\r\n", sentence_crc);

  sentence_length = strlen (sentence);

  return (g_socket_send (priv->socket, sentence, sentence_length, NULL, NULL) == sentence_length);
}
