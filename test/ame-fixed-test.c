#include "hyscan-ame-fixed.h"

#define ICON_NAME "media-record-symbolic"

#define N_BUTTONS 5

gchar *texts[N_BUTTONS] = {"Button 0", "Button 1", "Button 2", "Button 3", "Button 4"};
gchar *texts0[N_BUTTONS] = {"dir 0", "dir 1", "dir 2", "dir 3", "dir 4"};
gchar *texts1[N_BUTTONS] = {"fix 0", "fix 1", "fix 2", "fix 3", "fix 4"};
gchar *texts2[N_BUTTONS] = {"off 0", "off 1", "off 2", "off 3", "off 4"};
gchar *texts3[N_BUTTONS] = {"on 0", "on 1", "on 2", "on 3", "on 4"};

GtkWidget *ame_fixed;

static void
clicked (HyScanAmeButton *button,
         gpointer         data)
{
  int val = GPOINTER_TO_INT (data);
  g_print ("  Received: Clicked. %i.\n", val);
}

static void
toggled (HyScanAmeButton *button,
         gboolean         state,
         gpointer         data)
{
  int val = GPOINTER_TO_INT (data);
  g_print ("  Received: Toggled. %i. %s\n", val, state ? "ON" : "OFF");
}

static void
direct_pusher (HyScanAmeButton *button)
{
  g_print ("Act: Activate.\n");
  hyscan_ame_button_activate (button);
}

static void
fixed_pusher (GtkButton      *button,
              gpointer        value)
{
  int i = GPOINTER_TO_INT (value);

  g_print ("Act: Push. %i.\n", i);
  hyscan_ame_fixed_activate (HYSCAN_AME_FIXED (ame_fixed), i);
}

static void
on_setter (GtkButton      *button,
           gpointer        value)
{
  int i = GPOINTER_TO_INT (value);

  g_print ("Act: turn on. %i.\n", i);
  hyscan_ame_fixed_set_state (HYSCAN_AME_FIXED (ame_fixed), i, TRUE);
}

static void
off_setter (GtkButton      *button,
           gpointer        value)
{
  int i = GPOINTER_TO_INT (value);

  g_print ("Act: turn off. %i.\n", i);
  hyscan_ame_fixed_set_state (HYSCAN_AME_FIXED (ame_fixed), i, FALSE);
}

static gboolean
sensitive_setter (HyScanAmeButton *button,
                  gboolean         state)
{
  g_print ("Act: change sensitivity.\n");
  hyscan_ame_button_set_sensitive (button, state);
  return FALSE;
}

static gboolean
state_setter (HyScanAmeButton *button,
              gboolean         state)
{
  g_print ("Act: set active.\n");
  hyscan_ame_button_set_state (button, state);
  return FALSE;
}

static gboolean
prop_setter (HyScanAmeButton *button,
             gboolean         unused,
             GtkSwitch       *source)
{
  gboolean active;
  g_print ("Act: set active property.\n");
  g_object_get (source, "active", &active, NULL);
  g_object_set (button, "active", active, NULL);

  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *grid;
  gint i;


  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  grid = gtk_grid_new ();
  ame_fixed = hyscan_ame_fixed_new ();

  gtk_grid_attach(GTK_GRID (grid), ame_fixed, 0, 0, 1, 5);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("dir"), 1, -1, 1, 1);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("fix"), 2, -1, 1, 1);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("off"), 3, -1, 1, 1);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("on"), 4, -1, 1, 1);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("sens"), 5, -1, 1, 1);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("active"), 6, -1, 1, 1);
  gtk_grid_attach(GTK_GRID (grid), gtk_label_new ("act. prop"), 7, -1, 1, 1);

  for (i = 0; i < N_BUTTONS; ++i)
    {
      GtkWidget *ame_button;
      GtkWidget *b1, *b2, *b3, *b4, *b5, *b6, *b7;

      ame_button = hyscan_ame_button_new (ICON_NAME, texts[i], !!(i % 2), !!(i & 2));
      g_signal_connect (ame_button, "ame-activated", G_CALLBACK (clicked), GINT_TO_POINTER(i));
      g_signal_connect (ame_button, "ame-toggled", G_CALLBACK (toggled), GINT_TO_POINTER(i));

      b1 = gtk_button_new_with_label (texts0[i]);
      b2 = gtk_button_new_with_label (texts1[i]);
      b3 = gtk_button_new_with_label (texts2[i]);
      b4 = gtk_button_new_with_label (texts3[i]);
      b5 = gtk_switch_new ();
      b6 = gtk_switch_new ();
      b7 = gtk_switch_new ();

      gtk_switch_set_state (GTK_SWITCH (b5), TRUE);
      gtk_switch_set_state (GTK_SWITCH (b6), !!(i & 2));

      g_signal_connect_swapped (b1, "clicked", G_CALLBACK (direct_pusher), ame_button);
      g_signal_connect (b2, "clicked", G_CALLBACK (fixed_pusher), GINT_TO_POINTER (i));
      g_signal_connect (b3, "clicked", G_CALLBACK (off_setter), GINT_TO_POINTER (i));
      g_signal_connect (b4, "clicked", G_CALLBACK (on_setter), GINT_TO_POINTER (i));
      g_signal_connect_swapped (b5, "state-set", G_CALLBACK (sensitive_setter), ame_button);
      g_signal_connect_swapped (b6, "state-set", G_CALLBACK (state_setter), ame_button);
      g_signal_connect_swapped (b7, "state-set", G_CALLBACK (prop_setter), ame_button);

      hyscan_ame_button_create_value (HYSCAN_AME_BUTTON (ame_button), "value");

      hyscan_ame_fixed_pack (HYSCAN_AME_FIXED (ame_fixed), i, ame_button);
      gtk_grid_attach(GTK_GRID (grid), b1, 1, i, 1, 1);
      gtk_grid_attach(GTK_GRID (grid), b2, 2, i, 1, 1);
      gtk_grid_attach(GTK_GRID (grid), b3, 3, i, 1, 1);
      gtk_grid_attach(GTK_GRID (grid), b4, 4, i, 1, 1);
      gtk_grid_attach(GTK_GRID (grid), b5, 5, i, 1, 1);
      gtk_grid_attach(GTK_GRID (grid), b6, 6, i, 1, 1);
      gtk_grid_attach(GTK_GRID (grid), b7, 7, i, 1, 1);


    }

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_container_add (GTK_CONTAINER (window), grid);
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
