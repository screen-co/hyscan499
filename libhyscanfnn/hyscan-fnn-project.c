#include "hyscan-fnn-project.h"
#include <hyscan-gtk-project-viewer.h>
#include <fnn-types-common.h>

enum
{
  PROP_0,
  PROP_DB,
  PROP_DB_INFO,
};

enum
{
  ID,
  NAME,
  DATE,
  SORT,
  N_COLUMNS
};

struct _HyScanFnnProjectPrivate
{
  HyScanDB     *db;
  HyScanDBInfo *info;


  GtkButton    *del_project;
  GtkButton    *del_track;

  HyScanGtkProjectViewer *projects_pw;
  HyScanGtkProjectViewer *tracks_pw;
  GtkTreeView  *projects_tw;
  GtkTreeView  *tracks_tw;
  GtkListStore *project_ls;
  GtkListStore *track_ls;

  gchar        *track_name;
  gchar        *project_name;
};

static void    set_property      (GObject          *object,
                                  guint             prop_id,
                                  const GValue     *value,
                                  GParamSpec       *pspec);
static void    constructed       (GObject          *object);
static void    finalize          (GObject          *object);
static void    delete_project    (HyScanFnnProject *self);
static void    delete_track      (HyScanFnnProject *self);
static void    fill_grid         (HyScanFnnProject *self,
                                  GtkGrid          *grid);
static void    projects_changed  (HyScanDBInfo     *db_info,
                                  HyScanFnnProject *self);
static void    tracks_changed    (HyScanDBInfo     *db_info,
                                  HyScanFnnProject *self);
static void    project_selected  (GtkTreeView      *tree,
                                  HyScanFnnProject *self);
static void    track_selected    (GtkTreeView      *tree,
                                  HyScanFnnProject *self);
static void    set_button_text   (GtkButton        *button,
                                  const gchar      *mid,
                                  const gchar      *name);
static void    response_clicked  (GtkDialog        *self,
                                  gint              response_id);


G_DEFINE_TYPE_WITH_PRIVATE (HyScanFnnProject, hyscan_fnn_project, GTK_TYPE_DIALOG);

