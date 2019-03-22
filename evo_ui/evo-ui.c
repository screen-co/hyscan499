#include <gmodule.h>
#include <hyscan-gtk-area.h>
#include <hyscan-gtk-fnn-offsets.h>
#include "evo-settings.h"
#include "evo-sensors.h"
#include "evo-ui.h"
#include "evo-ui-overrides.h"

#define GETTEXT_PACKAGE "libhyscanfnn-evoui"
#include <glib/gi18n-lib.h>

EvoUI global_ui = {0,};
Global *_global = NULL;
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

gboolean
ui_start_stop (GtkSwitch *button,
               gboolean   state,
               gpointer   user_data)
{
  EvoUI *ui = &global_ui;
  gboolean status;

  set_dry (_global, FALSE);
  status = start_stop (_global, state);

  if (!status)
    return TRUE;

  gtk_widget_set_sensitive (ui->starter.dry, !state);

  return FALSE;
}

gboolean
ui_start_stop_dry (GtkSwitch *button,
                   gboolean   state,
                   gpointer   user_data)
{
  EvoUI *ui = &global_ui;
  gboolean status;

  set_dry (_global, TRUE);
  status = start_stop (_global, state);

  if (!status)
    return TRUE;

  gtk_widget_set_sensitive (ui->starter.all, !state);

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
  EvoUI *ui = &global_ui;
  const gchar *child;

  child = gtk_stack_get_visible_child_name (GTK_STACK (ui->acoustic_stack));
  if (child == NULL)
    return;

  /* Теперь всякие специальные случаи. */
  hyscan_gtk_area_set_bottom_visible (HYSCAN_GTK_AREA (ui->area),
                                      g_str_equal (child, "ForwardLook"));

}

