#include "hw-ui.h"
#include "hw-ui-definitions.h"

#include "cheese-flash.h"
#include <gmodule.h>

static HwuiGlobal global_ui = {0,};
static Global *_global = NULL;
static ButtonReceive brec = {0,};

#ifdef G_OS_UNIX
  CheeseFlash * flash;

  void flash_fire (const gchar *color)
  {
    GdkWindow * root;
    gint x, y, width, height;
    GdkRectangle rect;

    root = gdk_get_default_root_window ();
    gdk_window_get_geometry (root, &x, &y, &width, &height);

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    cheese_flash_fire_colored (flash, &rect, color);
  }
#endif

/***
 *     #     # ######     #    ######  ######  ####### ######   #####
 *     #  #  # #     #   # #   #     # #     # #       #     # #     #
 *     #  #  # #     #  #   #  #     # #     # #       #     # #
 *     #  #  # ######  #     # ######  ######  #####   ######   #####
 *     #  #  # #   #   ####### #       #       #       #   #         #
 *     #  #  # #    #  #     # #       #       #       #    #  #     #
 *      ## ##  #     # #     # #       #       ####### #     #  #####
 *
 */

void
start_stop_wrapper (HyScanFnnButton *button,
                    gboolean         state,
                    gpointer         user_data)
{
  HwuiGlobal *ui = &global_ui;
  gboolean status;
  gint i;

  status = start_stop (_global, NULL, state);

  if (!status)
    {
      hyscan_fnn_button_set_state (button, !state);
      return;
    }

  /*  Вкл: сухая дизейбл, остальные актив
   *  Выкл: сухая энейбл, остальные неактив
   */
  gtk_widget_set_sensitive (ui->starter.dry, !state);
  hyscan_fnn_button_set_state ((HyScanFnnButton*)ui->starter.all, state);

  for (i = 0; i < START_STOP_LAST; ++i)
    {
      if (ui->starter.panel[i] != NULL)
        hyscan_fnn_button_set_state ((HyScanFnnButton*)ui->starter.panel[i], state);
    }
}

void
start_stop_dry_wrapper (HyScanFnnButton *button,
                        gboolean         state,
                        gpointer         user_data)
{
  HwuiGlobal *ui = &global_ui;
  gboolean status;
  gint i;

  set_dry (_global, state);
  status = start_stop (_global, NULL, state);

  if (!status)
    {
      hyscan_fnn_button_set_state (button, !state);
      set_dry (_global, !state);
      return;
    }

  /*  Вкл: сухая актив, остальные дизейбл
   *  Выкл: сухая неактив, остальные энейблед
   */
  hyscan_fnn_button_set_state ((HyScanFnnButton*)ui->starter.dry, state);
  gtk_widget_set_sensitive (ui->starter.all, !state);

  for (i = 0; i < START_STOP_LAST; ++i)
    {
      if (ui->starter.panel[i] != NULL)
        gtk_widget_set_sensitive (ui->starter.panel[i], !state);
    }
}

/***
 *     ######     #     #####  #######    ######  #     # ### #       ######  ####### ######
 *     #     #   # #   #     # #          #     # #     #  #  #       #     # #       #     #
 *     #     #  #   #  #       #          #     # #     #  #  #       #     # #       #     #
 *     ######  #     # #  #### #####      ######  #     #  #  #       #     # #####   ######
 *     #       ####### #     # #          #     # #     #  #  #       #     # #       #   #
 *     #       #     # #     # #          #     # #     #  #  #       #     # #       #    #
 *     #       #     #  #####  #######    ######   #####  ### ####### ######  ####### #     #
 *
 */
/* ф-ия делает 1 элемент. */
GtkWidget *
make_item (HwuiPageItem  *item,
           GtkWidget   **value_widget)
{
  gboolean is_toggle = item->toggle != TOGGLE_NONE;
  gboolean state = item->toggle == TOGGLE_ON;
  const gchar * title = item->msg_id;

  GtkWidget *button = hyscan_fnn_button_new (item->icon_name,
                                             title, is_toggle, state);

  g_signal_connect (button, is_toggle ? "fnn-toggled" : "fnn-activated",
                    G_CALLBACK (item->callback), item->user_data);
  /* Если указано, куда сохранять виджет значения, создадим его. */
  if (item->value_offset > 0)
    {
      GtkLabel *value;

      value = hyscan_fnn_button_create_value (HYSCAN_FNN_BUTTON (button),
                                              item->value_default);
      *value_widget = GTK_WIDGET (value);
    }
  else
    {
      *value_widget = NULL;
    }

  gtk_widget_set_name (button, title);
  /* Возвращаем кнопку целиком. */
  return button;
}

