#ifndef __TRIPLE_TYPES_H__
#define __TRIPLE_TYPES_H__

#include <hyscan-gtk-area.h>
#include <hyscan-control.h>
#include <hyscan-forward-look-data.h>
#include <hyscan-gtk-waterfall.h>
#include <hyscan-gtk-waterfall-grid.h>
#include <hyscan-gtk-waterfall-control.h>
#include <hyscan-gtk-waterfall-meter.h>
#include <hyscan-gtk-waterfall-mark.h>
#include <hyscan-gtk-waterfall-player.h>
#include <hyscan-gtk-waterfall-magnifier.h>
#include <hyscan-gtk-waterfall-coord.h>
#include <hyscan-gtk-waterfall-shadowm.h>
#include <hyscan-gtk-project-viewer.h>
#include <hyscan-gtk-mark-editor.h>
#include <hyscan-gtk-nav-indicator.h>
#include <hyscan-gtk-model-manager.h>
#include <hyscan-gtk-gliko-view.h>
//#include <hyscan-gtk-gliko-grid.h>
#include <hyscan-gtk-fnn-gliko-wrapper.h>
#include <hyscan-tile-color.h>
#include <hyscan-nmea-parser.h>
#include <hyscan-mloc.h>
#include <hyscan-projector.h>
#include <hyscan-cached.h>
#include <urpc-server.h>
#include <math.h>
#include <string.h>
#include <locale.h>

#include <sonar-configure.h>
#include <hyscan-gtk-forward-look.h>
#include <hyscan-fl-coords.h>
#include <hyscan-mark-sync.h>
#include <hyscan-api.h>
#include <hyscan-control-model.h>

#define hyscan_return_val_if_fail(expr,val) do {if (!(expr)) {g_warning("Failed at line %i", __LINE__); return (val);}} while (FALSE)
#define hyscan_exit_if(expr,msg) do {if (!(expr)) break; g_message ((msg)); goto exit;} while (FALSE)
#define hyscan_exit_if_w_param(expr,msg,param) do {if (!(expr)) break; g_message ((msg),(param)); goto exit;} while (FALSE)
#define WRONG_SELECTOR { g_message ("Wrong sonar selector @%i", __LINE__); }

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

#define DRY_TRACK_SUFFIX                "-dry"

enum
{
  FNN_DATE_SORT_COLUMN,
  FNN_TRACK_COLUMN,
  FNN_DATE_COLUMN,
  FNN_N_COLUMNS
};

enum
{
  X_SIDESCAN  = 100,
  X_PROFILER  = 200,
  X_FORWARDL  = 300,
  X_ECHOSOUND = 400,
  X_SIDE_LOW  = 500,
  X_SIDE_HIGH = 600,
  X_ECHO_HIGH = 700,
  X_ECHO_LOW  = 800,
  X_LOOKARND  = 900,
  X_LOOK_HI   = 1000,
  X_LOOK_LOW  = 1100,
};

typedef struct _FnnPanel FnnPanel;

typedef gboolean (*ui_pack_fn) (FnnPanel *, gint);
typedef void (*ui_vadjust_fn) (HyScanTrackInfo *);

/* ??????????????????: ?????????????? + ???????????????????? */
typedef struct
{
  gchar                  *track;      /* ???????????????? ??????????. */
  HyScanmLoc             *mloc;       /* ???????????? ?????? ?????????????????????? ????????????????????????????. */

  HyScanFactoryAmplitude *factory;    /* C???????????????? ?????????? ?????????????? ????????????. */
  GHashTable             *projectors; /* ?????????????????? (?????? ?????????????? ????????????). */
} LocStore;

/* ?????????????????? ?????????? + ????????????????????. */
typedef struct
{
  HyScanMarkWaterfall *mark;

  gdouble              lat;
  gdouble              lon;
  gboolean             ok;
} MarkAndLocation;

typedef struct
{
  guint32 * colors;
  guint     len;
  gchar   * name;
  guint32   bg;
} FnnColormap;

typedef enum
{
  FNN_PANEL_WATERFALL,
  FNN_PANEL_PROFILER,
  FNN_PANEL_ECHO,
  FNN_PANEL_FORWARDLOOK,
  FNN_PANEL_LOOKAROUND,
} FnnPanelType;

