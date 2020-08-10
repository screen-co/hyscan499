#include <hyscan-profile-db.h>
#include <glib/gi18n-lib.h>
#include "hyscan-configurator.h"

#define PROFILE_DB_PATH     "db-profiles"
#define PROFILE_HW_PATH     "hw-profiles"
#define PROFILE_MAP_PATH    "map-profiles"
#define PROFILE_OFFSET_PATH "offset-profiles"

enum
{
  PROP_O,
  PROP_PATH,
  PROP_SETTINGS_FILE,
};

struct _HyScanConfiguratorPrivate
{
  gchar           *path;               /* Каталог с конфигурацией HyScan. */
  gchar           *settings_file;      /* Имя файла настроек. */
  gchar           *settings_ini;       /* Полный путь к файлу настроек. */
  gchar           *map_dir;            /* Каталог с кэшем карт. */
  gchar           *db_profile_name;    /* Имя профиля базы данных. */
  gchar           *db_dir;             /* Каталог базы данных. */

  HyScanProfileDB *db_profile;         /* Профиль базы данных. */
};

static void     hyscan_migrate_config_set_property           (GObject               *object,
                                                              guint                  prop_id,
                                                              const GValue          *value,
                                                              GParamSpec            *pspec);
static void     hyscan_configurator_object_constructed       (GObject               *object);
static void     hyscan_configurator_object_finalize          (GObject               *object);
static gboolean hyscan_configurator_read_map                 (HyScanConfigurator    *configurator);
static gboolean hyscan_configurator_read_db                  (HyScanConfigurator    *configurator);
static void     hyscan_configurator_mkdir                    (HyScanConfigurator    *configurator,
                                                              const gchar           *subdir);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanConfigurator, hyscan_configurator, G_TYPE_OBJECT)

