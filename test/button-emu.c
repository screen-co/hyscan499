#include <gio/gio.h>
#include <gtk/gtk.h>
#include <string.h>

GSocket *sock;
GSocketAddress *isocketadress;

GtkWidget *label;


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
  gssize sent;
  const gchar *msg = gtk_button_get_label (butt);
  gchar *text;

  sent = g_socket_send_to (sock, isocketadress, msg, strlen (msg) + 1, NULL, &error);
  g_message ("Message: <%s>, sent: %i", msg, (int)sent);

  if (sent > 0)
    text = g_strdup_printf ("Sent: <%.*s>", (int)sent-1, msg);
  else
    text = g_strdup (error->message);

  error_check (&error);

  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);
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
  gtk_widget_set_hexpand (button, TRUE);
  gtk_widget_set_vexpand (button, TRUE);
  g_signal_connect (button, "clicked", G_CALLBACK (clicked), NULL);

  gtk_grid_attach (grid, button, l, t, 1, 1);
}

int
main (int argc, char **argv)
{
  gint port_val;
  GInetAddress *addr;
  GtkWidget *window;
  GError *error = NULL;

  gtk_init (&argc, &argv);

  if (argc < 3)
    {
      g_print ("Usage: ./button-emu address port\n");
      return -1;
    }
  port_val = g_ascii_strtoll (argv[2], NULL, 10);

  sock = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
  addr = g_inet_address_new_from_string (argv[1]);
  isocketadress = g_inet_socket_address_new (addr, port_val);
  if (addr == NULL || isocketadress == NULL)
    return -1;

  g_socket_bind (sock, isocketadress, TRUE, &error);
  error_check (&error);
  g_socket_join_multicast_group (sock, addr, FALSE, NULL, &error);
  error_check (&error);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  {
    gint i;
    GtkWidget *grid;

    grid = gtk_grid_new ();
    label = gtk_label_new("No activity");
    gtk_grid_attach (GTK_GRID (grid), label, 0, -1, 2, 1);


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
