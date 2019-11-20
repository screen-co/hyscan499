#define GETTEXT_PACKAGE "libhyscanfnn"
#include <glib/gi18n-lib.h>
#include "hyscan-gtk-configurator.h"

enum
{
  PROP_O,
  PROP_CONFIG_DIR,
};

struct _HyScanGtkConfiguratorPrivate
{
  gchar            *config_dir;        /* Путь к директории с настройками. */
  gchar            *settings_ini;      /* Путь к файлу конфигурации. */

  gchar            *db_profile_file;   /* Имя файла профиля БД. */
  gchar            *db_profile_name;   /* Название профиля БД. */
  gchar            *db_profile_dir;    /* Путь к директории БД. */
  gchar            *map_cache_dir;     /* Путь к директории с кэшом карт. */

  GtkWidget        *db_dir_chooser;    /* Виджет выбора директории БД. */
  GtkWidget        *map_dir_chooser;   /* Виджет выбора директории кэша карты. */
  GtkWidget        *name_entry;        /* Виджет ввод названия профиля БД.*/
  GtkWidget        *apply_button;      /* Кнопка "Применить". */
};

static void     hyscan_gtk_configurator_set_property        (GObject               *object,
                                                             guint                  prop_id,
                                                             const GValue          *value,
                                                             GParamSpec            *pspec);
static void     hyscan_gtk_configurator_object_constructed  (GObject               *object);
static void     hyscan_gtk_configurator_object_finalize     (GObject               *object);
static void     hyscan_gtk_configurator_update_config       (HyScanGtkConfigurator *configurator);
static void     hyscan_gtk_configurator_configure           (HyScanGtkConfigurator *configurator);
static gboolean hyscan_gtk_configurator_read_db             (HyScanGtkConfigurator *configurator);
static gboolean hyscan_gtk_configurator_read_map            (HyScanGtkConfigurator *configurator);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkConfigurator, hyscan_gtk_configurator, GTK_TYPE_WINDOW)

