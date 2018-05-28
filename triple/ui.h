#include "triple-types.h"

#define hui_ss GINT_TO_POINTER (W_SIDESCAN)
#define hui_fl GINT_TO_POINTER (W_FORWARDL)
#define hui_pf GINT_TO_POINTER (W_PROFILER)
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


#if defined(__clang__)
  #define P_(x) (x)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wincompatible-pointer-types"
  #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
  #define P_(x) ((gpointer)(x))
#endif



static AmePage common_pages[] =
{
  {
    .path = "Меню",
    .items =
      {
        {L, 3, VMOR, "Общее",          TOGGLE_NONE, P_(switch_page), P_("Общее")},
        {L, 4, VMOR, "Галс",         TOGGLE_NONE, P_(switch_page), P_("ГАЛС")},
        {R, 0, VMOR, "ГБО Изображение",        TOGGLE_NONE, P_(switch_page), P_("И_ГБО")},
        {R, 1, VMOR, "ГК Изображение",         TOGGLE_NONE, P_(switch_page), P_("И_ГК")},
        {R, 2, VMOR, "ПФ Изображение",         TOGGLE_NONE, P_(switch_page), P_("И_ПФ")},
        // {R, 3, VMOR, "НСТ",          TOGGLE_NONE, P_(switch_page), P_("НСТ")},
        {R, 4, VMOR, "Вид",          TOGGLE_NONE, P_(widget_swap), GINT_TO_POINTER(ROTATE), P_(&global.gui.current_view), "--"},
        {END}
      }
  },
  {
    .path = "И_ГБО",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, switch_page,    "И_ГБОд"},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, scale_up,       hui_ss, P_(&global.GSS.gui.scale_value), "--"},
        {L, 3, LESS, NULL,          TOGGLE_NONE, scale_down,     hui_ss},
        {L, 4, DOT,  "Автопрок",    TOGGLE_ON,   live_view,      hui_ss, .place = P_(&global.GSS.live_view)},

        {R, 0, VMOR,  "Метки",      TOGGLE_NONE, switch_page,    "И_ГБОм"},
        {R, 1, UP,    "Вверх",      TOGGLE_NONE, nav_pg_up,      hui_ss},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, nav_pg_down,    hui_ss},
        {R, 3, RIGHT, "Вправо",     TOGGLE_NONE, nav_right,      hui_ss},
        {R, 4, LEFT,  "Влево",      TOGGLE_NONE, nav_left,       hui_ss},
        {END}
      }
  },
  {
    .path = "И_ГБОд",
    .items =
      {
        {L, 0, MENU,"Меню",         TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, BACK,"Назад",        TOGGLE_NONE, switch_page,    "И_ГБО"},
        {L, 4, REPT,"Цвет",         TOGGLE_NONE, color_map_up,   hui_ss, &global.GSS.gui.colormap_value},

        {R, 2, MORE, "Контр",       TOGGLE_NONE, brightness_up,  hui_ss, &global.GSS.gui.brightness_value},
        {R, 3, LESS, NULL,          TOGGLE_NONE, brightness_down,hui_ss},
        {R, 4, DOT, "Измерения",    TOGGLE_OFF,  turn_meter,     hui_ss},
        {END}
      }
  },
  {
    .path = "И_ГБОм",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, switch_page,    "И_ГБО"},
        // {L, 3, DOT,  "Уст",         TOGGLE_NONE, void_callback,  hui_ss},

        {R, 0, DOT,  "ВКЛ",         TOGGLE_ON,   hide_marks,     hui_ss},
        {R, 1, DOT,  "Удалить",     TOGGLE_NONE, nav_del,        hui_ss},
        {R, 3, UP,   "Выбор",       TOGGLE_NONE, list_scroll_up,   mrkls},
        {R, 4, DOWN, NULL,          TOGGLE_NONE, list_scroll_down, mrkls},
        {END}
      }
  },
  {
    .path = "И_ПФ",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, switch_page,    "И_ПФд"},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, scale_up,       hui_pf, &global.GPF.gui.scale_value, "--"},
        {L, 3, LESS, NULL,          TOGGLE_NONE, scale_down,     hui_pf},
        {L, 4, DOT,  "Автопрок",    TOGGLE_ON,   live_view,      hui_pf, .place = P_(&global.GPF.live_view)},

        {R, 0, VMOR,  "Метки",      TOGGLE_NONE, switch_page,    "И_ПФм"},
        {R, 1, UP,    "Вверх",      TOGGLE_NONE, nav_up,         hui_pf},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, nav_down,       hui_pf},
        {R, 3, RIGHT, "Вправо",     TOGGLE_NONE, nav_right,      hui_pf},
        {R, 4, LEFT,  "Влево",      TOGGLE_NONE, nav_left,       hui_pf},
        {END}
      }
  },
  {
    .path = "И_ПФд",
    .items =
      {
        {L, 0, MENU,"Меню",         TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, BACK,"Назад",        TOGGLE_NONE, switch_page,    "И_ПФ"},
        {L, 4, DOT, "Слои",         TOGGLE_ON,   pf_special,     hui_pf},

        {R, 0, MORE, "Шир диап",    TOGGLE_NONE, brightness_up,  hui_pf, &global.GPF.gui.brightness_value},
        {R, 1, LESS, NULL,          TOGGLE_NONE, brightness_down,hui_pf},
        {R, 2, MORE, "Нач ур",      TOGGLE_NONE, black_up,       hui_pf, &global.GPF.gui.black_value},
        {R, 3, LESS, NULL,          TOGGLE_NONE, black_down,     hui_pf},
        {R, 4, DOT, "Измерения",    TOGGLE_OFF,  turn_meter,     hui_pf},
        {0}
      }
  },
  {
    .path = "И_ПФм",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, switch_page,    "И_ГБО"},
        // {L, 3, DOT,  "Уст",         TOGGLE_NONE, void_callback,  hui_pf},

        {R, 0, DOT,  "ВКЛ",         TOGGLE_ON,   hide_marks,     hui_pf},
        {R, 1, DOT,  "Удалить",     TOGGLE_NONE, nav_del,        hui_pf},
        {R, 3, UP,   "Выбор",       TOGGLE_NONE, list_scroll_up,   mrkls},
        {R, 4, DOWN,  NULL,         TOGGLE_NONE, list_scroll_down, mrkls},
        {0}
      }
  },
  {
    .path = "И_ГК",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, VMOR, "Ещё",         TOGGLE_NONE, switch_page,    "И_ГКд"},
        {L, 2, MORE, "Масштаб",     TOGGLE_NONE, scale_up,       hui_fl, &global.GFL.gui.scale_value, "--"},
        {L, 3, LESS, NULL,          TOGGLE_NONE, scale_down,     hui_fl},
        {L, 4, DOT,  "Автопрок",    TOGGLE_OFF,  live_view,      hui_fl, .place = P_(&global.GFL.live_view)},

        {R, 1, UP,    "Вверх",      TOGGLE_NONE, nav_up,         hui_fl},
        {R, 2, DOWN,  "Вниз",       TOGGLE_NONE, nav_down,       hui_fl},
        {R, 3, RIGHT, "Позже",      TOGGLE_NONE, fl_next,        hui_fl},
        {R, 4, LEFT,  "Ранее",      TOGGLE_NONE, fl_prev,        hui_fl},
        {0}
      }
  },
  {
    .path = "И_ГКд",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, switch_page,    "И_ГК"},
        {L, 2, MORE, "Порог",       TOGGLE_NONE, sens_up,        hui_fl, &global.GFL.gui.sensitivity_value, "--"},
        {L, 3, LESS, NULL,          TOGGLE_NONE, sens_down,      hui_fl},
        {L, 4, DOT,  "Цель",        TOGGLE_ON,   mode_changed,   hui_fl},
        {R, 2, MORE, "Яркость",     TOGGLE_NONE, brightness_up,  hui_fl, &global.GFL.gui.brightness_value, "--"},
        {R, 3, LESS, NULL,          TOGGLE_NONE, brightness_down,hui_fl},

        {0}
      }
  },
  {
    .path = "ГАЛС",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, BACK, "Назад",       TOGGLE_NONE, switch_page,    "Меню"},
        {L, 4, MNGR, "Проекты",     TOGGLE_NONE, run_manager,    NULL},

        {R, 0, UP,   "В начало",    TOGGLE_NONE, list_scroll_start,trkls},
        {R, 1, DOWN, "В конец",     TOGGLE_NONE, list_scroll_end,  trkls},
        {R, 3, MORE, "Выбор галса", TOGGLE_NONE, list_scroll_up,   trkls},
        {R, 4, LESS, NULL,          TOGGLE_NONE, list_scroll_down, trkls},

        {0}
      }
  },
  {
    .path = "Общее",
    .items =
      {
        {L, 0, MENU, "Меню",         TOGGLE_NONE, switch_page,    "Меню"},
        {R, 1, DOT,  "Выгр. глубины",TOGGLE_NONE, depth_writer,   NULL},

        {0}
      }
  },

  /* Конец. */
  {
    .path = NULL
  }
};

