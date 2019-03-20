#include "hyscan-fnn-offsets.h"


#define HYSCAN_FNN_OFFSETS_MASTER "master"
#define HYSCAN_FNN_OFFSETS_X "x"
#define HYSCAN_FNN_OFFSETS_Y "y"
#define HYSCAN_FNN_OFFSETS_Z "z"
#define HYSCAN_FNN_OFFSETS_PSI "psi"
#define HYSCAN_FNN_OFFSETS_GAMMA "gamma"
#define HYSCAN_FNN_OFFSETS_THETA "theta"
enum
{
  PROP_0,
  PROP_CONTROL
};

typedef struct
{
  gchar               *master;
  gchar               *sensor;
  HyScanSourceType     source;
  HyScanAntennaOffset  offset;
} HyScanFnnOffsetsInfo;

struct _HyScanFnnOffsetsPrivate
{
  HyScanControl *control;

  GHashTable    *antennas;
};

static void    hyscan_fnn_offsets_set_property             (GObject               *object,
                                                             guint                  prop_id,
                                                             const GValue          *value,
                                                             GParamSpec            *pspec);
static void    hyscan_fnn_offsets_object_constructed       (GObject               *object);
static void    hyscan_fnn_offsets_object_finalize          (GObject               *object);
static void    hyscan_fnn_offsets_info_free                (HyScanFnnOffsetsInfo  *info);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanFnnOffsets, hyscan_fnn_offsets, G_TYPE_OBJECT);