typedef struct
{
  gboolean               disabled;
  gdouble                distance;
  gint                   signal;
  gdouble                gain0;       // lin
  gdouble                gain_step;   // lin
  gdouble                level;       // auto
  gdouble                sensitivity; // auto
  gdouble                log_gain0;   // log
  gdouble                log_beta;    // log
  gdouble                log_alpha;   // log
  gdouble                const_gain0; // const
  HyScanSonarTVGModeType mode;        // current mode (not bitmask)
} SonarCurrent;

typedef struct
{
  GtkLabel *distance_value;
  GtkLabel *tvg0_value; // lin
  GtkLabel *tvg_value; // lin

  GtkLabel *tvg_level_value; // auto
  GtkLabel *tvg_sens_value; // auto
  
  GtkLabel *logtvg_gain0_value; // log
  GtkLabel *logtvg_beta_value; // log
  GtkLabel *logtvg_alpha_value; // log
  
  GtkLabel *consttvg_value; // const 

  GtkLabel *signal_value;
} SonarGUI;

typedef struct
{
  gdouble black;
  gdouble gamma;
  gdouble white;
  gdouble sensitivity;
  gint    colormap;
          // scale;
} VisualCurrent;

/* !!!
 * ?????? ?????? ?????????????????? ???????? ???? ??????????????, ?? ???? ???????? ???????????? ??????????.
 * !!! */
typedef struct
{
  GtkWidget *main; /* ???????????? ???????? ???? 1 ??????????, ?????????? ?? ?????????????????? ????-??????
                    * ???????????? ?????? ???????????? ????????. ??????-???? ?? ?????????????? ??????????. */

  GtkLabel  *black_value;
  GtkLabel  *gamma_value;
  GtkLabel  *white_value;
  GtkLabel  *scale_value;
  GtkLabel  *colormap_value;
  GtkLabel  *sensitivity_value;
  GtkWidget *live_view;
} VisualCommon;

typedef struct
{
  VisualCommon                    common; /* ??????????, ?????????? ???????? ???? 1 ??????????. */

  GArray                        * colormaps; /* struct FnnColormap */

  HyScanGtkWaterfall            * wf;
  HyScanGtkWaterfallGrid        * wf_grid;
  HyScanGtkWaterfallControl     * wf_ctrl;
  HyScanGtkWaterfallMeter       * wf_metr;
  HyScanGtkWaterfallMark        * wf_mark;
  HyScanGtkWaterfallPlayer      * wf_play;
  HyScanGtkWaterfallMagnifier   * wf_magn;
  HyScanGtkWaterfallShadowm     * wf_shad;
  HyScanGtkWaterfallCoord       * wf_coor;
} VisualWF;

typedef struct
{
  VisualCommon                    common; /* ??????????, ?????????? ???????? ???? 1 ??????????. */

  // TODO: fl with player widget!
  HyScanGtkForwardLook          * fl;
  HyScanFlCoords                * coords;
  HyScanForwardLookPlayer       * player;
  GtkWidget                     * play_control;

  GtkAdjustment                 * position_range;
  GtkScale                      * position;
  GtkLabel                      * coords_label;
  GtkSwitch                     * mode_target;
} VisualFL;

typedef struct
{
  VisualCommon                    common; /* ??????????, ?????????? ???????? ???? 1 ??????????. */

  GArray                        * colormaps; /* struct FnnColormap */

  HyScanGtkGliko                * gliko;
  HyScanGtkGlikoView            * glikoview;
  //HyScanGtkGlikoGrid            * grid;
  HyScanDataPlayer              * player;
  HyScanGtkFnnGlikoWrapper      * wrapper;
  //GtkWidget                     * play_control;

  GtkWidget                     *la_a_degrees;
  GtkWidget                     *la_a_meters;
  GtkWidget                     *la_b_degrees;
  GtkWidget                     *la_b_meters;
  GtkWidget                     *la_ab_degrees;
  GtkWidget                     *la_ab_meters;
  GtkWidget                     *la_ba_degrees;

  GtkWidget *la_player_scale;
  GtkWidget *la_pause_toggle;
  GtkWidget *la_x10_toggle;

  gdouble speed;
  gint64 player_min;
  int player_position_changed;

} VisualLA;

