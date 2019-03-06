#ifndef _AME_UI_DEFINITIONS_H
#define _AME_UI_DEFINITIONS_H

#include "ame-ui.h"

#define XSS X_SIDESCAN
#define XFL X_FORWARDL
#define XPF X_PROFILER

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
#define INFO ("dialog-information-symbolic")
#define EXIT ("window-close-symbolic")

#define PATH(x) .path=(gchar*)(x)
#define DESTINATION_SELECTOR(x) .destination_selector=(x)

#define CBK(x) .callback = (gpointer)(x)
#define UD(x) .user_data = (gpointer)(x)
#define VALUE_OFFSET(x, y) .value_offset = offsetof(x,y)
#define VALUE_DEFAULT(x) .value_default = ((gchar*)(x))
#define BUTTON_OFFSET(x, y) .button_offset = offsetof(x,y)

static AmePage common_pages[] =
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {L, 3, VMOR, "Галс",            TOGGLE_NONE, CBK(switch_page), UD("ГАЛС")},
        {L, 4, VMOR, "Общее",           TOGGLE_NONE, CBK(switch_page), UD("Общее")},
        {R, 4, VMOR, "Вид",             TOGGLE_NONE, CBK(widget_swap), GINT_TO_POINTER (ROTATE),
            VALUE_OFFSET(AmeUI, current_view), VALUE_DEFAULT("--")},
        {END}
      }
  },
  {
    PATH("ГАЛС"), DESTINATION_SELECTOR(DEST_UNSET),
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
    PATH("Общее"), DESTINATION_SELECTOR(DEST_UNSET),
    .items =
      {
        {L, 0, MENU, "Меню",         TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {R, 1, DOT,  "Выгр. глубины",TOGGLE_NONE, CBK(depth_writer),   NULL},
        {R, 4, EXIT, "Выход",        TOGGLE_NONE, CBK(gtk_main_quit),  NULL},
        {END}
      }
  },
  {
    PATH(NULL)
  }
};

static AmePage ss_image_pages[] = 
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {R, 0, VMOR, "ГБО Изображение", TOGGLE_NONE, CBK(switch_page), UD("И_ГБО")},
        {END}
      }
  },
  {
    PATH("И_ГБО"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, CBK(switch_page),    UD("И_ГБОд")},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, CBK(scale_up),       UD(XSS),
            VALUE_OFFSET(VisualCommon, scale_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(scale_down),     UD(XSS)},
        {L, 4, DOT,  "Автопрок",    TOGGLE_ON,   CBK(automove_switched),      UD(XSS),
           BUTTON_OFFSET(VisualCommon, live_view)},

        {R, 0, VMOR,  "Метки",      TOGGLE_NONE, CBK(switch_page),    UD("И_ГБОм")},
        {R, 1, UP,    "Вверх",      TOGGLE_NONE, CBK(nav_pg_up),      UD(XSS)},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, CBK(nav_pg_down),    UD(XSS)},
        {R, 3, RIGHT, "Вправо",     TOGGLE_NONE, CBK(nav_right),      UD(XSS)},
        {R, 4, LEFT,  "Влево",      TOGGLE_NONE, CBK(nav_left),       UD(XSS)},
        {END}
      }
  },
  {
    PATH("И_ГБОд"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU,"Меню",         TOGGLE_NONE, CBK(switch_page),     UD("Меню")},
        {L, 1, BACK,"Назад",        TOGGLE_NONE, CBK(switch_page),     UD("И_ГБО")},
        {L, 4, REPT,"Цвет",         TOGGLE_NONE, CBK(color_map_cyclic),UD(XSS),
            VALUE_OFFSET(VisualCommon, colormap_value)},

        {R, 2, MORE, "Контр",       TOGGLE_NONE, CBK(brightness_up),   UD(XSS),
            VALUE_OFFSET(VisualCommon, brightness_value)},
        {R, 3, LESS, NULL,          TOGGLE_NONE, CBK(brightness_down), UD(XSS)},
        {R, 4, DOT, "Измерения",    TOGGLE_OFF,  CBK(turn_meter),      UD(XSS)},
        {END}
      }
  },
  {
    PATH("И_ГБОм"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("И_ГБО")},

        {R, 0, DOT,  "ВКЛ",         TOGGLE_ON,   CBK(hide_marks),     UD(XSS)},
        {R, 1, DOT,  "Удалить",     TOGGLE_NONE, CBK(nav_del),        UD(XSS)},
        {R, 3, UP,   "Выбор",       TOGGLE_NONE, CBK(list_scroll_up),   mrkls},
        {R, 4, DOWN, NULL,          TOGGLE_NONE, CBK(list_scroll_down), mrkls},
        {END}
      }
  },
  { PATH ( NULL ) }
};

