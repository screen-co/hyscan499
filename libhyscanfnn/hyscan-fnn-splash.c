#include "hyscan-fnn-splash.h"

struct _HyScanFnnSplashPrivate
{
  gchar          *message;
  guint           pulse;
};

static void    hyscan_fnn_splash_set_property             (GObject               *object,
                                                           guint                  prop_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static void    hyscan_fnn_splash_object_constructed       (GObject               *object);
static void    hyscan_fnn_splash_object_finalize          (GObject               *object);

static gboolean hyscan_fnn_splash_pulse (gpointer data);
static gboolean hyscan_fnn_splash_response (gpointer data);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanFnnSplash, hyscan_fnn_splash, GTK_TYPE_DIALOG);

static void
hyscan_fnn_splash_class_init (HyScanFnnSplashClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = hyscan_fnn_splash_object_constructed;
  object_class->finalize = hyscan_fnn_splash_object_finalize;

  object_class->set_property = hyscan_fnn_splash_set_property;
  g_object_class_install_property (object_class, 1,
    g_param_spec_string ("message", "message", "message", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_fnn_splash_init (HyScanFnnSplash *self)
{
  self->priv = hyscan_fnn_splash_get_instance_private (self);
}

static void
hyscan_fnn_splash_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  HyScanFnnSplash *self = HYSCAN_FNN_SPLASH (object);
  HyScanFnnSplashPrivate *priv = self->priv;

  if (prop_id == 1)
    priv->message = g_value_dup_string (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
hyscan_fnn_splash_object_constructed (GObject *object)
{
  HyScanFnnSplash *self = HYSCAN_FNN_SPLASH (object);

  HyScanFnnSplashPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_fnn_splash_parent_class)->constructed (object);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  GtkWidget *label = gtk_label_new (NULL);
  GtkWidget *wait = gtk_label_new ("Пожалуйста, подождите");
  GtkWidget *progress = gtk_progress_bar_new ();

  gchar *markup = g_strdup_printf ("<big>%s</big>", priv->message);

  g_object_set (label, "margin", 36, "label", markup, "use-markup", TRUE, NULL);
  g_object_set (box, "margin", 6, NULL);

  g_free (markup);

  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), progress, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (box), wait, TRUE, TRUE, 0);

  gtk_widget_show_all (GTK_WIDGET (self));

  priv->pulse = g_timeout_add (100, hyscan_fnn_splash_pulse, progress);
  g_timeout_add (100, hyscan_fnn_splash_response, self);
  gtk_dialog_run (GTK_DIALOG (self));
}

static void
hyscan_fnn_splash_object_finalize (GObject *object)
{
  HyScanFnnSplash *self = HYSCAN_FNN_SPLASH (object);

  g_source_remove (self->priv->pulse);
  g_free (self->priv->message);

  G_OBJECT_CLASS (hyscan_fnn_splash_parent_class)->finalize (object);
}

static gboolean
hyscan_fnn_splash_pulse (gpointer data)
{
  GtkProgressBar *progress = data;
  gtk_progress_bar_pulse (progress);
  return G_SOURCE_CONTINUE;
}

static gboolean
hyscan_fnn_splash_response (gpointer data)
{
  GtkDialog *dialog = data;
  gtk_dialog_response (dialog, 0);
  return G_SOURCE_CONTINUE;

}

HyScanFnnSplash*
hyscan_fnn_splash_new (const gchar *message)
{
  return g_object_new (HYSCAN_TYPE_FNN_SPLASH,
                       "message", message, NULL);
}