/* функция делает 1 страницу*/
void
build_page (HwuiGlobal   *ui,
            Global  *global,
            HwuiPage *page)
{
  gint i;
  GtkWidget *left, *right;
  GtkStack *lstack, *rstack;
  gchar *wname;

  lstack = GTK_STACK (ui->lstack);
  rstack = GTK_STACK (ui->rstack);
  /* Разбираемся, есть эти страницы в стеках или нет. */
  left = gtk_stack_get_child_by_name (lstack, page->path);
  right = gtk_stack_get_child_by_name (rstack, page->path);

  /* Если нет, то создаем и пихаем их в стеки. */
  if (left == NULL)
    {
      wname = g_strdup_printf ("%s", page->path);
      left = hyscan_fnn_fixed_new ();
      gtk_widget_set_name (GTK_WIDGET (left), wname);
      g_free (wname);
      gtk_stack_add_titled (lstack, left, page->path, page->path);
    }
  if (right == NULL)
    {
      wname = g_strdup_printf ("%s", page->path);
      right = hyscan_fnn_fixed_new ();
      gtk_widget_set_name (GTK_WIDGET (right), wname);
      g_free (wname);
      gtk_stack_add_titled (rstack, right, page->path, page->path);
    }

  // TODO: check, if this can help me avoid {END}
  for (i = 0; ; ++i)
    {
      const gchar *title;
      GtkWidget *button, *value;
      HyScanFnnFixed *fixed;
      HwuiPageItem *item = &page->items[i];

      if (item->side == L)
        fixed = (HyScanFnnFixed*)left;
      else if (item->side == R)
        fixed = (HyScanFnnFixed*)right;
      else if (item->side == END)
        break;
      else
        g_error ("HwuiGlobal: wrong item->side");

      button = make_item (item, &value);
      title = item->msg_id;
      wname = g_strdup_printf ("%i: %s", item->position, title);
      gtk_widget_set_name (button, wname);

      /* Ща сложно будет. В странице есть destination_selector. Он определяет,
       * _куда_ надо класть, если надо вообще. Надо или нет определяется оффсетом.
       * ----
       * В конце баттон безусловно кладется в фиксед в заданное место,
       * ради этого всё и затевалось.
       */
      if (item->value_offset > 0 || item->button_offset > 0)
        {
          gchar * base;
          GtkWidget ** dest;
          FnnPanel *panel;

          switch (page->destination_selector)
            {
            case DEST_PANEL:
              panel = get_panel (global, GPOINTER_TO_INT (item->user_data));
              if (panel == NULL)
                continue;
              base = (gchar*)panel;
              break;

            case DEST_PANEL_SPEC:
              panel = get_panel (global, GPOINTER_TO_INT (item->user_data));
              if (panel == NULL)
                continue;
              base = (gchar*)(panel->vis_gui);
              break;

            case DEST_HWUI_GLOBAL:
              base = (gchar*)ui;
              break;

            default:
              base = 0;
              g_warning ("HwuiGlobal: wrong page->destination_selector");
            }

          /* Кстати, я там специально не проверяю на NULL, потому что криво
           * составленный уй.х -- это ошибка. */
          /* Да, я чувствую себя очень важным, когда гоняю туда-сюда указатели
           * на указатели. Самоутверждаюсь за счёт указателей. */
          if (item->value_offset)
            {
              dest = (GtkWidget**)(base + item->value_offset);
              *dest = value;
            }
          if (item->button_offset)
            {
              dest = (GtkWidget**)(base + item->button_offset);
              *dest = button;
            }
        }

      hyscan_fnn_fixed_pack (fixed, item->position, button);
    }
}

