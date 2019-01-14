#include "ame-ui.h"

AmeUI global_ui = {0,};
Global *_global = NULL;
// void
// nav_common (GtkWidget *target,
//             guint      keyval,
//             guint      state)
// {
//   GdkEvent *event;

//   event = gdk_event_new (GDK_KEY_PRESS);
//   event->key.keyval = keyval;
//   event->key.state = state;

//   gtk_window_set_focus (GTK_WINDOW (global.gui.window), target);
//   event->key.window = g_object_ref (gtk_widget_get_window (target));
//   g_idle_add ((GSourceFunc)idle_key, (gpointer)event);
// }

// #define NAV_FN(fname, button) void \
//                               fname (GObject * emitter, \
//                                      gpointer udata) \
//                               {gint sel = GPOINTER_TO_INT (udata); \
//                                nav_common (global_ui.disp_widgets[sel], \
//                                            button, GDK_CONTROL_MASK);}

// NAV_FN (nav_del, GDK_KEY_Delete)
// NAV_FN (nav_pg_up, GDK_KEY_Page_Up)
// NAV_FN (nav_pg_down, GDK_KEY_Page_Down)
// NAV_FN (nav_up, GDK_KEY_Up)
// NAV_FN (nav_down, GDK_KEY_Down)
// NAV_FN (nav_left, GDK_KEY_Left)
// NAV_FN (nav_right, GDK_KEY_Right)

// void
// run_manager (GObject *emitter)
// {
//   HyScanDBInfo *info;
//   GtkWidget *dialog;
//   gint res;

//   info = hyscan_db_info_new (global.db);
//   dialog = hyscan_ame_project_new (global.db, info, GTK_WINDOW (global.gui.window));
//   res = gtk_dialog_run (GTK_DIALOG (dialog));

//   // TODO: start/stop?
//   if (res == HYSCAN_AME_PROJECT_OPEN || res == HYSCAN_AME_PROJECT_CREATE)
//     {
//       gchar *project;
//       hyscan_ame_project_get (HYSCAN_AME_PROJECT (dialog), &project, NULL);

//       start_stop (NULL, FALSE);
//       hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (global.gui.start_stop_all_switch), FALSE);
//       hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (global.gui.start_stop_dry_switch), FALSE);
//       hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_SIDESCAN]), FALSE);
//       hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_PROFILER]), FALSE);
//       hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (global.gui.start_stop_one_switch[W_FORWARDL]), FALSE);

//       g_clear_pointer (&global.project_name, g_free);
//       global.project_name = g_strdup (project);
//       g_message ("global.project_name set to <<%s>>", global.project_name);
//       hyscan_db_info_set_project (global.db_info, project);
//       // hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.control), project);
//       // hyscan_data_writer_set_project (HYSCAN_DATA_WRITER (global.control), project);

//       g_clear_pointer (&global.marks.loc_storage, g_hash_table_unref);
//       global.marks.loc_storage = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
//                                                         (GDestroyNotify) loc_store_free);


//       hyscan_mark_model_set_project (global.marks.model, global.db, project);
//       g_free (project);
//     }

//   g_object_unref (info);
//   gtk_widget_destroy (dialog);
// }