static AmePage pf_image_pages[] =
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {R, 2, VMOR, "ПФ Изображение",  TOGGLE_NONE, CBK(switch_page), UD("И_ПФ")},
        {END}
      }
  },
  {
    PATH("И_ПФ"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, CBK(switch_page),    UD("И_ПФд")},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, CBK(scale_up),       UD(XPF),
            VALUE_OFFSET(VisualCommon, scale_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(scale_down),     UD(XPF)},
        {L, 4, DOT,  "Автопрок",    TOGGLE_ON,   CBK(automove_switched),      UD(XPF),
            BUTTON_OFFSET(VisualCommon, live_view)},

        {R, 0, VMOR,  "Метки",      TOGGLE_NONE, CBK(switch_page),    UD("И_ПФм")},
        {R, 1, UP,    "Вверх",      TOGGLE_NONE, CBK(nav_up),         UD(XPF)},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, CBK(nav_down),       UD(XPF)},
        {R, 3, RIGHT, "Вправо",     TOGGLE_NONE, CBK(nav_right),      UD(XPF)},
        {R, 4, LEFT,  "Влево",      TOGGLE_NONE, CBK(nav_left),       UD(XPF)},
        {END}
      }
  },
  {
    PATH("И_ПФд"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU,"Меню",         TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK,"Назад",        TOGGLE_NONE, CBK(switch_page),    UD("И_ПФ")},
        {L, 4, DOT, "Слои",         TOGGLE_ON,   CBK(pf_special),     UD(XPF)},

        {R, 0, MORE, "Шир диап",    TOGGLE_NONE, CBK(brightness_up),  UD(XPF),
            VALUE_OFFSET(VisualCommon, brightness_value)},
        {R, 1, LESS, NULL,          TOGGLE_NONE, CBK(brightness_down),UD(XPF)},
        {R, 2, MORE, "Нач ур",      TOGGLE_NONE, CBK(black_up),       UD(XPF),
            VALUE_OFFSET(VisualCommon, black_value)},
        {R, 3, LESS, NULL,          TOGGLE_NONE, CBK(black_down),     UD(XPF)},
        {R, 4, DOT, "Измерения",    TOGGLE_OFF,  CBK(turn_meter),     UD(XPF)},
        {END}
      }
  },
  {
    PATH("И_ПФм"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("И_ГБО")},
        // {L, 3, DOT,  "Уст",         TOGGLE_NONE, void_callback,  UD(XPF)},

        {R, 0, DOT,  "ВКЛ",         TOGGLE_ON,   CBK(hide_marks),     UD(XPF)},
        {R, 1, DOT,  "Удалить",     TOGGLE_NONE, CBK(nav_del),        UD(XPF)},
        {R, 3, UP,   "Выбор",       TOGGLE_NONE, CBK(list_scroll_up),   mrkls},
        {R, 4, DOWN,  NULL,         TOGGLE_NONE, CBK(list_scroll_down), mrkls},
        {END}
      }
  },
  { PATH (NULL) }
};

