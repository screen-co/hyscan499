#include <gtk/gtk.h>
#include <string.h>
#include <glib/gi18n.h>
#include <locale.h>

#define GETTEXT_PACKAGE "hyscan-499"

#ifdef G_OS_WIN32 /* Входная точка для GUI wWinMain. */
  #include <Windows.h>
#endif

typedef struct {
  gchar *dir;      /* Путь к директории с БД. */
  gchar *name;     /* Имя профиля БД. */
} ConfiguratorConfig;

typedef struct {
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *dir_chooser;
  GtkWidget *name_entry;
  GtkWidget *path;
  GtkWidget *apply_button;
} ConfiguratorView;

typedef struct {
  ConfiguratorConfig config;
  ConfiguratorView   view;
} Configurator;

static void
configurator_update_view (Configurator *configurator)
{
  gboolean ok;

  ok = configurator->config.dir != NULL &&
       configurator->config.name != NULL &&
       strlen (configurator->config.name) > 0;

  gtk_widget_set_sensitive (configurator->view.apply_button, ok);
  gtk_label_set_text (GTK_LABEL (configurator->view.path), configurator->config.dir);
}

static void
configurator_update_config (Configurator *configurator)
{
  ConfiguratorConfig *config = &configurator->config;
  ConfiguratorView *view = &configurator->view;

  g_free (config->dir);
  config->dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (view->dir_chooser));

  g_free (config->name);
  config->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (view->name_entry)));

  configurator_update_view (configurator);
}

static gchar *
configurator_get_dir (const gchar *subdir)
{
  return g_build_path (G_DIR_SEPARATOR_S, g_get_user_config_dir (), "hyscan", subdir, NULL);
}


static void
configurator_mkdir (const gchar *subdir)
{
  gchar *profile_dir;

  profile_dir = configurator_get_dir (subdir);
  g_mkdir_with_parents (profile_dir, 0755);
  g_free (profile_dir);
}

static void
configurator_configure (GtkButton    *button,
                        Configurator *configurator)
{
  GKeyFile *key_file;
  gchar *profiles_dir, *profile_path;
  gchar *db_uri;
  GError *error = NULL;

  /* Создаём директорию с базой данных. */
  g_mkdir_with_parents (configurator->config.dir, 0755);

  /* Создаём пользовательские директории с профилями. */
  configurator_mkdir ("db-profiles");
  configurator_mkdir ("hw-profiles");
  configurator_mkdir ("map-profiles");
  configurator_mkdir ("offset-profiles");

  key_file = g_key_file_new ();
  db_uri = g_strdup_printf ("file://%s", configurator->config.dir);
  g_key_file_set_string (key_file, "db", "name", configurator->config.name);
  g_key_file_set_string (key_file, "db", "uri", db_uri);

  profiles_dir = configurator_get_dir ("db-profiles");
  profile_path = g_build_path (G_DIR_SEPARATOR_S, profiles_dir, "default.ini", NULL);
  g_key_file_save_to_file (key_file, profile_path, &error);
  if (error != NULL)
    {
      g_warning ("Configurator: %s", error->message);
      g_clear_error (&error);
    }

  g_free (db_uri);
  g_free (profiles_dir);
  g_free (profile_path);

  gtk_main_quit ();
}