// #     # ###          ######  #     # ### #       ######  ####### ######
// #     #  #           #     # #     #  #  #       #     # #       #     #
// #     #  #           #     # #     #  #  #       #     # #       #     #
// #     #  #           ######  #     #  #  #       #     # #####   ######
// #     #  #           #     # #     #  #  #       #     # #       #   #
// #     #  #           #     # #     #  #  #       #     # #       #    #
//  #####  ###          ######   #####  ### ####### ######  ####### #     #

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

  lstack = GTK_STACK (ui->lstack);
  rstack = GTK_STACK (ui->rstack);
  /* Разбираемся, есть эти страницы в стеках или нет. */
  left = gtk_stack_get_child_by_name (lstack, page->path);
  right = gtk_stack_get_child_by_name (rstack, page->path);

  /* Если нет, то создаем и пихаем их в стеки. */
  if (left == NULL)
    {
      left = hyscan_ame_fixed_new ();
      gtk_stack_add_titled (lstack, left, page->path, page->path);
    }
  if (right == NULL)
    {
      right = hyscan_ame_fixed_new ();
      gtk_stack_add_titled (rstack, right, page->path, page->path);
    }

  // TODO: check, if this can help me avoid {END}
  gint total = sizeof(page->items) / sizeof(page->items[0]);
  for (i = 0; i <= total; ++i)
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

      /* Ща сложно будет. В странице есть destination_selector. Он определяет,
       * куда надо класть, если надо вообще.
       * Надо или нет определяется оффсетом.
       * ----
       * При этом если вернулся валью, то кладем валью. Если нет, то кладем баттон.
       * ---
       * В конце баттон безусловно кладется в фиксед в заданное место,
       * ради этого всё и затевалось.
       */
      if (item->value_offset > 0 || item->button_offset > 0)
        {
          gchar * base;
          GtkWidget ** dest;
          GtkWidget * storable;
          AmePanel *panel;

          if (item->value_offset > 0)
            storable = value;
          if (item->button_offset > 0)
            storable = button;

          switch (page->destination_selector)
            {
            case DEST_SONAR:
              panel = get_panel (global, GPOINTER_TO_INT (item->user_data));
              base = (gchar*)panel;
              break;

            case DEST_SPEC:
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
          dest = (GtkWidget**)(base + item->value_offset);
          *dest = storable;
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

gboolean
start_stop_disabler (GtkWidget *sw,
                     gboolean   state)
{
  AmeUI *ui = &global_ui;

  if (sw == ui->start_stop_dry_switch)
    {
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (ui->start_stop_all_switch), !state);
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (ui->start_stop_one_switch[W_SIDESCAN]), !state);
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (ui->start_stop_one_switch[W_FORWARDL]), !state);
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (ui->start_stop_one_switch[W_PROFILER]), !state);
    }
  else
    {
      hyscan_ame_button_set_sensitive (HYSCAN_AME_BUTTON (ui->start_stop_dry_switch), !state);

      hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (ui->start_stop_all_switch), state);
      hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (ui->start_stop_one_switch[W_SIDESCAN]), state);
      hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (ui->start_stop_one_switch[W_PROFILER]), state);
      hyscan_ame_button_set_active (HYSCAN_AME_BUTTON (ui->start_stop_one_switch[W_FORWARDL]), state);
    }

  return FALSE;
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
   * 0 - гбо, 1 - пф, 2 - гк.
   */
  gint selector = GPOINTER_TO_INT (user_data);
  HyScanGtkAmeBox *abox = HYSCAN_GTK_AME_BOX (global_ui.acoustic);
  gchar *markup;
  const gchar *text;
  gint id = 0;

  if (selector == ALL)
    {
      hyscan_gtk_ame_box_show_all (abox);
      text = "Всё";
    }
  else
    {
      AmePanel *panel;
      id = selector;
      if (selector == ROTATE)
        id = hyscan_gtk_ame_box_next_visible (abox);
      else
        hyscan_gtk_ame_box_set_visible (abox, id);

      panel = get_panel (_global, id);
      text = panel->name;
    }

  markup = g_strdup_printf ("<small><b>%s</b></small>", text);

  gtk_label_set_markup (GTK_LABEL (global_ui.current_view), text);
  g_free (markup);

  // gtk_revealer_set_reveal_child (GTK_REVEALER (global.gui.bott_revealer), selector == W_FORWARDL);
}

