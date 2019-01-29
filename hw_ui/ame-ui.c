#include "ame-ui.h"
#include "ame-ui-definitions.h"

#include <gmodule.h>

AmeUI global_ui = {0,};
Global *_global = NULL;
ButtonReceive brec;
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
start_stop_wrapper (HyScanAmeButton *button,
                    gboolean         state,
                    gpointer         user_data)
{
  AmeUI *ui = &global_ui;
  gboolean status;
  gint i;

  status = start_stop (_global, state);

  if (!status)
    {
      hyscan_ame_button_set_state (button, !state);
      return;
    }

  /*  Вкл: сухая дизейбл, остальные актив
   *  Выкл: сухая энейбл, остальные неактив
   */
  gtk_widget_set_sensitive (ui->starter.dry, !state);
  hyscan_ame_button_set_state ((HyScanAmeButton*)ui->starter.all, state);

  for (i = 0; i < START_STOP_LAST; ++i)
    {
      if (ui->starter.panel[i] != NULL)
        hyscan_ame_button_set_state ((HyScanAmeButton*)ui->starter.panel[i], state);
    }
}

void
start_stop_dry_wrapper (HyScanAmeButton *button,
                        gboolean         state,
                        gpointer         user_data)
{
  AmeUI *ui = &global_ui;
  gboolean status;
  gint i;

  set_dry (_global, state);
  status = start_stop (_global, state);

  if (!status)
    {
      hyscan_ame_button_set_state (button, !state);
      return;
    }

  /*  Вкл: сухая актив, остальные дизейбл
   *  Выкл: сухая неактив, остальные энейблед
   */
  hyscan_ame_button_set_state ((HyScanAmeButton*)ui->starter.dry, state);
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
make_item (AmePageItem  *item,
           GtkWidget   **value_widget)
{
  gboolean is_toggle = item->toggle != TOGGLE_NONE;
  gboolean state = item->toggle == TOGGLE_ON;

  GtkWidget *button = hyscan_ame_button_new (item->icon_name,
                                             item->title,
                                             is_toggle,
                                             state);

  g_signal_connect (button, is_toggle ? "ame-toggled" : "ame-activated",
                    G_CALLBACK (item->callback), item->user_data);
  /* Если указано, куда сохранять виджет значения, создадим его. */
  if (item->value_offset > 0)
    {
      GtkLabel *value;

      value = hyscan_ame_button_create_value (HYSCAN_AME_BUTTON (button),
                                              item->value_default);
      *value_widget = GTK_WIDGET (value);
    }
  else
    {
      *value_widget = NULL;
    }

  gtk_widget_set_name (button, item->title);
  /* Возвращаем кнопку целиком. */
  return button;
}

/* функция делает 1 страницу*/
void
build_page (AmeUI   *ui,
            Global  *global,
            AmePage *page)
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
      left = hyscan_ame_fixed_new ();
      gtk_widget_set_name (GTK_WIDGET (left), wname);
      g_free (wname);
      gtk_stack_add_titled (lstack, left, page->path, page->path);
    }
  if (right == NULL)
    {
      wname = g_strdup_printf ("%s", page->path);
      right = hyscan_ame_fixed_new ();
      gtk_widget_set_name (GTK_WIDGET (right), wname);
      g_free (wname);
      gtk_stack_add_titled (rstack, right, page->path, page->path);
    }

  // TODO: check, if this can help me avoid {END}
  for (i = 0; ; ++i)
    {
      GtkWidget *button, *value;
      HyScanAmeFixed *fixed;
      AmePageItem *item = &page->items[i];

      if (item->side == L)
        fixed = (HyScanAmeFixed*)left;
      else if (item->side == R)
        fixed = (HyScanAmeFixed*)right;
      else if (item->side == END)
        break;
      else
        g_error ("AmeUI: wrong item->side");

      button = make_item (item, &value);
      wname = g_strdup_printf ("%i: %s", item->position, item->title);
      gtk_widget_set_name(button, wname);

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
          AmePanel *panel;

          switch (page->destination_selector)
            {
            case DEST_PANEL:
              panel = get_panel (global, GPOINTER_TO_INT (item->user_data));
              base = (gchar*)panel;
              break;

            case DEST_PANEL_SPEC:
              panel = get_panel (global, GPOINTER_TO_INT (item->user_data));
              base = (gchar*)(panel->vis_gui);
              break;

            case DEST_AME_UI:
              base = (gchar*)ui;
              break;

            default:
              base = 0;
              g_warning ("AmeUI: wrong page->destination_selector");
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

      hyscan_ame_fixed_pack (fixed, item->position, button);
    }
}

/* Функция делает массив переданных страниц. */
void
build_all (AmeUI   *ui,
           Global  *global,
           AmePage *page)
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
  HyScanGtkAmeBox *abox = HYSCAN_GTK_AME_BOX (global_ui.acoustic);
  gchar *markup;
  const gchar *text = (gchar*)0x1;
  gint id = 0;