struct _FnnPanel
{
  gchar            *short_name;
  gchar            *name;
  gchar            *name_local;
  FnnPanelType      type;    /* ?????? ????????????: ????, ????, ???? */
  gint              panelx;  /* id ????????????. */
  HyScanSourceType *sources; /* ?????????????????? ?????? ???????????? */

  SonarCurrent      current;   /* ?????????????? ???????????????? ???????????????????? ????. */
  SonarGUI          gui;     /* ??????????????????. */

  VisualCurrent     vis_current; /* ?????????????????? ??????????????????????, wf_visual/fl_visual, * ??????????????????????! */
  VisualCommon     *vis_gui;    /* ?????????????????? ???????????????????? ?????????????????????? */

  /* union { ????????????????, ??????????, ?????????????? ???????????
    VisualCommon common;
    VisualWF     wf;
    VisualFL     fl;
  } vis_gui; */

};

typedef struct _Global Global;
struct _Global
{
  gint32                  canary;
  HyScanDB               *db;
  HyScanDBInfo           *db_info;

  gchar                  *project_name;
  gchar                  *track_name;

  gboolean                dry;

  HyScanCache            *cache;

  gboolean                full_screen;
  gdouble                 sound_velocity;
  gdouble                 ship_speed;
  HyScanUnits            *units;

  gint                    view_selector;

  HyScanControl          *control;
  HyScanControlModel     *sonar_model;
  gboolean                on_air;

  GKeyFile               *settings;

  GHashTable             *panels; /* {panelx : FnnPanel} */
  GHashTable             *sonar_infos; /* {HyScanSourceType : HyScanSonarInfoSource} */
  GHashTable             *sensor_infos; /* {gchar* : HyScanSensorInfoSensor} */
  GHashTable             *actuator_infos; /* {gchar* : HyScanActuatorInfoActuator} */

  /* ???????????????? ??????????????. */
  HyScanGtkModelManager  *model_manager;

  struct
    {
      HyScanMarkSync     *sync;  /* ?????????????????????????? */

      GHashTable         *loc_storage;  /* ?????????????????? ???????????????????? ?? ???????????????????? */

      GHashTable         *current;  /* ???????????? ????????????*/
      GHashTable         *previous; /* ?????????? ???????????? */

      guint               request_update_tag;
    } marks;

  struct
    {
      GtkWidget          *window; // ????????
      GtkWidget          *grid;

      struct
        {
          GtkWidget      *view;   // ???????????? ??????????????
          GtkTreeView    *tree;   // tree view
          GtkTreeModel   *list;   // list store
          GtkAdjustment  *scroll; // ??????????????????
        } track;

      GtkWidget          *nav;

      GtkWidget          *mark_view;
      GtkWidget          *meditor;

    } gui;

  struct
  {
    gboolean (*levels_set)     (Global  *global,
                                gdouble  black,
                                gdouble  gamma,
                                gdouble  white,
                                gint     selector);

    void     (*project_changed) (Global      *global,
                                 const gchar *project_name);
    void     (*track_changed)   (Global      *global,
                                 const gchar *track_name);
  } override;

  struct 
  {
    ui_pack_fn    pack;
    ui_vadjust_fn adjust_visibility;
  } ui;

  gboolean request_restart;
}; // Global


HYSCAN_API void
source_informer (const gchar      *text,
                 HyScanSourceType  source);

HYSCAN_API gboolean
fnn_ensure_panel (gint panelx,
                  Global *global);

HYSCAN_API void
fnn_panel_destroy (gpointer data);

HYSCAN_API FnnPanel *
get_panel (Global *global,
           gint    panelx);

HYSCAN_API FnnPanel *
get_panel_quiet (Global *global,
                 gint    panelx);

HYSCAN_API gint
get_panel_id_by_name (Global      *global,
                      const gchar *name);

void
button_set_active (GObject *object,
                   gboolean active);

HYSCAN_API void
turn_meter (GObject     *emitter,
            gboolean     state,
            gint         selector);
HYSCAN_API void
turn_marks (GObject     *emitter,
            gint         selector);
HYSCAN_API void
hide_marks (GObject     *emitter,
            gboolean     state,
            gint         selector);

HYSCAN_API gboolean
key_press (GtkWidget   *widget,
           GdkEventKey *event,
           Global      *global);

