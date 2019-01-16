#include "hyscan-ame-button.h"

#define ICON_OFF "window-maximize-symbolic"
#define ICON_ON "emblem-ok-symbolic"
enum
{
  PROP_0,
  PROP_ICON,
  PROP_LABEL,
  PROP_TOGGLE,
  PROP_STATE,
  PROP_ACTIVE,
};

enum
{
  SIG_ACTIVATED,
  SIG_TOGGLED,
  SIG_LAST
};

struct _HyScanAmeButtonPrivate
{
  gchar           *icon_name;
  gchar           *label;
  gboolean         is_toggle;
  gboolean         state;

  gboolean         sensitivity;

  GtkButton       *button;
  GtkBox          *text_box;

  GtkWidget       *image_off;
  GtkWidget       *image_on;
};

static void    hyscan_ame_button_set_property             (GObject               *object,
                                                           guint                  prop_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static void    hyscan_ame_button_object_constructed       (GObject               *object);
static void    hyscan_ame_button_object_finalize          (GObject               *object);

static guint   hyscan_ame_button_signals[SIG_LAST] = {0};

G_DEFINE_TYPE_WITH_PRIVATE (HyScanAmeButton, hyscan_ame_button, GTK_TYPE_BOX);

static void
hyscan_ame_button_class_init (HyScanAmeButtonClass *klass)
{
  GParamFlags flags = G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_ame_button_set_property;

  object_class->constructed = hyscan_ame_button_object_constructed;
  object_class->finalize = hyscan_ame_button_object_finalize;

  g_object_class_install_property (object_class, PROP_ICON,
    g_param_spec_string ("icon-name", "icon-name", "icon-name", NULL, flags));
  g_object_class_install_property (object_class, PROP_LABEL,
    g_param_spec_string ("label", "label", "label", NULL, flags));
   g_object_class_install_property (object_class, PROP_TOGGLE,
    g_param_spec_boolean ("is_toggle", "is_toggle", "is_toggle", FALSE, flags));
   g_object_class_install_property (object_class, PROP_STATE,
    g_param_spec_boolean ("state", "state", "state", FALSE, flags));
   g_object_class_install_property (object_class, PROP_ACTIVE,
    g_param_spec_boolean ("active", "active", "active", FALSE, G_PARAM_WRITABLE));

  /* Инициализируем сигналы. */
  hyscan_ame_button_signals[SIG_ACTIVATED] =
    g_signal_new ("ame-activated", HYSCAN_TYPE_AME_BUTTON, G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  hyscan_ame_button_signals[SIG_TOGGLED] =
    g_signal_new ("ame-toggled", HYSCAN_TYPE_AME_BUTTON, G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE,
                  1, G_TYPE_BOOLEAN);
}

static void
hyscan_ame_button_init (HyScanAmeButton *self)
{
  self->priv = hyscan_ame_button_get_instance_private (self);
}

static void
hyscan_ame_button_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  HyScanAmeButton *self = HYSCAN_AME_BUTTON (object);
  HyScanAmeButtonPrivate *priv = self->priv;

  if (prop_id == PROP_ICON)
    priv->icon_name = g_value_dup_string (value);
  else if (prop_id == PROP_LABEL)
    priv->label = g_value_dup_string (value);
  else if (prop_id == PROP_TOGGLE)
    priv->is_toggle = g_value_get_boolean (value);
  else if (prop_id == PROP_STATE)
    priv->state = g_value_get_boolean (value);
  else if (prop_id == PROP_ACTIVE)
    hyscan_ame_button_set_active (self, g_value_get_boolean (value));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

GtkWidget *
label_maker (const gchar * markup)
{
  GtkWidget *label = gtk_label_new (NULL);
  gtk_label_set_angle (GTK_LABEL (label), 90);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  return label;

}

static gboolean
button_toggled (GtkToggleButton *button,
                HyScanAmeButton *self)
{
  gboolean state, changed;
  HyScanAmeButtonPrivate *priv = self->priv;

  state = gtk_toggle_button_get_active (button);
  changed = priv->state != state;

  if (changed)
    {
      priv->state = state;
      g_signal_emit (self, hyscan_ame_button_signals[SIG_TOGGLED], 0, priv->state);
    }

  gtk_button_set_image (GTK_BUTTON (button), priv->state ? priv->image_on : priv->image_off);
  gtk_widget_queue_draw (GTK_WIDGET (button));

  return FALSE;
}

static gboolean
button_clicked (GtkButton       *button,
                HyScanAmeButton *self)
{
  if (!self->priv->is_toggle)
    g_signal_emit (self, hyscan_ame_button_signals[SIG_ACTIVATED], 0);

  return FALSE;
}

static void
button_setup (HyScanAmeButton *self)
{
  GtkWidget *button;
  GtkStyleContext *context;
  HyScanAmeButtonPrivate *priv = self->priv;

  if (priv->is_toggle)
    {
      GtkWidget *image;
      button = gtk_toggle_button_new ();
      image = gtk_image_new_from_icon_name (priv->icon_name, GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);

      priv->image_off = gtk_image_new_from_icon_name (ICON_OFF, GTK_ICON_SIZE_BUTTON);
      priv->image_on = gtk_image_new_from_icon_name (ICON_ON, GTK_ICON_SIZE_BUTTON);
      g_object_ref (priv->image_off);
      g_object_ref (priv->image_on);
    }
  else
    {
      button = gtk_button_new_from_icon_name (priv->icon_name, GTK_ICON_SIZE_BUTTON);
    }

  gtk_widget_set_size_request (GTK_WIDGET (button), 44, 44);

  context = gtk_widget_get_style_context (button);
  gtk_style_context_add_class (context, "osd");

  priv->button = GTK_BUTTON (g_object_ref (button));

  gtk_box_pack_start (GTK_BOX (self), button, FALSE, TRUE, 0);

  if (priv->is_toggle)
    {
      hyscan_ame_button_set_sensitive (self, TRUE);
      button_toggled (GTK_TOGGLE_BUTTON (button), self);
      g_signal_connect (button, "toggled", G_CALLBACK (button_toggled), self);
    }
  else
    {
      g_signal_connect (button, "clicked", G_CALLBACK (button_clicked), self);
    }

}

static void
text_box_setup (HyScanAmeButton *self)
{
  GtkWidget *box;
  GtkWidget *title;
  gchar *bold;
  HyScanAmeButtonPrivate *priv = self->priv;

  if (priv->label == NULL)
    return;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0); ////!!

  bold = g_strdup_printf ("<b>%s</b>", priv->label);
  title = label_maker (bold);
  g_free (bold);

  priv->text_box = g_object_ref (box);

  gtk_box_pack_start (GTK_BOX (box), title, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (self), box, TRUE, TRUE, 0);
}

static void
hyscan_ame_button_object_constructed (GObject *object)
{
  HyScanAmeButton *self = HYSCAN_AME_BUTTON (object);

  G_OBJECT_CLASS (hyscan_ame_button_parent_class)->constructed (object);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing (GTK_BOX (self), 2);
  gtk_widget_set_margin_start (GTK_WIDGET (self), 6);
  gtk_widget_set_margin_end (GTK_WIDGET (self), 6);
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, HYSCAN_AME_BUTTON_HEIGHT_REQUEST);

  button_setup (self);
  text_box_setup (self);
}

