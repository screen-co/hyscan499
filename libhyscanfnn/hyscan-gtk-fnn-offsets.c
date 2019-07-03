#include "hyscan-gtk-fnn-offsets.h"

enum
{
  PROP_0,
  PROP_CONTROL,
};

enum
{
  ID,
  NFNN,
  DATE,
  SORT,
  N_COLUMNS
};

typedef struct
{
  // gchar         *path;
  GtkAdjustment *x;
  GtkAdjustment *y;
  GtkAdjustment *z;
  GtkAdjustment *psi;
  GtkAdjustment *gamma;
  GtkAdjustment *theta;
} HyScanGtkFnnOffsetsAdjustments;

struct _HyScanGtkFnnOffsetsPrivate
{
  HyScanControl *control;
  HyScanFnnOffsets *offset_man;

  GtkStack  *stack;
  GHashTable *adjustments; /* */
  gchar *offile;

};

static void    set_property      (GObject          *object,
                                  guint             prop_id,
                                  const GValue     *value,
                                  GParamSpec       *pspec);
static void    constructed       (GObject          *object);
static void    finalize          (GObject          *object);
static HyScanGtkFnnOffsetsAdjustments *
               adjustments_new   (HyScanGtkFnnOffsets *self);
static void    adjustments_free  (gpointer          data);

static void    fill_grid         (HyScanGtkFnnOffsets *self,
                                  GtkGrid          *grid);
static void    changed           (GtkAdjustment    *adj,
                                  gpointer          udata);
static void    row_activated     (GtkListBox       *box,
                                  GtkListBoxRow    *row,
                                  gpointer          udata);
static void    response_clicked  (GtkDialog        *self,
                                  gint              response_id);




G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkFnnOffsets, hyscan_gtk_fnn_offsets, GTK_TYPE_DIALOG);