void
run_offset_setup (GObject *emitter,
                  Global  *global)
{
  GtkWidget *dialog;

  dialog = hyscan_gtk_fnn_offsets_new (global->control, GTK_WINDOW (global->gui.window));
  gtk_widget_show_all (dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}

//   #     #    #    ### #     #
//   ##   ##   # #    #  ##    #
//   # # # #  #   #   #  # #   #
//   #  #  # #     #  #  #  #  #
//   #     # #######  #  #   # #
//   #     # #     #  #  #    ##
//   #     # #     # ### #     #

GtkWidget *
get_widget_from_builder (GtkBuilder * builder,
                         const gchar *name)
{
  GObject *obj = gtk_builder_get_object (builder, name);
  if (obj == NULL)
    {
      g_warning ("EvoUI: <%s> not found.", name);
      return NULL;
    }

  return GTK_WIDGET (obj);
}

GtkLabel *
get_label_from_builder (GtkBuilder * builder,
                        const gchar *name)
{
  return GTK_LABEL (get_widget_from_builder (builder, name));
}

GtkWidget *
make_stack (void)
{
  GtkWidget *stack;

  stack = gtk_stack_new ();
  gtk_stack_set_transition_type (GTK_STACK (stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 100);

  return stack;
}

GtkWidget *
make_record_control (Global *global,
                     EvoUI   *ui)
{
  GtkBuilder *b;
  GtkWidget *w;

  if (global->control_s == NULL)
    return NULL;

  b = gtk_builder_new_from_resource ("/org/evo/gtk/record.ui");

  w = g_object_ref (get_widget_from_builder (b, "record_control"));
  ui->starter.dry = g_object_ref (get_widget_from_builder (b, "start_stop_dry"));
  ui->starter.all = g_object_ref (get_widget_from_builder (b, "start_stop"));

  gtk_builder_add_callback_symbol (b, "ui_start_stop", G_CALLBACK (ui_start_stop));
  gtk_builder_add_callback_symbol (b, "ui_start_stop_dry", G_CALLBACK (ui_start_stop_dry));
  gtk_builder_connect_signals (b, global);

  g_object_unref (b);

  return w;
}


GtkBuilder *
get_builder_for_panel (EvoUI * ui,
                       gint   panelx)
{
  GtkBuilder * b;
  gpointer k = GINT_TO_POINTER (panelx);

  b = g_hash_table_lookup (ui->builders, k);

  if (b != NULL)
    return b;

  b = gtk_builder_new_from_resource ("/org/evo/gtk/evo.ui");
  gtk_builder_set_translation_domain (b, GETTEXT_PACKAGE);
  g_hash_table_insert (ui->builders, k, b);

  return b;
}

/* Ядро всего уйца. Подключает сигналы, достает виджеты. */
GtkWidget *
make_page_for_panel (EvoUI     *ui,
                     FnnPanel *panel,
                     gint      panelx,
                     Global   *global)
{
  GtkBuilder *b;
  GtkWidget *view = NULL, *sonar = NULL, *tvg = NULL;
  GtkWidget *box, *separ;
  GtkWidget *l_ctrl, *l_mark, *l_meter;
  VisualWF *wf;

  b = get_builder_for_panel (ui, panelx);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:

      view = get_widget_from_builder (b, "ss_view_control");
      panel->vis_gui->brightness_value  = get_label_from_builder (b, "ss_brightness_value");
      panel->vis_gui->black_value       = get_label_from_builder (b, "ss_black_value");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "ss_scale_value");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "ss_color_map_value");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "ss_live_view");

      wf = (VisualWF*)panel->vis_gui;
      l_ctrl = get_widget_from_builder (b, "ss_control_layer");
      l_mark = get_widget_from_builder (b, "ss_marks_layer");
      l_meter = get_widget_from_builder (b, "ss_meter_layer");

      g_signal_connect_swapped (l_ctrl, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_ctrl);
      g_signal_connect_swapped (l_meter, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_metr);
      g_signal_connect_swapped (l_mark, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_mark);

      if (global->control_s == NULL)
          break;

      sonar = get_widget_from_builder (b, "sonar_control");
      tvg = get_widget_from_builder (b, "auto_tvg_control");
      panel->gui.distance_value         = get_label_from_builder  (b, "distance_value");
      panel->gui.tvg_level_value        = get_label_from_builder  (b, "tvg_level_value");
      panel->gui.tvg_sens_value         = get_label_from_builder  (b, "tvg_sens_value");
      panel->gui.signal_value           = get_label_from_builder  (b, "signal_value");

      break;

    case FNN_PANEL_PROFILER:

      view = get_widget_from_builder (b, "pf_view_control");
      panel->vis_gui->brightness_value  = get_label_from_builder (b, "pf_brightness_value");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "pf_scale_value");
      panel->vis_gui->black_value       = get_label_from_builder (b, "pf_black_value");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "pf_live_view");

      wf = (VisualWF*)panel->vis_gui;
      l_ctrl = get_widget_from_builder (b, "pf_control_layer");
      l_mark = get_widget_from_builder (b, "pf_marks_layer");
      l_meter = get_widget_from_builder (b, "pf_meter_layer");

      g_signal_connect_swapped (l_ctrl, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_ctrl);
      g_signal_connect_swapped (l_meter, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_metr);
      g_signal_connect_swapped (l_mark, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_mark);

      if (global->control_s == NULL)
          break;

      sonar = get_widget_from_builder (b, "sonar_control");
      tvg = get_widget_from_builder (b, "lin_tvg_control");
      panel->gui.distance_value         = get_label_from_builder  (b, "distance_value");
      panel->gui.tvg_value              = get_label_from_builder  (b, "tvg_value");
      panel->gui.tvg0_value             = get_label_from_builder  (b, "tvg0_value");
      panel->gui.signal_value           = get_label_from_builder  (b, "signal_value");

      break;

    case FNN_PANEL_ECHO:

      view = get_widget_from_builder (b, "ss_view_control");
      panel->vis_gui->brightness_value  = get_label_from_builder (b, "ss_brightness_value");
      panel->vis_gui->black_value       = get_label_from_builder (b, "ss_black_value");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "ss_scale_value");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "ss_color_map_value");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "ss_live_view");

      wf = (VisualWF*)panel->vis_gui;
      l_ctrl = get_widget_from_builder (b, "ss_control_layer");
      l_mark = get_widget_from_builder (b, "ss_marks_layer");
      l_meter = get_widget_from_builder (b, "ss_meter_layer");

      g_signal_connect_swapped (l_ctrl, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_ctrl);
      g_signal_connect_swapped (l_meter, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_metr);
      g_signal_connect_swapped (l_mark, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_mark);

      if (global->control_s == NULL)
          break;

      sonar = get_widget_from_builder (b, "sonar_control");
      tvg = get_widget_from_builder (b, "lin_tvg_control");
      panel->gui.distance_value         = get_label_from_builder  (b, "distance_value");
      panel->gui.tvg_value              = get_label_from_builder  (b, "tvg_value");
      panel->gui.tvg0_value             = get_label_from_builder  (b, "tvg0_value");
      panel->gui.signal_value           = get_label_from_builder  (b, "signal_value");

      break;

    case FNN_PANEL_FORWARDLOOK:

      view = get_widget_from_builder (b, "fl_view_control");
      panel->vis_gui->brightness_value  = get_label_from_builder (b, "fl_brightness_value");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "fl_scale_value");
      panel->vis_gui->sensitivity_value = get_label_from_builder (b, "fl_sensitivity_value");

      if (global->control_s == NULL)
          break;

      sonar = get_widget_from_builder (b, "sonar_control");
      tvg = get_widget_from_builder (b, "lin_tvg_control");
      panel->gui.distance_value         = get_label_from_builder  (b, "distance_value");
      panel->gui.tvg_value              = get_label_from_builder  (b, "tvg_value");
      panel->gui.tvg0_value             = get_label_from_builder  (b, "tvg0_value");
      panel->gui.signal_value           = get_label_from_builder  (b, "signal_value");

      break;

    default:
      g_warning ("EvoUI: wrong panel type!");
    }

  gtk_builder_connect_signals (b, GINT_TO_POINTER (panelx));

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  gtk_box_pack_start (GTK_BOX (box), view, FALSE, FALSE, 0);
  if (sonar != NULL && tvg != NULL)
    {
      separ = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

      gtk_box_pack_start (GTK_BOX (box), separ, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (box), tvg, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (box), sonar, FALSE, TRUE, 0);
    }

  return box;
}

