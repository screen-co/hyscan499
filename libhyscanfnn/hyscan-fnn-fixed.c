#include "hyscan-fnn-fixed.h"

#define HYSCAN_FNN_FIXED_SHIFT (24)

struct _HyScanFnnFixedPrivate
{
  GHashTable *widgets;
};

static void    hyscan_fnn_fixed_object_constructed       (GObject               *object);
static void    hyscan_fnn_fixed_object_finalize          (GObject               *object);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanFnnFixed, hyscan_fnn_fixed, GTK_TYPE_FIXED);

static void
hyscan_fnn_fixed_class_init (HyScanFnnFixedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = hyscan_fnn_fixed_object_constructed;
  object_class->finalize = hyscan_fnn_fixed_object_finalize;
}

static void
hyscan_fnn_fixed_init (HyScanFnnFixed *ame_fixed)
{
  ame_fixed->priv = hyscan_fnn_fixed_get_instance_private (ame_fixed);
}


static void
hyscan_fnn_fixed_object_constructed (GObject *object)
{
  HyScanFnnFixed *ame_fixed = HYSCAN_FNN_FIXED (object);
  HyScanFnnFixedPrivate *priv = ame_fixed->priv;

  G_OBJECT_CLASS (hyscan_fnn_fixed_parent_class)->constructed (object);

  priv->widgets = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                         NULL, g_object_unref);
}

static void
hyscan_fnn_fixed_object_finalize (GObject *object)
{
  HyScanFnnFixed *ame_fixed = HYSCAN_FNN_FIXED (object);
  HyScanFnnFixedPrivate *priv = ame_fixed->priv;

  g_hash_table_unref (priv->widgets);

  G_OBJECT_CLASS (hyscan_fnn_fixed_parent_class)->finalize (object);
}

/* .*/
GtkWidget*
hyscan_fnn_fixed_new (void)
{
  return g_object_new (HYSCAN_TYPE_FNN_FIXED, NULL);
}

void
hyscan_fnn_fixed_pack (HyScanFnnFixed *self,
                       guint           position,
                       GtkWidget      *child)
{
  HyScanFnnFixedPrivate *priv;
  gpointer key = GUINT_TO_POINTER (position);
  gint y;

  g_return_if_fail (HYSCAN_IS_FNN_FIXED (self));
  priv = self->priv;

  if (g_hash_table_contains (priv->widgets, key))
    g_warning ("Replacing one FnnFixed child with another!");

  g_hash_table_insert (priv->widgets, key, g_object_ref (child));

  y = position * hyscan_fnn_button_get_size_request () + HYSCAN_FNN_FIXED_SHIFT;
  gtk_fixed_put (GTK_FIXED (self), child, 0, y);

}

void
hyscan_fnn_fixed_activate (HyScanFnnFixed *self,
                           guint           position)
{
  HyScanFnnButton *button;

  g_return_if_fail (HYSCAN_IS_FNN_FIXED (self));

  button = g_hash_table_lookup (self->priv->widgets, GUINT_TO_POINTER (position));

  g_return_if_fail (button != NULL);

  hyscan_fnn_button_activate (button);
}

void
hyscan_fnn_fixed_set_state (HyScanFnnFixed *self,
                            guint           position,
                            gboolean        state)
{
  HyScanFnnButton *button;

  g_return_if_fail (HYSCAN_IS_FNN_FIXED (self));

  button = g_hash_table_lookup (self->priv->widgets, GUINT_TO_POINTER (position));

  g_return_if_fail (button != NULL);

  hyscan_fnn_button_set_state (button, state);
}
