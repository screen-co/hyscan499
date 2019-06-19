#ifndef __EVO_UI_H__
#define __EVO_UI_H__

#include <fnn-types.h>
#include <hyscan-gtk-fnn-box.h>
#include "hyscan-gtk-map-kit.h"
#include <gio/gio.h>

typedef struct
{
  GHashTable *builders;

  GtkWidget  *grid;     // grid
  GtkWidget  *area;     // hyscan_gtk_area
  GtkWidget  *acoustic_stack; // центральная зона
  GtkWidget  *control_stack;    // стек упр-я локаторами
  GtkWidget  *nav_stack;    // стек упр-я галсами и метками
  GtkWidget  *switcher;

  struct
    {
      GtkWidget  *dry;
      GtkWidget  *all;
    } starter;

  GtkWidget  *more;

  GHashTable *balance_table; /* GtkAdjustment* */

  const gchar *restoreable;

  GKeyFile   *settings;

  HyScanGtkMapKit *mapkit;
  guint    track_select_tag;
} EvoUI;

/* Врапперы. */
void start_stop_wrapper (GtkSwitch   *switcher,
                         gboolean         state,
                         gpointer         user_data);
void start_stop_dry_wrapper (GtkSwitch   *switcher,
                         gboolean         state,
                         gpointer         user_data);

G_MODULE_EXPORT gboolean    build_interface (Global *global);
G_MODULE_EXPORT void        destroy_interface (void);

G_MODULE_EXPORT gboolean    kf_config    (GKeyFile *kf);
G_MODULE_EXPORT gboolean    kf_seting    (GKeyFile *kf);
G_MODULE_EXPORT gboolean    kf_desetup   (GKeyFile *kf);

G_MODULE_EXPORT gboolean    panel_pack        (FnnPanel *panel,
                                               gint      panelx);

G_MODULE_EXPORT void        panel_adjust_visibility (HyScanTrackInfo *track_info);
// void        switch_page (GObject     *emitter,
                         // const gchar *page);


#endif /* __EVO_UI_H__ */
