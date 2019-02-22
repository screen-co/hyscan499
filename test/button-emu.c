#include <gio/gio.h>
#include <gtk/gtk.h>
#include <string.h>

GtkWidget *label;
gint stop;
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
clicked (GtkButton *butt,
         GSocket   *sock)
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

void *
read_thread (void * data)
{
  gchar **argv = (gchar**)data;
  gint port_val;
  GInetAddress *addr;
  GError *error = NULL;
  GSocket *sock;
  gboolean triggered;
  gchar buf[128];
  gsize bytes;

  return NULL;
  sock = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
  addr = g_inet_address_new_from_string (argv[1]);
  port_val = g_ascii_strtoll (argv[2], NULL, 10);

  isocketadress = g_inet_socket_address_new (addr, port_val);
  if (addr == NULL || isocketadress == NULL)
    return NULL;

  g_socket_bind (sock, isocketadress, TRUE, &error);
  error_check (&error);
  g_socket_join_multicast_group (sock, addr, FALSE, NULL, &error);
  error_check (&error);

  while (!g_atomic_int_get (&stop))
    {
      if (error != NULL)
        {
          if (error->code != G_IO_ERROR_TIMED_OUT)
            g_warning ("Button error: %s", error->message);
          g_clear_pointer (&error, g_error_free);
        }

      triggered = g_socket_condition_timed_wait (sock,
                                                 G_IO_IN | G_IO_HUP | G_IO_ERR,
                                                 G_TIME_SPAN_MILLISECOND * 100 ,
                                                 NULL, &error);
      if (!triggered)
        continue;

      if (error != NULL)
        {
          g_warning ("Button condition wait: %s", error->message);
          g_clear_pointer (&error, g_error_free);
        }

      /* Я ожидаю "bt_xx", 5 символов. Но я не уверен в амешниках. */
      bytes = g_socket_receive (sock, buf, 5*10+1, NULL, &error);

      /* нуль-терминируем. */
      buf[bytes] = '\0';
      g_message ("SelfReceived <%s>", buf);
    }
  return NULL;
}


void
make_button (GtkGrid *grid,
             GSocket *sock,
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
  g_signal_connect (button, "clicked", G_CALLBACK (clicked), sock);

  gtk_grid_attach (grid, button, l, t, 1, 1);
}

int
main (int argc, char **argv)
{
  gchar * mcast_addr;
  guint16 mcast_port;
  gboolean source_specific = FALSE;

  GInetAddress *addr;
  GtkWidget *window;
  GError *error = NULL;
  GSocket *sock;
  GThread *thr;

  gtk_init (&argc, &argv);

  if (argc == 2)
    {
      GKeyFile * kf = g_key_file_new ();
      g_key_file_load_from_file (kf, argv[1], G_KEY_FILE_NONE, NULL);

      source_specific = g_key_file_get_boolean (kf, "ame", "source_specific", NULL);
      mcast_addr =  g_key_file_get_string (kf, "ame", "button_addr", NULL);
      mcast_port =  (guint16)g_key_file_get_integer (kf, "ame", "button_port", NULL);

      g_key_file_unref (kf);
    }
  else if (argc == 3)
    {
      mcast_addr = argv[1];
      mcast_port = g_ascii_strtoll (argv[2], NULL, 10);
    }
  else
    {
      g_print ("Usage:\n\t./button-emu address port\nOR\n\t./button-emu config-file \n");
      return -1;
    }
  g_message ("Connecting to %s:%u", mcast_addr, mcast_port);
  sock = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
  addr = g_inet_address_new_from_string (mcast_addr);
  isocketadress = g_inet_socket_address_new (addr, mcast_port);
  if (addr == NULL || isocketadress == NULL)
    return -1;

  g_socket_bind (sock, isocketadress, TRUE, &error);
  error_check (&error);
  g_socket_join_multicast_group (sock, addr, source_specific, NULL, &error);
  error_check (&error);

  stop = FALSE;
  // thr = g_thread_new ("emu-receive", read_thread, argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  {
    gint i;
    GtkWidget *grid;

    grid = gtk_grid_new ();
    label = gtk_label_new("No activity");
    gtk_grid_attach (GTK_GRID (grid), label, 0, -1, 2, 1);

    for (i = 1; i < 6; ++i)
      {
        make_button (GTK_GRID (grid), sock, 'l', i, 0, i);
        make_button (GTK_GRID (grid), sock, 'r', i, 1, i);
      }

    gtk_container_add (GTK_CONTAINER (window), grid);
  }

  gtk_widget_show_all (window);

  g_signal_connect (window, "destroy", gtk_main_quit, NULL);
  gtk_main ();

  stop = 1;
  // g_thread_join (thr);
  return 0;
}