static AmePage sonar_pages[] =
{
  {
    .path = "Меню",
    .items =
      {
        {L, 0, VMOR, "ГБО Локатор",        TOGGLE_NONE, P_(switch_page), P_("У_ГБО")},
        {L, 1, VMOR, "ГК Локатор",         TOGGLE_NONE, P_(switch_page), P_("У_ГК")},
        {L, 2, VMOR, "ПФ Локатор",         TOGGLE_NONE, P_(switch_page), P_("У_ПФ")},
        {0}
      }
  },
  {
    .path = "Общее",
    .items =
      {
        {L, 3, DOT,  "Сух. поверка", TOGGLE_OFF,  start_stop_dry, .place=P_(&global.gui.start_stop_dry_switch)},
        {L, 4, DOT,  "Работа",       TOGGLE_OFF,  start_stop,     .place=P_(&global.gui.start_stop_all_switch)},

        // {R, 0, DOT,  "Автопрок",     TOGGLE_ON,   live_view,      GINT_TO_POINTER (ALL)},

        {0}
      }
  },
  {
    .path = "У_ГБО",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, signal_up,      hui_ss, P_(&global.GSS.gui.signal_value), "--"},
        {L, 2, LESS, NULL,          TOGGLE_NONE, signal_down,    hui_ss},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, distance_up,    hui_ss, P_(&global.GSS.gui.distance_value), "--"},
        {L, 4, LESS, NULL,          TOGGLE_NONE, distance_down,  hui_ss},

        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  start_stop,     &global, .place=P_(&global.gui.start_stop_one_switch[W_SIDESCAN])},
        {R, 1, MORE, "Уровень",     TOGGLE_NONE, tvg_level_up,   hui_ss, P_(&global.GSS.gui.tvg_level_value), "-ss-"},
        {R, 2, LESS, NULL,          TOGGLE_NONE, tvg_level_down, hui_ss},
        {R, 3, MORE, "Чувств.",     TOGGLE_NONE, tvg_sens_up,    hui_ss, P_(&global.GSS.gui.tvg_sens_value), "-aa-"},
        {R, 4, LESS, NULL,          TOGGLE_NONE, tvg_sens_down,  hui_ss},
        {0}
      }
  },
  {
    .path = "У_ГК",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, signal_up,      hui_fl, &global.GFL.gui.signal_value, "--"},
        {L, 2, LESS, NULL,          TOGGLE_NONE, signal_down,    hui_fl},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, distance_up,    hui_fl, &global.GFL.gui.distance_value, "--"},
        {L, 4, LESS, NULL,          TOGGLE_NONE, distance_down,  hui_fl},

        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  start_stop,     &global, .place=P_(&global.gui.start_stop_one_switch[W_FORWARDL])},
        {R, 1, MORE, "Усиление-0",  TOGGLE_NONE, tvg0_up,        hui_fl, &global.GFL.gui.tvg0_value},
        {R, 2, LESS, NULL,          TOGGLE_NONE, tvg0_down,      hui_fl},
        {R, 3, MORE, "Усиление-Д",  TOGGLE_NONE, tvg_up,         hui_fl, &global.GFL.gui.tvg_value},
        {R, 4, LESS, NULL,          TOGGLE_NONE, tvg_down,       hui_fl},
        {0}
      }
  },
  {
    .path = "У_ПФ",
    .items =
      {
        {L, 0, MENU, "Меню",        TOGGLE_NONE, switch_page,    "Меню"},
        {L, 1, MORE, "Сигнал",      TOGGLE_NONE, signal_up,      hui_pf, &global.GPF.gui.signal_value, "--"},
        {L, 2, LESS, NULL,          TOGGLE_NONE, signal_down,    hui_pf},
        {L, 3, MORE, "Дальность",   TOGGLE_NONE, distance_up,    hui_pf, &global.GPF.gui.distance_value, "--"},
        {L, 4, LESS, NULL,          TOGGLE_NONE, distance_down,  hui_pf},

        {R, 0, DOT,  "Работа",      TOGGLE_OFF,  start_stop,     &global, .place=P_(&global.gui.start_stop_one_switch[W_PROFILER])},
        {R, 1, MORE, "Усиление-0",  TOGGLE_NONE, tvg0_up,        hui_pf, &global.GPF.gui.tvg0_value},
        {R, 2, LESS, NULL,          TOGGLE_NONE, tvg0_down,      hui_pf},
        {R, 3, MORE, "Усиление-Д",  TOGGLE_NONE, tvg_up,         hui_pf, &global.GPF.gui.tvg_value},
        {R, 4, LESS, NULL,          TOGGLE_NONE, tvg_down,       hui_pf},
        {0}
      }
  },
  /* Конец. */
  {
    .path = NULL
  }
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
