#include "hyscan-fnn-project.h"

int
main (int argc, char **argv)
{
  GtkWidget *dialog;
  HyScanDB *db;
  HyScanDBInfo *db_info;
  gint res;
  gchar *track, *project;

  gtk_init (&argc, &argv);

  db = hyscan_db_new ("file:///home/dmitriev.a/dev/hsdb");
  db_info = hyscan_db_info_new (db);

  dialog = hyscan_fnn_project_new (db, db_info, NULL);

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  hyscan_fnn_project_get (HYSCAN_FNN_PROJECT (dialog), &project, &track);

  g_message ("Result: %i", res);
  g_message ("project/track: %s, %s", project, track);

  g_free (project);
  g_free (track);

  gtk_widget_destroy (dialog);
  return 0;
}