static void
hyscan_gtk_configurator_class_init (HyScanGtkConfiguratorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_gtk_configurator_set_property;

  object_class->constructed = hyscan_gtk_configurator_object_constructed;
  object_class->finalize = hyscan_gtk_configurator_object_finalize;

  g_object_class_install_property (object_class, PROP_CONFIG_DIR,
    g_param_spec_string ("config-dir", "Config dir", "Directory containing configuration", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_configurator_init (HyScanGtkConfigurator *gtk_configurator)
{
  gtk_configurator->priv = hyscan_gtk_configurator_get_instance_private (gtk_configurator);
}

static void
hyscan_gtk_configurator_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  HyScanGtkConfigurator *gtk_configurator = HYSCAN_GTK_CONFIGURATOR (object);
  HyScanGtkConfiguratorPrivate *priv = gtk_configurator->priv;

  switch (prop_id)
    {
    case PROP_CONFIG_DIR:
      priv->config_dir = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_configurator_object_constructed (GObject *object)
{
  HyScanGtkConfigurator *configurator = HYSCAN_GTK_CONFIGURATOR (object);
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;
  GtkWidget *box, *grid, *db_description, *name_label, *db_dir_label, *map_description, *map_dir_label, *action_bar;
  gint row;

  G_OBJECT_CLASS (hyscan_gtk_configurator_parent_class)->constructed (object);

  /* Читаем текущую конфигурацию. */
  priv->settings_ini = g_build_path (G_DIR_SEPARATOR_S, priv->config_dir, "settings.ini", NULL);
  hyscan_gtk_configurator_read_map (configurator);
  if (!hyscan_gtk_configurator_read_db (configurator))
    {
      priv->db_profile_name = g_strdup (_("Default profile"));
      priv->db_profile_file = g_strdup ("default.ini");
    }

  gtk_window_set_position (GTK_WINDOW (configurator), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (configurator), 450, -1);
  gtk_window_set_title (GTK_WINDOW (configurator), _("Initial configuration"));

  grid = gtk_grid_new ();
  g_object_set (grid, "margin", 12, NULL);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  priv->db_dir_chooser = gtk_file_chooser_button_new (_("DB directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  if (priv->db_profile_dir != NULL)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->db_dir_chooser), priv->db_profile_dir);
  g_signal_connect_swapped (priv->db_dir_chooser, "selection-changed",
                            G_CALLBACK (hyscan_gtk_configurator_update_config), configurator);

  priv->map_dir_chooser = gtk_file_chooser_button_new (_("Map cache directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  if (priv->map_cache_dir != NULL)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->map_dir_chooser), priv->map_cache_dir);
  g_signal_connect_swapped (priv->map_dir_chooser, "selection-changed",
                            G_CALLBACK (hyscan_gtk_configurator_update_config), configurator);

  priv->name_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (priv->name_entry), priv->db_profile_name);
  g_signal_connect_swapped (priv->name_entry, "notify::text",
                            G_CALLBACK (hyscan_gtk_configurator_update_config), configurator);

  priv->apply_button = gtk_button_new_with_label (_("Apply"));
  g_signal_connect_swapped (priv->apply_button, "clicked",
                            G_CALLBACK (hyscan_gtk_configurator_configure), configurator);

  db_description = gtk_label_new (_("You should create a database profile to run HyScan. "
                                    "Please, specify the name of the profile and the path to the database directory."));
  gtk_widget_set_halign (db_description, GTK_ALIGN_START);
  gtk_label_set_xalign (GTK_LABEL (db_description), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (db_description), TRUE);

  map_description = gtk_label_new (_("Map cache stores tile images for the map offline work. "
                                     "Please, specify the path to map cache directory."));
  gtk_widget_set_halign (map_description, GTK_ALIGN_START);
  gtk_label_set_xalign (GTK_LABEL (map_description), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (map_description), TRUE);

  name_label = gtk_label_new (_("Name of DB-profile"));
  gtk_widget_set_halign (GTK_WIDGET (name_label), GTK_ALIGN_END);

  db_dir_label = gtk_label_new (_("DB directory"));
  gtk_widget_set_halign (GTK_WIDGET (db_dir_label), GTK_ALIGN_END);

  map_dir_label = gtk_label_new (_("Map cache directory"));
  gtk_widget_set_halign (GTK_WIDGET (map_dir_label), GTK_ALIGN_END);

  action_bar = gtk_action_bar_new ();
  gtk_action_bar_pack_end (GTK_ACTION_BAR (action_bar), priv->apply_button);

  row = 0;
  gtk_grid_attach (GTK_GRID (grid), db_description,        0, ++row, 3, 1);
  gtk_grid_attach (GTK_GRID (grid), name_label,            0, ++row, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), priv->name_entry,      1,   row, 2, 1);
  gtk_grid_attach (GTK_GRID (grid), db_dir_label,          0, ++row, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), priv->db_dir_chooser,  1,   row, 2, 1);
  gtk_widget_set_margin_top (map_description, 18);
  gtk_grid_attach (GTK_GRID (grid), map_description,       0, ++row, 3, 1);
  gtk_grid_attach (GTK_GRID (grid), map_dir_label,         0, ++row, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), priv->map_dir_chooser, 1,   row, 2, 1);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (box), grid, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box), action_bar, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (configurator), box);
}

static void
hyscan_gtk_configurator_object_finalize (GObject *object)
{
  HyScanGtkConfigurator *gtk_configurator = HYSCAN_GTK_CONFIGURATOR (object);
  HyScanGtkConfiguratorPrivate *priv = gtk_configurator->priv;

  g_free (priv->config_dir);
  g_free (priv->db_profile_dir);
  g_free (priv->db_profile_name);
  g_free (priv->db_profile_file);
  g_free (priv->map_cache_dir);
  g_free (priv->settings_ini);

  G_OBJECT_CLASS (hyscan_gtk_configurator_parent_class)->finalize (object);
}

static void
hyscan_gtk_configurator_update_view (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;
  gboolean ok;

  ok = priv->db_profile_dir != NULL &&
       priv->db_profile_name != NULL &&
       priv->map_cache_dir != NULL &&
       strlen (priv->db_profile_dir) > 0 &&
       strlen (priv->db_profile_name) > 0 &&
       strlen (priv->map_cache_dir) > 0;

  gtk_widget_set_sensitive (priv->apply_button, ok);
}

static void
hyscan_gtk_configurator_update_config (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;

  g_free (priv->db_profile_dir);
  priv->db_profile_dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->db_dir_chooser));

  g_free (priv->map_cache_dir);
  priv->map_cache_dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->map_dir_chooser));

  g_free (priv->db_profile_name);
  priv->db_profile_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->name_entry)));

  hyscan_gtk_configurator_update_view (configurator);
}

static gchar *
hyscan_gtk_configurator_get_dir (HyScanGtkConfigurator *configurator,
                                 const gchar           *subdir)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;

  return g_build_path (G_DIR_SEPARATOR_S, priv->config_dir, subdir, NULL);
}


static void
hyscan_gtk_configurator_mkdir (HyScanGtkConfigurator *configurator,
                               const gchar           *subdir)
{
  gchar *profile_dir;

  profile_dir = hyscan_gtk_configurator_get_dir (configurator, subdir);
  g_mkdir_with_parents (profile_dir, 0755);
  g_free (profile_dir);
}

