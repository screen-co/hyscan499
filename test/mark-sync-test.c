#include "hyscan-mark-sync.h"
#include <gtk/gtk.h>
#include <stdio.h>

int
main (int    argc,
      char **argv)
{
  HyScanMarkSync *sync;

  gtk_init (&argc, &argv);

  sync = hyscan_mark_sync_new ("127.0.0.1", 10000);

  hyscan_mark_sync_set (sync, "mark-id","Название",
                        -123.456, 123.456, 0.5);

  hyscan_mark_sync_remove (sync, "mark-id");

  g_object_unref (sync);

  return 0;
}
