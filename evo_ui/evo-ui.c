#include <gmodule.h>
#include <hyscan-gtk-area.h>
#include <hyscan-gtk-fnn-offsets.h>
#include "evo-settings.h"
#include "evo-sensors.h"
#include "evo-ui.h"

#define GETTEXT_PACKAGE "libhyscanfnn-evoui"
#include <glib/gi18n-lib.h>

// #define EVO_EMPTY_PAGE "empty_page"
#define EVO_NOT_MAP "evo-not-map"
#define EVO_MAP "evo-map"
#define EVO "evo"
#define EVO_MAP_CENTER_LAT "lat"
#define EVO_MAP_CENTER_LON "lon"
#define EVO_MAP_DIR "/tmp/"
#define EVO_MAP_PROFILE "profile"


EvoUI global_ui = {0,};
Global *_global = NULL;

/* OVERRIDES */
gboolean
evo_brightness_set_override (Global  *global,
                             gdouble  new_brightness,
                             gdouble  new_black,
                             gint     panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gchar *text_bright;
  gchar *text_black;
  gdouble b, g, w;
  FnnPanel *panel;

  panel = get_panel (global, panelx);

  if (new_brightness < 0.0)
    return FALSE;
  if (new_brightness > 100.0)
    return FALSE;
  if (new_black < 0.0)
    return FALSE;
  if (new_black > 100.0)
    return FALSE;

  text_bright = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_brightness);
  text_black = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_black);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
      {
        EvoUI *ui = &global_ui;
        GtkAdjustment *balance_adj;
        HyScanSourceType lsource, rsource;
        gdouble lw, rw, balance;

        w = 1 - 0.99 * new_brightness / 100.0;
        b = w * 0.99 * new_black / 100.0;
        g = 1.25 - 0.5 * (new_brightness / 100.0);


        balance_adj = g_hash_table_lookup (ui->balance_table, GINT_TO_POINTER (panelx));
        balance = gtk_adjustment_get_value (balance_adj);
        lw = balance < 0 ? b + (w - b) * (1 - 0.99 * ABS (balance)) : w;
        rw = balance > 0 ? b + (w - b) * (1 - 0.99 * ABS (balance)) : w;

        wf = (VisualWF*)panel->vis_gui;
        hyscan_gtk_waterfall_state_get_sources (HYSCAN_GTK_WATERFALL_STATE (wf->wf), &lsource, &rsource);
        hyscan_gtk_waterfall_set_levels (HYSCAN_GTK_WATERFALL (wf->wf), lsource, b, g, lw);
        hyscan_gtk_waterfall_set_levels (HYSCAN_GTK_WATERFALL (wf->wf), rsource, b, g, rw);
        gtk_label_set_markup (wf->common.brightness_value, text_bright);
        gtk_label_set_markup (wf->common.black_value, text_black);
        break;
      }

    case FNN_PANEL_ECHO:
      w = 1 - 0.99 * new_brightness / 100.0;
      b = w * new_black / 100.0;
      g = 1.25 - 0.5 * (new_brightness / 100.0);

      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w);
      gtk_label_set_markup (wf->common.brightness_value, text_bright);
      gtk_label_set_markup (wf->common.black_value, text_black);
      break;

    case FNN_PANEL_PROFILER:
      b = new_black / 250000;
      w = b + (1 - 0.99 * new_brightness / 100.0) * (1 - b);
      g = 1;
      if (b >= w)
        {
          g_message ("BBC error");
          return FALSE;
        }

      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w);

      gtk_label_set_markup (wf->common.brightness_value, text_bright);
      gtk_label_set_markup (wf->common.black_value, text_black);
      break;

    case FNN_PANEL_FORWARDLOOK:
      fl = (VisualFL*)panel->vis_gui;
      hyscan_gtk_forward_look_set_brightness (fl->fl, new_brightness);
      gtk_label_set_markup (fl->common.brightness_value, text_bright);
      break;

    default:
      g_warning ("brightness_set: wrong panel type!");
    }

  g_free (text_bright);
  g_free (text_black);

  return TRUE;
}

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
run_manager_wrapper (GObject *emitter,
                     gpointer data)
{
  EvoUI *ui = &global_ui;
  GtkToggleButton *tb = data;

  run_manager (emitter);

  hyscan_gtk_map_kit_set_project (ui->mapkit, _global->project_name);

  gtk_toggle_button_set_active (tb, FALSE);
}

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

