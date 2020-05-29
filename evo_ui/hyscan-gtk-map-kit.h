#ifndef __HYSCAN_GTK_MAP_KIT_H__
#define __HYSCAN_GTK_MAP_KIT_H__

#include <hyscan-gtk-map.h>
#include <hyscan-units.h>
#include <hyscan-db.h>
#include <hyscan-sensor.h>
#include <hyscan-gtk-mark-manager.h>
#include <hyscan-sonar-recorder.h>

typedef struct _HyScanGtkMapKitPrivate HyScanGtkMapKitPrivate;

typedef struct
{
  GtkWidget *navigation;          /* Виджет со списком меток и галсов. */
  GtkWidget *map;                 /* Виджет карты. */
  GtkWidget *control;             /* Виджет с кнопками управления картой. */
  GtkWidget *status_bar;          /* Виджет со статусом. */
  GtkWidget *paned;               /* Центральный виджет. */

  /* ## Private. ## */
  HyScanGtkMapKitPrivate *priv;
} HyScanGtkMapKit;

HyScanGtkMapKit * hyscan_gtk_map_kit_new              (HyScanGeoPoint             *center,
                                                       HyScanGtkModelManager      *model_manager,
                                                       HyScanCache                *cache,
                                                       HyScanUnits                *units,
                                                       const gchar                *cache_dir);

gboolean          hyscan_gtk_map_kit_set_profile_name (HyScanGtkMapKit            *kit,
                                                       const gchar                *profile_name);

gchar *           hyscan_gtk_map_kit_get_profile_name (HyScanGtkMapKit            *kit);

void              hyscan_gtk_map_kit_load_profiles    (HyScanGtkMapKit            *kit,
                                                       const gchar                *profile_dir);

void              hyscan_gtk_map_kit_add_nav          (HyScanGtkMapKit            *kit,
                                                       HyScanSensor               *sensor,
                                                       const gchar                *sensor_name,
                                                       HyScanSonarRecorder        *recorder,
                                                       const HyScanAntennaOffset  *offset,
                                                       gdouble                     delay_time);

void              hyscan_gtk_map_kit_add_marks        (HyScanGtkMapKit            *kit);

void              hyscan_gtk_map_kit_add_planner      (HyScanGtkMapKit            *kit);

gboolean          hyscan_gtk_map_kit_get_offline      (HyScanGtkMapKit            *kit);

void              hyscan_gtk_map_kit_set_offline      (HyScanGtkMapKit            *kit,
                                                       gboolean                    offline);

void              hyscan_gtk_map_kit_set_smooth       (HyScanGtkMapKit            *kit,
                                                       gboolean                    smooth);

void              hyscan_gtk_map_kit_free             (HyScanGtkMapKit            *kit);

GtkTreeView *     hyscan_gtk_map_kit_get_track_view   (HyScanGtkMapKit            *kit,
                                                       gint                       *track_col);

void              hyscan_gtk_map_kit_kf_setup         (HyScanGtkMapKit            *kit,
                                                       GKeyFile                   *kf);

void              hyscan_gtk_map_kit_kf_desetup       (HyScanGtkMapKit            *kit,
                                                       GKeyFile                   *keyfile);

HyScanObjectModel *
                  hyscan_gtk_map_kit_get_planner      (HyScanGtkMapKit            *kit);

void              hyscan_gtk_map_kit_run_planner_import (HyScanGtkMapKit          *kit);

HyScanTrackPlan * hyscan_gtk_map_kit_get_track_plan   (HyScanGtkMapKit           *kit);

#endif /* __HYSCAN_GTK_MAP_KIT_H__ */