/* Функция делает массив переданных страниц. */
void
build_all (HwuiGlobal   *ui,
           Global  *global,
           HwuiPage *page)
{
  for (; page->path != NULL; ++page)
    build_page (ui, global, page);
}


//  #####  ####### #     # ####### ######  ####### #
// #     # #     # ##    #    #    #     # #     # #
// #       #     # # #   #    #    #     # #     # #
// #       #     # #  #  #    #    ######  #     # #
// #       #     # #   # #    #    #   #   #     # #
// #     # #     # #    ##    #    #    #  #     # #
//  #####  ####### #     #    #    #     # ####### #######

/* функция переключает виджеты. */
void
widget_swap (GObject  *emitter,
             gpointer  user_data)
{
  /* В userdata лежит option.
   * -2 ROTATE циклическая прокрутка
   * -1 ALL - все три,
   * айдишники панелей -- соотв. панель.
   */
  gint selector = GPOINTER_TO_INT (user_data);
  HyScanGtkFnnBox *abox = HYSCAN_GTK_FNN_BOX (global_ui.acoustic);
  gchar *markup;
  const gchar *text = (gchar*)0x1;
  gint id = 0;

  if (!HYSCAN_IS_GTK_FNN_BOX (global_ui.acoustic))
    {
      g_warning ("HwUI: %i", __LINE__);
    }

  if (selector == ALL)
    {
      hyscan_gtk_fnn_box_show_all (abox);
      text = "Всё";
    }
  else
    {
      FnnPanel *panel;

      if (selector == ROTATE)
        id = hyscan_gtk_fnn_box_next_visible (abox);
      else
        id = selector;

      /* Если не получилось прокрутить, то ид будет -1. В таком случае, показываем
       * все панели. Иначе вернется юзердата с panelx. */
      if (id == -1)
        {
          hyscan_gtk_fnn_box_show_all (abox);
          text = "Всё";
        }
      else
        {
          hyscan_gtk_fnn_box_set_visible (abox, id);
          panel = get_panel (_global, id);
          text = panel->name_local;
        }
    }

  markup = g_strdup_printf ("<small><b>%s</b></small>", text);
  gtk_label_set_markup (GTK_LABEL (global_ui.current_view), text);
  g_free (markup);
}

void
switch_page (GObject     *emitter,
             const gchar *page)
{
  HwuiGlobal *ui = &global_ui;
  GtkStack * lstack = GTK_STACK (ui->lstack);
  GtkStack * rstack = GTK_STACK (ui->rstack);
  GtkRevealer * left = GTK_REVEALER (ui->left_revealer);
  GtkRevealer * bottom = GTK_REVEALER (ui->bott_revealer);

  // HyScanFnnFixed * rold = HYSCAN_FNN_FIXED (gtk_stack_get_visible_child (rstack));
  // const gchar * old = gtk_stack_get_visible_child_name (lstack);

  // if (g_str_equal (old, "И_ПФд"))
  //   hyscan_fnn_fixed_set_state (rold, 4, FALSE);
  // if (g_str_equal (old, "И_ГБОд"))
  //   hyscan_fnn_fixed_set_state (rold, 4, FALSE);

  g_message ("Opening page <%s>", page);

  gtk_stack_set_visible_child_name (lstack, page);
  gtk_stack_set_visible_child_name (rstack, page);

  /* Спецслучаи. */
  if (g_str_equal (page, "ГАЛС"))
    gtk_revealer_set_reveal_child (left, TRUE);
  else if (g_str_equal (page, "И_ГБОм"))
    {
      gtk_revealer_set_reveal_child (left, TRUE);
      turn_marks (NULL, X_SIDESCAN);
    }
  else if (g_str_equal (page, "И_ПФм"))
    {
      gtk_revealer_set_reveal_child (left, TRUE);
      turn_marks (NULL, X_PROFILER);
    }
  else
    {
      gtk_revealer_set_reveal_child (left, FALSE);
      turn_marks (NULL, -1);
      turn_meter (NULL, FALSE, -1);
    }


  if (g_str_equal (page, "И_ГК") || g_str_equal (page, "И_ГКд"))
    gtk_revealer_set_reveal_child (bottom, TRUE);
  else
    gtk_revealer_set_reveal_child (bottom, FALSE);
}


