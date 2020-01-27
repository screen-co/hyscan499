#define GETTEXT_PACKAGE "hyscanfnn-evoui"

#include "hyscan-gtk-map-preload.h"
#include <hyscan-map-tile-loader.h>
#include <hyscan-gtk-map-base.h>
#include <glib/gi18n-lib.h>

#define PRELOAD_STATE_DONE   1000         /* Статус кэширования тайлов 0 "Загрузка завершена". */
#define REFRESH_INTERVAL     500          /* Период обновления прогрессбара загрузки. */

enum
{
  PROP_O,
  PROP_MAP,
  PROP_BASE,
};

struct _HyScanGtkMapPreloadPrivate
{
  HyScanGtkMap                *map;              /* Виджет карты. */
  HyScanGtkMapBase            *base;             /* Слой подложки. */
  GtkWidget                   *download_btn;     /* Кнопка загрузки тайлов. */
  HyScanMapTileLoader         *loader;           /* Загрузчик тайлов. */
  GtkProgressBar              *progress_bar;     /* Прогресс загрузки. */

  guint                        preload_tag;      /* Timeout-функция обновления виджета progress_bar. */
  gint                         preload_state;    /* Статус загрузки тайлов.
                                                  * < 0                      - загрузка не началась,
                                                  * 0 - PRELOAD_STATE_DONE-1 - прогресс загрузки,
                                                  * >= PRELOAD_STATE_DONE    - загрузка завершена, не удалось загрузить
                                                  *                            PRELOAD_STATE_DONE - preload_state тайлов */

};

static void                hyscan_gtk_map_preload_set_property             (GObject                  *object,
                                                                            guint                     prop_id,
                                                                            const GValue             *value,
                                                                            GParamSpec               *pspec);
static void                hyscan_gtk_map_preload_object_constructed       (GObject                  *object);
static void                hyscan_gtk_map_preload_object_finalize          (GObject                  *object);
static void                hyscan_gtk_map_preload_start                    (HyScanGtkMapPreload      *preload);
static void                hyscan_gtk_map_preload_click                    (HyScanGtkMapPreload      *preload);
static void                hyscan_gtk_map_preload_done                     (HyScanGtkMapPreload      *preload,
                                                                            guint                     failed);
static void                hyscan_gtk_map_preload_progress                 (HyScanGtkMapPreload      *preload,
                                                                            gdouble                   fraction);
static gboolean            hyscan_gtk_map_preload_update_progress          (gpointer                  data);
static gboolean            hyscan_gtk_map_preload_progress_done            (gpointer                  data);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkMapPreload, hyscan_gtk_map_preload, GTK_TYPE_BOX)