void
balance_changed (GtkAdjustment *adj,
                 gpointer       udata)
{
  gint panelx = GPOINTER_TO_INT (udata);
  FnnPanel *panel = get_panel (_global, panelx);

  brightness_set (_global, panel->vis_current.brightness, panel->vis_current.black, panelx);
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

  gtk_stack_set_visible_child_name (GTK_STACK (ui->control_stack), child);

  if (g_str_equal (child, EVO_MAP))
    gtk_stack_set_visible_child_name (GTK_STACK (ui->nav_stack), EVO_MAP);
  else
    gtk_stack_set_visible_child_name (GTK_STACK (ui->nav_stack), EVO_NOT_MAP);
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
                       gint    panelx)
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

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:

      view = get_widget_from_builder (b, "ss_view_control");
      panel->vis_gui->brightness_value  = get_label_from_builder (b, "ss_brightness_value");
      panel->vis_gui->black_value       = get_label_from_builder (b, "ss_black_value");
      panel->vis_gui->scale_value       = get_label_from_builder (b, "ss_scale_value");
      panel->vis_gui->colormap_value    = get_label_from_builder (b, "ss_color_map_value");
      panel->vis_gui->live_view         = get_widget_from_builder(b, "ss_live_view");

      {
        GtkAdjustment *adj;
        GtkWidget *balance = get_widget_from_builder (b, "ss_balance_control");

        adj = GTK_ADJUSTMENT (gtk_builder_get_object (b, "ss_balance_adjustment"));
        gtk_scale_add_mark (GTK_SCALE (gtk_builder_get_object (b, "ss_balance_scale")),
                            0.0, GTK_POS_LEFT, NULL);
        g_signal_connect (adj, "value-changed", G_CALLBACK (balance_changed), GINT_TO_POINTER (panelx));

        gtk_box_pack_start (GTK_BOX (box), balance, FALSE, FALSE, 0);

        g_hash_table_insert (ui->balance_table, GINT_TO_POINTER (panelx), adj);
      }

      wf = (VisualWF*)panel->vis_gui;
      l_ctrl = get_widget_from_builder (b, "ss_control_layer");
      l_mark = get_widget_from_builder (b, "ss_marks_layer");
      l_meter = get_widget_from_builder (b, "ss_meter_layer");

      g_signal_connect_swapped (l_ctrl, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_ctrl);
      g_signal_connect_swapped (l_meter, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_metr);
      g_signal_connect_swapped (l_mark, "toggled", G_CALLBACK (hyscan_gtk_waterfall_layer_grab_input), wf->wf_mark);

      if (!panel_sources_are_in_sonar (global, panel))
      // if (global->control_s == NULL)
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

      // if (global->control_s == NULL)
      if (!panel_sources_are_in_sonar (global, panel))
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

      // if (global->control_s == NULL)
      if (!panel_sources_are_in_sonar (global, panel))
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

      // if (global->control_s == NULL)
      if (!panel_sources_are_in_sonar (global, panel))
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
  GtkWidget *rbox;

  global->override.brightness_set = evo_brightness_set_override;

  /* EvoUi это грид, вверху стек-свитчер с кнопками панелей,
   * внизу HyScanGtkArea. Внутри арии:
   * Справа у нас управление локаторами и отображением,
   * слева выезжает контроль над галсами и метками
   * (причем там тоже стек, т.к. у карты упр-е своё),
   * снизу выезжает управление ВСЛ.
   * под арией навиндикатор. */
  ui->grid = gtk_grid_new ();

  ui->area = hyscan_gtk_area_new ();

  /* Центральная зона: виджет с виджетами. Да. */
  ui->nav_stack = make_stack ();
  ui->acoustic_stack = make_stack ();

  g_signal_connect (ui->acoustic_stack, "notify::visible-child-name", G_CALLBACK (widget_swap), NULL);

  /* Cтек для управления локаторами. */
  rbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  ui->control_stack = make_stack ();

  /* Связываем стеки. */

  // g_object_bind_property (ui->acoustic_stack, "visible-child-name",
                          // ui->control_stack, "visible-child-name",
                          // G_BINDING_DEFAULT);

  /* Особенности реализации: хеш-таблица билдеров.*/
  ui->builders = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

  /* На данный момент все контейнеры готовы. Можно наполнять их. */

  /* Карта всегда в наличии. */
  {
    HyScanGeoGeodetic center = {0, 0};

    ui->mapkit = hyscan_gtk_map_kit_new (&center, global->db, ui->tile_cache);
    hyscan_gtk_map_kit_set_project (ui->mapkit, global->project_name);
    hyscan_gtk_map_kit_load_profiles (ui->mapkit, ui->profile_dir);
    hyscan_gtk_map_kit_add_marks_wf (ui->mapkit);
    hyscan_gtk_map_kit_add_marks_geo (ui->mapkit);

    if (global->control != NULL)
      hyscan_gtk_map_kit_add_nav (ui->mapkit, HYSCAN_SENSOR (global->control), "nmea", 0);

    gtk_stack_add_titled (GTK_STACK (ui->acoustic_stack), ui->mapkit->map, EVO_MAP, _("Map"));
    gtk_stack_add_named (GTK_STACK (ui->control_stack), ui->mapkit->control, EVO_MAP);
    gtk_stack_add_named (GTK_STACK (ui->nav_stack), ui->mapkit->navigation, EVO_MAP);
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
    GtkWidget * lbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget * tracks = GTK_WIDGET (global->gui.track.view);
    GtkWidget * mlist = GTK_WIDGET (global->gui.mark_view);
    GtkWidget * meditor = GTK_WIDGET (global->gui.meditor);

    gtk_widget_set_margin_end (lbox, 6);
    gtk_widget_set_margin_top (lbox, 0);
    gtk_widget_set_margin_bottom (lbox, 0);

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

    gtk_stack_add_named (GTK_STACK (ui->nav_stack), lbox, EVO_NOT_MAP);
  }

  /* Правая панель -- бокс с упр. записью и упр. локаторами. */
  /* Кстати, мои личные виджеты используются только в этом месте.  */
  ui->balance_table = g_hash_table_new (g_direct_hash, g_direct_equal);
  {
    GtkWidget * record;

    gtk_widget_set_margin_start (rbox, 6);
    gtk_widget_set_margin_top (rbox, 0);
    gtk_widget_set_margin_bottom (rbox, 0);

    record = make_record_control (global, ui);

    gtk_box_pack_start (GTK_BOX (rbox), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (rbox), ui->control_stack, TRUE, TRUE, 0);
    if (record != NULL)
      gtk_box_pack_start (GTK_BOX (rbox), record, FALSE, FALSE, 0);
  }

  /* Настройки. */
    {
      GtkWidget *grid;
      GtkWidget * manager;

      settings = evo_settings_new ();
      grid = evo_settings_get_grid (EVO_SETTINGS (settings));
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);

      manager = gtk_button_new_with_label (_("Project Manager"));
      gtk_widget_set_hexpand(manager, TRUE);

      g_signal_connect (manager, "clicked", G_CALLBACK (run_manager), settings);
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
          gtk_grid_attach (GTK_GRID (grid), sensors, 0, 3, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), offsets, 0, 4, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), info,    0, 5, 1, 1);
          gtk_grid_attach (GTK_GRID (grid), params,  0, 6, 1, 1);
        }
    }

  /* Пакуем всё. */
  hyscan_gtk_area_set_central (HYSCAN_GTK_AREA (ui->area), ui->acoustic_stack);
  hyscan_gtk_area_set_left (HYSCAN_GTK_AREA (ui->area), ui->nav_stack);
  hyscan_gtk_area_set_right (HYSCAN_GTK_AREA (ui->area), rbox);

  gtk_container_add (GTK_CONTAINER (global->gui.window), ui->grid);
  gtk_grid_attach (GTK_GRID (ui->grid), ui->switcher, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (ui->grid), settings,     1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (ui->grid), ui->area,     0, 1, 2, 1);

  if (global->gui.nav != NULL)
    {
      gtk_orientable_set_orientation (GTK_ORIENTABLE (global->gui.nav), GTK_ORIENTATION_HORIZONTAL);
      gtk_box_set_spacing (GTK_BOX (global->gui.nav), 10);
      gtk_widget_set_halign (global->gui.nav, GTK_ALIGN_CENTER);

      gtk_grid_attach (GTK_GRID (ui->grid), global->gui.nav, 0, 2, 1, 1);
    }

  return TRUE;
}