GtkTreeView *
list_scroll_tree_view_resolver (gint list)
{
  GtkTreeView * view = NULL;

  if (list == L_MARKS)
    view = hyscan_gtk_project_viewer_get_tree_view (HYSCAN_GTK_PROJECT_VIEWER (_global->gui.mark_view));
  else if (list == L_TRACKS)
    view = _global->gui.track.tree;
  else
    g_message ("Wrong list selector passed (%i)", list);

  return view;
}

void
list_scroll_up (GObject *emitter,
                gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT (udata));
  track_scroller (view, TRUE, FALSE);
}

void
list_scroll_down (GObject *emitter,
                  gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT (udata));
  track_scroller (view, FALSE, FALSE);
}

void
list_scroll_start (GObject *emitter,
                   gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT (udata));
  track_scroller (view, TRUE, TRUE);
}
void
list_scroll_end (GObject *emitter,
                 gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT (udata));
  track_scroller (view, FALSE, TRUE);
}

void
screenshooter (void)
{
  GdkWindow * root;
  GdkPixbuf * screenshot;
  GFileIOStream * fios;
  GFile * file;
  gint x, y, width, height;
  GDateTime * dt;
  gchar *path, *file_name;

  root = gdk_get_default_root_window ();
  gdk_window_get_geometry (root, &x, &y, &width, &height);
  screenshot = gdk_pixbuf_get_from_window (root, x, y, width, height);

  dt = g_date_time_new_now_local ();
  file_name = g_date_time_format (dt, "%y%m%d-%H%M%S.png");
  path = g_build_path (G_DIR_SEPARATOR_S, global_ui.screenshot_dir, file_name, NULL);
  file = g_file_new_for_path (path);
  fios = g_file_create_readwrite (file, G_FILE_CREATE_NONE, NULL, NULL);

  if (fios != NULL)
    {
      GOutputStream * ostream;
      ostream = g_io_stream_get_output_stream (G_IO_STREAM (fios));
      gdk_pixbuf_save_to_stream (screenshot, ostream, "png", NULL, NULL, NULL);
      g_message ("Screenshot saved: %s", path);
      g_object_unref (fios);

      #ifdef G_OS_UNIX
        flash_fire ("white");
      #endif
    }
  else
    {
      g_message ("Failed to save screenshot at %s", path);
    }

  g_date_time_unref (dt);
  g_free (file_name);
  g_free (path);
  g_object_unref (screenshot);
  g_object_unref (file);
}

//   #     #    #    ### #     #
//   ##   ##   # #    #  ##    #
//   # # # #  #   #   #  # #   #
//   #  #  # #     #  #  #  #  #
//   #     # #######  #  #   # #
//   #     # #     #  #  #    ##
//   #     # #     # ### #     #

