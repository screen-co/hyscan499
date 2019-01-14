#ifndef __AME_UI_H__
#define __AME_UI_H__

#include "triple-types.h"
#include <hyscan-ame-fixed.h>
#include <hyscan-ame-button.h>
#include <hyscan-gtk-ame-box.h>

#define AME_N_BUTTONS 5

typedef struct
{
  GtkWidget  *acoustic;
  GtkWidget  *sub_center_box;

  GtkWidget  *left_box;
  GtkWidget  *left_revealer;
  GtkWidget  *bott_revealer;

  GtkWidget  *lstack;
  GtkWidget  *rstack;

  GtkWidget  *start_stop_dry_switch;
  GtkWidget  *start_stop_all_switch;
  GtkWidget  *start_stop_one_switch[W_LAST];

  GtkWidget  *current_view;
} AmeUI;

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

/*
 * Ща сложно будет. В странице есть destination_selector.
 * Он определяет, куда кладется виджет. 
 * Поэтому нельзя в трипл-тайпсах ничего трогать.
 */
typedef enum
{
  DEST_UNSET,
  DEST_SONAR, // AmePanel.gui + offt
  DEST_SPEC,  // AmePanel.vis_gui + offt
  DEST_AME_UI // AmeUI + offt
} AmePageDestination;

typedef struct
{
  gchar              *path;
  AmePageDestination  destination_selector; //
  AmePageItem         items[2 * AME_N_BUTTONS + 1];
} AmePage;


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

#define hui_ss GINT_TO_POINTER (X_SIDESCAN)
#define hui_fl GINT_TO_POINTER (X_FORWARDL)
#define hui_pf GINT_TO_POINTER (X_PROFILER)

#define trkls GINT_TO_POINTER (L_TRACKS)
#define mrkls GINT_TO_POINTER (L_MARKS)
#define MORE ("list-add-symbolic")
#define LESS ("list-remove-symbolic")
#define DOT ("media-record-symbolic")
#define UP ("pan-up-symbolic")
#define DOWN ("pan-down-symbolic")
#define LEFT ("pan-start-symbolic")
#define RIGHT ("pan-end-symbolic")
#define MENU ("user-home-symbolic")
#define BACK ("edit-undo-symbolic")
#define VMOR ("view-list-symbolic")
#define REPT ("media-playlist-repeat-symbolic")
#define MNGR ("accessories-dictionary-symbolic")
#define P_(x) (gpointer)(x)
#define CBK(x) .callback = (gpointer)(x)
#define UD(x) .user_data = (gpointer)(x)
#define VALUE_OFFSET(x, y) .value_offset = offsetof(x,y)
#define VALUE_DEFAULT(x) .value_default = ((gchar*)(x))
#define BUTTON_OFFSET(x, y) .button_offset = offsetof(x,y)

extern Global global;