static void
hyscan_gtk_configurator_configure (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;
  GKeyFile *key_file;
  gchar *profiles_dir, *profile_path;
  gchar *db_uri;
  GError *error = NULL;

  /* Создаём директорию с базой данных. */
  g_mkdir_with_parents (priv->db_profile_dir, 0755);

  /* Создаём пользовательские директории с профилями. */
  hyscan_gtk_configurator_mkdir (configurator, "db-profiles");
  hyscan_gtk_configurator_mkdir (configurator, "hw-profiles");
  hyscan_gtk_configurator_mkdir (configurator, "map-profiles");
  hyscan_gtk_configurator_mkdir (configurator, "offset-profiles");

  key_file = g_key_file_new ();
  g_key_file_load_from_file (key_file, priv->db_profile_file,
                             G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
  db_uri = g_strdup_printf ("file://%s", priv->db_profile_dir);
  g_key_file_set_string (key_file, "db", "name", priv->db_profile_name);
  g_key_file_set_string (key_file, "db", "uri", db_uri);

  profiles_dir = hyscan_gtk_configurator_get_dir (configurator, "db-profiles");
  profile_path = g_build_path (G_DIR_SEPARATOR_S, profiles_dir, priv->db_profile_file, NULL);
  g_key_file_save_to_file (key_file, profile_path, &error);
  if (error != NULL)
    {
      g_warning ("HyScanGtkConfigurator: %s", error->message);
      g_clear_error (&error);
    }

  g_key_file_load_from_file (key_file, priv->settings_ini,
                             G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
  g_key_file_set_string (key_file, "evo-map", "cache", priv->map_cache_dir);
  g_key_file_save_to_file (key_file, priv->settings_ini, &error);
  if (error != NULL)
    {
      g_warning ("HyScanGtkConfigurator: %s", error->message);
      g_clear_error (&error);
    }

  g_free (db_uri);
  g_free (profiles_dir);
  g_free (profile_path);

  gtk_window_close (GTK_WINDOW (configurator));
}

static gboolean
hyscan_gtk_configurator_read_map (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;
  GKeyFile *key_file;

  key_file = g_key_file_new ();
  if (g_key_file_load_from_file (key_file, priv->settings_ini, G_KEY_FILE_NONE, NULL))
    priv->map_cache_dir = g_key_file_get_string (key_file, "evo-map", "cache", NULL);
  g_key_file_free (key_file);

  return priv->map_cache_dir != NULL;
}

static gboolean
hyscan_gtk_configurator_read_db (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;
  GDir *dir;
  gchar *profiles_path;
  const gchar *filename;
  gboolean db_profile_found = FALSE;
  GKeyFile *key_file;
  gchar *db_uri = NULL, *db_name = NULL, *db_file = NULL;

  profiles_path = hyscan_gtk_configurator_get_dir (configurator, "db-profiles");

  dir = g_dir_open (profiles_path, 0, NULL);
  if (dir == NULL)
    return FALSE;

  key_file = g_key_file_new ();
  while ((filename = g_dir_read_name (dir)) != NULL && !db_profile_found)
    {
      gchar *fullname;

      fullname = g_build_path (G_DIR_SEPARATOR_S, profiles_path, filename, NULL);
      if (!g_key_file_load_from_file (key_file, fullname, G_KEY_FILE_NONE, NULL))
        continue;

      g_free (db_name);
      g_free (db_uri);
      g_free (db_file);
      db_file = g_strdup (filename);
      db_name = g_key_file_get_string (key_file, "db", "name", NULL);
      db_uri = g_key_file_get_string (key_file, "db", "uri", NULL);
      db_profile_found = (db_name != NULL && db_uri != NULL);

      g_free (fullname);
    }
  g_key_file_free (key_file);
  g_dir_close (dir);

  if (db_profile_found)
    {
      priv->db_profile_name = g_strdup (db_name);
      priv->db_profile_file = g_strdup (db_file);
      if (g_pattern_match_simple ("file://*", db_uri))
        priv->db_profile_dir = g_strdup (db_uri + strlen ("file://"));
    }

  g_free (db_file);
  g_free (db_name);
  g_free (db_uri);

  g_free (profiles_path);

  return db_profile_found;
}

/**
 * hyscan_gtk_configurator_new:
 * @config_dir: путь к директории, где находятся конфигурационные файлы hyscan499
 *
 * Returns: виджет начальной настройки hyscan499
 */
GtkWidget *
hyscan_gtk_configurator_new (const gchar *config_dir)
{
  return g_object_new (HYSCAN_TYPE_GTK_CONFIGURATOR,
                       "config-dir", config_dir,
                       NULL);
}

/**
 * hyscan_gtk_configurator_configured:
 * @configurator
 *
 * Returns: возвращает %TRUE, если HyScan уже сконфигурирован
 */
gboolean
hyscan_gtk_configurator_configured (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;

  g_return_val_if_fail (HYSCAN_IS_GTK_CONFIGURATOR (configurator), FALSE);

  return priv->map_cache_dir != NULL &&
         priv->db_profile_dir != NULL &&
         priv->db_profile_name != NULL;
}
