#include <hyscan-profile.h>
#include "hyscan-migrate-config.h"

/* Функция определяет версию профиля. */
typedef gint     (*hyscan_migrate_config_version_fn) (GKeyFile *key_file);

/* Функция делает миграцию профиля. */
typedef gboolean (*hyscan_migrate_config_migrate_fn) (GKeyFile *key_file);

enum
{
  PROP_O,
  PROP_PATH,
};

/* Список известных версий профилей БД. */
enum HyScanMigrateConfigDb
{
  HYSCAN_MIGRATE_CONFIG_DB_INVALID = -1,
  HYSCAN_MIGRATE_CONFIG_DB_0,
  HYSCAN_MIGRATE_CONFIG_DB_965D2C45,
  HYSCAN_MIGRATE_CONFIG_DB_20200100,
  HYSCAN_MIGRATE_CONFIG_DB_LATEST  = HYSCAN_MIGRATE_CONFIG_DB_20200100,
};

/* Список известных версий профилей оборудования. */
enum HyScanMigrateConfigHW
{
  HYSCAN_MIGRATE_CONFIG_HW_INVALID = -1,
  HYSCAN_MIGRATE_CONFIG_HW_D93618D8,
  HYSCAN_MIGRATE_CONFIG_HW_20200100,
  HYSCAN_MIGRATE_CONFIG_HW_LATEST  = HYSCAN_MIGRATE_CONFIG_HW_20200100,
};

struct _HyScanMigrateConfigPrivate
{
  gchar             *path;    /* Путь к папке с профилями. */
};

static void               hyscan_migrate_config_set_property      (GObject                          *object,
                                                                   guint                             prop_id,
                                                                   const GValue                     *value,
                                                                   GParamSpec                       *pspec);
static void               hyscan_migrate_config_object_finalize   (GObject                          *object);
static guint64            hyscan_migrate_config_db_version_uint64 (gint                              version);
static gint               hyscan_migrate_config_db_version_id     (GKeyFile                         *key_file);
static guint64            hyscan_migrate_config_hw_version_uint64 (gint                              version);
static gint               hyscan_migrate_config_hw_version_id     (GKeyFile                         *key_file);
static gchar **           hyscan_migrate_config_profiles_list     (HyScanMigrateConfig              *migrate_config,
                                                                   const gchar                      *subdir);
static HyScanMigrateConfigStatus
                          hyscan_migrate_config_status_profile    (HyScanMigrateConfig              *migrate_config,
                                                                   const gchar                      *subdir,
                                                                   hyscan_migrate_config_version_fn  version_fn,
                                                                   gint                              latest_version);
static gboolean           hyscan_migrate_config_run_profile       (HyScanMigrateConfig              *migrate_config,
                                                                   HyScanCancellable                *cancellable,
                                                                   const gchar                      *subdir,
                                                                   hyscan_migrate_config_migrate_fn  migrate_fn);

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

/* Численное значение версии профиля БД. */
static guint64
hyscan_migrate_config_db_version_uint64 (gint version)
{
  switch (version)
    {
    case HYSCAN_MIGRATE_CONFIG_DB_0:
      return 0;

    case HYSCAN_MIGRATE_CONFIG_DB_965D2C45:
      return 0x965D2C45;

    case HYSCAN_MIGRATE_CONFIG_DB_20200100:
      return 20200100;

    default:
      g_warning ("HyScanMigrateConfig: unknown db version %d", version);
      return 0;
    }
}

/* Численное значение версии профиля оборудования. */
static guint64
hyscan_migrate_config_hw_version_uint64 (gint version)
{
  switch (version)
    {
    case HYSCAN_MIGRATE_CONFIG_HW_D93618D8:
      return 0xD93618D8;

    case HYSCAN_MIGRATE_CONFIG_HW_20200100:
      return 20200100;

    default:
      g_warning ("HyScanMigrateConfig: unknown hw version %d", version);
      return 0;
    }
}

/* Функция получает версию профиля БД из ini-файла. */
static gint
hyscan_migrate_config_db_version_id (GKeyFile *key_file)
{
  guint64 version;

  if (!g_key_file_has_group (key_file, HYSCAN_PROFILE_INFO_GROUP))
    return HYSCAN_MIGRATE_CONFIG_DB_0;

  version = g_key_file_get_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version", NULL);
  switch (version)
    {
    case 0x965D2C45:
      return HYSCAN_MIGRATE_CONFIG_DB_965D2C45;

    case 20200100:
      return HYSCAN_MIGRATE_CONFIG_DB_20200100;

    default:
      g_warning ("HyScanMigrateConfig: unknown db version %lu", version);
      return HYSCAN_MIGRATE_CONFIG_DB_INVALID;
    }
}