G_MODULE_EXPORT void
destroy_interface (void)
{
  EvoUI *ui = &global_ui;

  g_hash_table_unref (ui->builders);

  g_clear_object (&ui->starter.all);
  g_clear_object (&ui->starter.dry);

  g_clear_pointer (&ui->mapkit, hyscan_gtk_map_kit_free);
}


G_MODULE_EXPORT gboolean
kf_config (GKeyFile *kf)
{
  gchar *tile_cache_dir;
  gchar *profile_dir;

  profile_dir = keyfile_string_read_helper (kf, EVO, "profile_dir");
  if (profile_dir == NULL)
    profile_dir = g_strdup ("./maps");

  tile_cache_dir = keyfile_string_read_helper (kf, EVO, "tile_cache_dir");
  if (tile_cache_dir == NULL)
    tile_cache_dir = g_build_filename (g_get_tmp_dir(), "hyscan499", NULL);

  global_ui.tile_cache = tile_cache_dir;
  global_ui.profile_dir = profile_dir;

  return TRUE;
}

G_MODULE_EXPORT gboolean
kf_setting (GKeyFile *kf)
{
  gchar *profile = NULL;
  EvoUI *ui = &global_ui;
  ui->settings = g_key_file_ref (kf);

  /* Подгружаем центр карты. */
  HyScanGeoGeodetic center;
  center.lat = keyfile_double_read_helper (kf, EVO_MAP, EVO_MAP_CENTER_LAT, 0.0);
  center.lon = keyfile_double_read_helper (kf, EVO_MAP, EVO_MAP_CENTER_LON, 0.0);
  profile = keyfile_string_read_helper (kf, EVO_MAP, EVO_MAP_PROFILE);
  hyscan_gtk_map_move_to (HYSCAN_GTK_MAP (ui->mapkit->map), center);

  if (profile != NULL)
    {
      hyscan_gtk_map_kit_set_profile_name (ui->mapkit, profile);
      g_free (profile);
    }

  return TRUE;
}