static void
hyscan_fnn_project_class_init (HyScanFnnProjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;

  object_class->constructed = constructed;
  object_class->finalize = finalize;

  g_object_class_install_property (object_class, PROP_DB,
    g_param_spec_object ("db", "DB", "HyScan DB", HYSCAN_TYPE_DB,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_DB_INFO,
    g_param_spec_object ("info", "DB Info", "HyScanDBInfo", HYSCAN_TYPE_DB_INFO,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_fnn_project_init (HyScanFnnProject *self)
{
  self->priv = hyscan_fnn_project_get_instance_private (self);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  HyScanFnnProject *self = HYSCAN_FNN_PROJECT (object);
  HyScanFnnProjectPrivate *priv = self->priv;

  if (prop_id == PROP_DB)
    priv->db = g_value_dup_object (value);
  else if (prop_id == PROP_DB_INFO)
    priv->info = g_value_dup_object (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
fill_grid (HyScanFnnProject *self,
           GtkGrid          *grid)
{
  GtkWidget *projects, *tracks, *del_project, *del_track, *abar;
  GtkTreeView *project_tree, *track_tree;

  HyScanFnnProjectPrivate *priv = self->priv;

  projects = hyscan_gtk_project_viewer_new ();
  tracks = hyscan_gtk_project_viewer_new ();

  g_object_set (projects, "hexpand", TRUE, "vexpand", TRUE, "margin", 6, NULL);
  g_object_set (tracks, "hexpand", TRUE, "vexpand", TRUE, "margin", 6, NULL);
  gtk_grid_set_column_homogeneous (grid, TRUE);

  project_tree = hyscan_gtk_project_viewer_get_tree_view (HYSCAN_GTK_PROJECT_VIEWER (projects));
  track_tree = hyscan_gtk_project_viewer_get_tree_view (HYSCAN_GTK_PROJECT_VIEWER (tracks));

  priv->project_ls = hyscan_gtk_project_viewer_get_liststore (HYSCAN_GTK_PROJECT_VIEWER (projects));
  priv->track_ls = hyscan_gtk_project_viewer_get_liststore (HYSCAN_GTK_PROJECT_VIEWER (tracks));

  g_signal_connect (project_tree, "cursor-changed",
                    G_CALLBACK (project_selected), self);
  g_signal_connect (track_tree, "cursor-changed",
                    G_CALLBACK (track_selected), self);

  del_project = gtk_button_new_with_label (_("Delete project"));
  del_track = gtk_button_new_with_label (_("Delete track"));

  gtk_style_context_add_class (gtk_widget_get_style_context (del_project),
                               GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);
  gtk_style_context_add_class (gtk_widget_get_style_context (del_track),
                               GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

  set_button_text (GTK_BUTTON (del_project), _("Delete project"), NULL);
  set_button_text (GTK_BUTTON (del_track), _("Delete track"), NULL);

  g_signal_connect_swapped (del_project, "clicked",
                            G_CALLBACK (delete_project), self);
  g_signal_connect_swapped (del_track, "clicked",
                            G_CALLBACK (delete_track), self);

  priv->projects_pw = HYSCAN_GTK_PROJECT_VIEWER (projects);
  priv->tracks_pw = HYSCAN_GTK_PROJECT_VIEWER (tracks);
  priv->projects_tw = project_tree;
  priv->tracks_tw = track_tree;
  priv->del_project = GTK_BUTTON (del_project);
  priv->del_track = GTK_BUTTON (del_track);

  abar = gtk_action_bar_new ();
  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), del_track);
  gtk_action_bar_pack_end (GTK_ACTION_BAR (abar), del_project);

  gtk_grid_attach (grid, gtk_label_new (_("Projects")),    0, -1, 1, 1);
  gtk_grid_attach (grid, projects,    0, 0, 1, 1);
  gtk_grid_attach (grid, gtk_label_new (_("Tracks")),      1, -1, 1, 1);
  gtk_grid_attach (grid, tracks,      1, 0, 1, 1);
  gtk_grid_attach (grid, abar,        0, 1, 2, 1);
}

static void
delete_project (HyScanFnnProject *self)
{
  HyScanFnnProjectPrivate *priv = self->priv;
  GtkWidget *dialog;

  if (priv->project_name == NULL)
    return;

  dialog = gtk_message_dialog_new (gtk_window_get_transient_for (GTK_WINDOW (self)),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_YES_NO,
                                   "%s %s?\n%s",
                                   _("Do you really want to delete project"),
                                   priv->project_name,
                                   _("This can not be undone"));

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_NO);
  gtk_widget_show_all (dialog);

  if (GTK_RESPONSE_YES == gtk_dialog_run (GTK_DIALOG (dialog)))
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_OPEN, FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_CREATE, FALSE);

      hyscan_db_project_remove (priv->db, priv->project_name);

      g_clear_pointer (&priv->project_name, g_free);
    }

  gtk_widget_destroy (dialog);
}

static void
delete_track (HyScanFnnProject *self)
{
  HyScanFnnProjectPrivate *priv = self->priv;
  gint32 project_id;

  if (priv->track_name == NULL)
    return;

  project_id = hyscan_db_project_open (priv->db, priv->project_name);
  if (project_id < 0)
    {
      g_warning ("No such project");
      return;
    }

  hyscan_db_track_remove (priv->db, project_id, priv->track_name);
  hyscan_db_close (priv->db, project_id);
  g_clear_pointer (&priv->track_name, g_free);
}

static void
projects_changed (HyScanDBInfo     *db_info,
                  HyScanFnnProject *self)
{
  HyScanFnnProjectPrivate *priv = self->priv;
  GtkTreeIter tree_iter;
  GHashTable *projects;
  GHashTableIter htiter;
  gpointer key, value;

  if (db_info == NULL)
    return;

  gtk_list_store_clear (GTK_LIST_STORE (priv->project_ls));

  g_message ("Projects changed...");
  projects = hyscan_db_info_get_projects (db_info);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_OPEN, TRUE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_CREATE, TRUE);

  g_hash_table_iter_init (&htiter, projects);
  while (g_hash_table_iter_next (&htiter, &key, &value))
    {
      GDateTime *local = NULL;
      gchar *time_str;
      HyScanProjectInfo *pinfo = value;
      gint64 sort;

      if (pinfo->ctime != NULL)
        {
          local = g_date_time_to_local (pinfo->ctime);
          time_str = g_date_time_format (local, "%d.%m %H:%M");
          sort = g_date_time_to_unix (pinfo->ctime);
        }
      else
        {
          time_str = g_strdup ("?");
          sort = -1;
        }

      gtk_list_store_append (GTK_LIST_STORE (priv->project_ls), &tree_iter);
      gtk_list_store_set (GTK_LIST_STORE (priv->project_ls), &tree_iter,
                          ID,   pinfo->name,
                          NAME, pinfo->name,
                          DATE, time_str,
                          SORT, sort,
                          -1);
      g_free (time_str);

      /* Подсвечиваем текущий галс. */
      if (priv->project_name == NULL)
        continue;

      if (g_strcmp0 (priv->project_name, pinfo->name) == 0)
        {
          GtkTreePath *tree_path;
          tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->project_ls),
                                               &tree_iter);
          gtk_tree_view_set_cursor (priv->projects_tw, tree_path, NULL, FALSE);
          gtk_tree_path_free (tree_path);
        }

      g_clear_pointer (&local, g_date_time_unref);
    }

  g_hash_table_unref (projects);
}

