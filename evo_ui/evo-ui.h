#ifndef __EVO_UI_H__
#define __EVO_UI_H__

#include <triple-types.h>
#include <hyscan-gtk-ame-box.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>

typedef struct
{
  GHashTable *builders;

  GtkWidget  *grid;     // grid
  GtkWidget  *area;     // hyscan_gtk_area
  GtkWidget  *acoustic_stack; // центральная зона
  GtkWidget  *control_stack;    // стек упр-я локаторами
  GtkWidget  *switcher;

  struct
    {
      GtkWidget  *dry;
      GtkWidget  *all;
    } starter;

  GtkWidget  *more;
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

void        switch_page (GObject     *emitter,
                         const gchar *page);


#endif /* __EVO_UI_H__ */