gboolean idle_key (GdkEvent *event);
void     nav_common (GtkWidget *target,
                     guint      keyval,
                     guint      state);
HYSCAN_API void nav_del      (GObject *emitter, gpointer udata);
HYSCAN_API void nav_pg_up    (GObject *emitter, gpointer udata);
HYSCAN_API void nav_pg_down  (GObject *emitter, gpointer udata);
HYSCAN_API void nav_up    (GObject *emitter, gpointer udata);
HYSCAN_API void nav_down  (GObject *emitter, gpointer udata);
HYSCAN_API void nav_left  (GObject *emitter, gpointer udata);
HYSCAN_API void nav_right (GObject *emitter, gpointer udata);

HYSCAN_API void fl_prev (GObject *emitter, gint panelx);
HYSCAN_API void fl_next (GObject *emitter, gint panelx);

HYSCAN_API gint
run_manager (GObject     *emitter);

HYSCAN_API void
set_window_title (Global *global);

HYSCAN_API void
run_param (GObject     *emitter,
           const gchar *root);

HYSCAN_API void
run_show_sonar_info (GObject     *emitter,
                     const gchar *root);

HYSCAN_API void
projects_changed (HyScanDBInfo *db_info,
                  Global       *global);

HYSCAN_API void
run_export_data (GObject     *emitter,
                 gpointer     from_ui_params);

GtkTreePath *
ame_gtk_tree_model_get_last_path (GtkTreeView *tree);
GtkTreePath *
ame_gtk_tree_path_prev (GtkTreePath *path);
GtkTreePath *
ame_gtk_tree_path_next (GtkTreePath *path);

HYSCAN_API void
track_scroller (GtkTreeView *tree,
                gboolean     to_top,
                gboolean     to_end);

HYSCAN_API void
model_manager_tracks_changed (HyScanGtkModelManager *model_manager,
                              Global                *global);

HYSCAN_API void
tracks_changed (HyScanDBInfo *db_info,
                Global       *global);

HYSCAN_API void
active_mark_changed (HyScanGtkProjectViewer *marks_viewer,
                     Global                 *global);

gboolean
fnn_float_equal (gdouble a,
                 gdouble b);

gboolean
marks_equal (MarkAndLocation *a,
             MarkAndLocation *b);

void
mark_processor (gpointer        key,
                gpointer        value,
                GHashTable     *source,
                HyScanMarkSync *sync,
                gboolean        this_is_an_old_list);
void
mark_sync_func (GHashTable *marks,
                Global     *global);


LocStore *
loc_store_new (HyScanDB    *db,
               HyScanCache *cache,
               const gchar *project,
               const gchar *track);

HyScanProjector *
get_projector (GHashTable       *locstores,
               gchar            *track,
               HyScanSourceType  source,
               HyScanmLoc       **mloc,
               HyScanAmplitude  **_amp);

HYSCAN_API void
loc_store_free (gpointer data);

MarkAndLocation *
mark_and_location_new (HyScanMarkWaterfall *mark,
                       gboolean             ok,
                       gdouble              lat,
                       gdouble              lon);

MarkAndLocation *
mark_and_location_copy (gpointer data);

void
mark_and_location_free (gpointer data);

MarkAndLocation *
get_mark_coords (GHashTable           * locstores,
                 HyScanMarkWaterfall  * mark,
                 Global               * global);

HYSCAN_API void
model_manager_acoustic_mark_model_changed (HyScanGtkModelManager *model_manager,
                                           Global                *global);

HYSCAN_API void
mark_model_changed (HyScanObjectModel *mark_model,
                    Global            *global);

HYSCAN_API void
mark_modified (HyScanGtkMarkEditor *med,
               Global              *global);

HYSCAN_API gboolean
track_scroll (GtkWidget *widget,
              GdkEvent  *event,
               Global              *global);

gchar *
get_active_track (HyScanDBInfo *db_info);

gboolean
track_is_active (HyScanDBInfo *db_info,
                 const gchar  *name);

HYSCAN_API void
track_changed (GtkTreeView *list,
               Global      *global);

HYSCAN_API gboolean
levels_set (Global  *global,
            gdouble  new_black,
            gdouble  new_gamma,
            gdouble  new_white,
            gint     selector);

