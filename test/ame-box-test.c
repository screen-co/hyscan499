#include "hyscan-gtk-ame-box.h"

gchar *names[3] = {"pan-up-symbolic", "pan-down-symbolic", "pan-start-symbolic"};

HyScanGtkAmeBox *abox;

void
clicked (GObject *emitter,
         gint     id)
{
  hyscan_gtk_ame_box_set_visible (abox, id);
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *box;
  gint i;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  abox = hyscan_gtk_ame_box_new ();

  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (abox), TRUE, TRUE, 0);

  for (i = 0; i < 3; ++i)
    {
      GtkWidget *view, *control;
      gchar *label;

      label = g_strdup_printf ("Turn %i", i);

      // view = gtk_image_new_from_icon_name (names[i], GTK_ICON_SIZE_DIALOG);
      view = gtk_label_new (label);
      control = gtk_button_new_with_label (label);

      g_object_set (view, "hexpand", TRUE, "vexpand", TRUE,
                    "halign", GTK_ALIGN_FILL, "valign", GTK_ALIGN_FILL, NULL);

      // hyscan_gtk_ame_box_pack (HYSCAN_GTK_AME_BOX (abox), view, i, i, 0, 1, 1);
      hyscan_gtk_ame_box_pack (HYSCAN_GTK_AME_BOX (abox), view, i,
                               i == 0 ? 0 : 1, // 0 - 0; 1, 2 - 1
                               i != 2 ? 0 : 1,  // 0, 1 -- 0; 2 - 1r
                               1,
                               i == 0 ? 2 : 1); // 0 - 2; 1,2  - 1

      g_signal_connect (control, "clicked", G_CALLBACK (clicked), GINT_TO_POINTER (i));

      gtk_box_pack_start (GTK_BOX(box), control, FALSE, FALSE, 0);

      g_free (label);
    }

  {
    GtkWidget *control;
    control = gtk_button_new_with_label ("show all");
    g_signal_connect_swapped (control, "clicked", G_CALLBACK (hyscan_gtk_ame_box_show_all), abox);
    gtk_box_pack_start (GTK_BOX(box), control, FALSE, FALSE, 0);
  }
  {
    GtkWidget *control;
    control = gtk_button_new_with_label ("next");
    g_signal_connect_swapped (control, "clicked", G_CALLBACK (hyscan_gtk_ame_box_next_visible), abox);
    gtk_box_pack_start (GTK_BOX(box), control, FALSE, FALSE, 0);
  }

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