static void
tracks_changed (HyScanDBInfo     *db_info,
                HyScanFnnProject *self)
{
  HyScanFnnProjectPrivate *priv = self->priv;
  GtkTreeIter tree_iter;
  GHashTable *tracks;
  GHashTableIter htiter;
  gpointer key, value;

  gtk_list_store_clear (GTK_LIST_STORE (priv->track_ls));
  tracks = hyscan_db_info_get_tracks (db_info);

  g_hash_table_iter_init (&htiter, tracks);
  while (g_hash_table_iter_next (&htiter, &key, &value))
    {
      HyScanTrackInfo *tinfo = value;
      gchar *time_str;
      gint64 sort;

      if (tinfo->ctime != NULL)
        {
          time_str = g_date_time_format (tinfo->ctime, "%d.%m %H:%M");
          sort = g_date_time_to_unix (tinfo->ctime);
        }
      else
        {
          time_str = g_strdup ("??");
          sort = -1;
        }


      gtk_list_store_append (GTK_LIST_STORE (priv->track_ls), &tree_iter);
      gtk_list_store_set (GTK_LIST_STORE (priv->track_ls), &tree_iter,
                          ID,   g_strdup (tinfo->name),
                          NAME, g_strdup (tinfo->name),
                          DATE, time_str,
                          SORT, sort,
                          -1);
      g_free (time_str);

      /* Подсвечиваем текущий галс. */
      if (priv->track_name == NULL)
        continue;

      if (g_strcmp0 (priv->track_name, tinfo->name) == 0)
        {
          GtkTreePath *tree_path;
          tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->track_ls), &tree_iter);
          gtk_tree_view_set_cursor (priv->tracks_tw, tree_path, NULL, FALSE);
          gtk_tree_path_free (tree_path);
        }
    }

  g_hash_table_unref (tracks);
}

static void
set_button_text (GtkButton * button,
                 const gchar * prefix,
                 const gchar * postfix)
{
  gchar * text;

  if (postfix == NULL)
    text = g_strdup_printf ("%s", prefix);
  else
    text = g_strdup_printf ("%s %s", prefix, postfix);

  gtk_button_set_label (button, text);
  g_free (text);
  /* Нечувствительность виджета, если имя не задано. */
  gtk_widget_set_sensitive (GTK_WIDGET (button), postfix != NULL);
}