void
switch_page (GObject     *emitter,
             const gchar *page)
{
  AmeUI *ui = &global_ui;
  GtkStack * lstack = GTK_STACK (ui->lstack);
  GtkStack * rstack = GTK_STACK (ui->rstack);
  // GtkRevealer * sidebar = GTK_REVEALER (ui->left_revealer);
  // GtkRevealer * bottbar = GTK_REVEALER (ui->bott_revealer);

  // HyScanAmeFixed * rold = HYSCAN_AME_FIXED (gtk_stack_get_visible_child (rstack));
  // const gchar * old = gtk_stack_get_visible_child_name (lstack);

  // if (g_str_equal (old, "И_ПФд"))
  //   hyscan_ame_fixed_set_state (rold, 4, FALSE);
  // if (g_str_equal (old, "И_ГБОд"))
  //   hyscan_ame_fixed_set_state (rold, 4, FALSE);

  g_message ("Opening page <%s>", page);

  gtk_stack_set_visible_child_name (lstack, page);
  gtk_stack_set_visible_child_name (rstack, page);

  /*
  if (g_str_equal (page, "ГАЛС"))
    gtk_revealer_set_reveal_child (sidebar, TRUE);
  else if (g_str_equal (page, "И_ГБОм"))
    {
      gtk_revealer_set_reveal_child (sidebar, TRUE);
      turn_marks (NULL, W_SIDESCAN);
    }
  else if (g_str_equal (page, "И_ПФм"))
    {
      gtk_revealer_set_reveal_child (sidebar, TRUE);
      turn_marks (NULL, W_PROFILER);
    }
  else
    {
      gtk_revealer_set_reveal_child (sidebar, FALSE);
      turn_marks (NULL, W_LAST);
      turn_meter (NULL, FALSE, W_LAST);
    }


  if (g_str_equal (page, "И_ГК") ||
      g_str_equal (page, "И_ГКд"))
    gtk_revealer_set_reveal_child (bottbar, TRUE);
  else
    gtk_revealer_set_reveal_child (bottbar, FALSE);
  */
}

// GtkTreeView *
// list_scroll_tree_view_resolver (gint list)
// {
//   GtkTreeView * view = NULL;

//   if (list == L_MARKS)
//     view = hyscan_gtk_project_viewer_get_tree_view (HYSCAN_GTK_PROJECT_VIEWER (global.gui.mark_view));
//   else if (list == L_TRACKS)
//     view = global.gui.track_view;
//   else
//     g_message ("Wrong list selector passed (%i)", list);

//   return view;
// }

// void
// list_scroll_up (GObject *emitter,
//                 gpointer udata)
// {
//   GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
//   track_scroller (view, TRUE, FALSE);
// }

// void
// list_scroll_down (GObject *emitter,
//                   gpointer udata)
// {
//   GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
//   track_scroller (view, FALSE, FALSE);
// }

// void
// list_scroll_start (GObject *emitter,
//                    gpointer udata)
// {
//   GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
//   track_scroller (view, TRUE, TRUE);
// }
// void
// list_scroll_end (GObject *emitter,
//                  gpointer udata)
// {
//   GtkTreeView *view = list_scroll_tree_view_resolver (GPOINTER_TO_INT(udata));
//   track_scroller (view, FALSE, TRUE);
// }

/* Самая крутая функция, строит весь уй. */
gboolean
build_interface (Global *global)
{
  AmeUI *ui = &global_ui;
  _global = global;
  /* Кнопошный гуй представляет собой центральную область из 3 виджетов,
   * две боковых области с кнопками. Внизу выезжает управление ВСЛ,
   * слева выезжает контроль над галсами и метками. */

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
    GtkWidget * track_view = GTK_WIDGET (global->gui.track_view);
    GtkWidget * mark_list = GTK_WIDGET (global->gui.mark_view);
    GtkWidget * mark_editor = GTK_WIDGET (global->gui.meditor);

    g_object_set (track_view, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                                "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mark_list, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                            "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mark_editor, "vexpand", FALSE, "valign", GTK_ALIGN_END,
                               "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);

    gtk_box_pack_start (GTK_BOX (ui->left_box), track_view, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (ui->left_box), mark_list, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (ui->left_box), mark_editor, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (ui->left_revealer), ui->left_box);
  }

  /* Собираем воедино. */
                                                                  /* L  T  W  H */
  gtk_grid_attach (GTK_GRID (global->gui.grid), ui->lstack,          0, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (global->gui.grid), ui->rstack,          1, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (global->gui.grid), ui->left_revealer,   2, 0, 1, 3);
  gtk_grid_attach (GTK_GRID (global->gui.grid), ui->acoustic,        4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (global->gui.grid), ui->bott_revealer,   4, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (global->gui.grid), global->gui.nav,     4, 2, 1, 1);

   // gtk_container_add (GTK_CONTAINER (global.gui.bott_revealer), fl_play_control);

   // gtk_revealer_set_reveal_child (GTK_REVEALER (global.gui.bott_revealer), FALSE);

   // build_all (common_pages, GTK_STACK (global.gui.lstack), GTK_STACK (global.gui.rstack));
   // build_all (sonar_pages, GTK_STACK (global.gui.lstack), GTK_STACK (global.gui.rstack));



   return TRUE;
}

