#ifndef __AME_UI_H__
#define __AME_UI_H__

#include "triple-types.h"
#include <hyscan-ame-fixed.h>
#include <hyscan-ame-button.h>
#include <hyscan-gtk-ame-box.h>

#define AME_N_BUTTONS 5


/* Кнопки м.б. слева или справа. */
typedef enum
{
  END,
  L,
  R
} AmeSide;

typedef enum
{
  TOGGLE_NONE,
  TOGGLE_OFF,
  TOGGLE_ON
} AmeToggle;

/*
 * Ща сложно будет. В странице есть destination_selector.
 * Он определяет, куда кладется виджет. 
 * Поэтому нельзя в трипл-тайпсах ничего трогать.
 */
typedef enum
{
  DEST_UNSET,
  DEST_PANEL, // AmePanel.gui + offt
  DEST_PANEL_SPEC,  // AmePanel.vis_gui + offt
  DEST_AME_UI // AmeUI + offt
} AmePageDestination;

typedef struct
{
  AmeSide      side;
  gint         position;

  const gchar *icon_name;
  const gchar *title;
  AmeToggle    toggle;

  gpointer     callback;
  gpointer     user_data; // либо юзердата, либо панелх.

  gsize        value_offset; // оффсет до места для виджета зн-я в соотв структуре.
  gsize        button_offset; // оффсет до места для самой кнопки.

  gchar       *value_default; 
} AmePageItem;

typedef struct
{
  gchar              *path;
  AmePageDestination  destination_selector; //
  AmePageItem         items[2 * AME_N_BUTTONS + 1];
} AmePage;

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
  GtkWidget  *sub_center_box;

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
} AmeUI;

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
void start_stop_wrapper (HyScanAmeButton *button,
                         gboolean         state,
                         gpointer         user_data);
void start_stop_dry_wrapper (HyScanAmeButton *button,
                         gboolean         state,
                         gpointer         user_data);

/* Функции построения интерфейса. */
GtkWidget * make_item (AmePageItem  *item,
                       GtkWidget   **value);

void        build_page (AmeUI   *ui,
                        Global  *global,
                        AmePage *page);

void        build_all (AmeUI   *ui,
                       Global  *global,
                       AmePage *page);

gboolean    build_interface (Global *global);

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

#endif /* __AME_UI_H__ */