/* Самая крутая функция, строит весь уй. */
gboolean
build_interface (Global *global)
{
  HwuiGlobal *ui = &global_ui;
  _global = global;

  GtkWidget *grid;

  /* Кнопошный гуй представляет собой центральную область из 3 виджетов,
   * две боковых области с кнопками. Внизу выезжает управление ВСЛ,
   * слева выезжает контроль над галсами и метками. */

  /* Сетка для всего. */
  grid = gtk_grid_new ();

  /* Центральная зона: виджет с виджетами. Да. */
  ui->acoustic = hyscan_gtk_fnn_box_new ();

  /* Слева у нас список галсов и меток. */
  ui->left_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  /* Ривилеры, чтобы прятать и показывать самое интересное. */
  ui->left_revealer = gtk_revealer_new ();
  ui->bott_revealer = gtk_revealer_new ();

  gtk_revealer_set_transition_duration (GTK_REVEALER (ui->left_revealer), 100);
  gtk_revealer_set_transition_duration (GTK_REVEALER (ui->bott_revealer), 100);
  gtk_revealer_set_transition_type (GTK_REVEALER (ui->left_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  gtk_revealer_set_transition_type (GTK_REVEALER (ui->bott_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);

  /* Стеки для кнопочек. */
  ui->lstack = gtk_stack_new ();
  ui->rstack = gtk_stack_new ();
  gtk_widget_set_name (ui->lstack, "Left");
  gtk_widget_set_name (ui->rstack, "Right");

  /* На данный момент все контейнеры готовы. Можно наполнять их. */

  /* Левая панель содержит список галсов, меток и редактор меток. */
  {
    GtkWidget * tracks = GTK_WIDGET (global->gui.track.view);
    GtkWidget * mlist = GTK_WIDGET (global->gui.mark_view);
    GtkWidget * meditor = GTK_WIDGET (global->gui.meditor);

    g_object_set (tracks, "vexpand", TRUE,  "valign", GTK_ALIGN_FILL,
                          "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mlist, "vexpand", TRUE,  "valign", GTK_ALIGN_FILL,
                         "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (meditor, "vexpand", FALSE, "valign", GTK_ALIGN_END,
                           "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);

    gtk_box_pack_start (GTK_BOX (ui->left_box), tracks, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (ui->left_box), mlist, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (ui->left_box), meditor, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (ui->left_revealer), ui->left_box);
  }

  /* Нижняя панель содержит виджет управления впередсмотрящим. */
  {
    FnnPanel *panel = get_panel_quiet (global, X_FORWARDL);
    if (panel != NULL)
      {
        VisualFL *fl = (VisualFL*)panel->vis_gui;

        gtk_container_add (GTK_CONTAINER (ui->bott_revealer), fl->play_control);
        gtk_revealer_set_reveal_child (GTK_REVEALER (ui->bott_revealer), FALSE);
      }
  }

  /* Строим интерфейсос. */
  build_all (ui, global, common_pages);

  /* Общие ГЛ-виджеты. */
  if (global->control != NULL)
    build_all (ui, global, any_sonar_pages);

  /* Инициализация значений. */
  widget_swap (NULL, GINT_TO_POINTER (ALL));

  /* Собираем воедино.                                   L  T  W  H */
  gtk_grid_attach (GTK_GRID (grid), ui->lstack,          0, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (grid), ui->rstack,          5, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (grid), ui->left_revealer,   2, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (grid), ui->acoustic,        4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), ui->bott_revealer,   4, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), global->gui.nav,     4, 2, 1, 1);

  gtk_container_add (GTK_CONTAINER (global->gui.window), grid);

  #ifdef G_OS_UNIX
    flash = cheese_flash_new ();
  #endif

  return TRUE;
}

void
destroy_interface (void)
{
  g_atomic_int_set (&brec.stop, TRUE);
  g_clear_pointer (&brec.thread, g_thread_join);
  if (brec.socket != NULL)
    {
      g_socket_close (brec.socket, NULL);
      g_clear_object (&brec.socket);
    }

  #ifdef G_OS_UNIX
    g_object_unref (flash);
  #endif
}

gboolean
ame_button_clicker (gpointer data)
{
  gint code = GPOINTER_TO_INT (data);
  GtkWidget *stack, *fixed;

  if (code / 1000 == 1)
    stack = global_ui.lstack;
  else if (code / 1000 == 2)
    stack = global_ui.rstack;
  else
    {
      g_warning ("wrong code");
      return G_SOURCE_REMOVE;
    }
  code %= 1000;
  g_message ("FnnButton: %"G_GINT64_FORMAT" clicked <%s>(%i)",
             g_get_monotonic_time (),
             gtk_stack_get_visible_child_name (GTK_STACK (stack)),
             code);
  fixed = gtk_stack_get_visible_child (GTK_STACK (stack));
  hyscan_fnn_fixed_activate (HYSCAN_FNN_FIXED (fixed), code);

  return G_SOURCE_REMOVE;
}

gint
ame_button_parse (const gchar * buf)
{
  gint code;

  /* Если нет префикса, возвращаем 1. */
  if (buf[0] != 'b')
    return 1;
  if (buf[1] != 't')
    return 2;
  if (buf[2] != '_')
    return 3;

  if (buf[3] == 'l')
    code = 1000;
  else if (buf[3] == 'r')
    code = 2000;
  else
    return 4;

  if (buf[4] >= '0' && buf[4] <= '6')
    code += buf[4]-'0'-1;

  g_idle_add (ame_button_clicker, GINT_TO_POINTER (code));

  return 5;
};

void *
ame_button_thread (void * data)
{
  gboolean triggered;
  GError *error = NULL;
  gchar buf[128];
  gchar *pbuf;
  gssize bytes;
  while (!g_atomic_int_get (&brec.stop))
    {
      if (error != NULL)
        {
          if (error->code != G_IO_ERROR_TIMED_OUT)
            g_warning ("Button error: %s", error->message);
          g_clear_pointer (&error, g_error_free);
        }

      triggered = g_socket_condition_timed_wait (brec.socket,
                                                 G_IO_IN | G_IO_HUP | G_IO_ERR,
                                                 G_TIME_SPAN_MILLISECOND * 100 ,
                                                 NULL, &error);
      if (!triggered)
        continue;

      if (error != NULL)
        {
          g_warning ("Button condition wait: %s", error->message);
          g_clear_pointer (&error, g_error_free);
        }

      /* Я ожидаю "bt_xx", 5 символов. Но я не уверен в амешниках. */
      bytes = g_socket_receive (brec.socket, buf, 5*10+1, NULL, &error);

      /* нуль-терминируем. */
      buf[bytes] = '\0';
      g_message ("FnnButton: %"G_GINT64_FORMAT" received <%s>", g_get_monotonic_time (), buf);

      for (pbuf = buf; bytes > 0 && pbuf < buf + 127; )
        {
          gint res = ame_button_parse (pbuf);

          bytes -= res;
          pbuf += res;
        }

    }

  return NULL;
}

gboolean
kf_config (GKeyFile *kf)
{
  gchar * butt_addr;
  guint16 butt_port;
  GInetAddress *addr;
  gboolean src_spec;
  GSocketAddress *isocketadress;

  /* Синхр. меток. */
  {
    gchar * mark_addr =  keyfile_string_read_helper (kf, "ame", "mark_addr");
    guint16 mark_port =  keyfile_uint_read_helper (kf, "ame", "mark_port", 10010);

    if (mark_addr != NULL)
      {
        _global->marks.sync = hyscan_mark_sync_new (mark_addr, mark_port);
        g_message ("Mark sync: %s %u", mark_addr, mark_port);
        g_free (mark_addr);
      }
    else
      {
        g_message ("Mark sync: address not set");
      }

  }

  /* Выгрузка глубины. */
  {
    gchar * depth_writer_path =  keyfile_string_read_helper (kf, "ame", "depth_path");

    if (depth_writer_path == NULL)
      {
        g_message ("Depth export path not set. Using default.");
        depth_writer_path = g_strdup ("/srv/hyscan/lat-lon-depth.txt");
      }
    g_message ("Depth export path: %s", depth_writer_path);
    global_ui.depth_writer_path = depth_writer_path;
  }
  /* Скриншоты. */
  {
    gchar * screenshot_dir =  keyfile_string_read_helper (kf, "ame", "screenshot_dir");

    if (screenshot_dir == NULL)
      {
        g_message ("Screenshot dir not set. Using default.");
        screenshot_dir = g_strdup (g_get_tmp_dir ());
      }
    g_message ("Screenshot dir: %s", screenshot_dir);
    global_ui.screenshot_dir = screenshot_dir;
  }

  src_spec = keyfile_bool_read_helper (kf, "ame", "source_specific");
  butt_addr =  keyfile_string_read_helper (kf, "ame", "button_addr");
  butt_port =  keyfile_uint_read_helper (kf, "ame", "button_port", 9000);

  if (butt_addr != NULL)
    {
      brec.socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
      addr = g_inet_address_new_from_string (butt_addr);
      isocketadress = g_inet_socket_address_new (addr, butt_port);
      g_message ("Buttons: %s %u", butt_addr, butt_port);

      g_socket_bind (brec.socket, isocketadress, TRUE, NULL);

      brec.stop = FALSE;
      brec.thread = g_thread_new ("buttons", ame_button_thread, NULL);
      g_socket_join_multicast_group (brec.socket, addr, src_spec, NULL, NULL);

      g_free (butt_addr);
    }
  else
    {
      g_message ("Buttons: address not set");
    }

  return TRUE;
}

void
depth_writer (GObject *emitter)
{
  guint32 first, last, i;
  HyScanAntennaOffset antenna;
  HyScanGeoGeodetic coord;
  GString *string = NULL;
  gchar *words = NULL;
  GError *error = NULL;

  HyScanDB * db = _global->db;
  HyScanCache * cache = _global->cache;
  gchar *project = _global->project_name;
  gchar *track = _global->track_name;

  HyScanNavData * dpt;
  HyScanmLoc * mloc;

  dpt = HYSCAN_NAV_DATA (hyscan_nmea_parser_new (db, cache, project, track,
                                                 2, HYSCAN_NMEA_DATA_DPT,
                                                 HYSCAN_NMEA_FIELD_DEPTH));

  if (dpt == NULL)
    {
      g_warning ("Failed to open dpt parser.");
      return;
    }

  mloc = hyscan_mloc_new (db, cache, project, track);
  if (mloc == NULL)
    {
      g_warning ("Failed to open mLocation.");
      g_clear_object (&dpt);
      return;
    }

  string = g_string_new (NULL);
  g_string_append_printf (string, "%s;%s\n", project, track);

  hyscan_nav_data_get_range (dpt, &first, &last);
  antenna = hyscan_nav_data_get_offset (dpt);

  for (i = first; i <= last; ++i)
    {
      gdouble val;
      gint64 time;
      gboolean status;
      gchar lat_str[1024] = {'\0'};
      gchar lon_str[1024] = {'\0'};
      gchar dpt_str[1024] = {'\0'};

      status = hyscan_nav_data_get (dpt, NULL, i, &time, &val);
      if (!status)
        continue;

      status = hyscan_mloc_get (mloc, NULL, time, &antenna, 0, 0, 0, &coord);
      if (!status)
        continue;

      g_ascii_formatd (lat_str, 1024, "%f", coord.lat);
      g_ascii_formatd (lon_str, 1024, "%f", coord.lon);
      g_ascii_formatd (dpt_str, 1024, "%f", val);
      g_string_append_printf (string, "%s;%s;%s\n", lat_str, lon_str, dpt_str);
    }

  words = g_string_free (string, FALSE);

  if (words == NULL)
    return;

  g_file_set_contents (global_ui.depth_writer_path,
                       words, strlen (words), &error);

  if (error != NULL)
    {
      g_message ("Depth save failure: %s", error->message);
      g_error_free (error);
    }

  g_free (words);

  #ifdef G_OS_UNIX
    flash_fire ("#00ff7f");
  #endif
}

G_MODULE_EXPORT gboolean
kf_setting (GKeyFile *kf)
{
  return TRUE;
}

G_MODULE_EXPORT gboolean
kf_desetup (GKeyFile *kf)
{
  return TRUE;
}

#define HW_N 3
G_MODULE_EXPORT gboolean
panel_pack (FnnPanel *panel,
            gint      panelx)
{
  HwuiGlobal *ui = &global_ui;
  GtkWidget *w;

  gint order[HW_N]  = {X_SIDESCAN, X_PROFILER, X_FORWARDL};
  gint left[HW_N]   = {0, 1, 1};
  gint top[HW_N]    = {0, 0, 1};
  gint height[HW_N] = {2, 1, 1};

  HwuiPage *ipages[HW_N] = {ss_image_pages, pf_image_pages, fl_image_pages};
  HwuiPage *spages[HW_N] = {ss_pages, pf_pages, fl_pages};
  gint i;

  for (i = 0; i < HW_N + 1; ++i)
    {
      if (i == HW_N)
        return FALSE;

      if (panelx == order[i])
        break;
    }


  w = panel->vis_gui->main;

  g_object_set (w, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                   "hexpand", TRUE, "halign", GTK_ALIGN_FILL, NULL);

  hyscan_gtk_fnn_box_pack (HYSCAN_GTK_FNN_BOX (ui->acoustic), w, order[i],
                           left[i], top[i], 1, height[i]);

  build_all (ui, _global, ipages[i]);
  if (panel_sources_are_in_sonar (_global, panel))
    build_all (ui, _global, spages[i]);

  return TRUE;
}

G_MODULE_EXPORT void
panel_adjust_visibility (HyScanTrackInfo *track_info)
{
  return;
}