G_MODULE_EXPORT gboolean
kf_desetup (GKeyFile *kf)
{
  EvoUI *ui = &global_ui;
  Global *global = _global;
  GHashTableIter iter;
  gdouble balance;
  gpointer k, v;

  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, &k, &v)) /* panelx, fnnpanel */
    {
      FnnPanel *panel = v;
      GtkAdjustment *adj;

      if (panel->type != FNN_PANEL_WATERFALL)
        continue;

      adj = g_hash_table_lookup (ui->balance_table, k);
      balance = gtk_adjustment_get_value (adj);
      keyfile_double_write_helper (ui->settings, panel->name, "evo.balance", balance);
    }

  /* Запоминаем карту. */
  {
    gchar *proifle;
    HyScanGeoGeodetic geod;
    HyScanGeoCartesian2D  c2d;
    gdouble from_x, to_x, from_y, to_y;

    gtk_cifro_area_get_view (GTK_CIFRO_AREA (ui->mapkit->map), &from_x, &to_x, &from_y, &to_y);
    c2d.x = (to_x + from_x) / 2.0;
    c2d.y = (to_y + from_y) / 2.0;
    hyscan_gtk_map_value_to_geo (HYSCAN_GTK_MAP (ui->mapkit->map), &geod, c2d);

    keyfile_double_write_helper (kf, EVO_MAP, EVO_MAP_CENTER_LAT, geod.lat);
    keyfile_double_write_helper (kf, EVO_MAP, EVO_MAP_CENTER_LON, geod.lon);

    proifle = hyscan_gtk_map_kit_get_profile_name (ui->mapkit);
    keyfile_string_write_helper (kf, EVO_MAP, EVO_MAP_PROFILE, proifle);
    g_free (proifle);
  }

  g_key_file_unref (ui->settings);
  return TRUE;
}

void
panel_show (gint     panelx,
                  gboolean show)
{
  gboolean show_in_stack;
  GtkWidget *w;
  FnnPanel *panel = get_panel_quiet (_global, panelx);

  if (panel == NULL)
    {
      if (show)
        g_warning ("EvoUI: %i %i", __LINE__, panelx);
      return;
    }

  show_in_stack = (0 == g_strcmp0 (panel->name, global_ui.restoreable));

  w = gtk_stack_get_child_by_name (GTK_STACK (global_ui.acoustic_stack), panel->name);
  gtk_widget_set_visible (w, show);
  if (show_in_stack)
    gtk_stack_set_visible_child (GTK_STACK (global_ui.acoustic_stack), w);

  w = gtk_stack_get_child_by_name (GTK_STACK (global_ui.control_stack), panel->name);
  if (w != NULL)
    {
      gtk_widget_set_visible (w, show);
        if (show_in_stack)
      gtk_stack_set_visible_child (GTK_STACK (global_ui.control_stack), w);
    }
}


