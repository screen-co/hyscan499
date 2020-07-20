#include <hyscan-profile.h>
#include "hyscan-migrate-config.h"

enum
{
  PROP_O,
  PROP_PATH,
};

enum HyScanMigrateConfigDb
{
  HYSCAN_MIGRATE_CONFIG_DB_INVALID = -1,
  HYSCAN_MIGRATE_CONFIG_DB_0,
  HYSCAN_MIGRATE_CONFIG_DB_65D2C45,
  HYSCAN_MIGRATE_CONFIG_DB_20200100,
  HYSCAN_MIGRATE_CONFIG_DB_LATEST  = HYSCAN_MIGRATE_CONFIG_DB_20200100,
};

struct _HyScanMigrateConfigPrivate
{
  gchar             *path;
};

static void      hyscan_migrate_config_set_property             (GObject               *object,
                                                                 guint                  prop_id,
                                                                 const GValue          *value,
                                                                 GParamSpec            *pspec);
static void      hyscan_migrate_config_object_finalize          (GObject               *object);
static guint64   hyscan_migrate_config_db_version_uint64        (gint                   version);
static gint      hyscan_migrate_config_profile_version          (GKeyFile              *key_file);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanMigrateConfig, hyscan_migrate_config, G_TYPE_OBJECT)