HYSCAN_API void
zoom_changed (HyScanGtkWaterfall *wfall,
              gint                index,
              gdouble             gost,
              gint                panelx);

HYSCAN_API gboolean
scale_set (Global   *global,
           gboolean  scale_up,
           gint      selector);

HYSCAN_API
guint32 *
hyscan_tile_color_compose_colormap_pf (guint *length);

HYSCAN_API gboolean
color_map_set (Global *global,
               guint   cur_color_map,
               gint    selector);

HYSCAN_API void
sensitivity_label (FnnPanel *panel,
                   gdouble   sens);

HYSCAN_API const HyScanDataSchemaEnumValue *
signal_finder (Global           *global,
               FnnPanel         *panel,
               HyScanSourceType  source,
               gint              n);

HYSCAN_API void
signal_label (FnnPanel    *panel,
              const gchar *name);

HYSCAN_API void
tvg_label (FnnPanel *panel,
           gdouble   gain0,
           gdouble   step);
HYSCAN_API void
auto_tvg_label (FnnPanel *panel,
                gdouble   level,
                gdouble   sensitivity);

HYSCAN_API void
log_tvg_label (FnnPanel *panel,
               gdouble   gain0,
               gdouble   beta,
               gdouble   alpha);

HYSCAN_API void
const_tvg_label (FnnPanel *panel,
                 gdouble   gain0);


HYSCAN_API void
distance_label (FnnPanel *panel,
                gdouble   distance);



HYSCAN_API gboolean
sensitivity_set (Global  *global,
                 gdouble  cur_sensitivity,
                 guint    panelx);

HYSCAN_API gboolean
gen_disable (Global   *global,
             gint      panelx);

HYSCAN_API gboolean
signal_set (Global   *global,
            gint      cur_signal,
            gint      selector);

#define TVG_FUNC_DEF(fname) void fname (GtkWidget *widget, gint panelx)

HYSCAN_API TVG_FUNC_DEF(consttvg_up);
HYSCAN_API TVG_FUNC_DEF(consttvg_down);

HYSCAN_API TVG_FUNC_DEF(logtvg_gain0_up);
HYSCAN_API TVG_FUNC_DEF(logtvg_gain0_down);
HYSCAN_API TVG_FUNC_DEF(logtvg_beta_up);
HYSCAN_API TVG_FUNC_DEF(logtvg_beta_down);
HYSCAN_API TVG_FUNC_DEF(logtvg_alpha_up);
HYSCAN_API TVG_FUNC_DEF(logtvg_alpha_down);

HYSCAN_API TVG_FUNC_DEF(tvg0_up);
HYSCAN_API TVG_FUNC_DEF(tvg0_down);
HYSCAN_API TVG_FUNC_DEF(tvg_up);
HYSCAN_API TVG_FUNC_DEF(tvg_down);

HYSCAN_API TVG_FUNC_DEF(tvg_level_up);
HYSCAN_API TVG_FUNC_DEF(tvg_level_down);
HYSCAN_API TVG_FUNC_DEF(tvg_sens_up);
HYSCAN_API TVG_FUNC_DEF(tvg_sens_down);

HYSCAN_API gboolean
lin_tvg_set (Global  *global,
             gdouble *gain0,
             gdouble  step,
             gint     selector);


HYSCAN_API gboolean
log_tvg_set (Global  *global,
             gdouble *gain0,
             gdouble *beta,
             gdouble *alpha,
             gint     selector);

HYSCAN_API gboolean
const_tvg_set (Global  *global,
               gdouble *gain0,
               gint     selector);

HYSCAN_API gboolean
auto_tvg_set (Global  *global,
              gdouble  level,
              gdouble  sensitivity,
              gint     selector);

HYSCAN_API gboolean
tvg_set_preferred_mode (Global                 *global,
                        HyScanSonarTVGModeType  mode,
                        gint                    panelx);

gboolean
distance_set (Global  *global,
              gdouble *distance,
              gint     sonar_selector);

HYSCAN_API void
void_callback (gpointer data);

HYSCAN_API void
white_up (GtkWidget *widget,
          gint        selector);

HYSCAN_API void
white_down (GtkWidget *widget,
            gint       selector);

HYSCAN_API void
gamma_up (GtkWidget *widget,
          gint        selector);