if (!HYSCAN_IS_GTK_AME_BOX (global_ui.acoustic))
{
  g_warning ("fuck");

}
  if (selector == ALL)
    {
      hyscan_gtk_ame_box_show_all (abox);
      text = "Всё";
    }
  else
    {
      AmePanel *panel;

      if (selector == ROTATE)
        id = hyscan_gtk_ame_box_next_visible (abox);
      else
        id = selector;

      /* Если не получилось прокрутить, то ид будет -1. В таком случае, показываем
       * все панели. Иначе вернется юзердата с panelx. */
      if (id == -1)
        {
          hyscan_gtk_ame_box_show_all (abox);
          text = "Всё";
        }
      else
        {
          hyscan_gtk_ame_box_set_visible (abox, id);
          panel = get_panel (_global, id);
          text = panel->name;
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
  AmeUI *ui = &global_ui;
  GtkStack * lstack = GTK_STACK (ui->lstack);
  GtkStack * rstack = GTK_STACK (ui->rstack);
  GtkRevealer * left = GTK_REVEALER (ui->left_revealer);
  GtkRevealer * bottom = GTK_REVEALER (ui->bott_revealer);

  // HyScanAmeFixed * rold = HYSCAN_AME_FIXED (gtk_stack_get_visible_child (rstack));
  // const gchar * old = gtk_stack_get_visible_child_name (lstack);

  // if (g_str_equal (old, "И_ПФд"))
  //   hyscan_ame_fixed_set_state (rold, 4, FALSE);
  // if (g_str_equal (old, "И_ГБОд"))
  //   hyscan_ame_fixed_set_state (rold, 4, FALSE);

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
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, TRUE, FALSE);
}

void
list_scroll_down (GObject *emitter,
                  gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, FALSE, FALSE);
}

void
list_scroll_start (GObject *emitter,
                   gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, TRUE, TRUE);
}
void
list_scroll_end (GObject *emitter,
                 gpointer udata)
{
  GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
  track_scroller (view, FALSE, TRUE);
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
  AmeUI *ui = &global_ui;
  _global = global;

  GtkWidget *grid;

  /* Кнопошный гуй представляет собой центральную область из 3 виджетов,
   * две боковых области с кнопками. Внизу выезжает управление ВСЛ,
   * слева выезжает контроль над галсами и метками. */

  /* Сетка для всего. */
  grid = gtk_grid_new ();

  /* Центральная зона: виджет с виджетами. Да. */
  ui->acoustic = hyscan_gtk_ame_box_new ();

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

  /* Начнем с центра, добавим все отображаемые виджеты. */
  {
    gint order[3]  = {X_SIDESCAN, X_PROFILER, X_FORWARDL};
    gint left[3]   = {0, 1, 1};
    gint top[3]    = {0, 0, 1};
    gint height[3] = {2, 1, 1};
    gint i;

    for (i = 0; i < 3; ++i)
      {
        GtkWidget *w;
        AmePanel *panel = get_panel (global, order[i]);

        w = panel->vis_gui->main;

        g_object_set (w, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                         "hexpand", TRUE, "halign", GTK_ALIGN_FILL, NULL);

        hyscan_gtk_ame_box_pack (HYSCAN_GTK_AME_BOX (ui->acoustic), w, order[i],
                                 left[i], top[i], 1, height[i]);
      }
  }

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
    AmePanel *panel = get_panel (global, X_FORWARDL);
    VisualFL *fl = (VisualFL*)panel->vis_gui;

    gtk_container_add (GTK_CONTAINER (ui->bott_revealer), fl->play_control);
    gtk_revealer_set_reveal_child (GTK_REVEALER (ui->bott_revealer), FALSE);
  }

  /* Строим интерфейсос. */
  build_all (ui, global, common_pages);
  if (global->control_s != NULL)
    build_all (ui, global, sonar_pages);

  /* Инициализация значений. */
  widget_swap (NULL, GINT_TO_POINTER (ALL));

  /* Собираем воедино.                                   L  T  W  H */
  gtk_grid_attach (GTK_GRID (grid), ui->lstack,          0, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (grid), ui->rstack,          1, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (grid), ui->left_revealer,   2, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (grid), ui->acoustic,        4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), ui->bott_revealer,   4, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), global->gui.nav,     4, 2, 1, 1);

  gtk_container_add (GTK_CONTAINER (global->gui.window), grid);

  return TRUE;
}

void
destroy_interface (void)
{
  g_atomic_int_set (&brec.stop, TRUE);
  g_clear_pointer (&brec.thread, g_thread_join);
  g_socket_close (brec.socket, NULL);
  g_clear_object (&brec.socket);
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
g_message ("push, %s, %i", gtk_stack_get_visible_child_name (GTK_STACK (stack)), code);
  fixed = gtk_stack_get_visible_child (GTK_STACK (stack));
  hyscan_ame_fixed_activate (HYSCAN_AME_FIXED (fixed), code % 1000);

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
    code += buf[4]-'0';

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
      g_message ("Received <%s>", buf);

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
  gchar * mcast_addr;
  guint16 mcast_port;
  GInetAddress *addr;
  gboolean source_specific;
  GSocketAddress *isocketadress;

  source_specific = keyfile_bool_read_helper (kf, "ame", "source_specific");
  mcast_addr =  keyfile_string_read_helper (kf, "ame", "button_addr");
  mcast_port =  keyfile_uint_read_helper (kf, "ame", "button_port", 9000);
  g_return_val_if_fail(mcast_addr != NULL, FALSE);

  brec.socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, NULL);
  addr = g_inet_address_new_from_string (mcast_addr);
  isocketadress = g_inet_socket_address_new(addr, mcast_port);

  g_socket_bind (brec.socket, isocketadress, TRUE, NULL);

  brec.stop = FALSE;
  brec.thread = g_thread_new ("buttons", ame_button_thread, NULL);
  g_socket_join_multicast_group (brec.socket, addr, source_specific, NULL, NULL);

  return TRUE;
}