static void
hyscan_migrate_config_class_init (HyScanMigrateConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_migrate_config_set_property;

  object_class->finalize = hyscan_migrate_config_object_finalize;

  g_object_class_install_property (object_class, PROP_PATH,
    g_param_spec_string ("path", "Path", "Path to configuration files", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_migrate_config_init (HyScanMigrateConfig *migrate_config)
{
  migrate_config->priv = hyscan_migrate_config_get_instance_private (migrate_config);
}

static void
hyscan_migrate_config_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  HyScanMigrateConfig *migrate_config = HYSCAN_MIGRATE_CONFIG (object);
  HyScanMigrateConfigPrivate *priv = migrate_config->priv;

  switch (prop_id)
    {
    case PROP_PATH:
      priv->path = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_migrate_config_object_finalize (GObject *object)
{
  HyScanMigrateConfig *migrate_config = HYSCAN_MIGRATE_CONFIG (object);
  HyScanMigrateConfigPrivate *priv = migrate_config->priv;

  g_free (priv->path);

  G_OBJECT_CLASS (hyscan_migrate_config_parent_class)->finalize (object);
}

static guint64
hyscan_migrate_config_db_version_uint64 (gint version)
{
  switch (version)
    {
    case HYSCAN_MIGRATE_CONFIG_DB_0:
      return 0;

    case HYSCAN_MIGRATE_CONFIG_DB_65D2C45:
      return 0x965D2C45;

    case HYSCAN_MIGRATE_CONFIG_DB_20200100:
      return 20200100;

    default:
      g_warning ("HyScanMigrateConfig: unknown db version %lu", version);
      return 0;
    }
}

static gint
hyscan_migrate_config_profile_version (GKeyFile *key_file)
{
  guint64 version;

  if (!g_key_file_has_group (key_file, HYSCAN_PROFILE_INFO_GROUP))
    return HYSCAN_MIGRATE_CONFIG_DB_0;

  version = g_key_file_get_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version", NULL);
  switch (version)
    {
    case 0x965D2C45:
      return HYSCAN_MIGRATE_CONFIG_DB_65D2C45;

    default:
      g_warning ("HyScanMigrateConfig: unknown db version %lu", version);
      return HYSCAN_MIGRATE_CONFIG_DB_INVALID;
    }
}

/* Миграция первой версии: название профиля перенесно в группу "_". */
static gboolean
hyscan_migrate_config_profile_db_0 (GKeyFile *key_file)
{
  gchar *name;

  name = g_key_file_get_string (key_file, "db", "name", NULL);
  g_key_file_remove_key (key_file, "db", "name", NULL);

  g_key_file_set_string (key_file, HYSCAN_PROFILE_INFO_GROUP, "name", name);
  g_key_file_set_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version",
                         hyscan_migrate_config_db_version_uint64 (HYSCAN_MIGRATE_CONFIG_DB_65D2C45));

  g_free (name);

  return TRUE;
}

static gboolean
hyscan_migrate_config_profile_db (GKeyFile *key_file)
{
  gint version;

  version = hyscan_migrate_config_profile_version (key_file);
  switch (version)
    {
      case HYSCAN_MIGRATE_CONFIG_DB_0:
        if (!hyscan_migrate_config_profile_db_0 (key_file))
          return FALSE;

      case HYSCAN_MIGRATE_CONFIG_DB_65D2C45:
        return TRUE;

      default:
        g_warning ("HyScanMigrateConfig: unknown db profile version %lu", version);
        return FALSE;
    }
}

static gchar **
hyscan_migrate_config_profiles_db (HyScanMigrateConfig *migrate_config,
                                   const gchar         *subdir)
{
  HyScanMigrateConfigPrivate *priv = migrate_config->priv;
  GDir *dir;
  gchar *path;
  const gchar *name;
  GArray *names = NULL;

  path = g_build_filename (priv->path, subdir, NULL);
  dir = g_dir_open (path, 0, NULL);

  if (dir == NULL)
    goto exit;

  names = g_array_new (TRUE, TRUE, sizeof (gchar *));

  while ((name = g_dir_read_name (dir)) != NULL)
    {
      gchar *full = g_build_filename (path, name, NULL);

      if (g_file_test (full, G_FILE_TEST_IS_REGULAR))
        g_array_append_val (names, full);
      else
        g_free (full);
    }

exit:
  g_clear_pointer (&dir, g_dir_close);
  g_free (path);

  return names != NULL ? (gchar**) g_array_free (names, FALSE) : NULL;
}

HyScanMigrateConfig *
hyscan_migrate_config_new (const gchar *path)
{
  return g_object_new (HYSCAN_TYPE_MIGRATE_CONFIG,
                       "path", path,
                       NULL);
}

gint
hyscan_migrate_config_status (HyScanMigrateConfig *migrate_config)
{
  gint status;
  gchar **db_profiles;
  gint i, n_profiles;
  GKeyFile *key_file = NULL;

  g_return_val_if_fail (HYSCAN_IS_MIGRATE_CONFIG (migrate_config), HYSCAN_MIGRATE_CONFIG_STATUS_INVALID);

  db_profiles = hyscan_migrate_config_profiles_db (migrate_config, "db-profiles");
  if (db_profiles == NULL)
    return HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY;

  status = HYSCAN_MIGRATE_CONFIG_STATUS_LATEST;
  key_file = g_key_file_new ();
  n_profiles = g_strv_length (db_profiles);
  for (i = 0; i < n_profiles; i++)
    {
      if (!g_key_file_load_from_file (key_file, db_profiles[i], G_KEY_FILE_KEEP_COMMENTS, NULL))
        {
          g_warning ("HyScanMigrateConfig: failed to read ini-file %s", db_profiles[i]);
          continue;
        }

      if (hyscan_migrate_config_profile_version (key_file) != HYSCAN_MIGRATE_CONFIG_DB_LATEST)
        {
          status = HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED;
          break;
        }
    }

  g_clear_pointer (&key_file, g_key_file_unref);
  g_strfreev (db_profiles);

  return status;
}

gboolean
hyscan_migrate_config_run (HyScanMigrateConfig *migrate_config,
                           HyScanCancellable   *cancellable)
{
  gboolean status = FALSE;
  GKeyFile *key_file;
  gchar **db_profiles;
  gint i, n_profiles;

  g_return_val_if_fail (HYSCAN_IS_MIGRATE_CONFIG (migrate_config), FALSE);

  hyscan_cancellable_push (cancellable);

  db_profiles = hyscan_migrate_config_profiles_db (migrate_config, "db-profiles");
  if (db_profiles == NULL)
    goto exit;

  key_file = g_key_file_new ();
  n_profiles = g_strv_length (db_profiles);
  for (i = 0; i < n_profiles; i++)
    {
      const gchar *fullname = db_profiles[i];

      hyscan_cancellable_set_total (cancellable, i, 0, n_profiles);
      if (g_cancellable_is_cancelled (G_CANCELLABLE (cancellable)))
        break;

      if (!g_key_file_load_from_file (key_file, fullname, G_KEY_FILE_KEEP_COMMENTS, NULL))
        {
          g_warning ("HyScanMigrateConfig: failed to read ini-file %s", fullname);
          continue;
        }

      if (!hyscan_migrate_config_profile_db (key_file))
        {
          g_warning ("HyScanMigrateConfig: failed to migrate %s", fullname);
          goto exit;
        }

      if (!g_key_file_save_to_file (key_file, fullname, NULL))
        {
          g_warning ("HyScanMigrateConfig: failed to save file %s", fullname);
          goto exit;
        }
    }

  status = TRUE;

exit:
  hyscan_cancellable_pop (cancellable);
  g_clear_pointer (&key_file, g_key_file_unref);
  g_clear_pointer (&db_profiles, g_strfreev);

  return status;
}