G_MODULE_EXPORT gboolean
panel_pack (FnnPanel *panel,
            gint      panelx)
{
  EvoUI *ui = &global_ui;
  Global *global = _global;
  GtkWidget *control;
  GtkWidget *visual;

  visual = panel->vis_gui->main;
  g_object_set (visual, "vexpand", TRUE, "valign", GTK_ALIGN_FILL,
                        "hexpand", TRUE, "halign", GTK_ALIGN_FILL, NULL);

  control = make_page_for_panel (ui, panel, GPOINTER_TO_INT (panelx), global);

  gtk_stack_add_titled (GTK_STACK (ui->control_stack), control, panel->name, panel->name_local);
  gtk_stack_add_titled (GTK_STACK (ui->acoustic_stack), visual, panel->name, panel->name_local);

  /* Нижняя панель содержит виджет управления впередсмотрящим. */
  if (panelx == X_FORWARDL)
  {
    VisualFL *fl = (VisualFL*)panel->vis_gui;
    hyscan_gtk_area_set_bottom (HYSCAN_GTK_AREA (ui->area), fl->play_control);
    gtk_widget_show_all(ui->area);
  }

  if (panel->type == FNN_PANEL_WATERFALL)
    {
      GtkAdjustment *adj;
      gdouble balance;

      balance = keyfile_double_read_helper (ui->settings, panel->name, "evo.balance", 0.0);
      adj = g_hash_table_lookup (ui->balance_table, GINT_TO_POINTER (panelx));
      gtk_adjustment_set_value (adj, balance);
    }

  return TRUE;
}

G_MODULE_EXPORT void
panel_adjust_visibility (HyScanTrackInfo *track_info)
{
  gboolean sidescan_v = FALSE;
  gboolean side_low_v = FALSE;
  gboolean profiler_v = FALSE;
  gboolean echosound_v = FALSE;
  gboolean forwardl_v = FALSE;
  guint i;

  for (i = 0; i < HYSCAN_SOURCE_LAST; ++i)
    {
      gboolean in_track_info = track_info != NULL && track_info->sources[i];
      gboolean in_sonar = _global->control != NULL && g_hash_table_lookup (_global->infos, GINT_TO_POINTER (i));

      if (!in_track_info && !in_sonar)
        continue;

      switch (i)
      {
        case HYSCAN_SOURCE_SIDE_SCAN_STARBOARD:      sidescan_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_PORT:           sidescan_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_LOW:  side_low_v = TRUE; break;
        case HYSCAN_SOURCE_SIDE_SCAN_PORT_LOW:       side_low_v = TRUE; break;
        case HYSCAN_SOURCE_PROFILER:                 profiler_v = TRUE; break;
        case HYSCAN_SOURCE_PROFILER_ECHO:            profiler_v = TRUE; break;
        case HYSCAN_SOURCE_ECHOSOUNDER:              echosound_v = TRUE; break;
        case HYSCAN_SOURCE_FORWARD_LOOK:             forwardl_v = TRUE; break;
      }
    }

  /* Показываем активированные страницы. */
  if (sidescan_v)
    panel_show (X_SIDESCAN, TRUE);
  if (side_low_v)
    panel_show (X_SIDE_LOW, TRUE);
  if (profiler_v)
    panel_show (X_PROFILER, TRUE);
  if (echosound_v)
    panel_show (X_ECHOSOUND, TRUE);
  if (forwardl_v)
    panel_show (X_FORWARDL, TRUE);

  /* Прячем неактивированные страницы. */
  if (!sidescan_v)
    panel_show (X_SIDESCAN, FALSE);
  if (!side_low_v)
    panel_show (X_SIDE_LOW, FALSE);
  if (!profiler_v)
    panel_show (X_PROFILER, FALSE);
  if (!echosound_v)
    panel_show (X_ECHOSOUND, FALSE);
  if (!forwardl_v)
    panel_show (X_FORWARDL, FALSE);

}
