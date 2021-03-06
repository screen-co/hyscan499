#ifndef __HW_UI_H__
#define __HW_UI_H__

#include <fnn-types.h>
#include <hyscan-fnn-fixed.h>
#include <hyscan-fnn-button.h>
#include <hyscan-gtk-fnn-box.h>

#include <gio/gio.h>

#define FNN_N_BUTTONS 5

typedef struct 
{
  GSocket *socket;
  GThread *thread;
  gint     stop;
} ButtonReceive;

/* Кнопки м.б. слева или справа. */
typedef enum
{
  END,
  L,
  R
} HwuiSide;

typedef enum
{
  TOGGLE_NONE,
  TOGGLE_OFF,
  TOGGLE_ON
} HwuiToggle;

/*
 * Ща сложно будет. В странице есть destination_selector.
 * Он определяет, куда кладется виджет. 
 * Поэтому нельзя в трипл-тайпсах ничего трогать.
 */
typedef enum
{
  DEST_UNSET,
  DEST_PANEL, // FnnPanel.gui + offt
  DEST_PANEL_SPEC,  // FnnPanel.vis_gui + offt
  DEST_HWUI_GLOBAL // HwuiGlobal + offt
} HwuiPageDestination;

typedef struct
{
  HwuiSide      side;
  gint         position;

  const gchar *icon_name;
  // const gchar *title;
  const gchar *msg_id;
  HwuiToggle   toggle;

  gpointer     callback;
  gpointer     user_data; // либо юзердата, либо панелх.

  gsize        value_offset; // оффсет до места для виджета зн-я в соотв структуре.
  gsize        button_offset; // оффсет до места для самой кнопки.

  gchar       *value_default; 
} HwuiPageItem;

typedef struct
{
  gchar              *path;
  HwuiPageDestination  destination_selector; //
  HwuiPageItem         items[2 * FNN_N_BUTTONS + 1];
} HwuiPage;

enum
{
  START_STOP_SIDESCAN,
  START_STOP_PROFILER,
  START_STOP_FORWARDL,
  START_STOP_LAST
};

typedef struct
{
  // gpointer    padding;
  
  GtkWidget  *acoustic;
  // GtkWidget  *sub_center_box;

  GtkWidget  *left_box;
  GtkWidget  *left_revealer;
  GtkWidget  *bott_revealer;

  GtkWidget  *lstack;
  GtkWidget  *rstack;

  struct
    {
      GtkWidget  *dry;
      GtkWidget  *all;
      GtkWidget  *panel[START_STOP_LAST];
    } starter;

  GtkWidget  *current_view;

  gchar * depth_writer_path;
  gchar * screenshot_dir;
} HwuiGlobal;

enum
{
  L_MARKS,
  L_TRACKS
};

/* Важно, чтобы эти парни не совпадали ни с одним из ид панелей. */
enum
{
  ROTATE = -2,
  ALL = -1
};

/* Врапперы. */
void start_stop_wrapper (HyScanFnnButton *button,
                         gboolean         state,
                         gpointer         user_data);
void start_stop_dry_wrapper (HyScanFnnButton *button,
                         gboolean         state,
                         gpointer         user_data);

/* Функции построения интерфейса. */
GtkWidget * make_item (HwuiPageItem  *item,
                       GtkWidget   **value);

void        build_page (HwuiGlobal   *ui,
                        Global  *global,
                        HwuiPage *page);

void        build_all (HwuiGlobal   *ui,
                       Global  *global,
                       HwuiPage *page);

void screenshooter (void);

G_MODULE_EXPORT gboolean    build_interface (Global *global);

G_MODULE_EXPORT void        destroy_interface (void);

/* Для кнопочек. */
G_MODULE_EXPORT gboolean    kf_config    (GKeyFile *kf);
G_MODULE_EXPORT gboolean    kf_setting    (GKeyFile *kf);
G_MODULE_EXPORT gboolean    kf_desetup   (GKeyFile *kf);

/* Внутренние, для ожидания и получения кнопок. */
gboolean    ame_button_clicker (gpointer data);
gint        ame_button_parse   (const gchar * buf);
void *      ame_button_thread  (void * data);


/* Функции управления. */
void        widget_swap (GObject  *emitter,
                         gpointer  user_data);
void        switch_page (GObject     *emitter,
                         const gchar *page);

void
list_scroll_up (GObject *emitter,
                gpointer udata);
void
list_scroll_down (GObject *emitter,
                  gpointer udata);

void
list_scroll_start (GObject *emitter,
                   gpointer udata);
void
list_scroll_end (GObject *emitter,
                 gpointer udata);

void
depth_writer (GObject *emitter);

#endif /* __HW_UI_H__ */