static void
hyscan_gtk_fnn_offsets_class_init (HyScanGtkFnnOffsetsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;

  object_class->constructed = constructed;
  object_class->finalize = finalize;

  g_object_class_install_property (object_class, PROP_CONTROL,
    g_param_spec_object ("control", "control", "HyScanControl",
                         HYSCAN_TYPE_CONTROL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_fnn_offsets_init (HyScanGtkFnnOffsets *self)
{
  self->priv = hyscan_gtk_fnn_offsets_get_instance_private (self);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  HyScanGtkFnnOffsets *self = HYSCAN_GTK_FNN_OFFSETS (object);
  HyScanGtkFnnOffsetsPrivate *priv = self->priv;

  if (prop_id == PROP_CONTROL)
    priv->control = g_value_dup_object (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static GtkWidget *
make_page (HyScanGtkFnnOffsets *self,
           const gchar         *label)
{
  GtkWidget *grid;
  GtkWidget *entry;
  gint i;
  HyScanAntennaOffset offt;
  HyScanGtkFnnOffsetsAdjustments * adjs;

  const gchar *labels[] = {"x",
                           "y",
                           "z",
                           "psi",
                           "gamma",
                           "theta",
                           NULL};

  grid = gtk_grid_new ();

  adjs = adjustments_new (self);

  offt = hyscan_fnn_offsets_get_offset (self->priv->offset_man, label);
  gtk_adjustment_set_value (adjs->x, offt.x);
  gtk_adjustment_set_value (adjs->y, offt.y);
  gtk_adjustment_set_value (adjs->z, offt.z);
  gtk_adjustment_set_value (adjs->psi, offt.psi);
  gtk_adjustment_set_value (adjs->gamma, offt.gamma);
  gtk_adjustment_set_value (adjs->theta, offt.theta);

  g_hash_table_insert (self->priv->adjustments, g_strdup (label), adjs);

  for (i = 0; i < 6; ++i)
    {
      GtkWidget * label = gtk_label_new (labels[i]);
      gtk_grid_attach (GTK_GRID (grid), label, 0, i, 1, 1);
    }

  i = 0;
  entry = gtk_spin_button_new (adjs->x, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->y, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->z, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->psi, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->gamma, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->theta, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  return grid;
}

static void
fill_grid (HyScanGtkFnnOffsets *self,
           GtkGrid             *grid)
{
  HyScanGtkFnnOffsetsPrivate *priv = self->priv;
  GtkWidget *list, *page;
  GList * keys;

  list = gtk_list_box_new ();

  g_signal_connect (list, "row-activated", G_CALLBACK (row_activated), self);

  keys = hyscan_fnn_offsets_get_keys (priv->offset_man);
  for (; keys != NULL; keys = keys->next)
    {
      gchar *path = (gchar*)keys->data;
      gtk_list_box_insert (GTK_LIST_BOX (list), gtk_label_new (path), -1);

      page = make_page (self, path);
      gtk_stack_add_titled (priv->stack, page, path, path);
    }

  gtk_grid_attach (grid, list,                     0, 0, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (priv->stack), 1, 0, 3, 1);
  gtk_grid_set_column_homogeneous (grid, TRUE);
  gtk_widget_set_hexpand (GTK_WIDGET (grid), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (grid), TRUE);
}

static void
changed (GtkAdjustment    *src,
         gpointer          udata)
{
  GHashTableIter iter;
  const gchar *key;
  HyScanGtkFnnOffsetsAdjustments *adj;
  HyScanGtkFnnOffsets *self = udata;
  HyScanGtkFnnOffsetsPrivate *priv = self->priv;

  /* iterate */
  g_hash_table_iter_init (&iter, priv->adjustments);
  while (g_hash_table_iter_next (&iter, (gpointer*)&key, (gpointer*)&adj))
    {
      HyScanAntennaOffset offt;

      offt.x = gtk_adjustment_get_value (adj->x);
      offt.y = gtk_adjustment_get_value (adj->y);
      offt.z = gtk_adjustment_get_value (adj->z);
      offt.psi = gtk_adjustment_get_value (adj->psi);
      offt.gamma = gtk_adjustment_get_value (adj->gamma);
      offt.theta = gtk_adjustment_get_value (adj->theta);

      hyscan_fnn_offsets_set_offset (priv->offset_man, key, offt);
    }
}

static void
row_activated (GtkListBox    *box,
               GtkListBoxRow *row,
               gpointer       udata)
{
  const gchar *path;
  GtkWidget *label;
  HyScanGtkFnnOffsets *self = udata;
  HyScanGtkFnnOffsetsPrivate *priv = self->priv;

  g_return_if_fail (row != NULL);

  label = gtk_bin_get_child (GTK_BIN (row));
  path = gtk_label_get_text (GTK_LABEL (label));

  gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), path);
}

static void
response_clicked (GtkDialog        *self,
                  gint              response_id)
{
  HyScanGtkFnnOffsetsPrivate *priv = HYSCAN_GTK_FNN_OFFSETS (self)->priv;

  if (response_id == GTK_RESPONSE_OK)
    {
      hyscan_fnn_offsets_execute (priv->offset_man);
      hyscan_fnn_offsets_write (priv->offset_man, priv->offile);
    }
}

static void
constructed (GObject *object)
{
  GtkWidget *content, *grid;

  HyScanGtkFnnOffsets *self = HYSCAN_GTK_FNN_OFFSETS (object);
  HyScanGtkFnnOffsetsPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_gtk_fnn_offsets_parent_class)->constructed (object);

  priv->offile = g_build_filename (g_get_user_config_dir(), "hyscan", "offsets.ini", NULL);
  priv->offset_man = hyscan_fnn_offsets_new (priv->control);
  hyscan_fnn_offsets_read (priv->offset_man, priv->offile);

  priv->adjustments = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)adjustments_free);

  priv->stack = GTK_STACK (gtk_stack_new ());
  grid = gtk_grid_new ();
  gtk_widget_set_vexpand (grid, TRUE);
  gtk_widget_set_hexpand (grid, TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (priv->stack), TRUE);
  gtk_widget_set_hexpand (GTK_WIDGET (priv->stack), TRUE);
  gtk_widget_set_halign (GTK_WIDGET (priv->stack), GTK_ALIGN_CENTER);
  fill_grid (self, GTK_GRID (grid));

  content = gtk_dialog_get_content_area (GTK_DIALOG (self));
  gtk_box_pack_start (GTK_BOX (content), grid, TRUE, TRUE, 0);
  g_signal_connect (self, "response", G_CALLBACK (response_clicked), NULL);

  gtk_dialog_add_button (GTK_DIALOG (self), "Done", GTK_RESPONSE_OK);

  gtk_widget_set_size_request (GTK_WIDGET (self), 800, 600);
  gtk_widget_show_all (GTK_WIDGET (self));


}

static void
finalize (GObject *object)
{
  HyScanGtkFnnOffsets *self = HYSCAN_GTK_FNN_OFFSETS (object);
  HyScanGtkFnnOffsetsPrivate *priv = self->priv;

  g_clear_object (&priv->offset_man);
  g_clear_object (&priv->control);
  g_clear_pointer (&priv->offile, g_free);

  G_OBJECT_CLASS (hyscan_gtk_fnn_offsets_parent_class)->finalize (object);
}

static HyScanGtkFnnOffsetsAdjustments *
adjustments_new (HyScanGtkFnnOffsets *self)
{
  HyScanGtkFnnOffsetsAdjustments *adj = g_new (HyScanGtkFnnOffsetsAdjustments, 1);

  adj->x = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->y = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->z = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->psi = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->gamma = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->theta = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);

  g_signal_connect (adj->x, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->y, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->z, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->psi, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->gamma, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->theta, "value-changed", G_CALLBACK (changed), self);

  return adj;
}

static void
adjustments_free (gpointer data)
{
  HyScanGtkFnnOffsetsAdjustments *adj = data;
  if (adj == NULL)
    return;

  g_object_unref (adj->x);
  g_object_unref (adj->y);
  g_object_unref (adj->z);
  g_object_unref (adj->psi);
  g_object_unref (adj->gamma);
  g_object_unref (adj->theta);
  g_free (adj);
}


/* .*/
GtkWidget *
hyscan_gtk_fnn_offsets_new (HyScanControl *control,
                            GtkWindow     *parent)
{
  return g_object_new (HYSCAN_TYPE_GTK_FNN_OFFSETS,
                       "control", control,
                       "use-header-bar", TRUE,
                       "transient-for", parent,
                       NULL);
}
