#include <gio/gio.h>
#include <gtk/gtk.h>
#include <string.h>

GSocket *sock;
GSocketAddress *isocketadress;


void error_check (GError **error)
{
  if (*error == NULL)
    return;

  g_message ("%s", (*error)->message);
  g_error_free (*error);
  *error = NULL;
}

void
clicked (GtkButton *butt)
{
  GError *error = NULL;
  gssize sent = -1;
  const gchar *text = gtk_button_get_label (butt);

  sent = g_socket_send_to (sock, isocketadress, text, strlen (text) + 1, NULL, &error);
  g_message ("sent %lli <%s>, %i", sent, text, strlen(text));
  error_check (&error);
}

void
make_button (GtkGrid *grid,
             gchar    side,
             gint     num,
             gint     l,
             gint     t)
{
  gchar *text;
  GtkWidget *button;

  text = g_strdup_printf ("bt_%c%i", side, num);

  button = gtk_button_new_with_label (text);
  g_signal_connect (button, "clicked", G_CALLBACK (clicked), NULL);

  gtk_grid_attach (grid, button, l, t, 1, 1);
}

int
main (int argc, char **argv)
{
  GInetAddress *addr;
  GtkWidget *window;
  GError *error = NULL;

  gtk_init (&argc, &argv);

  sock = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
  addr = g_inet_address_new_from_string (argv[1]);
  isocketadress = g_inet_socket_address_new(addr, 9000);
  if (addr == NULL || isocketadress == NULL)
    return -1;

  g_socket_bind (sock, isocketadress, TRUE, &error);
  error_check (&error);
  g_socket_join_multicast_group (sock, addr, FALSE, NULL, &error);
  error_check (&error);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  {
    GtkWidget *grid = gtk_grid_new ();
    gint i;

    for (i = 0; i < 5; ++i)
      {
        make_button (GTK_GRID (grid), 'l', i, 0, i);
        make_button (GTK_GRID (grid), 'r', i, 1, i);
      }

    gtk_container_add (GTK_CONTAINER (window), grid);
  }

  gtk_widget_show_all (window);

  g_signal_connect (window, "destroy", gtk_main_quit, NULL);
  gtk_main ();

  return 0;
}