static void
hyscan_ame_button_object_finalize (GObject *object)
{
  HyScanAmeButton *self = HYSCAN_AME_BUTTON (object);
  HyScanAmeButtonPrivate *priv = self->priv;

  g_free (priv->icon_name);
  g_free (priv->label);

  g_clear_object (&priv->button);
  g_clear_object (&priv->text_box);

  g_clear_object (&priv->image_on);
  g_clear_object (&priv->image_off);

  G_OBJECT_CLASS (hyscan_ame_button_parent_class)->finalize (object);
}

/* .*/
GtkWidget *
hyscan_ame_button_new (const gchar     *icon_name,
                       const gchar     *label,
                       gboolean         is_toggle,
                       gboolean         state)
{
  return g_object_new (HYSCAN_TYPE_AME_BUTTON,
                       "icon-name", icon_name,
                       "label", label,
                       "is_toggle", is_toggle,
                       "state", state,
                       NULL);
}

GtkLabel *
hyscan_ame_button_create_value (HyScanAmeButton *self,
                                const gchar     *text)
{
  GtkWidget * value;
  HyScanAmeButtonPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_AME_BUTTON (self), NULL);
  priv = self->priv;

  value = label_maker (text);

  gtk_box_pack_start (GTK_BOX (priv->text_box), value, TRUE, TRUE, 0);

  return GTK_LABEL (value);
}

void
hyscan_ame_button_activate (HyScanAmeButton *self)
{
  HyScanAmeButtonPrivate *priv;

  g_return_if_fail (HYSCAN_IS_AME_BUTTON (self));
  priv = self->priv;

  if (priv->sensitivity)
    gtk_button_clicked (priv->button);
}

void
hyscan_ame_button_set_active (HyScanAmeButton *self,
                              gboolean         state)
{
  HyScanAmeButtonPrivate *priv;

  g_return_if_fail (HYSCAN_IS_AME_BUTTON (self));
  priv = self->priv;

  if (!priv->sensitivity)
    return;

  priv->state = state;

  if (priv->is_toggle)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), state);
}

void
hyscan_ame_button_set_sensitive (HyScanAmeButton *self,
                                 gboolean         state)
{
  HyScanAmeButtonPrivate *priv;

  g_return_if_fail (HYSCAN_IS_AME_BUTTON (self));
  priv = self->priv;

  priv->sensitivity = state;
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->button), state);
}
