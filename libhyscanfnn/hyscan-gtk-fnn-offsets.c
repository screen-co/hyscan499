#include "hyscan-gtk-fnn-offsets.h"
#include "fnn-types-common.h"

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
  GtkAdjustment *starboard;
  GtkAdjustment *forward;
  GtkAdjustment *vertical;
  GtkAdjustment *yaw;
  GtkAdjustment *pitch;
  GtkAdjustment *roll;
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

  const gchar *labels[] = {"starboard",
                           "forward",
                           "vertical",
                           "yaw",
                           "pitch",
                           "roll",
                           NULL};
  const gchar *hints[]  = {N_("Positive: starboard, negative: portside"),
                           N_("Positive: bow, negative: stern"),
                           N_("Positive: bottom, negative: sky"),
                           N_("Yaw"),
                           N_("Pitch"),
                           N_("Roll"),
                           NULL};

  grid = gtk_grid_new ();

  adjs = adjustments_new (self);

  offt = hyscan_fnn_offsets_get_offset (self->priv->offset_man, label);
  gtk_adjustment_set_value (adjs->starboard, offt.starboard);
  gtk_adjustment_set_value (adjs->forward, offt.forward);
  gtk_adjustment_set_value (adjs->vertical, offt.vertical);
  gtk_adjustment_set_value (adjs->yaw, offt.yaw);
  gtk_adjustment_set_value (adjs->pitch, offt.pitch);
  gtk_adjustment_set_value (adjs->roll, offt.roll);

  g_hash_table_insert (self->priv->adjustments, g_strdup (label), adjs);

  for (i = 0; i < 6; ++i)
    {
      GtkWidget * label = gtk_label_new (labels[i]);
      gtk_widget_set_tooltip_text (label, _(hints[i]));
      gtk_grid_attach (GTK_GRID (grid), label, 0, i, 1, 1);
    }

  i = 0;
  entry = gtk_spin_button_new (adjs->starboard, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->forward, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->vertical, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->yaw, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->pitch, 1.0, 3);
  gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, i++, 1, 1);

  entry = gtk_spin_button_new (adjs->roll, 1.0, 3);
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

      offt.starboard = gtk_adjustment_get_value (adj->starboard);
      offt.forward = gtk_adjustment_get_value (adj->forward);
      offt.vertical = gtk_adjustment_get_value (adj->vertical);
      offt.yaw = gtk_adjustment_get_value (adj->yaw);
      offt.pitch = gtk_adjustment_get_value (adj->pitch);
      offt.roll = gtk_adjustment_get_value (adj->roll);

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

  gtk_dialog_add_button (GTK_DIALOG (self), _("Done"), GTK_RESPONSE_OK);

  gtk_widget_set_size_request (GTK_WIDGET (self), 800, 600);
  gtk_widget_show_all (GTK_WIDGET (self));

  gtk_header_bar_set_title (GTK_HEADER_BAR (gtk_dialog_get_header_bar (GTK_DIALOG (self))),
                            _("Offset setup"));

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

  adj->starboard = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->forward = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->vertical = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->yaw = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->pitch = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);
  adj->roll = gtk_adjustment_new (0.0, -10000, 10000, 1.0, 10, 10);

  g_signal_connect (adj->starboard, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->forward, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->vertical, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->yaw, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->pitch, "value-changed", G_CALLBACK (changed), self);
  g_signal_connect (adj->roll, "value-changed", G_CALLBACK (changed), self);

  return adj;
}

static void
adjustments_free (gpointer data)
{
  HyScanGtkFnnOffsetsAdjustments *adj = data;
  if (adj == NULL)
    return;

  g_object_unref (adj->starboard);
  g_object_unref (adj->forward);
  g_object_unref (adj->vertical);
  g_object_unref (adj->yaw);
  g_object_unref (adj->pitch);
  g_object_unref (adj->roll);
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
