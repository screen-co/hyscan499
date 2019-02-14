#include "hyscan-gtk-ame-box.h"

gchar *names[3] = {"pan-up-symbolic", "pan-down-symbolic", "pan-start-symbolic"};

HyScanGtkAmeBox *abox;

void
draw_h_lines (GtkWidget   *area,
              cairo_t     *cr,
              guint        step,
              gint         line_width,
              const gchar *color_name)
{
  guint width, height, i;
  GdkRGBA color;

  width = gtk_widget_get_allocated_width (area);
  height = gtk_widget_get_allocated_height (area);

  if (!gdk_rgba_parse (&color, color_name))
    {
      g_warning ("color <%s> not parsed", color_name);
      return;
    }

  for (i = 0; i < height; i+=step)
    {
      cairo_move_to (cr, 0, i);
      cairo_line_to(cr, line_width, i);
      cairo_move_to (cr, width, i);
      cairo_line_to(cr, width-line_width, i);
    }

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);
}

void
draw_v_lines (GtkWidget   *area,
              cairo_t     *cr,
              guint        step,
              gint         line_height,
              const gchar *color_name)
{
  guint width, height, i;
  GdkRGBA color;

  width = gtk_widget_get_allocated_width (area);
  height = gtk_widget_get_allocated_height (area);

  if (!gdk_rgba_parse (&color, color_name))
    {
      g_warning ("color <%s> not parsed", color_name);
      return;
    }

  for (i = 0; i < width; i+=step)
    {
      cairo_move_to (cr, i, 0);
      cairo_line_to(cr, i, line_height);
      cairo_move_to (cr, i, height);
      cairo_line_to(cr, i, height-line_height);
    }

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);
}

gboolean
do_draw (GtkWidget   *area,
         cairo_t     *cr,
         gpointer     udata)
{
  draw_h_lines (area, cr, 10, 20, "green");
  draw_h_lines (area, cr, 50, 50, "red");
  draw_v_lines (area, cr, 10, 20, "blue");
  draw_v_lines (area, cr, 50, 50, "black");
  return FALSE;
}

gboolean
click_to_exit (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  if (event->type == GDK_2BUTTON_PRESS)
    gtk_main_quit ();

  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *area;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  area = gtk_drawing_area_new ();
  g_signal_connect (area, "draw", G_CALLBACK (do_draw), NULL);

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (click_to_exit), NULL);
  gtk_container_add (GTK_CONTAINER (window), area);
  gtk_window_fullscreen (GTK_WINDOW (window));
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