static AmePage common_pages[] =
{
  {
    .path = "Меню",
    .destination_selector = DEST_AME_UI,
    .items =
      {
        {L, 3, VMOR, "Общее",           TOGGLE_NONE, CBK(switch_page), UD("Общее")},
        {L, 4, VMOR, "Галс",            TOGGLE_NONE, CBK(switch_page), UD("ГАЛС")},
        {R, 0, VMOR, "ГБО Изображение", TOGGLE_NONE, CBK(switch_page), UD("И_ГБО")},
        {R, 1, VMOR, "ГК Изображение",  TOGGLE_NONE, CBK(switch_page), UD("И_ГК")},
        {R, 2, VMOR, "ПФ Изображение",  TOGGLE_NONE, CBK(switch_page), UD("И_ПФ")},
        {R, 4, VMOR, "Вид",             TOGGLE_NONE, CBK(widget_swap), GINT_TO_POINTER (ROTATE),
            VALUE_OFFSET(AmeUI, current_view), VALUE_DEFAULT("--")},
        {END}
      }
  },
  {
    .path = "И_ГБО",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, CBK(switch_page),    UD("И_ГБОд")},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, CBK(scale_up),       hui_ss,
            VALUE_OFFSET(VisualCommon, scale_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(scale_down),     hui_ss},
        {L, 4, DOT,  "Автопрок",    TOGGLE_ON,   CBK(live_view),      hui_ss,
           BUTTON_OFFSET(VisualCommon, live_view)},

        {R, 0, VMOR,  "Метки",      TOGGLE_NONE, CBK(switch_page),    UD("И_ГБОм")},
        {R, 1, UP,    "Вверх",      TOGGLE_NONE, CBK(nav_pg_up),      hui_ss},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, CBK(nav_pg_down),    hui_ss},
        {R, 3, RIGHT, "Вправо",     TOGGLE_NONE, CBK(nav_right),      hui_ss},
        {R, 4, LEFT,  "Влево",      TOGGLE_NONE, CBK(nav_left),       hui_ss},
        {END}
      }
  },
  {
    .path = "И_ГБОд",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU,"Меню",         TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK,"Назад",        TOGGLE_NONE, CBK(switch_page),    UD("И_ГБО")},
        {L, 4, REPT,"Цвет",         TOGGLE_NONE, CBK(color_map_up),   hui_ss,
            VALUE_OFFSET(VisualCommon, colormap_value)},

        {R, 2, MORE, "Контр",       TOGGLE_NONE, CBK(brightness_up),  hui_ss,
            VALUE_OFFSET(VisualCommon, brightness_value)},
        {R, 3, LESS, NULL,          TOGGLE_NONE, CBK(brightness_down),hui_ss},
        {R, 4, DOT, "Измерения",    TOGGLE_OFF,  CBK(turn_meter),     hui_ss},
        {END}
      }
  },
  {
    .path = "И_ГБОм",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("И_ГБО")},
        
        {R, 0, DOT,  "ВКЛ",         TOGGLE_ON,   CBK(hide_marks),     hui_ss},
        {R, 1, DOT,  "Удалить",     TOGGLE_NONE, CBK(nav_del),        hui_ss},
        {R, 3, UP,   "Выбор",       TOGGLE_NONE, CBK(list_scroll_up),   mrkls},
        {R, 4, DOWN, NULL,          TOGGLE_NONE, CBK(list_scroll_down), mrkls},
        {END}
      }
  },
  {
    .path = "И_ПФ",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, CBK(switch_page),    UD("И_ПФд")},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, CBK(scale_up),       hui_pf, 
            VALUE_OFFSET(VisualCommon, scale_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(scale_down),     hui_pf},
        {L, 4, DOT,  "Автопрок",    TOGGLE_ON,   CBK(live_view),      hui_pf,
            BUTTON_OFFSET(VisualCommon, live_view)},

        {R, 0, VMOR,  "Метки",      TOGGLE_NONE, CBK(switch_page),    UD("И_ПФм")},
        {R, 1, UP,    "Вверх",      TOGGLE_NONE, CBK(nav_up),         hui_pf},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, CBK(nav_down),       hui_pf},
        {R, 3, RIGHT, "Вправо",     TOGGLE_NONE, CBK(nav_right),      hui_pf},
        {R, 4, LEFT,  "Влево",      TOGGLE_NONE, CBK(nav_left),       hui_pf},
        {END}
      }
  },
  {
    .path = "И_ПФд",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU,"Меню",         TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK,"Назад",        TOGGLE_NONE, CBK(switch_page),    UD("И_ПФ")},
        {L, 4, DOT, "Слои",         TOGGLE_ON,   CBK(pf_special),     hui_pf},

        {R, 0, MORE, "Шир диап",    TOGGLE_NONE, CBK(brightness_up),  hui_pf, 
            VALUE_OFFSET(VisualCommon, brightness_value)},
        {R, 1, LESS, NULL,          TOGGLE_NONE, CBK(brightness_down),hui_pf},
        {R, 2, MORE, "Нач ур",      TOGGLE_NONE, CBK(black_up),       hui_pf, 
            VALUE_OFFSET(VisualCommon, black_value)},
        {R, 3, LESS, NULL,          TOGGLE_NONE, CBK(black_down),     hui_pf},
        {R, 4, DOT, "Измерения",    TOGGLE_OFF,  CBK(turn_meter),     hui_pf},
        {END}
      }
  },
  {
    .path = "И_ПФм",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("И_ГБО")},
        // {L, 3, DOT,  "Уст",         TOGGLE_NONE, void_callback,  hui_pf},

        {R, 0, DOT,  "ВКЛ",         TOGGLE_ON,   CBK(hide_marks),     hui_pf},
        {R, 1, DOT,  "Удалить",     TOGGLE_NONE, CBK(nav_del),        hui_pf},
        {R, 3, UP,   "Выбор",       TOGGLE_NONE, CBK(list_scroll_up),   mrkls},
        {R, 4, DOWN,  NULL,         TOGGLE_NONE, CBK(list_scroll_down), mrkls},
        {END}
      }
  },
  {
    .path = "И_ГК",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, CBK(switch_page),    UD("И_ГКд")},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, CBK(scale_up),       hui_fl,
            VALUE_OFFSET (VisualCommon, scale_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(scale_down),     hui_fl},
        {L, 4, DOT,  "Автопрок",    TOGGLE_OFF,  CBK(live_view),      hui_fl,
            BUTTON_OFFSET(VisualCommon, live_view)},

        {R, 1, UP,    "Вверх",      TOGGLE_NONE, CBK(nav_up),         hui_fl},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, CBK(nav_down),       hui_fl},
        {R, 3, RIGHT, "Позже",      TOGGLE_NONE, CBK(fl_next),        hui_fl},
        {R, 4, LEFT,  "Ранее",      TOGGLE_NONE, CBK(fl_prev),        hui_fl},
        {END}
      }
  },
  {
    .path = "И_ГКд",
    .destination_selector = DEST_SPEC,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("И_ГК")},
        {L, 2, MORE, "Порог",       TOGGLE_NONE, CBK(sens_up),        hui_fl,
            VALUE_OFFSET (VisualCommon, sensitivity_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(sens_down),      hui_fl},
        {L, 4, DOT,  "Цель",        TOGGLE_ON,   CBK(mode_changed),   hui_fl},
        {R, 2, MORE, "Яркость",     TOGGLE_NONE, CBK(brightness_up),  hui_fl,
            VALUE_OFFSET (VisualCommon, brightness_value), VALUE_DEFAULT ("--")},
        {R, 3, LESS, NULL,          TOGGLE_NONE, CBK(brightness_down),hui_fl},

        {END}
      }
  },
  {
    .path = "ГАЛС",
    .destination_selector = DEST_UNSET,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 4, MNGR, "Проекты",     TOGGLE_NONE, CBK(run_manager),    NULL},

        {R, 0, UP,   "В начало",    TOGGLE_NONE, CBK(list_scroll_start),trkls},
        {R, 1, DOWN, "В конец",     TOGGLE_NONE, CBK(list_scroll_end),  trkls},
        {R, 3, MORE, "Выбор галса", TOGGLE_NONE, CBK(list_scroll_up),   trkls},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(list_scroll_down), trkls},
        {END}
      }
  },
  {
    .path = "Общее",
    .destination_selector = DEST_UNSET,
    .items =
      {
        {L, 0, MENU, "Меню",         TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {R, 1, DOT,  "Выгр. глубины",TOGGLE_NONE, CBK(depth_writer),   NULL},
        {END}
      }
  },
  {
    .path = NULL
  }
};

