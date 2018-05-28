#ifndef __TRIPLE_H__
#define __TRIPLE_H__

#include <hyscan-gtk-area.h>
#include <hyscan-sonar-driver.h>
#include <hyscan-sonar-client.h>
#include <hyscan-sonar-control.h>
#include <hyscan-forward-look-data.h>
#include <hyscan-gtk-waterfall.h>
#include <hyscan-gtk-waterfall-grid.h>
#include <hyscan-gtk-waterfall-control.h>
#include <hyscan-gtk-waterfall-meter.h>
#include <hyscan-gtk-waterfall-mark.h>
#include <hyscan-mark-manager.h>
#include <hyscan-gtk-project-viewer.h>
#include <hyscan-gtk-mark-editor.h>
#include <hyscan-tile-color.h>
#include <hyscan-nmea-parser.h>
#include <hyscan-mloc.h>
#include <hyscan-db-info.h>
#include <hyscan-cached.h>
#include <urpc-server.h>
#include <math.h>
#include <string.h>

#include "sonar-configure.h"
#include "hyscan-gtk-forward-look.h"
#include "hyscan-fl-coords.h"
#include "hyscan-mark-sync.h"

#define hyscan_return_val_if_fail(expr,val) do {if (!(expr)) {g_message("fail @ %i", __LINE__); return (val);}} while (FALSE)
#define hyscan_exit_if(expr,msg) do {if (!(expr)) break; g_message ((msg)); goto exit;} while (FALSE)
#define hyscan_exit_if_w_param(expr,msg,param) do {if (!(expr)) break; g_message ((msg),(param)); goto exit;} while (FALSE)

#define WRONG_SELECTOR { g_message ("Wrong sonar selector @%i", __LINE__); }

// #define GUI_TEST 0

#define STARBOARD HYSCAN_SOURCE_SIDE_SCAN_STARBOARD
#define PORTSIDE HYSCAN_SOURCE_SIDE_SCAN_PORT
#define PROFILER HYSCAN_SOURCE_PROFILER
#define ECHOSOUNDER HYSCAN_SOURCE_ECHOSOUNDER
#define FORWARDLOOK HYSCAN_SOURCE_FORWARD_LOOK

#define FORWARD_LOOK_MAX_DISTANCE      75.0
#define SIDE_SCAN_MAX_DISTANCE         150.0
#define PROFILER_MAX_DISTANCE          150.0

#define MAX_COLOR_MAPS                 4
#define BLACK_BG                       0xff000000
#define WHITE_BG                       0xffdddddd

#define PF_TRACK_PREFIX                "pf"
#define DRY_TRACK_SUFFIX                "-dry"

#define AME_N_BUTTONS 10
#define AME_KEY_RIGHT 39
#define AME_KEY_LEFT 37
#define AME_KEY_ENTER 10
#define AME_KEY_UP 38
#define AME_KEY_ESCAPE 27
#define AME_KEY_EXIT 1

#define URPC_KEYCODE_NO_ACTION -1

enum
{
  L_MARKS,
  L_TRACKS
};

enum
{
  ROTATE = -2,
  ALL = -1
};

enum
{
  W_SIDESCAN,
  W_PROFILER,
  W_FORWARDL,
  W_LAST
};

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


typedef struct
{
  AmeSide      side;
  gint         position;

  const gchar *icon_name;
  const gchar *title;
  AmeToggle    toggle;

  gpointer     callback;
  gpointer     user_data;

  gpointer    *value;
  gchar       *value_default;
  
  gpointer    *place;
} AmePageItem;

typedef struct
{
  gchar       *path;
  AmePageItem  items[AME_N_BUTTONS + 1];
} AmePage;

typedef struct
{
  HyScanSonarControl      *sonar_ctl;
  HyScanTVGControl        *tvg_ctl;
  HyScanGeneratorControl  *gen_ctl;

  gdouble                  cur_distance;
  guint                    cur_signal;
  gdouble                  cur_gain0;
  gdouble                  cur_gain_step;
  gdouble                  cur_level;
  gdouble                  cur_sensitivity;
} SonarCommon;

