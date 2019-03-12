#include "hyscan-gtk-mark-editor.h"

enum
{
  SIGNAL_MARK_MODIFIED,
  SIGNAL_LAST
};

guint hyscan_gtk_mark_editor_signals[SIGNAL_LAST] = {0};

struct _HyScanGtkMarkEditorPrivate
{
  gchar           *id;
  gchar           *title;
  gchar           *operator_name;
  gchar           *description;

  GtkLabel        *latitude;
  GtkLabel        *longitude;
  GtkLabel        *descr_label ;

  GtkEntryBuffer  *title_entrybuf;
};

static void  hyscan_gtk_mark_editor_finalize          (GObject              *object);
static void  hyscan_gtk_mark_editor_clear             (HyScanGtkMarkEditor  *me);

static void  hyscan_gtk_mark_editor_title_changed     (HyScanGtkMarkEditor  *me,
                                                       GtkEntryBuffer       *buf);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkMarkEditor, hyscan_gtk_mark_editor, GTK_TYPE_GRID)

static void
hyscan_gtk_mark_editor_class_init (HyScanGtkMarkEditorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = hyscan_gtk_mark_editor_finalize;

  hyscan_gtk_mark_editor_signals[SIGNAL_MARK_MODIFIED] =
    g_signal_new ("mark-modified", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/ame/gtk/hyscan-gtk-mark-editor.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMarkEditor, title_entrybuf);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMarkEditor, latitude);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMarkEditor, longitude);
  gtk_widget_class_bind_template_callback_full (widget_class, "title_changed",
                                                G_CALLBACK (hyscan_gtk_mark_editor_title_changed));
}

static void
hyscan_gtk_mark_editor_init (HyScanGtkMarkEditor *me)
{
  me->priv = hyscan_gtk_mark_editor_get_instance_private (me);
  gtk_widget_init_template (GTK_WIDGET (me));
}

static void
hyscan_gtk_mark_editor_finalize (GObject *object)
{
  hyscan_gtk_mark_editor_clear (HYSCAN_GTK_MARK_EDITOR (object));

  G_OBJECT_CLASS (hyscan_gtk_mark_editor_parent_class)->finalize (object);
}

static void
hyscan_gtk_mark_editor_clear (HyScanGtkMarkEditor *me)
{
  HyScanGtkMarkEditorPrivate *priv = me->priv;

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->operator_name, g_free);
  g_clear_pointer (&priv->description, g_free);
}

static void
hyscan_gtk_mark_editor_title_changed (HyScanGtkMarkEditor *me,
                                      GtkEntryBuffer      *buf)
{
  HyScanGtkMarkEditorPrivate *priv = me->priv;
  const gchar *text = gtk_entry_buffer_get_text (priv->title_entrybuf);

  if (priv->id != NULL && g_strcmp0 (text, priv->title))
    {
      g_free (priv->title);
      priv->title = g_strdup (text);
      g_signal_emit (me, hyscan_gtk_mark_editor_signals[SIGNAL_MARK_MODIFIED], 0, NULL);
    }
}

GtkWidget *
hyscan_gtk_mark_editor_new (void)
{
  return GTK_WIDGET (g_object_new (HYSCAN_TYPE_GTK_MARK_EDITOR, NULL));
}

void
hyscan_gtk_mark_editor_set_mark (HyScanGtkMarkEditor *mark_editor,
                                 const gchar         *id,
                                 const gchar         *title,
                                 const gchar         *operator_name,
                                 const gchar         *description,
                                 gdouble lat,
                                 gdouble lon)
{
  HyScanGtkMarkEditorPrivate *priv;
  gchar *lat_text, *lon_text;

  g_return_if_fail (HYSCAN_IS_GTK_MARK_EDITOR (mark_editor));

  priv = mark_editor->priv;

  hyscan_gtk_mark_editor_clear (mark_editor);
  priv->id = g_strdup (id);
  priv->title = g_strdup (title);
  priv->operator_name = g_strdup (operator_name);
  priv->description = g_strdup (description);

  gtk_entry_buffer_set_text (priv->title_entrybuf, title, -1);

  lat_text = g_strdup_printf ("%f", lat);
  lon_text = g_strdup_printf ("%f", lon);

  gtk_label_set_text (priv->latitude, lat_text);
  gtk_label_set_text (priv->longitude, lon_text);

  g_free (lat_text);
  g_free (lon_text);
}

void
hyscan_gtk_mark_editor_get_mark (HyScanGtkMarkEditor  *mark_editor,
                                 gchar               **id,
                                 gchar               **title,
                                 gchar               **operator_name,
                                 gchar               **description)
{
  HyScanGtkMarkEditorPrivate *priv;

  g_return_if_fail (HYSCAN_IS_GTK_MARK_EDITOR (mark_editor));

  priv = mark_editor->priv;

  if (id != NULL)
    *id = g_strdup (priv->id);
  if (title != NULL)
    *title = g_strdup (priv->title);
  if (operator_name != NULL)
    *operator_name = g_strdup (priv->operator_name);
  if (description != NULL)
    *description = g_strdup (priv->description);
}
