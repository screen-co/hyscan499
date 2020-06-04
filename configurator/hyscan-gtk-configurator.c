#include "hyscan-gtk-configurator.h"
#include <glib/gi18n-lib.h>

enum
{
  PROP_O,
  PROP_MODEL,
};

struct _HyScanGtkConfiguratorPrivate
{
  HyScanConfigurator *model;

  GtkWidget          *db_dir_chooser;    /* Виджет выбора директории БД. */
  GtkWidget          *map_dir_chooser;   /* Виджет выбора директории кэша карты. */
  GtkWidget          *name_entry;        /* Виджет ввод названия профиля БД.*/
  GtkWidget          *apply_button;      /* Кнопка "Применить". */
};

static void     hyscan_gtk_configurator_set_property        (GObject               *object,
                                                             guint                  prop_id,
                                                             const GValue          *value,
                                                             GParamSpec            *pspec);
static void     hyscan_gtk_configurator_object_constructed  (GObject               *object);
static void     hyscan_gtk_configurator_object_finalize     (GObject               *object);
static void     hyscan_gtk_configurator_update              (HyScanGtkConfigurator *configurator);
static void     hyscan_gtk_configurator_apply_clicked       (HyScanGtkConfigurator *configurator);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkConfigurator, hyscan_gtk_configurator, GTK_TYPE_WINDOW)

static void
hyscan_gtk_configurator_class_init (HyScanGtkConfiguratorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_gtk_configurator_set_property;

  object_class->constructed = hyscan_gtk_configurator_object_constructed;
  object_class->finalize = hyscan_gtk_configurator_object_finalize;

  g_object_class_install_property (object_class, PROP_MODEL,
    g_param_spec_object("model", "Configurator", "HyScanConfigurator instance", HYSCAN_TYPE_CONFIGURATOR,
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
    case PROP_MODEL:
      priv->model = g_value_dup_object (value);
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
  const gchar *map_dir, *db_dir, *db_profile_name;

  G_OBJECT_CLASS (hyscan_gtk_configurator_parent_class)->constructed (object);

  /* Читаем текущую конфигурацию. */
  map_dir = hyscan_configurator_get_map_dir (priv->model);
  db_profile_name = hyscan_configurator_get_db_profile_name (priv->model);
  db_dir = hyscan_configurator_get_db_dir (priv->model);

  gtk_window_set_position (GTK_WINDOW (configurator), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (configurator), 450, -1);
  gtk_window_set_title (GTK_WINDOW (configurator), _("Initial configuration"));

  grid = gtk_grid_new ();
  g_object_set (grid, "margin", 12, NULL);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  priv->db_dir_chooser = gtk_file_chooser_button_new (_("DB directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  if (db_dir != NULL)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->db_dir_chooser), db_dir);
  g_signal_connect_swapped (priv->db_dir_chooser, "selection-changed",
                            G_CALLBACK (hyscan_gtk_configurator_update), configurator);

  priv->map_dir_chooser = gtk_file_chooser_button_new (_("Map cache directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  if (map_dir != NULL)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->map_dir_chooser), map_dir);
  g_signal_connect_swapped (priv->map_dir_chooser, "selection-changed",
                            G_CALLBACK (hyscan_gtk_configurator_update), configurator);

  priv->name_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (priv->name_entry), db_profile_name);
  g_signal_connect_swapped (priv->name_entry, "notify::text",
                            G_CALLBACK (hyscan_gtk_configurator_update), configurator);

  priv->apply_button = gtk_button_new_with_label (_("Apply"));
  g_signal_connect_swapped (priv->apply_button, "clicked",
                            G_CALLBACK (hyscan_gtk_configurator_apply_clicked), configurator);

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

  g_clear_object (&priv->model);

  G_OBJECT_CLASS (hyscan_gtk_configurator_parent_class)->finalize (object);
}

static void
hyscan_gtk_configurator_update (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;
  gchar *db_dir, *map_dir;

  db_dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->db_dir_chooser));
  hyscan_configurator_set_db_dir (priv->model, db_dir);
  g_free (db_dir);

  map_dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->map_dir_chooser));
  hyscan_configurator_set_map_dir (priv->model, map_dir);
  g_free (map_dir);

  hyscan_configurator_set_db_profile_name (priv->model, gtk_entry_get_text (GTK_ENTRY (priv->name_entry)));

  gtk_widget_set_sensitive (priv->apply_button, hyscan_configurator_is_valid (priv->model));
}

static void
hyscan_gtk_configurator_apply_clicked (HyScanGtkConfigurator *configurator)
{
  HyScanGtkConfiguratorPrivate *priv = configurator->priv;

  hyscan_configurator_write (priv->model);

  gtk_window_close (GTK_WINDOW (configurator));
}

/**
 * hyscan_gtk_configurator_new:
 * @settings_ini: путь к файлу конфигурации settings.ini
 *
 * Returns: виджет начальной настройки hyscan499
 */
GtkWidget *
hyscan_gtk_configurator_new (HyScanConfigurator *model)
{
  return g_object_new (HYSCAN_TYPE_GTK_CONFIGURATOR,
                       "model", model,
                       NULL);
}

