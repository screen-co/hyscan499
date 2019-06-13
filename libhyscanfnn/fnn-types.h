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
#include <hyscan-mark-model.h>
#include <hyscan-gtk-project-viewer.h>
#include <hyscan-gtk-mark-editor.h>
#include <hyscan-gtk-nav-indicator.h>
#include <hyscan-tile-color.h>
#include <hyscan-nmea-parser.h>
#include <hyscan-mloc.h>
#include <hyscan-projector.h>
#include <hyscan-db-info.h>
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
  X_SIDESCAN = 155642,
  X_PROFILER = 251539,
  X_FORWARDL = 356753,
  X_ECHOSOUND = 429878,
  X_SIDE_LOW = 568351,
};

typedef struct FnnPanel _FnnPanel;

typedef gboolean (*ui_pack_fn) (_FnnPanel *, gint);
typedef void (*ui_adjust_visibility_fn) (HyScanTrackInfo *);

/* структура: локейшн + прожекторы */
typedef struct
{
  gchar               *track;      /* Название галса. */
  HyScanmLoc          *mloc;   /* Объект для определения местоположения. */

  HyScanAmplitudeFactory *factory; /* CСоздание новых каналов данных. */
  GHashTable          *projectors; /* Проекторы (для каждого канала). */
} LocStore;

/* структура метка + координаты. */
typedef struct
{
  HyScanMark *mark;

  gdouble     lat;
  gdouble     lon;
  gboolean    ok;
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
} FnnPanelType;

typedef struct
{
  gdouble   distance;
  gint      signal;
  gdouble   gain0;
  gdouble   gain_step;
  gdouble   level;
  gdouble   sensitivity;
} SonarCurrent;

typedef struct
{
  GtkLabel *distance_value;
  GtkLabel *tvg_value;
  GtkLabel *tvg0_value;
  GtkLabel *tvg_level_value;
  GtkLabel *tvg_sens_value;
  GtkLabel *signal_value;
} SonarGUI;

typedef struct
{
  gdouble brightness;
  gdouble black;
  gdouble sensitivity;
  gint    colormap;
          // scale;
} VisualCurrent;

/* !!!
 * Вот эти структуры ниже не трогать, а то руки оторву нахуй.
 * !!! */
typedef struct
{
  GtkWidget *main; /* Должно быть на 1 месте, чтобы у остальных эл-тов
                    * оффсет был больше нуля. Исп-ся в амешной проге. */

  GtkLabel  *brightness_value;
  GtkLabel  *black_value;
  GtkLabel  *scale_value;
  GtkLabel  *colormap_value;
  GtkLabel  *sensitivity_value;
  GtkWidget *live_view;
} VisualCommon;

typedef struct
{
  VisualCommon                    common; /* Важно, чтобы было на 1 месте. */

  GArray                        * colormaps; /* struct FnnColormap */

  HyScanGtkWaterfall            * wf;
  HyScanGtkWaterfallGrid        * wf_grid;
  HyScanGtkWaterfallControl     * wf_ctrl;
  HyScanGtkWaterfallMeter       * wf_metr;
  HyScanGtkWaterfallMark        * wf_mark;
} VisualWF;

typedef struct
{
  VisualCommon                    common; /* Важно, чтобы было на 1 месте. */

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
  gchar            *short_name;
  gchar            *name;
  gchar            *name_local;
  FnnPanelType      type;    /* тип панели: вф, фл, пф */
  HyScanSourceType *sources; /* Источники для панели */

  SonarCurrent      current;   /* Текущие значения параметров ГЛ. */
  SonarGUI          gui;     /* Виджетики. */

  VisualCurrent     vis_current; /* параметры отображения, wf_visual/fl_visual, * обязательно! */
  VisualCommon     *vis_gui;    /* виджетосы параметров отображения */

  /* union { подумать, может, сделать юнион?
    VisualCommon common;
    VisualWF     wf;
    VisualFL     fl;
  } vis_gui; */

} FnnPanel;

typedef struct _Global Global;
struct _Global
{
  HyScanDB               *db;
  HyScanDBInfo           *db_info;

  gchar                  *project_name;
  gchar                  *track_name;

  gboolean                dry;

  HyScanCache            *cache;

  gboolean                full_screen;
  gdouble                 sound_velocity;
  gdouble                 ship_speed;

  gint                    view_selector;

  HyScanControl          *control;
  HyScanSonar            *control_s;
  gboolean                on_air;
  gint64                  last_click_time;
  gboolean                synced;

  GKeyFile               *settings;

  GHashTable             *panels; /* {panelx : FnnPanel} */
  GHashTable             *infos; /* {HyScanSourceType : HyScanSonarInfoSource} */