typedef struct
{
  GtkLabel                *distance_value;
  GtkLabel                *tvg_value;
  GtkLabel                *tvg0_value;
  GtkLabel                *tvg_level_value;
  GtkLabel                *tvg_sens_value;

  GtkLabel                *signal_value;

  GtkLabel                *brightness_value;
  GtkLabel                *black_value;
  GtkLabel                *scale_value;
  GtkLabel                *colormap_value;
  GtkLabel                *sensitivity_value; /* Sensitivity or colormap. */
} SonarSpecificGui;

typedef struct
{
  HyScanDB                            *db;
  HyScanDBInfo                        *db_info;

  gchar                               *project_name;
  gchar                               *track_name;

  gboolean                             dry;

  HyScanCache                         *cache;
  HyScanMarkManager                   *mman;

  gboolean                             full_screen;
  gdouble                              sound_velocity;

  gint                                 view_selector;

  uRpcServer                          *urpc_server;
  gint                                 urpc_keycode;

  struct
    {
      HyScanMarkSync *msync;
      GHashTable     *previous;
    } marksync;

  struct
    {
      GMutex                               lock;

      HyScanNMEAParser                    *time;
      HyScanNMEAParser                    *date;
      HyScanNMEAParser                    *lat;
      HyScanNMEAParser                    *lon;
      HyScanNMEAParser                    *trk;
      HyScanNMEAParser                    *spd;
      HyScanNMEAParser                    *dpt;

      GtkLabel                            *l_tmd;
      GtkLabel                            *l_lat;
      GtkLabel                            *l_lon;
      GtkLabel                            *l_trk;
      GtkLabel                            *l_spd;
      GtkLabel                            *l_dpt;

      gchar                               *tmd_value;
      gchar                               *lat_value;
      gchar                               *lon_value;
      gchar                               *trk_value;
      gchar                               *spd_value;
      gchar                               *dpt_value;
    } nmea;

  struct
    {
      GtkWidget                           *window; // окно

      GtkWidget                           *grid;
      GtkWidget                           *center_box;
      GtkWidget                           *sub_center_box;

      GtkWidget                           *left_box;
      GtkWidget                           *side_revealer;
      GtkWidget                           *bott_revealer;

      GtkWidget                           *start_stop_dry_switch;
      GtkWidget                           *start_stop_all_switch;
      GtkWidget                           *start_stop_one_switch[W_LAST];

      GtkTreeView                         *track_view;
      GtkTreeModel                        *track_list;
      GtkAdjustment                       *track_range;

      GtkWidget                           *disp_widgets[W_LAST];
      gchar                               *widget_names[W_LAST];

      GtkWidget                           *mark_view;
      GtkWidget                           *meditor;

      GtkWidget                           *lstack;
      GtkWidget                           *rstack;

      GtkWidget                           *current_view;

  } gui;

  struct
  {
    gdouble                              cur_brightness;
    gdouble                              cur_sensitivity;
    gdouble                              cur_black;

    SonarCommon                          sonar;
    HyScanDataSchemaEnumValue          **fl_signals;
    guint                                fl_n_signals;

    SonarSpecificGui                     gui;

    HyScanGtkForwardLook                *fl;
    HyScanFlCoords                      *fl_coords;
    HyScanForwardLookPlayer             *fl_player;
    // TODO: fl with player widget!
    GtkAdjustment                       *position_range;
    GtkScale                            *position;
    GtkLabel                            *coords_label;
    GtkSwitch                           *mode_target;

    GtkWidget                           *live_view;
  } GFL;

  struct
  {
    guint32                             *color_maps[MAX_COLOR_MAPS];
    guint                                color_map_len[MAX_COLOR_MAPS];
    guint                                cur_color_map;
    gdouble                              cur_brightness;
    gdouble                              cur_black;

    SonarCommon                          sonar;
    HyScanDataSchemaEnumValue          **pf_signals;
    guint                                pf_n_signals;

    gboolean                             have_es;
    HyScanDataSchemaEnumValue          **es_signals;

    SonarSpecificGui                     gui;

    HyScanGtkWaterfall                  *wf;
    HyScanGtkWaterfallGrid              *wf_grid;
    HyScanGtkWaterfallControl           *wf_ctrl;
    HyScanGtkWaterfallMeter             *wf_metr;
    HyScanGtkWaterfallMark              *wf_mark;
    GtkWidget                           *live_view;
  } GPF;

  struct
  {
    guint32                             *color_maps[MAX_COLOR_MAPS];
    guint                                color_map_len[MAX_COLOR_MAPS];
    guint                                cur_color_map;
    gdouble                              cur_brightness;
    gdouble                              cur_black;

    SonarCommon                          sonar;
    HyScanDataSchemaEnumValue          **sb_signals;
    guint                                sb_n_signals;
    HyScanDataSchemaEnumValue          **ps_signals;
    guint                                ps_n_signals;

    SonarSpecificGui                     gui;
    HyScanGtkWaterfall                  *wf;
    HyScanGtkWaterfallGrid              *wf_grid;
    HyScanGtkWaterfallControl           *wf_ctrl;
    HyScanGtkWaterfallMeter             *wf_metr;
    HyScanGtkWaterfallMark              *wf_mark;
    GtkWidget                           *live_view;
  } GSS;

} Global;