static AmePage fl_image_pages[] =
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {R, 1, VMOR, "ГК Изображение",  TOGGLE_NONE, CBK(switch_page), UD("И_ГК")},
        {END}
      }
  },
  {
    PATH("И_ГК"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, CBK(switch_page),    UD("И_ГКд")},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, CBK(scale_up),       UD(XFL),
            VALUE_OFFSET (VisualCommon, scale_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(scale_down),     UD(XFL)},
        {L, 4, DOT,  "Автопрок",    TOGGLE_OFF,  CBK(automove_switched),      UD(XFL),
            BUTTON_OFFSET(VisualCommon, live_view)},

        {R, 1, UP,    "Вверх",      TOGGLE_NONE, CBK(nav_up),         UD(XFL)},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, CBK(nav_down),       UD(XFL)},
        {R, 3, RIGHT, "Позже",      TOGGLE_NONE, CBK(fl_next),        UD(XFL)},
        {R, 4, LEFT,  "Ранее",      TOGGLE_NONE, CBK(fl_prev),        UD(XFL)},
        {END}
      }
  },
  {
    PATH("И_ГКд"), DESTINATION_SELECTOR(DEST_PANEL_SPEC),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, CBK(switch_page),    UD("И_ГК")},
        {L, 2, MORE, "Порог",       TOGGLE_NONE, CBK(sensitivity_up), UD(XFL),
            VALUE_OFFSET (VisualCommon, sensitivity_value), VALUE_DEFAULT ("--")},
        {L, 3, LESS, NULL,          TOGGLE_NONE, CBK(sensitivity_down), UD(XFL)},
        {L, 4, DOT,  "Цель",        TOGGLE_ON,   CBK(mode_changed),   UD(XFL)},
        {R, 2, MORE, "Яркость",     TOGGLE_NONE, CBK(brightness_up),  UD(XFL),
            VALUE_OFFSET (VisualCommon, brightness_value), VALUE_DEFAULT ("--")},
        {R, 3, LESS, NULL,          TOGGLE_NONE, CBK(brightness_down),UD(XFL)},

        {END}
      }
  },
  { PATH (NULL) }
};

static AmePage any_sonar_pages[] =
{
  {
    PATH("Общее"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {L, 3, DOT,  "Сух. поверка", TOGGLE_OFF,  CBK(start_stop_dry_wrapper),
          BUTTON_OFFSET(AmeUI, starter.dry)},
        {L, 4, DOT,  "Работа",       TOGGLE_OFF,  CBK(start_stop_wrapper),
          BUTTON_OFFSET(AmeUI, starter.all)},
        {R, 0, INFO, "Версия ГЛ",    TOGGLE_NONE, CBK(run_show_sonar_info),
          UD("/info"), },
        {END}
      }
  },
  { PATH(NULL) } /* Конец. */
};

static AmePage ss_pages[] =
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_UNSET),
    .items =
      {
        {L, 0, VMOR, "ГБО Локатор",  TOGGLE_NONE, CBK(switch_page), UD("У_ГБО")},
        {END}
      }
  },
  {
    PATH("У_ГБО"), DESTINATION_SELECTOR(DEST_PANEL),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, CBK(signal_up),      UD(XSS),
            VALUE_OFFSET(AmePanel, gui.signal_value), VALUE_DEFAULT("--")},
        {L, 2, LESS, NULL,          TOGGLE_NONE, CBK(signal_down),    UD(XSS)},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, CBK(distance_up),    UD(XSS),
            VALUE_OFFSET(AmePanel, gui.distance_value), VALUE_DEFAULT("--")},
        {L, 4, LESS, NULL,          TOGGLE_NONE, CBK(distance_down),  UD(XSS)},

        {R, 1, MORE, "Уровень",     TOGGLE_NONE, CBK(tvg_level_up),   UD(XSS),
            VALUE_OFFSET(AmePanel, gui.tvg_level_value), VALUE_DEFAULT("-ss-")},
        {R, 2, LESS, NULL,          TOGGLE_NONE, CBK(tvg_level_down), UD(XSS)},
        {R, 3, MORE, "Чувств.",     TOGGLE_NONE, CBK(tvg_sens_up),    UD(XSS),
            VALUE_OFFSET(AmePanel, gui.tvg_sens_value), VALUE_DEFAULT("-aa-")},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(tvg_sens_down),  UD(XSS)},
        {END}
      }
  },{
    PATH("У_ГБО"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  CBK(start_stop_wrapper),
            BUTTON_OFFSET(AmeUI, starter.panel[START_STOP_SIDESCAN])},
        {END}
      }
  },
  { PATH(NULL) } /* Конец. */
};