static void
response_clicked (GtkDialog        *self,
                  gint              response_id)
{
  HyScanFnnProjectPrivate *priv = HYSCAN_FNN_PROJECT (self)->priv;
  gchar *name;
  const gchar *text;

  if (response_id == HYSCAN_FNN_PROJECT_CREATE)
    {
      /* Часть 1. Название по-умолчанию. */
      {
        GHashTable *projects;
        gchar *date;
        gint i;
        GDateTime *dt = g_date_time_new_now_local ();
        g_clear_pointer (&priv->project_name, g_free);
        g_clear_pointer (&priv->track_name, g_free);

        date = g_date_time_format (dt, "%Y.%m.%d");
        name = g_strdup (date);

        projects = hyscan_db_info_get_projects (priv->info);

        /* Ищем проект с таким же именем, если находим - меняем имя нового проекта. */
        for (i = 2; ; ++i)
          {
            GHashTableIter iter;
            gpointer key, value;

            g_hash_table_iter_init (&iter, projects);
            while (g_hash_table_iter_next (&iter, &key, &value))
              {
                HyScanProjectInfo *pinfo = value;
                if (g_str_equal (name, pinfo->name))
                  {
                    g_message ("Same project found %s %s", name, pinfo->name);
                    goto increment;
                  }
              }

            break;

            increment:
            g_free (name);
            name = g_strdup_printf ("%s-%i", date, i);
          }

        g_date_time_unref (dt);
        g_hash_table_unref (projects);
      }


      GtkWidget *dialog, *content, *entry;
      dialog = gtk_dialog_new_with_buttons (_("Enter project name"),
                                            gtk_window_get_transient_for (GTK_WINDOW (self)),
                                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            _("OK"), GTK_RESPONSE_ACCEPT,
                                            NULL);

      content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
      entry = gtk_entry_new ();

      gtk_entry_set_text (GTK_ENTRY (entry), name);
      gtk_entry_set_placeholder_text (GTK_ENTRY (entry), name);
      g_free (name);

      gtk_box_pack_start (GTK_BOX (content), entry, TRUE, TRUE, 0);
      gtk_widget_show_all (content);

      if (GTK_RESPONSE_ACCEPT == gtk_dialog_run (GTK_DIALOG (dialog)))
        {
          text = gtk_entry_get_text (GTK_ENTRY (entry));
          if (text == NULL || g_str_equal (text, ""))
            text = gtk_entry_get_placeholder_text (GTK_ENTRY (entry));

          priv->project_name = g_strdup (text);
        }
      else
        {
          g_signal_stop_emission_by_name (self, "response");
        }
      gtk_widget_destroy (dialog);
    }
}

static gchar *
get_selected (GtkTreeView *tree)
{
  GtkTreeModel * model = gtk_tree_view_get_model (tree);
  GValue value = G_VALUE_INIT;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  const gchar *text;

  gtk_tree_view_get_cursor (tree, &path, NULL);
  if (path == NULL)
    return NULL;

  if (!gtk_tree_model_get_iter (model, &iter, path))
    return NULL;

  gtk_tree_model_get_value (model, &iter, NAME, &value);
  text = g_value_get_string (&value);

  g_message ("Selected item is <%s>", text);
  return g_strdup (text);
}