/* Самая крутая функция, строит весь уй. */
G_MODULE_EXPORT gboolean
build_interface (Global *global)
{
  EvoUI *ui = &global_ui;
  _global = global;

  GtkWidget *settings;
  GtkWidget *lbox, *rbox;

  global->override.brightness_set = evo_brightness_set_override;

  /* EvoUi это грид, вверху стек-свитчер с кнопками панелей,
   * внизу HyScanGtkArea. Внутри арии:
   * Справа у нас управление локаторами и отображением,
   * слева выезжает контроль над галсами и метками,
   * снизу выезжает управление ВСЛ. */
  ui->grid = gtk_grid_new ();

  ui->area = hyscan_gtk_area_new ();

  /* Центральная зона: виджет с виджетами. Да. */
  ui->acoustic_stack = make_stack ();

  g_signal_connect (ui->acoustic_stack, "notify::visible-child-name", G_CALLBACK (widget_swap), NULL);

  /* Коробки и стек для управления локаторами. */
  lbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  rbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  ui->control_stack = make_stack ();

  /* Связываем стеки. */
  g_object_bind_property (ui->acoustic_stack, "visible-child-name",
                          ui->control_stack, "visible-child-name",
                          G_BINDING_DEFAULT);

  /* Особенности реализации: хеш-таблица билдеров.*/
  ui->builders = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

  /* На данный момент все контейнеры готовы. Можно наполнять их. */

  /* Начнем с центра, добавим все отображаемые виджеты. */
  {
    #define N_PANELS 5
    gint order[N_PANELS]  = {X_SIDE_LOW, X_SIDESCAN, X_ECHOSOUND, X_PROFILER, X_FORWARDL};
    gint i, n;

    /* Проверяем размеры моего могучего списка панелей. */
    n = g_hash_table_size (global->panels);
    if (n > 5)
      g_warning ("Discovered %i panels. Please, check.", n);

    for (i = 0; i < N_PANELS; ++i)
      {
        GtkWidget *w;
        FnnPanel *panel = get_panel_quiet (global, order[i]);
        if (panel == NULL)
          continue;

        w = panel->vis_gui->main;

        g_object_set (w, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                         "hexpand", TRUE, "halign", GTK_ALIGN_FILL, NULL);

        gtk_stack_add_titled (GTK_STACK (ui->acoustic_stack), w, panel->name, panel->name_local);
      }

    hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (ui->area), ui->acoustic_stack);
  }

  /* Stack switcher */
  {
    ui->switcher = gtk_stack_switcher_new ();
    g_object_set (ui->switcher, "margin", 6,
                  "hexpand", TRUE, "halign", GTK_ALIGN_CENTER, NULL);
    gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (ui->switcher), GTK_STACK (ui->acoustic_stack));
  }

  /* Левая панель содержит список галсов, меток и редактор меток. */
  {
    GtkWidget * tracks = GTK_WIDGET (global->gui.track.view);
    GtkWidget * mlist = GTK_WIDGET (global->gui.mark_view);
    GtkWidget * meditor = GTK_WIDGET (global->gui.meditor);

    gtk_widget_set_margin_end (lbox, 6);
    gtk_widget_set_margin_top (lbox, 0);
    gtk_widget_set_margin_bottom (lbox, 0);

    gtk_orientable_set_orientation (GTK_ORIENTABLE (global->gui.nav), GTK_ORIENTATION_VERTICAL);

    g_object_set (global->gui.nav, "vexpand", FALSE,  "valign", GTK_ALIGN_FILL,
                          "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);

    g_object_set (tracks, "vexpand", FALSE,  "valign", GTK_ALIGN_FILL,
                          "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (mlist, "vexpand", FALSE,  "valign", GTK_ALIGN_FILL,
                         "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);
    g_object_set (meditor, "vexpand", FALSE, "valign", GTK_ALIGN_END,
                           "hexpand", FALSE, "halign", GTK_ALIGN_FILL, NULL);

    {
      gchar ** env;
      const gchar * param_set;
      GtkWidget * prm;

      env = g_get_environ ();
      param_set = g_environ_getenv (env, "HY_PARAM");
      if (param_set != NULL)
        {
          prm = gtk_button_new_with_label ("Hardware info");
          g_signal_connect (prm, "clicked", G_CALLBACK (run_param), "/");
          gtk_box_pack_start (GTK_BOX (lbox), prm, FALSE, FALSE, 0);
        }
      g_strfreev (env);
    }

    gtk_box_pack_start (GTK_BOX (lbox), tracks, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), mlist, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), meditor, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (lbox), global->gui.nav, FALSE, FALSE, 0);

    hyscan_gtk_area_set_left (HYSCAN_GTK_AREA (ui->area), lbox);
  }

  /* Нижняя панель содержит виджет управления впередсмотрящим. */
  {
    FnnPanel *panel = get_panel_quiet (global, X_FORWARDL);

    if (panel != NULL)
      {
        VisualFL *fl = (VisualFL*)panel->vis_gui;
        hyscan_gtk_area_set_bottom (HYSCAN_GTK_AREA (ui->area), fl->play_control);
      }
  }


  /* Правая панель -- бокс с упр. записью и упр. локаторами. */
  /* Кстати, мои личные виджеты используются только в этом месте.  */
  {
    GtkWidget * record;
    GHashTableIter iter;
    FnnPanel *panel;
    gpointer k;

    gtk_widget_set_margin_start (rbox, 6);
    gtk_widget_set_margin_top (rbox, 0);
    gtk_widget_set_margin_bottom (rbox, 0);

    g_hash_table_iter_init (&iter, global->panels);
    while (g_hash_table_iter_next (&iter, &k, (gpointer*)&panel))
      {
        GtkWidget *packable;

        packable = make_page_for_panel (ui, panel, GPOINTER_TO_INT (k), global);
        gtk_stack_add_titled (GTK_STACK (ui->control_stack), packable, panel->name, panel->name_local);
      }

    record = make_record_control (global, ui);

    gtk_box_pack_start (GTK_BOX (rbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (rbox), ui->control_stack, TRUE, TRUE, 0);
    if (record != NULL)
      gtk_box_pack_start (GTK_BOX (rbox), record, FALSE, FALSE, 0);

    hyscan_gtk_area_set_right (HYSCAN_GTK_AREA (ui->area), rbox);
  }

  /* Настройки. */
    {
      GtkWidget *grid;
      GtkWidget * manager;

      settings = evo_settings_new ();
      grid = evo_settings_get_grid (EVO_SETTINGS (settings));

      manager = gtk_button_new_with_label (_("Project Manager"));
      gtk_widget_set_hexpand (manager, TRUE);
      g_signal_connect (manager, "clicked", G_CALLBACK (run_manager), NULL);
      gtk_grid_attach (GTK_GRID (grid), manager, 0, 0, 1, 1);

      if (global->control != NULL)
        {
          GtkWidget * sensors, * offsets, * info, * params;
          sensors = evo_sensors_new (global->control);
          offsets = gtk_button_new_with_label ("Setup offsets");
          g_signal_connect (offsets, "clicked", G_CALLBACK (run_offset_setup), global);
          info = gtk_button_new_with_label ("Show sonar info");
          g_signal_connect (info, "clicked", G_CALLBACK (run_show_sonar_info), "/info");
          params = gtk_button_new_with_label ("Show sonar params");
          g_signal_connect (params, "clicked", G_CALLBACK (run_show_sonar_info), "/params");

          gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Sensors"), 0, 2, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), sensors, 0, 2, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), offsets, 0, 3, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), info,    0, 4, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), params,  0, 5, 1, 1);
        }
    }

  /* Пакуем всё. */
  gtk_container_add (GTK_CONTAINER (global->gui.window), ui->grid);
  gtk_grid_attach (GTK_GRID (ui->grid), ui->switcher, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (ui->grid), settings,     1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (ui->grid), ui->area,     0, 1, 2, 1);

  return TRUE;
}

G_MODULE_EXPORT void
destroy_interface (void)
{
  EvoUI *ui = &global_ui;

  g_hash_table_unref (ui->builders);

  g_clear_object (&ui->starter.all);
  g_clear_object (&ui->starter.dry);
}


G_MODULE_EXPORT gboolean
kf_config (GKeyFile *kf)
{
  return TRUE;
}