HYSCAN_API void
gamma_down (GtkWidget *widget,
            gint       selector);

HYSCAN_API void
black_up (GtkWidget *widget,
          gint       selector);

HYSCAN_API void
black_down (GtkWidget *widget,
            gint       selector);

HYSCAN_API void
scale_up (GtkWidget *widget,
          gint       selector);

HYSCAN_API void
scale_down (GtkWidget *widget,
            gint       selector);

HYSCAN_API void
sensitivity_up (GtkWidget *widget,
                gint       selector);

HYSCAN_API void
sensitivity_down (GtkWidget *widget,
                  gint       selector);

HYSCAN_API void
color_map_up (GtkWidget *widget,
              gint       selector);

HYSCAN_API void
color_map_down (GtkWidget *widget,
                gint       selector);

HYSCAN_API void
color_map_cyclic (GtkWidget *widget,
                  gint       panelx);

HYSCAN_API void 
enable_switched (GtkWidget *widget,
                 gint       panelx);

HYSCAN_API gboolean
disable_changed (GtkWidget *widget,
                 gboolean   disabled,
                 gint       panelx);

HYSCAN_API gboolean
mode_changed (GtkWidget *widget,
              gboolean   state,
              gint       selector);

HYSCAN_API gboolean
pf_special (GtkWidget  *widget,
            gboolean    state,
            gint        selector);

HYSCAN_API gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           gint        selector);

HYSCAN_API void
live_view_off (GtkWidget  *widget,
               gboolean    state,
               Global     *global);

HYSCAN_API gboolean
automove_switched (GtkWidget  *widget,
                   gboolean    state);

HYSCAN_API void
automove_state_changed (GtkWidget  *widget,
               gboolean    state,
               Global     *global);

HYSCAN_API void
distance_up (GtkWidget *widget,
             gint       sonar_selector);

HYSCAN_API void
distance_down (GtkWidget *widget,
               gint       sonar_selector);

HYSCAN_API void
lookaround_image_rotation (GtkAdjustment *adj,
                           gint           panelx);

HYSCAN_API void
lookaround_axis_rotation (GtkAdjustment *adj,
                          gint           panelx);

HYSCAN_API void
lookaround_bottom (GtkAdjustment *adj,
                   gint           panelx);

HYSCAN_API void
signal_up (GtkWidget *widget,
           gint       selector);

HYSCAN_API void
signal_down (GtkWidget *widget,
             gint       selector);

HYSCAN_API gboolean
panel_turn_on_off (Global   *global,
                   gboolean on_air,
                   FnnPanel *panel);

HYSCAN_API gboolean
start_stop (Global                *global,
            const HyScanTrackPlan *track_plan,
            gboolean               state);

HYSCAN_API gboolean
before_start (HyScanControlModel *model,
              Global           *global);

HYSCAN_API void
sonar_state_changed (Global *global);

HYSCAN_API gboolean
set_dry (Global    *global,
         gboolean   state);

HYSCAN_API void
fl_coords_callback (HyScanFlCoords *coords,
                    GtkLabel       *label);

HYSCAN_API  GtkWidget *
make_overlay (HyScanGtkWaterfall           *wf,
              HyScanGtkWaterfallGrid      **_grid,
              HyScanGtkWaterfallControl   **_ctrl,
              HyScanGtkWaterfallMark      **_mark,
              HyScanGtkWaterfallMeter     **_meter,
              HyScanGtkWaterfallPlayer    **_player,
              HyScanGtkWaterfallMagnifier **_magnifier,
              HyScanGtkWaterfallShadowm   **_shad,
              HyScanGtkWaterfallCoord     **_coord,
              HyScanObjectModel            *mark_model);

HYSCAN_API void
fnn_colormap_free (gpointer data);

HYSCAN_API GArray *
fnn_make_color_maps (gboolean profiler,
                     gboolean simple_only);

HYSCAN_API void
update_panels (Global          *global,
               HyScanTrackInfo *track_info);

HYSCAN_API gboolean
panel_sources_are_in_sonar (Global   *global,
                            FnnPanel *panel);

HYSCAN_API void
fnn_init (Global *ext_global);

HYSCAN_API void
fnn_deinit (Global *ext_global);



#endif /* __TRIPLE_TYPES_H__ */