Global global = { 0, };

static void
depth_writer (GObject *emitter);

static void
switch_page (GObject     *emitter,
             const gchar *page);

static void
turn_meter (GObject     *emitter,
            gboolean     state,
            gint         selector);
static void
turn_marks (GObject     *emitter,
            gint         selector);
static void
hide_marks (GObject     *emitter,
            gboolean     state,
            gint         selector);

static HyScanDataSchemaEnumValue *
find_signal_by_name (HyScanDataSchemaEnumValue ** where,
                     HyScanDataSchemaEnumValue  * what);

static gboolean
key_press (GtkWidget   *widget,
           GdkEventKey *event,
           Global      *global);

static void nav_del      (GObject *emitter, gpointer udata);
static void nav_pg_up    (GObject *emitter, gpointer udata);
static void nav_pg_down  (GObject *emitter, gpointer udata);
static void nav_up    (GObject *emitter, gpointer udata);
static void nav_down  (GObject *emitter, gpointer udata);
static void nav_left  (GObject *emitter, gpointer udata);
static void nav_right (GObject *emitter, gpointer udata);

static void fl_prev (GObject *emitter, gpointer udata);
static void fl_next (GObject *emitter, gpointer udata);

static void 
run_manager    (GObject     *emitter);

static void
projects_changed (HyScanDBInfo *db_info,
                  Global       *global);

static GtkTreePath *
ame_gtk_tree_model_get_last_path (GtkTreeView *tree);
static GtkTreePath *
ame_gtk_tree_path_prev (GtkTreePath *path);
static GtkTreePath *
ame_gtk_tree_path_next (GtkTreePath *path);

static void
track_scroller (GtkTreeView *tree,
                gboolean     to_top,
                gboolean     to_end);

static void
list_scroll_up (GObject *emitter,
                gpointer udata);
static void
list_scroll_down (GObject *emitter,
                  gpointer udata);

static void
list_scroll_start (GObject *emitter,
                   gpointer udata);
static void
list_scroll_end (GObject *emitter,
                 gpointer udata);


static void
tracks_changed (HyScanDBInfo *db_info,
                Global       *global);

static void
active_mark_changed (HyScanGtkProjectViewer *marks_viewer,
                     Global                 *global);

static inline gboolean
ame_float_equal (gdouble a,
                 gdouble b);

static gboolean
marks_equal (HyScanMarkManagerMarkLoc *a,
             HyScanMarkManagerMarkLoc *b);

static void
mark_processor (gpointer        key,
                gpointer        value,
                GHashTable     *source,
                HyScanMarkSync *sync,
                gboolean        this_is_an_old_list);
static void
mark_sync_func (GHashTable *marks,
                Global     *global);

static void
mark_manager_changed (HyScanMarkManager *mark_manager,
                      Global            *global);

static void
mark_modified (HyScanGtkMarkEditor *med,
               Global              *global);

static gboolean
track_scroll (GtkWidget *widget,
              GdkEvent  *event,
               Global              *global);

static gchar * 
get_active_track (HyScanDBInfo *db_info);

static gboolean 
track_is_active (HyScanDBInfo *db_info,
                 const gchar  *name);

static void
track_changed (GtkTreeView *list,
               Global      *global);

static void
position_changed (GtkAdjustment *range,
                  Global        *global);

static gboolean
brightness_set (Global  *global,
                gdouble  cur_brightness,
                gdouble  cur_black,
                gint     selector);