/* Функция получает версию профиля оборудования из ini-файла. */
static gint
hyscan_migrate_config_hw_version_id (GKeyFile *key_file)
{
  guint64 version;

  version = g_key_file_get_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version", NULL);
  switch (version)
    {
    case 0xD93618D8:
      return HYSCAN_MIGRATE_CONFIG_HW_D93618D8;

    case 20200100:
      return HYSCAN_MIGRATE_CONFIG_HW_20200100;

    default:
      g_warning ("HyScanMigrateConfig: unknown hw version %lu", version);
      return HYSCAN_MIGRATE_CONFIG_HW_INVALID;
    }
}

/* Миграция первой версии профиля БД: название профиля перенесно в группу "_". */
static gint
hyscan_migrate_config_profile_db_0 (GKeyFile *key_file)
{
  gchar *name;

  name = g_key_file_get_string (key_file, "db", "name", NULL);
  g_key_file_remove_key (key_file, "db", "name", NULL);

  g_key_file_set_string (key_file, HYSCAN_PROFILE_INFO_GROUP, "name", name);
  g_key_file_set_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version",
                         hyscan_migrate_config_db_version_uint64 (HYSCAN_MIGRATE_CONFIG_DB_965D2C45));

  g_free (name);

  return HYSCAN_MIGRATE_CONFIG_DB_965D2C45;
}

/* Миграция версии 0x65d2c45 профиля БД: изменён только номер версии. */
static gint
hyscan_migrate_config_profile_db_965d2c45 (GKeyFile *key_file)
{
  g_key_file_set_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version",
                         hyscan_migrate_config_db_version_uint64 (HYSCAN_MIGRATE_CONFIG_DB_20200100));

  return HYSCAN_MIGRATE_CONFIG_DB_20200100;
}

/* Миграция версии 0xd93618d8 профиля оборудования: изменён только номер версии. */
static gint
hyscan_migrate_config_profile_hw_d93618d8 (GKeyFile *key_file)
{
  g_key_file_set_uint64 (key_file, HYSCAN_PROFILE_INFO_GROUP, "version",
                         hyscan_migrate_config_hw_version_uint64 (HYSCAN_MIGRATE_CONFIG_HW_20200100));

  return HYSCAN_MIGRATE_CONFIG_HW_20200100;
}

/* Функция выполняет миграцию профиля БД. */
static gboolean
hyscan_migrate_config_profile_db (GKeyFile *key_file)
{
  gint version;

  version = hyscan_migrate_config_db_version_id (key_file);

  if (version == HYSCAN_MIGRATE_CONFIG_DB_0)
    version = hyscan_migrate_config_profile_db_0 (key_file);

  if (version == HYSCAN_MIGRATE_CONFIG_DB_965D2C45)
    version = hyscan_migrate_config_profile_db_965d2c45 (key_file);

  if (version == HYSCAN_MIGRATE_CONFIG_DB_20200100)
    return TRUE;

  g_warning ("HyScanMigrateConfig: unknown db profile version %d", version);

  return FALSE;
}

/* Функция выполняет миграцию профиля оборудования. */
static gboolean
hyscan_migrate_config_profile_hw (GKeyFile *key_file)
{
  gint version;

  version = hyscan_migrate_config_hw_version_id (key_file);

  if (version == HYSCAN_MIGRATE_CONFIG_HW_D93618D8)
    version = hyscan_migrate_config_profile_hw_d93618d8 (key_file);

  if (version == HYSCAN_MIGRATE_CONFIG_HW_20200100)
    return TRUE;

  g_warning ("HyScanMigrateConfig: unknown hw profile version %d", version);

  return FALSE;
}

