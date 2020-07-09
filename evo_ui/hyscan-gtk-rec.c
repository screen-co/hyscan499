#include <hyscan-sonar-model.h>
#include "hyscan-gtk-rec.h"

enum
{
  PROP_O,
  PROP_RECORDER,
  PROP_SONAR,
};

struct _HyScanGtkRecPrivate
{
  HyScanSonarRecorder        *recorder;        /* Модель работы ГЛ. */
  gulong                      handler_id;      /* ИД обработчика переключения тумблера. */
  HyScanSonar *sonar;
};

static void     hyscan_gtk_rec_set_property             (GObject               *object,
                                                         guint                  prop_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     hyscan_gtk_rec_object_constructed       (GObject               *object);
static void     hyscan_gtk_rec_object_finalize          (GObject               *object);
static gboolean hyscan_gtk_rec_state_set                (HyScanGtkRec          *gtk_rec,
                                                         gboolean               state);
static void     hyscan_gtk_rec_state_start_stop         (HyScanGtkRec          *gtk_rec);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkRec, hyscan_gtk_rec, GTK_TYPE_SWITCH)

static void
hyscan_gtk_rec_class_init (HyScanGtkRecClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_gtk_rec_set_property;

  object_class->constructed = hyscan_gtk_rec_object_constructed;
  object_class->finalize = hyscan_gtk_rec_object_finalize;

  g_object_class_install_property (object_class, PROP_RECORDER,
    g_param_spec_object ("recorder", "HyScanSonarRecorder", "HyScanSonarRecorder object",
                         HYSCAN_TYPE_SONAR_RECORDER,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));
}

static void
hyscan_gtk_rec_init (HyScanGtkRec *gtk_rec)
{
  gtk_rec->priv = hyscan_gtk_rec_get_instance_private (gtk_rec);
}

static void
hyscan_gtk_rec_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  HyScanGtkRec *gtk_rec = HYSCAN_GTK_REC (object);
  HyScanGtkRecPrivate *priv = gtk_rec->priv;

  switch (prop_id)
    {
    case PROP_RECORDER:
      priv->recorder = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_rec_object_constructed (GObject *object)
{
  HyScanGtkRec *gtk_rec = HYSCAN_GTK_REC (object);
  HyScanGtkRecPrivate *priv = gtk_rec->priv;

  G_OBJECT_CLASS (hyscan_gtk_rec_parent_class)->constructed (object);

  priv->handler_id = g_signal_connect_swapped (object, "state-set",
                                               G_CALLBACK (hyscan_gtk_rec_state_set), gtk_rec);

  /* Объект управления гидролокатором может оказаться не HyScanSonarModel, поэтому надо убедиться в этом. */
  priv->sonar = hyscan_sonar_recorder_get_sonar (priv->recorder);
  if (HYSCAN_IS_SONAR_MODEL (priv->sonar))
    g_signal_connect_swapped (priv->sonar, "start-stop", G_CALLBACK (hyscan_gtk_rec_state_start_stop), gtk_rec);
  else
    g_critical ("HyScanGtkRec: sonar is not a HyScanSonarModel, widget will not work properly");
}

static void
hyscan_gtk_rec_object_finalize (GObject *object)
{
  HyScanGtkRec *gtk_rec = HYSCAN_GTK_REC (object);
  HyScanGtkRecPrivate *priv = gtk_rec->priv;

  if (HYSCAN_IS_SONAR_MODEL (priv->sonar))
    g_signal_handlers_disconnect_by_data (priv->sonar, object);

  g_object_unref (priv->recorder);
  g_object_unref (priv->sonar);

  G_OBJECT_CLASS (hyscan_gtk_rec_parent_class)->finalize (object);
}

/* Обработчик сигнала "start-stop" синхронизирует положение тумблера со статусом работы ГЛ. */
static void
hyscan_gtk_rec_state_start_stop (HyScanGtkRec *gtk_rec)
{
  HyScanGtkRecPrivate *priv = gtk_rec->priv;
  gboolean state;

  state = hyscan_sonar_state_get_start (HYSCAN_SONAR_STATE (priv->sonar), NULL, NULL, NULL, NULL);

  /* Переводим виджет в новое состояние без обработчика. */
  g_signal_handler_block (gtk_rec, priv->handler_id);
  gtk_switch_set_active (GTK_SWITCH (gtk_rec), state);
  gtk_switch_set_state (GTK_SWITCH (gtk_rec), state);
  g_signal_handler_unblock (gtk_rec, priv->handler_id);
}

/* Обработчик "state-set" включает/выключает ГЛ при переключении тумблера. */
static gboolean
hyscan_gtk_rec_state_set (HyScanGtkRec *gtk_rec,
                          gboolean      state)
{
  HyScanGtkRecPrivate *priv = gtk_rec->priv;

  if (state)
    hyscan_sonar_recorder_start (priv->recorder);
  else
    hyscan_sonar_recorder_stop (priv->recorder);

  /* Ждём сигнала "start-stop", чтобы переключить тумблер в то состояние, которое соответствует статусу работы ГЛ. */
  return TRUE;
}

GtkWidget *
hyscan_gtk_rec_new (HyScanSonarRecorder *recorder)
{
  return g_object_new (HYSCAN_TYPE_GTK_REC,
                       "recorder", recorder,
                       NULL);
}