static AmePage pf_pages[] =
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_UNSET),
    .items =
      {
        {L, 2, VMOR, "ПФ Локатор",   TOGGLE_NONE, CBK(switch_page), UD("У_ПФ")},
        {END}
      }
  },
  {
    PATH("У_ПФ"), DESTINATION_SELECTOR(DEST_PANEL),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, CBK(signal_up),      UD(XPF),
            VALUE_OFFSET (AmePanel, gui.signal_value), VALUE_DEFAULT("--")},
        {L, 2, LESS, NULL,          TOGGLE_NONE, CBK(signal_down),    UD(XPF)},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, CBK(distance_up),    UD(XPF),
            VALUE_OFFSET (AmePanel, gui.distance_value), VALUE_DEFAULT("--")},
        {L, 4, LESS, NULL,          TOGGLE_NONE, CBK(distance_down),  UD(XPF)},

        {R, 1, MORE, "Усиление-0",  TOGGLE_NONE, CBK(tvg0_up),        UD(XPF),
            VALUE_OFFSET (AmePanel, gui.tvg0_value)},
        {R, 2, LESS, NULL,          TOGGLE_NONE, CBK(tvg0_down),      UD(XPF)},
        {R, 3, MORE, "Усиление-Д",  TOGGLE_NONE, CBK(tvg_up),         UD(XPF),
            VALUE_OFFSET (AmePanel, gui.tvg_value)},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(tvg_down),       UD(XPF)},
        {END}
      }
  },{
    PATH("У_ПФ"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  CBK(start_stop_wrapper),
            BUTTON_OFFSET(AmeUI, starter.panel[START_STOP_PROFILER])},
        {END}
      }
  },
  { PATH(NULL) } /* Конец. */
};

static AmePage fl_pages[] =
{
  {
    PATH("Меню"), DESTINATION_SELECTOR(DEST_UNSET),
    .items =
      {
        {L, 1, VMOR, "ГК Локатор",   TOGGLE_NONE, CBK(switch_page), UD("У_ГК")},
        {END}
      }
  },
  {
    PATH("У_ГК"), DESTINATION_SELECTOR(DEST_PANEL),
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, CBK(switch_page),    UD("Меню")},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, CBK(signal_up),      UD(XFL),
            VALUE_OFFSET(AmePanel, gui.signal_value), VALUE_DEFAULT("--")},
        {L, 2, LESS, NULL,          TOGGLE_NONE, CBK(signal_down),    UD(XFL)},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, CBK(distance_up),    UD(XFL),
            VALUE_OFFSET(AmePanel, gui.distance_value), VALUE_DEFAULT("--")},
        {L, 4, LESS, NULL,          TOGGLE_NONE, CBK(distance_down),  UD(XFL)},

        {R, 1, MORE, "Усиление-0",  TOGGLE_NONE, CBK(tvg0_up),        UD(XFL),
            VALUE_OFFSET(AmePanel, gui.tvg0_value)},
        {R, 2, LESS, NULL,          TOGGLE_NONE, CBK(tvg0_down),      UD(XFL)},
        {R, 3, MORE, "Усиление-Д",  TOGGLE_NONE, CBK(tvg_up),         UD(XFL),
            VALUE_OFFSET(AmePanel, gui.tvg_value)},
        {R, 4, LESS, NULL,          TOGGLE_NONE, CBK(tvg_down),       UD(XFL)},
        {END}
      }
  },{
    PATH("У_ГК"), DESTINATION_SELECTOR(DEST_AME_UI),
    .items =
      {
        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  CBK(start_stop_wrapper),
            BUTTON_OFFSET(AmeUI, starter.panel[START_STOP_FORWARDL])},
        {END}
      }
  },
  { PATH(NULL) } /* Конец. */
};

#endif