/* Список профилей в директории subdir. */
static gchar **
hyscan_migrate_config_profiles_list (HyScanMigrateConfig *migrate_config,
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

/* Статус профилей в директории subdir. */
static HyScanMigrateConfigStatus
hyscan_migrate_config_status_profile (HyScanMigrateConfig              *migrate_config,
                                      const gchar                      *subdir,
                                      hyscan_migrate_config_version_fn  version_fn,
                                      gint                              latest_version)
{
  gint status;
  gchar **profiles;
  gint i, n_profiles;
  GKeyFile *key_file = NULL;

  profiles = hyscan_migrate_config_profiles_list (migrate_config, subdir);
  if (profiles == NULL)
    return HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY;

  status = HYSCAN_MIGRATE_CONFIG_STATUS_LATEST;
  key_file = g_key_file_new ();
  n_profiles = g_strv_length (profiles);
  for (i = 0; i < n_profiles; i++)
    {
      if (!g_key_file_load_from_file (key_file, profiles[i], G_KEY_FILE_KEEP_COMMENTS, NULL))
        {
          g_warning ("HyScanMigrateConfig: failed to read ini-file %s", profiles[i]);
          continue;
        }

      if (version_fn (key_file) != latest_version)
        {
          status = HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED;
          break;
        }
    }

  g_clear_pointer (&key_file, g_key_file_unref);
  g_strfreev (profiles);

  return status;
}

/* Функция выполняет миграцию профилей в директории subdir. */
static gboolean
hyscan_migrate_config_run_profile (HyScanMigrateConfig              *migrate_config,
                                   HyScanCancellable                *cancellable,
                                   const gchar                      *subdir,
                                   hyscan_migrate_config_migrate_fn  migrate_fn)
{
  gboolean status = FALSE;
  GKeyFile *key_file;
  gchar **profiles;
  gint i, n_profiles;

  hyscan_cancellable_push (cancellable);

  profiles = hyscan_migrate_config_profiles_list (migrate_config, subdir);
  if (profiles == NULL)
    {
      status = TRUE;
      goto exit;
    }

  key_file = g_key_file_new ();
  n_profiles = g_strv_length (profiles);
  for (i = 0; i < n_profiles; i++)
    {
      const gchar *fullname = profiles[i];

      hyscan_cancellable_set_total (cancellable, i, 0, n_profiles);
      if (g_cancellable_is_cancelled (G_CANCELLABLE (cancellable)))
        break;

      if (!g_key_file_load_from_file (key_file, fullname, G_KEY_FILE_KEEP_COMMENTS, NULL))
        {
          g_warning ("HyScanMigrateConfig: failed to read ini-file %s", fullname);
          continue;
        }

      if (!migrate_fn (key_file))
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
  g_clear_pointer (&profiles, g_strfreev);

  return status;
}

/**
 * hyscan_migrate_config_new:
 * @path: путь к папке с профилями
 *
 * Функция создаёт новый объект для миграции профилей.
 *
 * Returns: (transfer full): новый объект #HyScanMigrateConfig, для удаления g_object_unref().
 */
HyScanMigrateConfig *
hyscan_migrate_config_new (const gchar *path)
{
  return g_object_new (HYSCAN_TYPE_MIGRATE_CONFIG,
                       "path", path,
                       NULL);
}

/**
 * hyscan_migrate_config_status:
 * @migrate_config: указатель на #HyScanMigrateConfig
 *
 * Функция определяет текущее состояние профилей.
 *
 * Returns: текущее состояние профилей.
 */
HyScanMigrateConfigStatus
hyscan_migrate_config_status (HyScanMigrateConfig *migrate_config)
{
  HyScanMigrateConfigStatus db_status, hw_status;

  g_return_val_if_fail (HYSCAN_IS_MIGRATE_CONFIG (migrate_config), HYSCAN_MIGRATE_CONFIG_STATUS_INVALID);

  db_status = hyscan_migrate_config_status_profile (migrate_config, "db-profiles",
                                                    hyscan_migrate_config_db_version_id,
                                                    HYSCAN_MIGRATE_CONFIG_DB_LATEST);

  if (db_status == HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED || db_status == HYSCAN_MIGRATE_CONFIG_STATUS_INVALID)
    return db_status;

  hw_status = hyscan_migrate_config_status_profile (migrate_config, "hw-profiles",
                                                    hyscan_migrate_config_hw_version_id,
                                                    HYSCAN_MIGRATE_CONFIG_HW_LATEST);

  if (hw_status == HYSCAN_MIGRATE_CONFIG_STATUS_OUTDATED || hw_status == HYSCAN_MIGRATE_CONFIG_STATUS_INVALID)
    return hw_status;

  if (hw_status == HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY && db_status == HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY)
    return HYSCAN_MIGRATE_CONFIG_STATUS_EMPTY;

  return HYSCAN_MIGRATE_CONFIG_STATUS_LATEST;
}

/**
 * hyscan_migrate_config_run:
 * @migrate_config: указатель на #HyScanMigrateConfig
 * @cancellable: указатель на #HyScanCancellable для отслеживания прогресса
 *
 * Функция выполняет миграцию профилей.
 *
 * Returns: %TRUE, если миграция прошла успешна и все профили теперь имеют последнюю версию.
 */
gboolean
hyscan_migrate_config_run (HyScanMigrateConfig *migrate_config,
                           HyScanCancellable   *cancellable)
{
  gboolean status = FALSE;

  g_return_val_if_fail (HYSCAN_IS_MIGRATE_CONFIG (migrate_config), FALSE);

  hyscan_cancellable_push (cancellable);

  /* Миграция профилей базы данных. */
  hyscan_cancellable_set_total (cancellable, 1, 1, 2);
  status = hyscan_migrate_config_run_profile (migrate_config, cancellable,
                                              "db-profiles",
                                              hyscan_migrate_config_profile_db);
  if (!status)
    goto exit;

  /* Миграция профилей оборудования. */
  hyscan_cancellable_set_total (cancellable, 2, 1, 2);
  status = hyscan_migrate_config_run_profile (migrate_config, cancellable,
                                              "hw-profiles",
                                              hyscan_migrate_config_profile_hw);

exit:
  hyscan_cancellable_pop (cancellable);

  return status;
}