static AmePage sonar_pages[] =
{
  {
    .path = "Меню",
    .destination_selector = DEST_UNSET,
    .items =
      {
        {L, 0, VMOR, "ГБО Локатор",        TOGGLE_NONE, CBK(switch_page), UD("У_ГБО")},
        {L, 1, VMOR, "ГК Локатор",         TOGGLE_NONE, CBK(switch_page), UD("У_ГК")},
        {L, 2, VMOR, "ПФ Локатор",         TOGGLE_NONE, CBK(switch_page), UD("У_ПФ")},
        {END}
      }
  },
  {
    .path = "Общее",
    .destination_selector = DEST_AME_UI,
    .items =
      {
        {L, 3, DOT,  "Сух. поверка", TOGGLE_OFF,  CBK(start_stop_dry),
          BUTTON_OFFSET(AmeUI, start_stop_dry_switch)},
        {L, 4, DOT,  "Работа",       TOGGLE_OFF,  CBK(start_stop),
          BUTTON_OFFSET(AmeUI, start_stop_all_switch)},

        // {R, 0, DOT,  "Автопрок",     TOGGLE_ON,   live_view,      GINT_TO_POINTER (ALL)},

        {END}
      }
  },
  {
    .path = "У_ГБО",
    .destination_selector = DEST_SONAR,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, CBK(signal_up),      hui_ss, 
            VALUE_OFFSET(AmePanel, gui.signal_value), VALUE_DEFAULT("--")},
        {L, 2, LESS, NULL,          TOGGLE_NONE, CBK(signal_down),    hui_ss},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, CBK(distance_up),    hui_ss, 
            VALUE_OFFSET(AmePanel, gui.distance_value), VALUE_DEFAULT("--")},
        {L, 4, LESS, NULL,          TOGGLE_NONE, CBK(distance_down),  hui_ss},

        {R, 1, MORE, "Уровень",     TOGGLE_NONE, CBK(tvg_level_up),   hui_ss, 
            VALUE_OFFSET(AmePanel, gui.tvg_level_value), VALUE_DEFAULT("-ss-")},
        {R, 2, LESS, NULL,          TOGGLE_NONE, CBK(tvg_level_down), hui_ss},
        {R, 3, MORE, "Чувств.",     TOGGLE_NONE, CBK(tvg_sens_up),    hui_ss, 
            VALUE_OFFSET(AmePanel, gui.tvg_sens_value), VALUE_DEFAULT("-aa-")},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(tvg_sens_down),  hui_ss},
        {END}
      }
  },{
    .path = "У_ГБО",
    .destination_selector = DEST_AME_UI,
    .items =
      {
        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  CBK(start_stop),     &global, 
            BUTTON_OFFSET(AmeUI, start_stop_one_switch[W_SIDESCAN])},
        {END}
      }
  },
  {
    .path = "У_ГК",
    .destination_selector = DEST_SONAR,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    "Меню"},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, CBK(signal_up),      hui_fl, 
            VALUE_OFFSET(AmePanel, gui.signal_value), VALUE_DEFAULT("--")},
        {L, 2, LESS, NULL,          TOGGLE_NONE, CBK(signal_down),    hui_fl},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, CBK(distance_up),    hui_fl, 
            VALUE_OFFSET(AmePanel, gui.distance_value), VALUE_DEFAULT("--")},
        {L, 4, LESS, NULL,          TOGGLE_NONE, CBK(distance_down),  hui_fl},

        {R, 1, MORE, "Усиление-0",  TOGGLE_NONE, CBK(tvg0_up),        hui_fl, 
            VALUE_OFFSET(AmePanel, gui.tvg0_value)},
        {R, 2, LESS, NULL,          TOGGLE_NONE, CBK(tvg0_down),      hui_fl},
        {R, 3, MORE, "Усиление-Д",  TOGGLE_NONE, CBK(tvg_up),         hui_fl, 
            VALUE_OFFSET(AmePanel, gui.tvg_value)},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(tvg_down),       hui_fl},
        {END}
      }
  },{
    .path = "У_ГК",
    .destination_selector = DEST_AME_UI,
    .items =
      {
        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  start_stop,     &global,
            BUTTON_OFFSET(AmeUI, start_stop_one_switch[W_FORWARDL])},
        {END}
      }
  },
  {
    .path = "У_ПФ",
    .destination_selector = DEST_SONAR,
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, CBK(signal_up),      hui_pf,
            VALUE_OFFSET (AmePanel, gui.signal_value), VALUE_DEFAULT("--")},
        {L, 2, LESS, NULL,          TOGGLE_NONE, CBK(signal_down),    hui_pf},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, CBK(distance_up),    hui_pf,
            VALUE_OFFSET (AmePanel, gui.distance_value), VALUE_DEFAULT("--")},
        {L, 4, LESS, NULL,          TOGGLE_NONE, CBK(distance_down),  hui_pf},
        
        {R, 1, MORE, "Усиление-0",  TOGGLE_NONE, CBK(tvg0_up),        hui_pf,
            VALUE_OFFSET (AmePanel, gui.tvg0_value)},
        {R, 2, LESS, NULL,          TOGGLE_NONE, CBK(tvg0_down),      hui_pf},
        {R, 3, MORE, "Усиление-Д",  TOGGLE_NONE, CBK(tvg_up),         hui_pf,
            VALUE_OFFSET (AmePanel, gui.tvg_value)},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(tvg_down),       hui_pf},
        {END}
      }
  },{
    .path = "У_ПФ",
    .destination_selector = DEST_AME_UI,
    .items =
      {
        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  CBK(start_stop),     &global,
            BUTTON_OFFSET(AmeUI, start_stop_one_switch[W_PROFILER])},
        {END}
      }
  },
  // Конец.
  {
    .path = NULL
  }
};

#endif /* __AME_UI_H__ */