static void
hyscan_fnn_offsets_class_init (HyScanFnnOffsetsClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = hyscan_fnn_offsets_set_property;
  oclass->constructed = hyscan_fnn_offsets_object_constructed;
  oclass->finalize = hyscan_fnn_offsets_object_finalize;

  g_object_class_install_property (oclass, PROP_CONTROL,
    g_param_spec_object ("control", "control", "HyScanControl",
                         HYSCAN_TYPE_CONTROL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_fnn_offsets_init (HyScanFnnOffsets *fnn_offsets)
{
  fnn_offsets->priv = hyscan_fnn_offsets_get_instance_private (fnn_offsets);
}

static void
hyscan_fnn_offsets_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HyScanFnnOffsets *self = HYSCAN_FNN_OFFSETS (object);
  HyScanFnnOffsetsPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_CONTROL:
      priv->control = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_fnn_offsets_object_constructed (GObject *object)
{
  HyScanFnnOffsets *self = HYSCAN_FNN_OFFSETS (object);
  HyScanFnnOffsetsPrivate *priv = self->priv;
  const gchar * const * sensors;
  const HyScanSourceType * sources;
  HyScanFnnOffsetsInfo *info;
  guint32 n_sources, i;

  G_OBJECT_CLASS (hyscan_fnn_offsets_parent_class)->constructed (object);

  priv->antennas = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, (GDestroyNotify)hyscan_fnn_offsets_info_free);

  sensors = hyscan_control_sensors_list (priv->control);
  sources = hyscan_control_sources_list (priv->control, &n_sources);

  for (i = 0; i < n_sources; ++i)
    {
      const gchar *name;
      name = hyscan_source_get_name_by_type (sources[i]);

      info = g_new0 (HyScanFnnOffsetsInfo, 1);
      info->source = sources[i];
      g_hash_table_insert (priv->antennas, g_strdup (name), info);
    }

  for (; sensors != NULL && *sensors != NULL; ++sensors)
    {
      info = g_new0 (HyScanFnnOffsetsInfo, 1);
      info->sensor = g_strdup (*sensors);
      g_hash_table_insert (priv->antennas, g_strdup (*sensors), info);
    }
}

static void
hyscan_fnn_offsets_object_finalize (GObject *object)
{
  HyScanFnnOffsets *self = HYSCAN_FNN_OFFSETS (object);
  HyScanFnnOffsetsPrivate *priv = self->priv;

  g_clear_object (&priv->control);
  g_clear_pointer (&priv->antennas, g_hash_table_unref);


  G_OBJECT_CLASS (hyscan_fnn_offsets_parent_class)->finalize (object);
}

HyScanFnnOffsetsInfo *
hyscan_fnn_offsets_find_master (GHashTable  *antennas,
                                const gchar *key)
{
  HyScanFnnOffsetsInfo * info;

  info = g_hash_table_lookup (antennas, key);

  if (info->master == NULL)
    return info;

  do
    {
      info = g_hash_table_lookup (antennas, info->master);
    }
  while (info->master != NULL);

  return info;
}

static void
hyscan_fnn_offsets_info_free (HyScanFnnOffsetsInfo *info)
{
  if (info == NULL)
    return;

  g_clear_pointer (&info->master, g_free);
  g_clear_pointer (&info->sensor, g_free);
  g_free (info);
}

HyScanFnnOffsets *
hyscan_fnn_offsets_new (HyScanControl *control)
{
  if (control == NULL)
    {
      g_warning ("No control");
      return NULL;
    }

  return g_object_new (HYSCAN_TYPE_FNN_OFFSETS, "control", control, NULL);
}

gboolean
hyscan_fnn_offsets_read (HyScanFnnOffsets  *self,
                         const gchar       *file)
{
  GError *error = NULL;
  GKeyFile *keyfile;
  gchar **groups, **iter;
  HyScanFnnOffsetsPrivate *priv;
  gboolean status = TRUE;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), FALSE);
  priv = self->priv;

  keyfile = g_key_file_new ();

  /* Считываю файл. */
  g_key_file_load_from_file (keyfile, file, G_KEY_FILE_NONE, &error);
  if (error != NULL)
    {
      g_warning ("HyScanFnnOffsets: %s", error->message);
      g_error_free (error);
      return FALSE;
    }

  /* Список групп. */
  groups = g_key_file_get_groups (keyfile, NULL);
  if (groups == NULL)
    return FALSE;

  /* Парсим каждую группу. */
  for (iter = groups; *iter != NULL; ++iter)
    {
      HyScanFnnOffsetsInfo *info;

      info = g_hash_table_lookup (priv->antennas, *iter);

      if (info == NULL)
        continue;

      info->master = g_key_file_get_string (keyfile, *iter, HYSCAN_FNN_OFFSETS_MASTER, NULL);

      info->offset.x     = g_key_file_get_double (keyfile, *iter, HYSCAN_FNN_OFFSETS_X, NULL);
      info->offset.y     = g_key_file_get_double (keyfile, *iter, HYSCAN_FNN_OFFSETS_Y, NULL);
      info->offset.z     = g_key_file_get_double (keyfile, *iter, HYSCAN_FNN_OFFSETS_Z, NULL);
      info->offset.psi   = g_key_file_get_double (keyfile, *iter, HYSCAN_FNN_OFFSETS_PSI, NULL);
      info->offset.gamma = g_key_file_get_double (keyfile, *iter, HYSCAN_FNN_OFFSETS_GAMMA, NULL);
      info->offset.theta = g_key_file_get_double (keyfile, *iter, HYSCAN_FNN_OFFSETS_THETA, NULL);
    }

  g_key_file_unref (keyfile);
  return status;
}

gboolean
hyscan_fnn_offsets_write (HyScanFnnOffsets *self,
                          const gchar      *file)
{
  GError *error = NULL;
  GKeyFile *keyfile;
  HyScanFnnOffsetsPrivate *priv;
  GHashTableIter iter;
  const gchar *key;
  const HyScanFnnOffsetsInfo *info;
  gboolean status = TRUE;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), FALSE);
  priv = self->priv;

  keyfile = g_key_file_new ();

  /* Парсим каждую группу. */
  g_hash_table_iter_init (&iter, priv->antennas);
  while (g_hash_table_iter_next (&iter, (gpointer*)&key, (gpointer*)&info))
    {
      if (info->master != NULL)
        g_key_file_set_string (keyfile, key, HYSCAN_FNN_OFFSETS_MASTER, info->master);

      g_key_file_set_double (keyfile, key, HYSCAN_FNN_OFFSETS_X, info->offset.x);
      g_key_file_set_double (keyfile, key, HYSCAN_FNN_OFFSETS_Y, info->offset.y);
      g_key_file_set_double (keyfile, key, HYSCAN_FNN_OFFSETS_Z, info->offset.z);
      g_key_file_set_double (keyfile, key, HYSCAN_FNN_OFFSETS_PSI, info->offset.psi);
      g_key_file_set_double (keyfile, key, HYSCAN_FNN_OFFSETS_GAMMA, info->offset.gamma);
      g_key_file_set_double (keyfile, key, HYSCAN_FNN_OFFSETS_THETA, info->offset.theta);
    }

  /* Считываю файл. */
  g_key_file_save_to_file (keyfile, file, &error);
  if (error != NULL)
    {
      g_warning ("HyScanFnnOffsets: %s", error->message);
      g_error_free (error);
      status = FALSE;
    }

  g_key_file_unref (keyfile);
  return status;
}