static void
configurator_build_view (Configurator *configurator)
{
  ConfiguratorView *view = &configurator->view;
  GtkWidget *description_label, *name_label, *path_label;

  const gchar *description;
  gint row;

  view->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (view->window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (view->window), 800, 400);
  gtk_window_set_title (GTK_WINDOW (view->window),  _("Initial configuration"));
  g_signal_connect_swapped (view->window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  view->grid = gtk_grid_new ();
  g_object_set (view->grid, "margin", 10, NULL);
  gtk_grid_set_row_spacing (GTK_GRID (view->grid), 10);
  gtk_grid_set_column_spacing (GTK_GRID (view->grid), 10);
  gtk_grid_set_column_homogeneous (GTK_GRID (view->grid), TRUE);

  view->dir_chooser = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  g_signal_connect_swapped (view->dir_chooser, "selection-changed",
                            G_CALLBACK (configurator_update_config), configurator);

  view->name_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (view->name_entry), configurator->config.name);
  g_signal_connect_swapped (view->name_entry, "notify::text",
                            G_CALLBACK (configurator_update_config), configurator);

  view->path = gtk_label_new (NULL);
  gtk_widget_set_halign (GTK_WIDGET (view->path), GTK_ALIGN_START);
  gtk_label_set_ellipsize (GTK_LABEL (view->path), PANGO_ELLIPSIZE_MIDDLE);

  view->apply_button = gtk_button_new_with_label (_("Apply"));
  g_signal_connect (view->apply_button, "clicked", G_CALLBACK (configurator_configure), configurator);

  description = _( "You should create a database profile to run HyScan. "
                   "Please, specify the name of the profile and the path to the database directory.");
  description_label = gtk_label_new (description);
  gtk_widget_set_halign (GTK_WIDGET (description_label), GTK_ALIGN_START);
  gtk_label_set_line_wrap (GTK_LABEL (description_label), TRUE);

  name_label = gtk_label_new (_("Name of DB-profile"));
  gtk_widget_set_halign (GTK_WIDGET (name_label), GTK_ALIGN_END);

  path_label = gtk_label_new (_("Path to DB"));
  gtk_widget_set_halign (GTK_WIDGET (path_label), GTK_ALIGN_END);

  row = 0;
  gtk_grid_attach (GTK_GRID (view->grid), description_label,  1, ++row, 3, 1);
  gtk_grid_attach (GTK_GRID (view->grid), name_label,         1, ++row, 1, 1);
  gtk_grid_attach (GTK_GRID (view->grid), view->name_entry,   2,   row, 2, 1);
  gtk_grid_attach (GTK_GRID (view->grid), path_label,         1, ++row, 1, 1);
  gtk_grid_attach (GTK_GRID (view->grid), view->path,         2,   row, 2, 1);
  gtk_grid_attach (GTK_GRID (view->grid), view->dir_chooser,  1, ++row, 3, 1);
  gtk_grid_attach (GTK_GRID (view->grid), view->apply_button, 3, ++row, 1, 1);

  gtk_container_add (GTK_CONTAINER (view->window), view->grid);
}

/* Определяет, есть ли какие-либо созданные профили. */
static gboolean
configurator_profile_exists (void)
{
  GDir *dir;
  gchar *profiles_path;
  const gchar *filename;
  gboolean any_profiles = FALSE;

  profiles_path = configurator_get_dir ("db-profiles");

  dir = g_dir_open (profiles_path, 0, NULL);
  if (dir == NULL)
    return FALSE;

  while ((filename = g_dir_read_name (dir)) != NULL && !any_profiles )
    {
      gchar *fullname;

      fullname = g_build_path (G_DIR_SEPARATOR_S, profiles_path, filename, NULL);
      any_profiles = g_file_test (fullname, G_FILE_TEST_IS_REGULAR);
      g_free (fullname);
    }

  g_dir_close (dir);
  g_free (profiles_path);

  return any_profiles;
}

static const gchar *
configurator_get_locale_dir (void)
{
  static gchar *locale_dir = NULL;

  if (locale_dir == NULL)
    {
#ifdef G_OS_WIN32
        gchar *utf8_path;
        gchar *install_dir;

        install_dir = g_win32_get_package_installation_directory_of_module (NULL);
        utf8_path = g_build_filename (install_dir, "share", "locale", NULL);
        locale_dir = g_win32_locale_filename_from_utf8 (utf8_path);

        g_free (install_dir);
        g_free (utf8_path);
#else
        locale_dir = FNN_LOCALE_DIR;
#endif
    }

  return locale_dir;
}

int
main (int argc, char **argv)
{
  Configurator *configurator;

  gtk_init (&argc, &argv);

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, configurator_get_locale_dir ());
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  configurator = g_new0 (Configurator, 1);
  configurator->config.name = g_strdup (_("Default profile"));

  if (configurator_profile_exists ())
    return 0;

  configurator_build_view (configurator);
  configurator_update_view (configurator);
  gtk_widget_show_all (configurator->view.window);

  gtk_main ();

  g_free (configurator->config.name);
  g_free (configurator->config.dir);

  return 0;
}

#ifdef G_OS_WIN32
/* Точка входа в приложение для Windows. */
int WINAPI
wWinMain (HINSTANCE hInst,
          HINSTANCE hPreInst,
          LPSTR     lpCmdLine,
          int       nCmdShow)
{
  int argc;
  char **argv;

  gint result;

  argv = g_win32_get_command_line ();
  argc = g_strv_length (argv);

  result = main (argc, argv);

  g_strfreev (argv);

  return result;
}
#endif
