#include "hyscan-ame-fixed.h"

#define HYSCAN_AME_FIXED_SHIFT (24)

struct _HyScanAmeFixedPrivate
{
  GHashTable *widgets;
};

static void    hyscan_ame_fixed_object_constructed       (GObject               *object);
static void    hyscan_ame_fixed_object_finalize          (GObject               *object);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanAmeFixed, hyscan_ame_fixed, GTK_TYPE_FIXED);

static void
hyscan_ame_fixed_class_init (HyScanAmeFixedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = hyscan_ame_fixed_object_constructed;
  object_class->finalize = hyscan_ame_fixed_object_finalize;
}

static void
hyscan_ame_fixed_init (HyScanAmeFixed *ame_fixed)
{
  ame_fixed->priv = hyscan_ame_fixed_get_instance_private (ame_fixed);
}


static void
hyscan_ame_fixed_object_constructed (GObject *object)
{
  HyScanAmeFixed *ame_fixed = HYSCAN_AME_FIXED (object);
  HyScanAmeFixedPrivate *priv = ame_fixed->priv;

  G_OBJECT_CLASS (hyscan_ame_fixed_parent_class)->constructed (object);

  priv->widgets = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                         NULL, g_object_unref);
}

static void
hyscan_ame_fixed_object_finalize (GObject *object)
{
  HyScanAmeFixed *ame_fixed = HYSCAN_AME_FIXED (object);
  HyScanAmeFixedPrivate *priv = ame_fixed->priv;

  g_hash_table_unref (priv->widgets);

  G_OBJECT_CLASS (hyscan_ame_fixed_parent_class)->finalize (object);
}

/* .*/
GtkWidget*
hyscan_ame_fixed_new (void)
{
  return g_object_new (HYSCAN_TYPE_AME_FIXED, NULL);
}

void
hyscan_ame_fixed_pack (HyScanAmeFixed *self,
                       guint           position,
                       GtkWidget      *child)
{
  HyScanAmeFixedPrivate *priv;
  gpointer key = GUINT_TO_POINTER (position);
  gint y;

  g_return_if_fail (HYSCAN_IS_AME_FIXED (self));
  priv = self->priv;

  if (g_hash_table_contains (priv->widgets, key))
    g_warning ("Replacing one AmeFixed child with another!");

  g_hash_table_insert (priv->widgets, key, g_object_ref (child));

  y = position * HYSCAN_AME_BUTTON_HEIGHT_REQUEST + HYSCAN_AME_FIXED_SHIFT;
  gtk_fixed_put (GTK_FIXED (self), child, 0, y);

}

void
hyscan_ame_fixed_activate (HyScanAmeFixed *self,
                           guint           position)
{
  HyScanAmeButton *button;

  g_return_if_fail (HYSCAN_IS_AME_FIXED (self));

  button = g_hash_table_lookup (self->priv->widgets, GUINT_TO_POINTER (position));

  g_return_if_fail (button != NULL);

  hyscan_ame_button_activate (button);
}

void
hyscan_ame_fixed_set_state (HyScanAmeFixed *self,
                            guint           position,
                            gboolean        state)
{
  HyScanAmeButton *button;

  g_return_if_fail (HYSCAN_IS_AME_FIXED (self));

  button = g_hash_table_lookup (self->priv->widgets, GUINT_TO_POINTER (position));

  g_return_if_fail (button != NULL);

  hyscan_ame_button_set_active (button, state);
}