  struct
    {
      HyScanMarkModel    *model; /* модель */
      HyScanMarkSync     *sync;  /* синхронизация */

      GHashTable         *loc_storage;  /* хранилище локейшенов и проекторов */

      GHashTable         *current;  /* старый список*/
      GHashTable         *previous; /* новый список */

      guint               request_update_tag;
    } marks;

  struct
    {
      GtkWidget          *window; // окно
      GtkWidget          *grid;

      struct
        {
          GtkWidget      *view;   // виджет целиком
          GtkTreeView    *tree;   // tree view
          GtkTreeModel   *list;   // list store
          GtkAdjustment  *scroll; // прокрутка
        } track;

      GtkWidget          *nav;

      GtkWidget          *mark_view;
      GtkWidget          *meditor;
    } gui;

  struct
  {
    gboolean (*brightness_set) (Global  *global,
                                gdouble  brightness,
                                gdouble  black,
                                gint     selector);
  } override;

  struct 
  {
    ui_pack_fn              pack;
    ui_adjust_visibility_fn adjust_visibility;
  } ui;


}; // Global


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
run_param (GObject     *emitter,
           const gchar *root);

HYSCAN_API void
run_show_sonar_info (GObject     *emitter,
                     const gchar *root);

HYSCAN_API void
projects_changed (HyScanDBInfo *db_info,
                  Global       *global);

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
mark_and_location_new (HyScanMark *mark,
                       gboolean    ok,
                       gdouble     lat,
                       gdouble     lon);

MarkAndLocation *
mark_and_location_copy (gpointer data);

void
mark_and_location_free (gpointer data);

MarkAndLocation *
get_mark_coords (GHashTable  * locstores,
                 HyScanMark  * mark,
                 Global      * global);

HYSCAN_API void
mark_model_changed (HyScanMarkModel   *mark_model,
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

HYSCAN_API  void
track_changed (GtkTreeView *list,
               Global      *global);

HYSCAN_API void
position_changed (GtkAdjustment *range,
                  Global        *global);

HYSCAN_API gboolean
brightness_set (Global  *global,
                gdouble  cur_brightness,
                gdouble  cur_black,
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

HYSCAN_API gboolean
tvg_set (Global  *global,
         gdouble *gain0,
         gdouble  step,
         gint     selector);

HYSCAN_API gboolean
auto_tvg_set (Global  *global,
              gdouble  level,
              gdouble  sensitivity,
              gint     selector);

void
distance_printer (GtkLabel *label,
                  gdouble   value);

gboolean
distance_set (Global  *global,
              gdouble *distance,
              gint     sonar_selector);

HYSCAN_API void
void_callback (gpointer data);

HYSCAN_API void
brightness_up (GtkWidget *widget,
               gint        selector);

HYSCAN_API void
brightness_down (GtkWidget *widget,
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
tvg0_up (GtkWidget *widget,
         gint       selector);

HYSCAN_API void
tvg0_down (GtkWidget *widget,
           gint       selector);

HYSCAN_API void
tvg_up (GtkWidget *widget,
        gint       selector);

HYSCAN_API void
tvg_down (GtkWidget *widget,
          gint       selector);

HYSCAN_API void
tvg_level_up (GtkWidget *widget,
              gint       selector);

HYSCAN_API void
tvg_level_down (GtkWidget *widget,
                gint       selector);

HYSCAN_API void
tvg_sens_up (GtkWidget *widget,
             gint       selector);

HYSCAN_API void
tvg_sens_down (GtkWidget *widget,
               gint       selector);

HYSCAN_API void
signal_up (GtkWidget *widget,
           gint       selector);

HYSCAN_API void
signal_down (GtkWidget *widget,
             gint       selector);


HYSCAN_API gboolean
start_stop (Global    *global,
            gboolean   state);

HYSCAN_API gboolean
set_dry (Global    *global,
         gboolean   state);

HYSCAN_API void
sensor_cb (HyScanSensor     *sensor,
           const gchar      *name,
           HyScanSourceType  source,
           gint64            recv_time,
           HyScanBuffer     *buffer,
           Global           *global);

HYSCAN_API void
fl_coords_callback (HyScanFlCoords *coords,
                    GtkLabel       *label);

HYSCAN_API  GtkWidget *
make_overlay (HyScanGtkWaterfall          *wf,
              HyScanGtkWaterfallGrid     **_grid,
              HyScanGtkWaterfallControl  **_ctrl,
              HyScanGtkWaterfallMark     **_mark,
              HyScanGtkWaterfallMeter    **_meter,
              HyScanMarkModel             *mark_model);

HYSCAN_API void
fnn_colormap_free (gpointer data);

HYSCAN_API GArray *
fnn_make_color_maps (gboolean profiler);

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
