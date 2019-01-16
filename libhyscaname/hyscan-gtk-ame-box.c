#include "hyscan-gtk-ame-box.h"

struct _HyScanGtkAmeBoxPrivate
{
  GHashTable *ht; // key: id, value: widget
  GList      *list;    // data: id

  gint        current;
};

static void    hyscan_gtk_ame_box_object_constructed       (GObject               *object);
static void    hyscan_gtk_ame_box_object_finalize          (GObject               *object);

/* TODO: Change G_TYPE_OBJECT to type of the base class. */
G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkAmeBox, hyscan_gtk_ame_box, GTK_TYPE_GRID);

static void
hyscan_gtk_ame_box_class_init (HyScanGtkAmeBoxClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->constructed = hyscan_gtk_ame_box_object_constructed;
  oclass->finalize = hyscan_gtk_ame_box_object_finalize;
}

static void
hyscan_gtk_ame_box_init (HyScanGtkAmeBox *gtk_ame_box)
{
  gtk_ame_box->priv = hyscan_gtk_ame_box_get_instance_private (gtk_ame_box);
}

static void
hyscan_gtk_ame_box_object_constructed (GObject *object)
{
  HyScanGtkAmeBox *self = HYSCAN_GTK_AME_BOX (object);
  HyScanGtkAmeBoxPrivate *priv = self->priv;

  /* TODO: Remove this call only when class is derived from GObject. */
  G_OBJECT_CLASS (hyscan_gtk_ame_box_parent_class)->constructed (object);

  priv->ht = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->current = -1;
}

static void
hyscan_gtk_ame_box_object_finalize (GObject *object)
{
  HyScanGtkAmeBox *self = HYSCAN_GTK_AME_BOX (object);
  HyScanGtkAmeBoxPrivate *priv = self->priv;

  g_hash_table_unref (priv->ht);

  G_OBJECT_CLASS (hyscan_gtk_ame_box_parent_class)->finalize (object);
}

GtkWidget *
hyscan_gtk_ame_box_new (void)
{
  return g_object_new (HYSCAN_TYPE_GTK_AME_BOX, NULL);
}

void
hyscan_gtk_ame_box_pack (HyScanGtkAmeBox *box,
                         GtkWidget       *widget,
                         gint             id,
                         gint             left,
                         gint             top,
                         gint             width,
                         gint             height)
{
  HyScanGtkAmeBoxPrivate *priv;

  g_return_if_fail (HYSCAN_IS_GTK_AME_BOX (box));
  priv = box->priv;

  g_hash_table_insert (priv->ht, GINT_TO_POINTER (id), widget);
  priv->list = g_list_append (priv->list, GINT_TO_POINTER (id));
  gtk_grid_attach (GTK_GRID (box), widget, left, top, width, height);
}

void
hyscan_gtk_ame_box_set_visible (HyScanGtkAmeBox *box,
                                gint             id)
{
  GHashTableIter iter;
  gpointer k, v;
  HyScanGtkAmeBoxPrivate *priv;

  g_return_if_fail (HYSCAN_IS_GTK_AME_BOX (box));
  priv = box->priv;

  /* Ищем виджет в таблице. */
  g_hash_table_iter_init(&iter, priv->ht);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      gboolean visible = GPOINTER_TO_INT (k) == id;
      gtk_widget_set_visible ((GtkWidget*)v, visible);
    }

  /* Хоп. */
  priv->current = g_list_index (priv->list, GINT_TO_POINTER (id));
}

gint
hyscan_gtk_ame_box_next_visible (HyScanGtkAmeBox *box)
{
  GList *link;
  HyScanGtkAmeBoxPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_GTK_AME_BOX (box), -1);
  priv = box->priv;

  link = g_list_nth (priv->list, priv->current + 1);

  if (link == NULL)
    return -1;

  hyscan_gtk_ame_box_set_visible (box, GPOINTER_TO_INT (link->data));

  return GPOINTER_TO_INT (link->data);
}

gint
hyscan_gtk_ame_box_nth_visible (HyScanGtkAmeBox *box,
                                gint             n)
{
  GList *link;
  HyScanGtkAmeBoxPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_GTK_AME_BOX (box), -1);
  priv = box->priv;

  link = g_list_nth (priv->list, n);

  if (link == NULL)
    return -1;

  hyscan_gtk_ame_box_set_visible (box, GPOINTER_TO_INT (link->data));

  return GPOINTER_TO_INT (link->data);
}

void
hyscan_gtk_ame_box_show_all (HyScanGtkAmeBox *box)
{
  GHashTableIter iter;
  gpointer k, v;
  HyScanGtkAmeBoxPrivate *priv;

  g_return_if_fail (HYSCAN_IS_GTK_AME_BOX (box));
  priv = box->priv;

  /* Всем виджетам ставим визибл по настройке. */
  g_hash_table_iter_init(&iter, priv->ht);
  while (g_hash_table_iter_next (&iter, &k, &v))
    gtk_widget_set_visible ((GtkWidget*)v, TRUE);

  priv->current = -1;
}
