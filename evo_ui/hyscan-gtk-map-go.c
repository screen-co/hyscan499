#include "hyscan-gtk-map-go.h"

enum
{
  PROP_O,
  PROP_MAP
};

struct _HyScanGtkMapGoPrivate
{
  HyScanGtkMap                *map;    /* Карта. */
  GtkWidget                   *lat;    /* GtkSpinButton для ввода широты. */
  GtkWidget                   *lon;    /* GtkSpinButton для ввода долготы. */
  GtkWidget                   *btn;    /* Кнопка перехода. */
};

static void    hyscan_gtk_map_go_set_property          (GObject               *object,
                                                        guint                  prop_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
static void    hyscan_gtk_map_go_object_constructed    (GObject               *object);
static void    hyscan_gtk_map_go_object_finalize       (GObject               *object);
static void    hyscan_gtk_map_go_click                 (HyScanGtkMapGo        *map_go);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkMapGo, hyscan_gtk_map_go, GTK_TYPE_BOX)

static void
hyscan_gtk_map_go_class_init (HyScanGtkMapGoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = hyscan_gtk_map_go_set_property;

  object_class->constructed = hyscan_gtk_map_go_object_constructed;
  object_class->finalize = hyscan_gtk_map_go_object_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/evo/gtk/hyscan-gtk-map-go.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMapGo, lat);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMapGo, lon);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkMapGo, btn);

  g_object_class_install_property (object_class, PROP_MAP,
    g_param_spec_object ("map", "HyScanGtkMap", "HyScanGtkMap", HYSCAN_TYPE_GTK_MAP,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_map_go_init (HyScanGtkMapGo *map_go)
{
  map_go->priv = hyscan_gtk_map_go_get_instance_private (map_go);
  gtk_widget_init_template (GTK_WIDGET (map_go));
}

static void
hyscan_gtk_map_go_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  HyScanGtkMapGo *gtk_map_go = HYSCAN_GTK_MAP_GO (object);
  HyScanGtkMapGoPrivate *priv = gtk_map_go->priv;

  switch (prop_id)
    {
    case PROP_MAP:
      priv->map = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_map_go_object_constructed (GObject *object)
{
  HyScanGtkMapGo *gtk_map_go = HYSCAN_GTK_MAP_GO (object);
  HyScanGtkMapGoPrivate *priv = gtk_map_go->priv;

  G_OBJECT_CLASS (hyscan_gtk_map_go_parent_class)->constructed (object);

  g_signal_connect_swapped (priv->btn, "clicked", G_CALLBACK (hyscan_gtk_map_go_click), gtk_map_go);
}

static void
hyscan_gtk_map_go_object_finalize (GObject *object)
{
  HyScanGtkMapGo *gtk_map_go = HYSCAN_GTK_MAP_GO (object);
  HyScanGtkMapGoPrivate *priv = gtk_map_go->priv;

  g_object_unref (priv->map);

  G_OBJECT_CLASS (hyscan_gtk_map_go_parent_class)->finalize (object);
}

static void
hyscan_gtk_map_go_click (HyScanGtkMapGo *map_go)
{
  HyScanGtkMapGoPrivate *priv = map_go->priv;
  HyScanGeoPoint center;

  center.lat = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->lat));
  center.lon = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->lon));

  hyscan_gtk_map_move_to (priv->map, center);
}

GtkWidget *
hyscan_gtk_map_go_new (HyScanGtkMap *map)
{
  return g_object_new (HYSCAN_TYPE_GTK_MAP_GO,
                       "map", map,
                       NULL);
}
