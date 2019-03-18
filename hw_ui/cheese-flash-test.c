#include <gtk/gtk.h>
#include "cheese-flash.h"

void
fire (CheeseFlash *flash)
{
  GdkRectangle rect = {.x = 0, .y = 0, .width = 500, .height = 500};

  cheese_flash_fire (flash, &rect);
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *button;
  CheeseFlash *flash;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  button = gtk_button_new_with_label ("fire");
  flash = cheese_flash_new ();
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (fire), flash);

  gtk_container_add (GTK_CONTAINER (window), button);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}