static void
hyscan_configurator_class_init (HyScanConfiguratorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_migrate_config_set_property;
  object_class->constructed = hyscan_configurator_object_constructed;
  object_class->finalize = hyscan_configurator_object_finalize;

  g_object_class_install_property (object_class, PROP_PATH,
    g_param_spec_string ("path", "Path", "Path to configuration files", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_SETTINGS_FILE,
    g_param_spec_string ("settings-file", "Settings file", "Settings file base name", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_configurator_init (HyScanConfigurator *configurator)
{
  configurator->priv = hyscan_configurator_get_instance_private (configurator);
}

static void
hyscan_migrate_config_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  HyScanConfigurator *configurator = HYSCAN_CONFIGURATOR (object);
  HyScanConfiguratorPrivate *priv = configurator->priv;

  switch (prop_id)
    {
      case PROP_PATH:
        priv->path = g_value_dup_string (value);
        break;

      case PROP_SETTINGS_FILE:
        priv->settings_file = g_value_dup_string (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
hyscan_configurator_object_constructed (GObject *object)
{
  HyScanConfigurator *configurator = HYSCAN_CONFIGURATOR (object);
  HyScanConfiguratorPrivate *priv = configurator->priv;

  G_OBJECT_CLASS (hyscan_configurator_parent_class)->constructed (object);

  priv->settings_ini = g_build_filename (priv->path, priv->settings_file, NULL);
  hyscan_configurator_read_map (configurator);
  if (!hyscan_configurator_read_db (configurator))
    {
      gchar *filename;
      filename = g_build_filename (priv->path, PROFILE_DB_PATH, "default.ini", NULL);

      priv->db_profile = hyscan_profile_db_new (filename);
      priv->db_profile_name = g_strdup (_("Default profile"));
      priv->db_dir = NULL;
      g_free (filename);
    }
}

static void
hyscan_configurator_object_finalize (GObject *object)
{
  HyScanConfigurator *configurator = HYSCAN_CONFIGURATOR (object);
  HyScanConfiguratorPrivate *priv = configurator->priv;

  g_object_unref (priv->db_profile);
  g_free (priv->db_profile_name);
  g_free (priv->db_dir);
  g_free (priv->map_dir);
  g_free (priv->settings_file);
  g_free (priv->settings_ini);
  g_free (priv->path);

  G_OBJECT_CLASS (hyscan_configurator_parent_class)->finalize (object);
}

static gboolean
hyscan_configurator_read_map (HyScanConfigurator *configurator)
{
  HyScanConfiguratorPrivate *priv = configurator->priv;
  GKeyFile *key_file;

  key_file = g_key_file_new ();
  if (g_key_file_load_from_file (key_file, priv->settings_ini, G_KEY_FILE_NONE, NULL))
    priv->map_dir = g_key_file_get_string (key_file, "evo-map", "cache", NULL);
  g_key_file_free (key_file);

  return priv->map_dir != NULL;
}

static gboolean
hyscan_configurator_read_db (HyScanConfigurator *configurator)
{
  HyScanConfiguratorPrivate *priv = configurator->priv;
  GDir *dir;
  gchar *profiles_path;
  const gchar *filename;
  const gchar *db_uri;

  profiles_path = g_build_filename (priv->path, PROFILE_DB_PATH, NULL);

  dir = g_dir_open (profiles_path, 0, NULL);
  if (dir == NULL)
    return FALSE;

  /* Ищем первый профиль базы данных, который удастся прочитать. */
  while ((filename = g_dir_read_name (dir)) != NULL)
    {
      gchar *fullname;

      fullname = g_build_filename (profiles_path, filename, NULL);
      priv->db_profile = hyscan_profile_db_new (fullname);
      g_free (fullname);

      if (hyscan_profile_read (HYSCAN_PROFILE (priv->db_profile)))
        break;

      g_clear_object (&priv->db_profile);
    }
  g_dir_close (dir);
  g_free (profiles_path);

  if (priv->db_profile == NULL)
    return FALSE;

  db_uri = hyscan_profile_db_get_uri (priv->db_profile);
  if (g_pattern_match_simple ("file://*", db_uri))
    priv->db_dir = g_strdup (db_uri + strlen ("file://"));

  priv->db_profile_name = g_strdup (hyscan_profile_get_name (HYSCAN_PROFILE (priv->db_profile)));

  return TRUE;
}

static void
hyscan_configurator_mkdir (HyScanConfigurator *configurator,
                           const gchar        *subdir)
{
  HyScanConfiguratorPrivate *priv = configurator->priv;
  gchar *profile_dir;

  profile_dir = g_build_filename (priv->path, subdir, NULL);
  g_mkdir_with_parents (profile_dir, 0755);
  g_free (profile_dir);
}

HyScanConfigurator *
hyscan_configurator_new (const gchar *path,
                         const gchar *settings_file)
{
  return g_object_new (HYSCAN_TYPE_CONFIGURATOR,
                       "path", path,
                       "settings-file", settings_file,
                       NULL);
}

const gchar *
hyscan_configurator_get_db_profile_name (HyScanConfigurator *configurator)
{
  g_return_val_if_fail (HYSCAN_IS_CONFIGURATOR (configurator), NULL);

  return configurator->priv->db_profile_name;
}

const gchar *
hyscan_configurator_get_db_dir (HyScanConfigurator *configurator)
{
  g_return_val_if_fail (HYSCAN_IS_CONFIGURATOR (configurator), NULL);

  return configurator->priv->db_dir;
}

const gchar *
hyscan_configurator_get_map_dir (HyScanConfigurator *configurator)
{
  g_return_val_if_fail (HYSCAN_IS_CONFIGURATOR (configurator), NULL);

  return configurator->priv->map_dir;
}

void
hyscan_configurator_set_db_profile_name (HyScanConfigurator *configurator,
                                         const gchar        *db_profile_name)
{
  HyScanConfiguratorPrivate *priv;

  g_return_if_fail (HYSCAN_IS_CONFIGURATOR (configurator));
  priv = configurator->priv;

  g_free (priv->db_profile_name);
  priv->db_profile_name = g_strdup (db_profile_name);
}

void
hyscan_configurator_set_map_dir (HyScanConfigurator *configurator,
                                 const gchar        *map_dir)
{
  HyScanConfiguratorPrivate *priv;

  g_return_if_fail (HYSCAN_IS_CONFIGURATOR (configurator));
  priv = configurator->priv;

  g_free (priv->map_dir);
  priv->map_dir = g_strdup (map_dir);
}

void
hyscan_configurator_set_db_dir (HyScanConfigurator *configurator,
                                const gchar        *db_dir)
{
  HyScanConfiguratorPrivate *priv;

  g_return_if_fail (HYSCAN_IS_CONFIGURATOR (configurator));
  priv = configurator->priv;

  g_free (priv->db_dir);
  priv->db_dir = g_strdup (db_dir);
}

gboolean
hyscan_configurator_write (HyScanConfigurator *configurator)
{
  HyScanConfiguratorPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_CONFIGURATOR (configurator), FALSE);
  priv = configurator->priv;

  /* Создаём директорию с базой данных. */
  if (priv->db_dir != NULL)
    g_mkdir_with_parents (priv->db_dir, 0755);

  /* Создаём пользовательские директории с профилями. */
  hyscan_configurator_mkdir (configurator, PROFILE_DB_PATH);
  hyscan_configurator_mkdir (configurator, PROFILE_HW_PATH);
  hyscan_configurator_mkdir (configurator, PROFILE_MAP_PATH);
  hyscan_configurator_mkdir (configurator, PROFILE_OFFSET_PATH);

  /* Записываем профиль БД. */
  {
    gchar *db_uri;

    db_uri = g_strdup_printf ("file://%s", priv->db_dir);

    hyscan_profile_set_name (HYSCAN_PROFILE (priv->db_profile), priv->db_profile_name);
    hyscan_profile_db_set_uri (priv->db_profile, db_uri);
    if (!hyscan_profile_write (HYSCAN_PROFILE (priv->db_profile)))
      g_warning ("HyScanGtkConfigurator: failed to write db profile");

    g_free (db_uri);
  }

  /* Обновляем settings.ini. */
  {
    GKeyFile *key_file = g_key_file_new ();
    GError *error = NULL;

    g_key_file_load_from_file (key_file, priv->settings_ini,
                               G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    g_key_file_set_string (key_file, "evo-map", "cache", priv->map_dir);
    g_key_file_save_to_file (key_file, priv->settings_ini, &error);
    if (error != NULL)
      {
        g_warning ("HyScanGtkConfigurator: %s", error->message);
        g_clear_error (&error);
      }
    g_key_file_unref (key_file);
  }

  return TRUE;
}

gboolean
hyscan_configurator_is_valid (HyScanConfigurator *configurator)
{
  HyScanConfiguratorPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_CONFIGURATOR (configurator), FALSE);
  priv = configurator->priv;

  return priv->db_dir != NULL &&
         priv->db_profile_name != NULL &&
         priv->map_dir != NULL &&
         strlen (priv->db_dir) > 0 &&
         strlen (priv->db_profile_name) > 0 &&
         strlen (priv->map_dir) > 0;
}