gboolean
hyscan_fnn_offsets_execute (HyScanFnnOffsets *self)
{
  HyScanFnnOffsetsPrivate *priv;
  GHashTableIter iter;
  const gchar *key;
  const HyScanFnnOffsetsInfo *info;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), FALSE);
  priv = self->priv;

  /* Парсим каждую группу. */
  g_hash_table_iter_init (&iter, priv->antennas);
  while (g_hash_table_iter_next (&iter, (gpointer*)&key, (gpointer*)&info))
    {
      if (info->master != NULL)
        info = hyscan_fnn_offsets_find_master (priv->antennas, key);

      if (info->sensor != NULL)
        hyscan_control_sensor_set_offset (priv->control, info->sensor, &info->offset);
      else
        hyscan_control_source_set_offset (priv->control, info->source, &info->offset);
    }

  return TRUE;
}

GList *
hyscan_fnn_offsets_get_keys (HyScanFnnOffsets *self)
{
  HyScanFnnOffsetsPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), NULL);
  priv = self->priv;

  return g_hash_table_get_keys (priv->antennas);
}

const gchar *
hyscan_fnn_offsets_get_master (HyScanFnnOffsets *self,
                               const gchar      *key)
{
  HyScanFnnOffsetsPrivate *priv;
  HyScanFnnOffsetsInfo *info;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), NULL);
  priv = self->priv;

  info = g_hash_table_lookup (priv->antennas, key);

  if (info->master != NULL)
    return info->master;
  else
    return NULL;
}

HyScanAntennaOffset
hyscan_fnn_offsets_get_offset (HyScanFnnOffsets *self,
                               const gchar      *key)
{
  HyScanFnnOffsetsPrivate *priv;
  HyScanAntennaOffset offt = {0};
  HyScanFnnOffsetsInfo *info;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), offt);
  priv = self->priv;
  info = hyscan_fnn_offsets_find_master (priv->antennas, key);

  return info->offset;
}

gboolean
hyscan_fnn_offsets_set_master (HyScanFnnOffsets *self,
                               const gchar      *key,
                               const gchar      *master)
{
  HyScanFnnOffsetsPrivate *priv;
  HyScanFnnOffsetsInfo *info;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), FALSE);
  priv = self->priv;

  if (g_str_equal (key, master))
    return FALSE;

  info = g_hash_table_lookup (priv->antennas, key);

  /* Ищем, нет ли тут цикла ? */
  if (info->master != NULL)
    {
      HyScanFnnOffsetsInfo *link;

      for (link = info; link->master != NULL;)
        {
          link = g_hash_table_lookup (priv->antennas, link->master);

          /* Нашли конец цепи мастеров. */
          if (link->master == NULL)
            break;

          /* Если у одного из ключей мастер -- текущий ключ, выходим. */
          if (g_str_equal (link->master, key))
            return FALSE;
        }
    }

  if (info->master != NULL)
    g_clear_pointer (&info->master, g_free);

  info->master = g_strdup (master);

  return TRUE;
}

gboolean
hyscan_fnn_offsets_set_offset (HyScanFnnOffsets    *self,
                               const gchar         *key,
                               HyScanAntennaOffset  offset)
{
  HyScanFnnOffsetsPrivate *priv;
  HyScanFnnOffsetsInfo *info;

  g_return_val_if_fail (HYSCAN_IS_FNN_OFFSETS (self), FALSE);
  priv = self->priv;

  info = g_hash_table_lookup (priv->antennas, key);

  if (info->master != NULL)
    g_clear_pointer (&info->master, g_free);

  info->offset = offset;

  return TRUE;
}