static void
hyscan_gtk_map_preload_class_init (HyScanGtkMapPreloadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = hyscan_gtk_map_preload_set_property;

  object_class->constructed = hyscan_gtk_map_preload_object_constructed;
  object_class->finalize = hyscan_gtk_map_preload_object_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/evo/gtk/hyscan-gtk-map-preload.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMapPreload, progress_bar);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMapPreload, download_btn);

  g_object_class_install_property (object_class, PROP_MAP,
    g_param_spec_object ("map", "HyScanGtkMap", "Map object",
                         HYSCAN_TYPE_GTK_MAP,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_BASE,
    g_param_spec_object ("base", "HyScanGtkMapBas", "Map base layer",
                         HYSCAN_TYPE_GTK_MAP_BASE,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_map_preload_init (HyScanGtkMapPreload *preload)
{
  preload->priv = hyscan_gtk_map_preload_get_instance_private (preload);
  gtk_widget_init_template (GTK_WIDGET (preload));
}

static void
hyscan_gtk_map_preload_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  HyScanGtkMapPreload *gtk_map_preload = HYSCAN_GTK_MAP_PRELOAD (object);
  HyScanGtkMapPreloadPrivate *priv = gtk_map_preload->priv;

  switch (prop_id)
    {
    case PROP_MAP:
      priv->map = g_value_dup_object (value);
      break;

    case PROP_BASE:
      priv->base = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_map_preload_object_constructed (GObject *object)
{
  HyScanGtkMapPreload *preload = HYSCAN_GTK_MAP_PRELOAD (object);
  HyScanGtkMapPreloadPrivate *priv = preload->priv;

  G_OBJECT_CLASS (hyscan_gtk_map_preload_parent_class)->constructed (object);

  priv->preload_state = -1;
  g_signal_connect_swapped (priv->download_btn, "clicked", G_CALLBACK (hyscan_gtk_map_preload_click), preload);
}

static void
hyscan_gtk_map_preload_object_finalize (GObject *object)
{
  HyScanGtkMapPreload *gtk_map_preload = HYSCAN_GTK_MAP_PRELOAD (object);
  HyScanGtkMapPreloadPrivate *priv = gtk_map_preload->priv;

  g_clear_object (&priv->map);
  g_clear_object (&priv->base);
  g_clear_object (&priv->loader);

  G_OBJECT_CLASS (hyscan_gtk_map_preload_parent_class)->finalize (object);
}

static void
hyscan_gtk_map_preload_click (HyScanGtkMapPreload *preload)
{
  HyScanGtkMapPreloadPrivate *priv = preload->priv;

  if (g_atomic_int_compare_and_exchange (&priv->preload_state, -1, 0))
    hyscan_gtk_map_preload_start (preload);
  else
    hyscan_map_tile_loader_stop (priv->loader);
}

static void
hyscan_gtk_map_preload_start (HyScanGtkMapPreload *preload)
{
  HyScanGtkMapPreloadPrivate *priv = preload->priv;
  HyScanMapTileSource *source;
  gdouble from_x, to_x, from_y, to_y;
  GThread *thread;

  g_clear_object (&priv->loader);
  priv->loader = hyscan_map_tile_loader_new ();

  gtk_button_set_label (GTK_BUTTON (priv->download_btn), _("Stop download"));
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), NULL);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), 0.0);

  /* Регистрируем обработчики сигналов для loader'а и обновление индикатора загрузки. */
  g_signal_connect_swapped (priv->loader, "progress", G_CALLBACK (hyscan_gtk_map_preload_progress), preload);
  g_signal_connect_swapped (priv->loader, "done", G_CALLBACK (hyscan_gtk_map_preload_done), preload);
  priv->preload_tag = g_timeout_add (REFRESH_INTERVAL, hyscan_gtk_map_preload_update_progress, preload);

  /* Запускаем загрузку тайлов. */
  source = hyscan_gtk_map_base_get_source (priv->base);
  gtk_cifro_area_get_view (GTK_CIFRO_AREA (priv->map), &from_x, &to_x, &from_y, &to_y);
  thread = hyscan_map_tile_loader_start (priv->loader, source, from_x, to_x, from_y, to_y);
  g_thread_unref (thread);
  g_object_unref (source);
}

static void
hyscan_gtk_map_preload_done (HyScanGtkMapPreload *preload,
                             guint                failed)
{
  HyScanGtkMapPreloadPrivate *priv = preload->priv;
  g_atomic_int_set (&priv->preload_state, PRELOAD_STATE_DONE + failed);

  g_source_remove (priv->preload_tag);
  g_idle_add (hyscan_gtk_map_preload_progress_done, preload);
}

static void
hyscan_gtk_map_preload_progress (HyScanGtkMapPreload *preload,
                                 gdouble              fraction)
{
  HyScanGtkMapPreloadPrivate *priv = preload->priv;

  g_atomic_int_set (&priv->preload_state, CLAMP ((gint) (PRELOAD_STATE_DONE * fraction), 0, PRELOAD_STATE_DONE - 1));
}

static gboolean
hyscan_gtk_map_preload_update_progress (gpointer data)
{
  HyScanGtkMapPreload *kit = data;
  HyScanGtkMapPreloadPrivate *priv = kit->priv;
  gint state;

  state = g_atomic_int_get (&priv->preload_state);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
                                 CLAMP ((gdouble) state / PRELOAD_STATE_DONE, 0.0, 1.0));

  return G_SOURCE_CONTINUE;
}

static gboolean
hyscan_gtk_map_preload_progress_done (gpointer data)
{
  HyScanGtkMapPreload *kit = data;
  HyScanGtkMapPreloadPrivate *priv = kit->priv;

  gint state;
  gchar *text = NULL;

  state = g_atomic_int_get (&priv->preload_state);

  if (state == PRELOAD_STATE_DONE)
    text = g_strdup (_("Done!"));
  else if (state > PRELOAD_STATE_DONE)
    text = g_strdup_printf (_("Done! Failed to load %d tiles"), state - PRELOAD_STATE_DONE);
  else
    g_warn_if_reached ();

  gtk_button_set_label (GTK_BUTTON (priv->download_btn), _("Start download"));
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), 1.0);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
  g_free (text);

  g_signal_handlers_disconnect_by_data (priv->loader, kit);
  g_clear_object (&priv->loader);

  g_atomic_int_set (&priv->preload_state, -1);

  return G_SOURCE_REMOVE;
}

GtkWidget *
hyscan_gtk_map_preload_new (HyScanGtkMap     *map,
                            HyScanGtkMapBase *base)
{
  return g_object_new (HYSCAN_TYPE_GTK_MAP_PRELOAD,
                       "map", map,
                       "base", base,
                       NULL);
}