static gboolean
scale_set (Global   *global,
           gboolean  scale_up,
           gpointer  user_data,
           gint      selector);

static guint32*
hyscan_tile_color_compose_colormap_pf (guint *length);

static gboolean
color_map_set (Global *global,
               guint   cur_color_map,
               gint    selector);

static gboolean
sensitivity_set (Global  *global,
                 gdouble  cur_sensitivity);

static gboolean
signal_set (Global *global,
            guint   cur_signal,
            gint    selector);

static gboolean
tvg_set (Global  *global,
         gdouble *gain0,
         gdouble  step,
         gint     selector);

static gboolean
auto_tvg_set (Global  *global,
              gdouble  level,
              gdouble  sensitivity,
              gint     selector);

static void
distance_printer (GtkLabel *label,
                  gdouble   value);

static gboolean
distance_set (Global  *global,
              gdouble  wanted_distance,
              gint sonar_selector);

static void
void_callback (gpointer data);

static void
brightness_up (GtkWidget *widget,
               gint        selector);

static void
brightness_down (GtkWidget *widget,
                 gint       selector);

static void
black_up (GtkWidget *widget,
          gint       selector);

static void
black_down (GtkWidget *widget,
            gint       selector);

static void
scale_up (GtkWidget *widget,
          gint       selector);

static void
scale_down (GtkWidget *widget,
            gint       selector);

static void
sens_up (GtkWidget *widget,
          gint       selector);

static void
sens_down (GtkWidget *widget,
            gint       selector);

static void
color_map_up (GtkWidget *widget,
              gint       selector);

static void
color_map_down (GtkWidget *widget,
                gint       selector);

static void
mode_changed (GtkWidget *widget,
              gboolean   state,
              gint       selector);

static gboolean
pf_special (GtkWidget  *widget,
            gboolean    state,
            gint        selector);

static gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           gint        selector);

static void
live_view_off (GtkWidget  *widget,
               gboolean    state,
               Global     *global);

static void
distance_up (GtkWidget *widget,
             gint       sonar_selector);

static void
distance_down (GtkWidget *widget,
               gint       sonar_selector);

static void
tvg0_up (GtkWidget *widget,
         gint       selector);

static void
tvg0_down (GtkWidget *widget,
           gint       selector);

static void
tvg_up (GtkWidget *widget,
        gint       selector);

static void
tvg_down (GtkWidget *widget,
          gint       selector);

static void
tvg_level_up (GtkWidget *widget,
              gint       selector);

static void
tvg_level_down (GtkWidget *widget,
                gint       selector);

static void
tvg_sens_up (GtkWidget *widget,
             gint       selector);

static void
tvg_sens_down (GtkWidget *widget,
               gint       selector);

static void
signal_up (GtkWidget *widget,
           gint       selector);

static void
signal_down (GtkWidget *widget,
             gint       selector);

static void
widget_swap (GObject  *emitter,
             gpointer  user_data);

static void
start_stop (GtkWidget *widget,
            gboolean   state);

static gboolean
sensor_label_writer (Global *global);

static void
nav_printer (gchar        **target,
             GMutex        *lock,
             const gchar   *format,
             ...);

static void
sensor_cb (HyScanSensorControl      *src,
           const gchar              *name,
           HyScanSensorProtocolType  protocol,
           HyScanDataType            type,
           HyScanDataWriterData     *data,
           Global                   *global);

static gboolean
start_stop_disabler (GtkWidget *sw,
                     gboolean   state);

static gboolean
start_stop_dry (GtkWidget *widget,
                gboolean   state);


static gint
urpc_cb (uint32_t  session,
         uRpcData *urpc_data,
         void     *proc_data,
         void     *key_data);

static gboolean
ame_key_func (gpointer user_data);

static void
fl_coords_callback (HyScanFlCoords *coords,
                    GtkLabel       *label);

static GtkWidget *
make_overlay (HyScanGtkWaterfall          *wf,
              HyScanGtkWaterfallGrid     **_grid,
              HyScanGtkWaterfallControl  **_ctrl,
              HyScanGtkWaterfallMark     **_mark,
              HyScanGtkWaterfallMeter    **_meter);

static HyScanParam *
connect_to_sonar (const gchar *driver_path,
                  const gchar *driver_name,
                  const gchar *uri);


#endif /* __TRIPLE_H__ */