static void
project_selected (GtkTreeView      *tree,
                  HyScanFnnProject *self)
{
  HyScanFnnProjectPrivate *priv = self->priv;
  GtkWidget *resp_widget;
  gchar *project;

  project = get_selected (tree);

  if (project == NULL)
    return;

  if (priv->project_name != NULL && g_str_equal (project, priv->project_name))
    return;

  g_clear_pointer (&priv->project_name, g_free);
  g_clear_pointer (&priv->track_name, g_free);
  priv->project_name = g_strdup (project);

  hyscan_gtk_project_viewer_clear (priv->tracks_pw);
  hyscan_db_info_set_project (priv->info, project);

  set_button_text (priv->del_project, _("Delete project"), priv->project_name);

  resp_widget = gtk_dialog_get_widget_for_response (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_OPEN);
  set_button_text (GTK_BUTTON (resp_widget), _("Open project"), priv->project_name);
  g_free (project);
}

static void
track_selected (GtkTreeView      *tree,
                HyScanFnnProject *self)
{
  HyScanFnnProjectPrivate *priv = self->priv;
  gchar *track;

  track = get_selected (tree);

  g_clear_pointer (&priv->track_name, g_free);
  priv->track_name = g_strdup (track);

  set_button_text (priv->del_track, _("Delete track"), priv->track_name);

  g_free (track);
}

static void
constructed (GObject *object)
{
  GtkWidget *content, *grid, *button;


  HyScanFnnProject *self = HYSCAN_FNN_PROJECT (object);
  HyScanFnnProjectPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_fnn_project_parent_class)->constructed (object);

  grid = gtk_grid_new ();
  fill_grid (self, GTK_GRID (grid));

  content = gtk_dialog_get_content_area (GTK_DIALOG (self));
  gtk_box_pack_start (GTK_BOX (content), grid, TRUE, TRUE, 0);

  g_signal_connect (self, "response", G_CALLBACK (response_clicked), NULL);
  g_signal_connect (priv->info, "projects-changed", G_CALLBACK (projects_changed), self);
  g_signal_connect (priv->info, "tracks-changed", G_CALLBACK (tracks_changed), self);
  projects_changed (priv->info, self);

  button = gtk_dialog_add_button (GTK_DIALOG (self), _("Open project"), HYSCAN_FNN_PROJECT_OPEN);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), GTK_STYLE_CLASS_SUGGESTED_ACTION);
  button = gtk_dialog_add_button (GTK_DIALOG (self), _("New project"), HYSCAN_FNN_PROJECT_CREATE);
  gtk_style_context_add_class (gtk_widget_get_style_context (button), GTK_STYLE_CLASS_SUGGESTED_ACTION);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_OPEN, FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (self), HYSCAN_FNN_PROJECT_CREATE, FALSE);

  gtk_header_bar_set_title (GTK_HEADER_BAR (gtk_dialog_get_header_bar (GTK_DIALOG (self))),
                            C_("widget_title", "Project manager"));
  gtk_widget_set_size_request (GTK_WIDGET (self), 800, 600);
  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
finalize (GObject *object)
{
  HyScanFnnProject *self = HYSCAN_FNN_PROJECT (object);
  HyScanFnnProjectPrivate *priv = self->priv;

  g_signal_handlers_disconnect_by_data (priv->info, self);

  g_clear_object (&priv->db);
  g_clear_object (&priv->info);


  g_clear_pointer (&priv->track_name, g_free);
  g_clear_pointer (&priv->project_name, g_free);

  G_OBJECT_CLASS (hyscan_fnn_project_parent_class)->finalize (object);
}

/* .*/
GtkWidget *
hyscan_fnn_project_new (HyScanDB     *db,
                        HyScanDBInfo *info,
                        GtkWindow    *parent)
{
  return g_object_new (HYSCAN_TYPE_FNN_PROJECT,
                       "db", db, "info", info,
                       "use-header-bar", TRUE,
                       "transient-for", parent,
                       NULL);
}

void
hyscan_fnn_project_get (HyScanFnnProject *self,
                        gchar           **project,
                        gchar           **track)
{
  g_return_if_fail (HYSCAN_IS_FNN_PROJECT (self));

  if (project != NULL)
    *project = g_strdup (self->priv->project_name);
  if (track != NULL)
    *track = g_strdup (self->priv->track_name);
}
