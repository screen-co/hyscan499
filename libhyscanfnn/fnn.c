#include "fnn-types.h"
#include "fnn-types-common.h"
#include "hyscan-fnn-splash.h"
#include "hyscan-fnn-project.h"
#include "hyscan-fnn-button.h"
#include "hyscan-fnn-button.h"
#include <hyscan-gtk-param-tree.h>
#include <hyscan-gtk-param-cc.h>
#include <hyscan-gtk-export.h>
#include <hyscan-data-schema-builder.h>
#include <gmodule.h>
#include <math.h>

Global *tglobal;

void
source_informer (const gchar      *text,
                 HyScanSourceType  source)
{
  g_message ("  %s for %s", text, hyscan_source_get_id_by_type (source));
}

GArray *
make_svp_from_velocity (gdouble velocity)
{
  GArray *svp;
  HyScanSoundVelocity svp_val;

  svp = g_array_new (FALSE, FALSE, sizeof (HyScanSoundVelocity));
  svp_val.depth = 0.0;
  svp_val.velocity = velocity;
  g_array_insert_val (svp, 0, svp_val);

  return svp;
}

gboolean
fnn_ensure_panel (gint    panelx,
                  Global *global)
{
  GArray *svp;
  FnnPanel *panel;

  /* Вдруг панель уже есть? Тогда выходим вот отседова. */
  if (g_hash_table_contains (tglobal->panels, GINT_TO_POINTER (panelx)))
    return FALSE;

  /* Ну, нет так нет, на нет и суда нет, как говорится. */
  svp = make_svp_from_velocity (global->sound_velocity);

  panel = g_new0 (FnnPanel, 1);
  panel->panelx = panelx;
  /* Создаем панель. */
  if (panelx == X_SIDESCAN || panelx == X_SIDE_LOW || panelx == X_SIDE_HIGH)
    { /* ГБО */
      GtkWidget *main_widget;
      VisualWF *vwf = g_new0 (VisualWF, 1);

      panel->vis_gui = (VisualCommon*)vwf;
      panel->type = FNN_PANEL_WATERFALL;
      panel->sources = g_new0 (HyScanSourceType, 3);
      panel->sources[2] = HYSCAN_SOURCE_INVALID;

      if (panelx == X_SIDESCAN)
        {
          panel->name = g_strdup ("SideScan");
          panel->name_local = g_strdup (_("SideScan"));
          panel->short_name = g_strdup ("SS");

          panel->sources[0] = HYSCAN_SOURCE_SIDE_SCAN_PORT;
          panel->sources[1] = HYSCAN_SOURCE_SIDE_SCAN_STARBOARD;
        }
      else if (panelx == X_SIDE_LOW)
        {
          panel->name = g_strdup ("SideScanLow");
          panel->name_local = g_strdup (_("SideScanLow"));
          panel->short_name = g_strdup ("SSLow");

          panel->sources[0] = HYSCAN_SOURCE_SIDE_SCAN_PORT_LOW;
          panel->sources[1] = HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_LOW;
        }
      else if (panelx == X_SIDE_HIGH)
        {
          panel->name = g_strdup ("SideScanHigh");
          panel->name_local = g_strdup (_("SideScanHigh"));
          panel->short_name = g_strdup ("SSHigh");

          panel->sources[0] = HYSCAN_SOURCE_SIDE_SCAN_PORT_HI;
          panel->sources[1] = HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_HI;
        }

      vwf->colormaps = fnn_make_color_maps (FALSE);
      vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global->cache));
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

      main_widget = make_overlay (vwf->wf,
                                  &vwf->wf_grid, &vwf->wf_ctrl,
                                  &vwf->wf_mark, &vwf->wf_metr,
                                  &vwf->wf_play, &vwf->wf_magn,
                                  &vwf->wf_shad, &vwf->wf_coor,
                                  global->marks.model);

      g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (automove_switched), global);
      g_signal_connect (vwf->wf, "waterfall-zoom", G_CALLBACK (zoom_changed), GINT_TO_POINTER (panelx));

      hyscan_gtk_waterfall_state_sidescan (HYSCAN_GTK_WATERFALL_STATE (vwf->wf),
                                           panel->sources[0], panel->sources[1]);
      hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), global->ship_speed);
      hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
      hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
      hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

      vwf->common.main = main_widget;
      gtk_widget_show_all (main_widget);
    }

  else if (/*need_pf*/ panelx == X_PROFILER)
    { /* ПФ */
      GtkWidget *main_widget;
      VisualWF *vwf = g_new0 (VisualWF, 1);

      panel->name = g_strdup ("Profiler");
      panel->name_local = g_strdup (_("Profiler"));
      panel->short_name = g_strdup ("PF");
      panel->type = FNN_PANEL_PROFILER;

      panel->sources = g_new0 (HyScanSourceType, 3);
      panel->sources[0] = HYSCAN_SOURCE_PROFILER;
      panel->sources[1] = HYSCAN_SOURCE_PROFILER_ECHO;
      panel->sources[2] = HYSCAN_SOURCE_INVALID;

      panel->vis_gui = (VisualCommon*)vwf;

      vwf->colormaps = fnn_make_color_maps (TRUE);

      vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global->cache));
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

      main_widget = make_overlay (vwf->wf,
                                  &vwf->wf_grid, &vwf->wf_ctrl,
                                  &vwf->wf_mark, &vwf->wf_metr,
                                  &vwf->wf_play, &vwf->wf_magn,
                                  &vwf->wf_shad, &vwf->wf_coor,
                                  global->marks.model);

      hyscan_gtk_waterfall_grid_set_condence (vwf->wf_grid, 10.0);
      hyscan_gtk_waterfall_grid_set_grid_color (vwf->wf_grid, hyscan_tile_color_converter_c2i (32, 32, 32, 255));

      g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (automove_switched), global);
      g_signal_connect (vwf->wf, "waterfall-zoom", G_CALLBACK (zoom_changed), GINT_TO_POINTER (panelx));

      hyscan_gtk_waterfall_state_echosounder (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), PROFILER);
      hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), global->ship_speed / 10);
      hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
      hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
      hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);

      vwf->common.main = main_widget;

      gtk_widget_show_all (main_widget);
    }

  else if (/*need_es*/ panelx == X_ECHOSOUND || panelx == X_ECHO_HIGH || panelx == X_ECHO_LOW)
    { /* Эхолот */
      GtkWidget *main_widget;
      VisualWF *vwf = g_new0 (VisualWF, 1);

      panel->type = FNN_PANEL_ECHO;
      panel->sources = g_new0 (HyScanSourceType, 2);
      panel->sources[1] = HYSCAN_SOURCE_INVALID;

      if (panelx == X_ECHOSOUND)
        {
          panel->name = g_strdup ("Echosounder");
          panel->name_local = g_strdup (_("Echosounder"));
          panel->short_name = g_strdup ("ES");
          panel->sources[0] = HYSCAN_SOURCE_ECHOSOUNDER;
        }
      else if (panelx == X_ECHO_HIGH)
        {
          panel->name = g_strdup ("EchosounderHigh");
          panel->name_local = g_strdup (_("EchosounderHigh"));
          panel->short_name = g_strdup ("ESHigh");
          panel->sources[0] = HYSCAN_SOURCE_ECHOSOUNDER_HI;
        }
      else if (panelx == X_ECHO_LOW)
        {
          panel->name = g_strdup ("EchosounderLow");
          panel->name_local = g_strdup (_("EchosounderLow"));
          panel->short_name = g_strdup ("ESLow");
          panel->sources[0] = HYSCAN_SOURCE_ECHOSOUNDER_LOW;
        }

      panel->vis_gui = (VisualCommon*)vwf;

      vwf->colormaps = fnn_make_color_maps (FALSE);

      vwf->wf = HYSCAN_GTK_WATERFALL (hyscan_gtk_waterfall_new (global->cache));
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vwf->wf), FALSE);

      main_widget = make_overlay (vwf->wf,
                                  &vwf->wf_grid, &vwf->wf_ctrl,
                                  &vwf->wf_mark, &vwf->wf_metr,
                                  &vwf->wf_play, &vwf->wf_magn,
                                  &vwf->wf_shad, &vwf->wf_coor,
                                  global->marks.model);

      g_signal_connect (vwf->wf, "automove-state", G_CALLBACK (automove_switched), global);
      g_signal_connect (vwf->wf, "waterfall-zoom", G_CALLBACK (zoom_changed), GINT_TO_POINTER (panelx));

      hyscan_gtk_waterfall_state_echosounder (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), panel->sources[0]);
      hyscan_gtk_waterfall_state_set_ship_speed (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), global->ship_speed / 10);
      hyscan_gtk_waterfall_state_set_sound_velocity (HYSCAN_GTK_WATERFALL_STATE (vwf->wf), svp);
      hyscan_gtk_waterfall_set_automove_period (HYSCAN_GTK_WATERFALL (vwf->wf), 100000);
      hyscan_gtk_waterfall_set_regeneration_period (HYSCAN_GTK_WATERFALL (vwf->wf), 500000);
      hyscan_gtk_waterfall_grid_set_condence(vwf->wf_grid, 10);

      vwf->common.main = main_widget;

      gtk_widget_show_all (main_widget);
    }

  else if (/*need_fl*/ panelx == X_FORWARDL)
    { /* ВСЛ */
      GtkBuilder *fl_ui = gtk_builder_new ();
      gtk_builder_set_translation_domain (fl_ui, GETTEXT_PACKAGE);
      gtk_builder_add_from_resource (fl_ui, "/org/libhyscanfnn/gtk/hyscan-fnn-fl.ui", NULL);

      VisualFL *vfl = g_new0 (VisualFL, 1);

      panel->name = g_strdup ("ForwardLook");
      panel->name_local = g_strdup (_("ForwardLook"));
      panel->short_name = g_strdup ("FL");
      panel->type = FNN_PANEL_FORWARDLOOK;

      panel->sources = g_new0 (HyScanSourceType, 2);
      panel->sources[0] = HYSCAN_SOURCE_FORWARD_LOOK;
      panel->sources[1] = HYSCAN_SOURCE_INVALID;

      panel->vis_gui = (VisualCommon*)vfl;

      vfl->fl = HYSCAN_GTK_FORWARD_LOOK (hyscan_gtk_forward_look_new ());
      gtk_cifro_area_set_scale_on_resize (GTK_CIFRO_AREA (vfl->fl), TRUE);

      vfl->player = hyscan_gtk_forward_look_get_player (vfl->fl);
      vfl->coords = hyscan_fl_coords_new (vfl->fl, global->cache);

      /* Управление воспроизведением FL. */
      vfl->play_control = GTK_WIDGET (g_object_ref (gtk_builder_get_object (fl_ui, "fl_play_control")));
      vfl->position = GTK_SCALE (g_object_ref (gtk_builder_get_object (fl_ui, "position")));
      vfl->coords_label = GTK_LABEL (g_object_ref (gtk_builder_get_object (fl_ui, "fl_latlong")));

      g_signal_connect (vfl->coords, "coords", G_CALLBACK (fl_coords_callback), vfl->coords_label);
      gtk_builder_connect_signals (fl_ui, global);

      vfl->position_range = hyscan_gtk_forward_look_get_adjustment (vfl->fl);
      gtk_range_set_adjustment (GTK_RANGE (vfl->position), vfl->position_range);

      hyscan_forward_look_player_set_sv (vfl->player, global->sound_velocity);

      vfl->common.main = GTK_WIDGET (vfl->fl);

      g_object_unref(fl_ui);

      gtk_widget_show_all (GTK_WIDGET (vfl->fl));
      gtk_widget_show_all (GTK_WIDGET (vfl->play_control));
    }
  else
    {
      g_warning ("Not implemented %i", __LINE__);
      return TRUE;
    }

  g_hash_table_insert (global->panels, GINT_TO_POINTER (panelx), panel);
  g_array_free (svp, TRUE);

  /* Пакуем её в гуй. */
  if (!global->ui.pack (panel, panelx))
    return FALSE;

  /* Преднастройки. */
  if (global->settings != NULL)
    {
      panel->current.distance =    keyfile_double_read_helper (global->settings, panel->name, "sonar.distance", 50);
      panel->current.signal =      keyfile_double_read_helper (global->settings, panel->name, "sonar.signal", 0);
      panel->current.gain0 =       keyfile_double_read_helper (global->settings, panel->name, "sonar.gain0", 0);
      panel->current.gain_step =   keyfile_double_read_helper (global->settings, panel->name, "sonar.gain_step", 10);
      panel->current.level =       keyfile_double_read_helper (global->settings, panel->name, "sonar.level", 0.5);
      panel->current.sensitivity = keyfile_double_read_helper (global->settings, panel->name, "sonar.sensitivity", 0.6);
      panel->current.log_gain0 =   keyfile_double_read_helper (global->settings, panel->name, "sonar.log_gain0", 5);
      panel->current.log_beta =    keyfile_double_read_helper (global->settings, panel->name, "sonar.log_beta", 0.0);
      panel->current.log_alpha =   keyfile_double_read_helper (global->settings, panel->name, "sonar.log_alpha", 0.0);
      panel->current.const_gain0 = keyfile_double_read_helper (global->settings, panel->name, "sonar.const_gain0", 1);
      panel->current.mode =        keyfile_double_read_helper (global->settings, panel->name, "sonar.tvg_mode", HYSCAN_SONAR_TVG_MODE_NONE);

      panel->vis_current.black =       keyfile_double_read_helper (global->settings, panel->name, "cur_black2",       0);
      panel->vis_current.gamma =       keyfile_double_read_helper (global->settings, panel->name, "cur_gamma2",       0.45);
      panel->vis_current.white =       keyfile_double_read_helper (global->settings, panel->name, "cur_white2",       50.0);
      panel->vis_current.colormap =    keyfile_double_read_helper (global->settings, panel->name, "cur_color_map2",   0);
      panel->vis_current.sensitivity = keyfile_double_read_helper (global->settings, panel->name, "cur_sensitivity2", 8.0);

      if (panel->type == FNN_PANEL_WATERFALL ||
          panel->type == FNN_PANEL_ECHO ||
          panel->type == FNN_PANEL_PROFILER)
        {
          color_map_set (global, panel->vis_current.colormap, panelx);
        }
      else if (panel->type == FNN_PANEL_FORWARDLOOK)
        {
          sensitivity_set (global, panel->vis_current.sensitivity, panelx);
        }

      levels_set (global, panel->vis_current.black, panel->vis_current.gamma, panel->vis_current.white, panelx);
      scale_set (global, FALSE, panelx);

      /* Если нет локатора, нечего и задавать. */
      // if (global->control != NULL)
      if (panel_sources_are_in_sonar (global, panel))
        {
          /* Для локаторов мы ничего не задаем, но лейблы всёрно надо проинициализировать. */
          distance_label (panel, panel->current.distance);

          tvg_label (panel, panel->current.gain0, panel->current.gain_step);
          auto_tvg_label (panel, panel->current.level, panel->current.sensitivity);
          log_tvg_label (panel, panel->current.log_gain0, panel->current.log_beta, panel->current.log_alpha);
          const_tvg_label (panel, panel->current.const_gain0);
          auto_tvg_label (panel, panel->current.level, panel->current.sensitivity);
          tvg_set_preferred_mode (global, panel->current.mode, panelx);

          /* С сигналом чуть сложней, т.к. надо найти сигнал и вытащить из него имя. */
          {
            const HyScanDataSchemaEnumValue * signal;
            signal = signal_finder (global, panel, *panel->sources, panel->current.signal);
            if (signal == NULL)
              {
                panel->current.signal = 0;
                signal = signal_finder (global, panel, *panel->sources, panel->current.signal);
              }
            if (signal != NULL)
              signal_label (panel, signal->name);
          }
        }
    }

  return TRUE;
}


void
update_panels (Global          *global,
               HyScanTrackInfo *track_info)
{
  guint i;
  struct SrcToPan
  {
    HyScanSourceType source;
    gint             panelx;
  } src_to_pan[] =
  {
    {HYSCAN_SOURCE_SIDE_SCAN_STARBOARD,      X_SIDESCAN},
    {HYSCAN_SOURCE_SIDE_SCAN_PORT,           X_SIDESCAN},
    {HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_LOW,  X_SIDE_LOW},
    {HYSCAN_SOURCE_SIDE_SCAN_PORT_LOW,       X_SIDE_LOW},
    {HYSCAN_SOURCE_SIDE_SCAN_STARBOARD_HI,   X_SIDE_HIGH},
    {HYSCAN_SOURCE_SIDE_SCAN_PORT_HI,        X_SIDE_HIGH},
    {HYSCAN_SOURCE_PROFILER,                 X_PROFILER},
    {HYSCAN_SOURCE_PROFILER_ECHO,            X_PROFILER},
    {HYSCAN_SOURCE_ECHOSOUNDER,              X_ECHOSOUND},
    {HYSCAN_SOURCE_ECHOSOUNDER_HI,           X_ECHO_HIGH},
    {HYSCAN_SOURCE_ECHOSOUNDER_LOW,          X_ECHO_LOW},
    {HYSCAN_SOURCE_FORWARD_LOOK,             X_FORWARDL},
  };

  for (i = 0; i < G_N_ELEMENTS(src_to_pan); ++i)
    {
      struct SrcToPan s2p = src_to_pan[i];

      /* ХайСканСорсТайп есть либо в галсе, либо в ГЛ.
       * Либо его нет, но тогда и показывать нечего. */
      if (track_info != NULL && track_info->sources[s2p.source])
        fnn_ensure_panel(s2p.panelx, global);
      else if (global->control != NULL && g_hash_table_lookup (global->infos, GINT_TO_POINTER (s2p.source)))
        fnn_ensure_panel(s2p.panelx, global);
    }

  global->ui.adjust_visibility(track_info);
}

void
fnn_panel_destroy (gpointer data)
{
  FnnPanel *panel = data;
  VisualWF *wf;
  VisualFL *fl;

  if (panel == NULL)
    return;

  g_free (panel->name);
  g_free (panel->name_local);
  g_free (panel->short_name);
  g_free (panel->sources);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
    case FNN_PANEL_ECHO:
    case FNN_PANEL_PROFILER:
      wf = (VisualWF*)panel->vis_gui;

      g_array_unref (wf->colormaps);
      break;

    case FNN_PANEL_FORWARDLOOK:
      fl = (VisualFL*)panel->vis_gui;

      g_object_unref (fl->coords);
      break;
    }

  g_free (panel);
}

FnnPanel *
get_panel (Global *global,
           gint    panelx)
{
  FnnPanel * panel = get_panel_quiet (global, panelx);

  if (panel == NULL)
    g_warning ("Panel %i not found.", panelx);

  return panel;
}

FnnPanel *
get_panel_quiet (Global *global,
                 gint    panelx)
{
  return g_hash_table_lookup (global->panels, GINT_TO_POINTER (panelx));
}

gint
get_panel_id_by_name (Global      *global,
                      const gchar *name)
{
  GHashTableIter iter;
  gpointer k;
  FnnPanel *panel;

  g_return_val_if_fail (name != NULL, -1);

  g_hash_table_iter_init (&iter, global->panels);

  while (g_hash_table_iter_next (&iter, &k, (gpointer*)&panel))
    {
      if (g_str_equal (panel->name, name))
        return GPOINTER_TO_INT (k);
    }

  return -1;
}

void
button_set_active (GObject *object,
                   gboolean setting)
{
  if (object == NULL)
    return;

  g_object_set (object, "state", setting, NULL);
}

void
set_window_title (Global *global)
{
  gchar *title = g_strdup_printf ("HyScan - %s", global->project_name);
  gtk_window_set_title (GTK_WINDOW (global->gui.window), title);
  g_free (title);
}

gint
run_manager (GObject *emitter)
{
  HyScanDBInfo *info;
  GtkWidget *dialog;
  gint res;

  if (tglobal->sonar_model != NULL)
    start_stop (tglobal, NULL, FALSE);

  info = hyscan_db_info_new (tglobal->db);
  dialog = hyscan_fnn_project_new (tglobal->db, info, GTK_WINDOW (tglobal->gui.window));
  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res == HYSCAN_FNN_PROJECT_OPEN || res == HYSCAN_FNN_PROJECT_CREATE)
    {
      gchar *project;

      g_clear_pointer (&tglobal->project_name, g_free);
      hyscan_fnn_project_get (HYSCAN_FNN_PROJECT (dialog), &project, NULL);

      tglobal->project_name = g_strdup (project);
      hyscan_db_info_set_project (tglobal->db_info, project);

      g_clear_pointer (&tglobal->marks.loc_storage, g_hash_table_unref);
      tglobal->marks.loc_storage = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                        (GDestroyNotify) loc_store_free);

      hyscan_object_model_set_project (tglobal->marks.model, tglobal->db, project);

      if (tglobal->recorder != NULL)
        hyscan_sonar_recorder_set_project (tglobal->recorder, project);

      if (tglobal->override.project_changed != NULL)
        tglobal->override.project_changed (tglobal, project);

      set_window_title (tglobal);
      g_free (project);
    }

  g_object_unref (info);
  gtk_widget_destroy (dialog);

  return res;
}

void
run_param (GObject     *emitter,
           const gchar *root)
{
  GtkWidget *dialog, *content, *tree;
  gint res;

  dialog = gtk_dialog_new_with_buttons (_("Hardware parameters"),
                                        GTK_WINDOW (tglobal->gui.window),
                                        GTK_DIALOG_USE_HEADER_BAR,
                                        _("Apply"), GTK_RESPONSE_APPLY,
                                        _("Reset"), GTK_RESPONSE_CANCEL,
                                        _("Close"), GTK_RESPONSE_CLOSE,
                                        NULL);
  gtk_header_bar_set_title (GTK_HEADER_BAR (gtk_dialog_get_header_bar (GTK_DIALOG (dialog))),
                            _("Hardware parameters"));
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  tree = hyscan_gtk_param_tree_new (HYSCAN_PARAM (tglobal->control), root, TRUE);
  hyscan_gtk_param_set_watch_period (HYSCAN_GTK_PARAM (tree), 200);

  gtk_container_add (GTK_CONTAINER (content), tree);
  gtk_widget_set_size_request (dialog, 800, 600);
  gtk_widget_show_all (dialog);

  while (TRUE)
    {
      res = gtk_dialog_run (GTK_DIALOG (dialog));

      if (res == GTK_RESPONSE_APPLY)
        hyscan_gtk_param_apply (HYSCAN_GTK_PARAM (tree));
      else if (res == GTK_RESPONSE_CANCEL)
        hyscan_gtk_param_discard (HYSCAN_GTK_PARAM (tree));
      else
        break;
    };

  gtk_widget_destroy (dialog);
}

void
run_show_sonar_info (GObject     *emitter,
                     const gchar *root)
{
  GtkWidget *dialog, *content, *cc;

  dialog = gtk_dialog_new_with_buttons (_("Sonar info"),
                                        GTK_WINDOW (tglobal->gui.window),
                                        GTK_DIALOG_USE_HEADER_BAR,
                                        _("Close"), GTK_RESPONSE_CLOSE,
                                        NULL);
  gtk_header_bar_set_title (GTK_HEADER_BAR (gtk_dialog_get_header_bar (GTK_DIALOG (dialog))),
                            _("Sonar info"));
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  cc = hyscan_gtk_param_cc_new (HYSCAN_PARAM (tglobal->control), root, TRUE);
  hyscan_gtk_param_set_watch_period (HYSCAN_GTK_PARAM (cc), 200);

  gtk_container_add (GTK_CONTAINER (content), cc);
  gtk_widget_set_size_request (dialog, 800, 600);
  gtk_widget_show_all (dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));
  hyscan_gtk_param_discard (HYSCAN_GTK_PARAM (cc));
  gtk_widget_destroy (dialog);
}

void
turn_meter_helper (FnnPanel  *panel,
                   gboolean   state)
{
  void * layer = NULL;
  VisualWF *wf;

  /* Только вотерфольные панели. */
  if (panel->type != FNN_PANEL_WATERFALL &&
      panel->type != FNN_PANEL_ECHO &&
      panel->type != FNN_PANEL_PROFILER)
      {
        return;
      }

  wf = (VisualWF*)panel->vis_gui;

  layer = state ? (void*)wf->wf_metr : (void*)wf->wf_ctrl;
  hyscan_gtk_layer_grab_input (HYSCAN_GTK_LAYER (layer));
}

void
turn_meter (GObject     *emitter,
            gboolean     state,
            gint         panelx)
{

  GHashTableIter iter;
  gpointer k, v;

  /* Если передан конкретный селектор, пробуем найти страницу для него. */
  v = g_hash_table_lookup (tglobal->panels, GINT_TO_POINTER (panelx));
  if (v != NULL)
    {
      turn_meter_helper ((FnnPanel*)v, state);
    }
  else
    {
      g_hash_table_iter_init (&iter, tglobal->panels);
      while (g_hash_table_iter_next (&iter, &k, &v))
        turn_meter_helper ((FnnPanel*)v, state);
    }
}

void
turn_mark_helper (FnnPanel  *panel,
                  gboolean   state)
{
  void * layer = NULL;
  VisualWF *wf;

  /* Только вотерфольные панели. */
  if (panel->type != FNN_PANEL_WATERFALL &&
      panel->type != FNN_PANEL_ECHO &&
      panel->type != FNN_PANEL_PROFILER)
    {
      return;
    }

  wf = (VisualWF*)panel->vis_gui;

  layer = state ? (void*)wf->wf_mark : (void*)wf->wf_ctrl;
  hyscan_gtk_layer_grab_input (HYSCAN_GTK_LAYER (layer));
}

void
turn_marks (GObject     *emitter,
            gint         panelx)
{
  GHashTableIter iter;
  gpointer k, v;

  /* Сначала везде включаем вфконтрол, а потом в требуемом включаем метки. */
  g_hash_table_iter_init (&iter, tglobal->panels);
  while (g_hash_table_iter_next (&iter, &k, &v))
    turn_mark_helper ((FnnPanel*)v, FALSE);

  v = g_hash_table_lookup (tglobal->panels, GINT_TO_POINTER (panelx));
  if (v != NULL)
    turn_mark_helper ((FnnPanel*)v, TRUE);
}

void
hide_marks (GObject     *emitter,
            gboolean     state,
            gint         selector)
{
  gpointer v;

  v = g_hash_table_lookup (tglobal->panels, GINT_TO_POINTER (selector));
  if (v != NULL)
    {
      FnnPanel *panel = v;
      VisualWF *wf;

      if (panel->type != FNN_PANEL_WATERFALL &&
          panel->type != FNN_PANEL_ECHO &&
          panel->type != FNN_PANEL_PROFILER)
        return;
      wf = (VisualWF*)panel->vis_gui;

      hyscan_gtk_layer_set_visible (HYSCAN_GTK_LAYER (wf->wf_mark), state);
    }

}

/* Функция изменяет режим окна full screen. */
gboolean
key_press (GtkWidget   *widget,
           GdkEventKey *event,
           Global      *global)
{
  if (event->keyval != GDK_KEY_F11)
    return FALSE;

  if (global->full_screen)
    gtk_window_unfullscreen (GTK_WINDOW (global->gui.window));
  else
    gtk_window_fullscreen (GTK_WINDOW (global->gui.window));

  global->full_screen = !global->full_screen;

  return FALSE;
}

gboolean
idle_key (GdkEvent *event)
{
  gtk_main_do_event (event);
  gdk_event_free (event);
  g_message ("Event! %i %i", event->key.keyval, event->key.state);
  return G_SOURCE_REMOVE;
}

void
nav_common (GtkWidget *target,
            guint      keyval,
            guint      state)
{
  GdkEvent *event;

  event = gdk_event_new (GDK_KEY_PRESS);
  event->key.keyval = keyval;
  event->key.state = state;

  gtk_window_set_focus (GTK_WINDOW (tglobal->gui.window), target);
  event->key.window = g_object_ref (gtk_widget_get_window (target));
  g_idle_add ((GSourceFunc)idle_key, (gpointer)event);
}

/* Макроселло для нав. ф-ий, чтобы их не плодить без надобности. */
#define NAV_FN(fname, button)                                                \
void                                                                         \
fname (GObject * emitter,                                                    \
       gpointer  udata)                                                      \
{                                                                            \
  VisualWF *wf;                                                              \
  VisualFL *fl;                                                              \
  GtkWidget *widget = NULL;                                                  \
  FnnPanel *panel = get_panel (tglobal, GPOINTER_TO_INT (udata));            \
  switch (panel->type)                                                       \
    {                                                                        \
      case FNN_PANEL_WATERFALL:                                              \
      case FNN_PANEL_PROFILER:                                               \
      case FNN_PANEL_ECHO:                                                   \
        wf = (VisualWF*)panel->vis_gui;                                      \
        widget = GTK_WIDGET (wf->wf);                                        \
        break;                                                               \
      case FNN_PANEL_FORWARDLOOK:                                            \
        fl = (VisualFL*)panel->vis_gui;                                      \
        widget = GTK_WIDGET (fl->fl);                                        \
        break;                                                               \
      default:                                                               \
        g_message ("Nav not available");                                     \
        return;                                                              \
    }                                                                        \
  nav_common (widget, button, GDK_CONTROL_MASK);                             \
}

NAV_FN (nav_del, GDK_KEY_Delete)
NAV_FN (nav_pg_up, GDK_KEY_Page_Up)
NAV_FN (nav_pg_down, GDK_KEY_Page_Down)
NAV_FN (nav_up, GDK_KEY_Up)
NAV_FN (nav_down, GDK_KEY_Down)
NAV_FN (nav_left, GDK_KEY_Left)
NAV_FN (nav_right, GDK_KEY_Right)

void
fl_prev (GObject *emitter,
         gint     panelx)
{
  gdouble value;
  GtkAdjustment *adj;
  VisualFL *fl;
  FnnPanel *panel = get_panel (tglobal, panelx);

  fl = (VisualFL*)panel->vis_gui;
  adj = fl->position_range;

  value = gtk_adjustment_get_value (adj);
  gtk_adjustment_set_value (adj, value - 10);
}

void
fl_next (GObject *emitter,
         gint     panelx)
{
  gdouble value;
  GtkAdjustment *adj;
  VisualFL *fl;
  FnnPanel *panel = get_panel (tglobal, panelx);

  fl = (VisualFL*)panel->vis_gui;
  adj = fl->position_range;

  value = gtk_adjustment_get_value (adj);
  gtk_adjustment_set_value (adj, value + 10);
}

/* Функция вызывается при изменении списка проектов. */
void
projects_changed (HyScanDBInfo *db_info,
                  Global       *global)
{
  GHashTable *projects = hyscan_db_info_get_projects (db_info);

  /* Если рабочий проект есть в списке, мониторим его. */
  if (global->project_name != NULL)
    if (g_hash_table_lookup (projects, global->project_name))
      hyscan_db_info_set_project (db_info, global->project_name);

  g_hash_table_unref (projects);
}

void
run_export_data (GObject     *emitter,
                 gpointer     from_ui_params)
{
  Global *gl = from_ui_params;

  GtkWidget *dialog, *content, *hsx_export;

  dialog = gtk_dialog_new_with_buttons (_("Export data"),
                                        GTK_WINDOW (tglobal->gui.window),
                                        GTK_DIALOG_USE_HEADER_BAR,
                                        _("Close"), GTK_RESPONSE_CLOSE,
                                        NULL);
  gtk_header_bar_set_title (GTK_HEADER_BAR (gtk_dialog_get_header_bar (GTK_DIALOG (dialog))),
                            _("Export data"));
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  hsx_export = hyscan_gtk_export_new (gl->db, gl->project_name, gl->track_name);
  hyscan_gtk_export_set_watch_period (HYSCAN_GTK_EXPORT (hsx_export), 100);

  gtk_container_add (GTK_CONTAINER (content), hsx_export);
  gtk_widget_set_size_request (dialog, 800, 600);
  gtk_widget_show_all (dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

GtkTreePath *
fnn_gtk_tree_model_get_last_path (GtkTreeView *tree)
{
  GtkTreeModel * model;
  GtkTreeIter cur, prev;

  model = gtk_tree_view_get_model (tree);

  /* Чтобы узнать последний элемент, я прохожусь по
   * всем, и запоминаю последний валидный итер. */
  gtk_tree_model_get_iter_first (model, &cur);
  do {
    prev = cur;
  } while (gtk_tree_model_iter_next (model, &cur));

  return gtk_tree_model_get_path (model, &prev);
}

GtkTreePath *
fnn_gtk_tree_path_prev (GtkTreePath *path)
{
  gtk_tree_path_prev (path);
  return path;
}

GtkTreePath *
fnn_gtk_tree_path_next (GtkTreePath *path)
{
  gtk_tree_path_next (path);
  return path;
}

void
track_scroller (GtkTreeView *tree,
                gboolean     to_top,
                gboolean     to_end)
{
  GtkTreePath *path = NULL; /* Не освобождать! */
  GtkTreePath *real = NULL;
  GtkTreePath *last = NULL;
  GtkTreePath *first = NULL;

  /* Текущий выбранный элемент, первый и последний. */
  gtk_tree_view_get_cursor (tree, &real, NULL);
  first = gtk_tree_path_new_first ();
  last = fnn_gtk_tree_model_get_last_path (tree);

  if (to_top) /* Вверх. */
    {
      if (real == NULL || to_end)
        path = first;
      else if (0 == gtk_tree_path_compare (first, real))
        path = last;
      else
        path = fnn_gtk_tree_path_prev (real);
    }
  else /* Вниз. */
    {
      if (real == NULL || to_end)
        path = last;
      else if (0 == gtk_tree_path_compare (last, real))
        path = first;
      else
        path = fnn_gtk_tree_path_next (real);
    }

  gtk_tree_view_set_cursor (tree, path, NULL, FALSE);

  gtk_tree_path_free (real);
  gtk_tree_path_free (last);
  gtk_tree_path_free (first);
}

/* Функция вызывается при изменении списка галсов. */
void
tracks_changed (HyScanDBInfo *db_info,
                Global       *global)
{
  GtkTreePath *null_path;
  GtkTreeIter tree_iter;
  GHashTable *tracks;
  GHashTableIter hash_iter;
  gpointer key, value;
  gchar *cur_track_name;

  cur_track_name = g_strdup (global->track_name);

  null_path = gtk_tree_path_new ();
  gtk_tree_view_set_cursor (global->gui.track.tree, null_path, NULL, FALSE);
  gtk_tree_path_free (null_path);

  gtk_list_store_clear (GTK_LIST_STORE (global->gui.track.list));

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&hash_iter, tracks);

  while (g_hash_table_iter_next (&hash_iter, &key, &value))
    {
      GDateTime *local;
      gchar *time_str;
      HyScanTrackInfo *track_info = value;

      if (track_info->id == NULL)
        {
          g_message ("Unable to open track <%s>", track_info->name);
          continue;
        }
      /* Добавляем в список галсов. */
      local = g_date_time_to_local (track_info->ctime);
      time_str = g_date_time_format (local, "%d.%m %H:%M");

      gtk_list_store_append (GTK_LIST_STORE (global->gui.track.list), &tree_iter);
      gtk_list_store_set (GTK_LIST_STORE (global->gui.track.list), &tree_iter,
                          FNN_DATE_SORT_COLUMN, g_date_time_to_unix (track_info->ctime),
                          FNN_TRACK_COLUMN, track_info->name,
                          FNN_DATE_COLUMN, time_str,
                          -1);

      /* Подсвечиваем текущий галс. */
      if (g_strcmp0 (cur_track_name, track_info->name) == 0)
        {
          GtkTreePath *tree_path = gtk_tree_model_get_path (global->gui.track.list, &tree_iter);
          gtk_tree_view_set_cursor (global->gui.track.tree, tree_path, NULL, FALSE);
          gtk_tree_path_free (tree_path);

          update_panels (global, track_info);
        }

      g_free (time_str);
      g_date_time_unref (local);
    }

  g_hash_table_unref (tracks);
  g_free (cur_track_name);
}

void
active_mark_changed (HyScanGtkProjectViewer *marks_viewer,
                     Global                 *global)
{
  const gchar *mark_id;
  MarkAndLocation *mark;

  mark_id = hyscan_gtk_project_viewer_get_selected_item (marks_viewer);

  if ((mark = g_hash_table_lookup (global->marks.current, mark_id)) != NULL)
    {
      hyscan_gtk_mark_editor_set_mark (HYSCAN_GTK_MARK_EDITOR (global->gui.meditor),
                                       mark_id,
                                       mark->mark->name,
                                       mark->mark->operator_name,
                                       mark->mark->description,
                                       mark->lat, mark->lon);
    }
}

inline gboolean
fnn_float_equal (gdouble a, gdouble b)
{
  return (ABS (a - b) < 1e-6);
}

gboolean
marks_equal (MarkAndLocation *a,
             MarkAndLocation *b)
{
  gboolean coords;
  gboolean name;
  gboolean size;

  if (a == NULL || b == NULL)
    {
      g_warning ("Null mark. It's a mistake");
      return FALSE;
    }

  coords = fnn_float_equal (a->lat, b->lat) && fnn_float_equal (a->lon, b->lon);
  // name = g_strcmp0 (a->mark->name, b->mark->name) == 0;
  name = g_strcmp0 (a->mark->name, b->mark->name) == 0;
  size = (a->mark->width == b->mark->width) && (a->mark->height == b->mark->height);

  return (coords && name && size);
}

void
mark_processor (gpointer        key,
                gpointer        value,
                GHashTable     *source,
                HyScanMarkSync *sync,
                gboolean        this_is_an_old_list)
{
  MarkAndLocation *our, *their;

  our = value;
  their = g_hash_table_lookup (source, key);

  /* Логика различается для текущего и предыдущего списка меток.
   * Если ищем в старом, то ненайденную создаем.
   * Если ищем в новом, то ненайденную удаляем. */
  if (this_is_an_old_list && their == NULL)
    {
      hyscan_mark_sync_remove (sync, key);
      return;
    }
  else if (this_is_an_old_list && their != NULL)
    {
      return;
    }

  if (their == NULL)
    {
      gdouble radius = sqrt (pow (our->mark->width, 2) + pow (our->mark->height, 2)) / 1e3;
      hyscan_mark_sync_set (sync, key, our->mark->name, our->lat, our->lon, radius);
      return;
    }

  /* Если была, но изменилась - удалить и переотправить*/
  if (!marks_equal (our, their))
    {
      gdouble radius = sqrt (pow (our->mark->width, 2) + pow (our->mark->height, 2)) / 1e3;
      hyscan_mark_sync_remove (sync, key);
      hyscan_mark_sync_set (sync, key, our->mark->name, our->lat, our->lon, radius);
      return;
    }
}

void
mark_sync_func (GHashTable *marks,
                Global     *global)
{
  GHashTableIter iter;
  gpointer key, value;

  if (global->marks.previous == NULL)
    {
      global->marks.previous = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                      (GDestroyNotify)mark_and_location_free);
      goto copy;
    }

  /* Надо пройтись сначала по старому, чтобы знать, что удалилось.
   * А потом по актуальному списку, посмотреть,
   * что появилось нового, а что поменялось */
  if (global->marks.sync != NULL)
    {
      g_hash_table_iter_init (&iter, global->marks.previous);
      while (g_hash_table_iter_next (&iter, &key, &value))
        mark_processor (key, value, marks, global->marks.sync, TRUE);

      g_hash_table_iter_init (&iter, marks);
      while (g_hash_table_iter_next (&iter, &key, &value))
        mark_processor (key, value, global->marks.previous, global->marks.sync, FALSE);
    }

  g_hash_table_remove_all (global->marks.previous);

  copy:
  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_hash_table_insert (global->marks.previous, g_strdup (key), mark_and_location_copy (value));
}


static gchar *
get_track_name_by_id (HyScanDBInfo *info,
                      const gchar  *id)
{
  GHashTable *tracks; /* HyScanTrackInfo. */
  HyScanTrackInfo *value;
  gchar *key;
  gchar *retval = NULL;
  GHashTableIter iter;

  tracks = hyscan_db_info_get_tracks (info);
  g_hash_table_iter_init (&iter, tracks);

  while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&value))
    {
      if (0 == g_strcmp0 (value->id, id))
        {
          retval = g_strdup (key);
          break;
        }
    }

  g_hash_table_unref (tracks);
  return retval;
}

/* создает LocStore. */
LocStore *
loc_store_new (HyScanDB    *db,
               HyScanCache *cache,
               const gchar *project,
               const gchar *track)
{
  LocStore *loc;

  loc = g_new0 (LocStore, 1);
  loc->track = g_strdup (track);
  loc->mloc = hyscan_mloc_new (db, cache, project, track);
  loc->projectors = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
                                           (GDestroyNotify)g_object_unref);
  loc->factory = hyscan_factory_amplitude_new (cache);
  hyscan_factory_amplitude_set_project (loc->factory, db, project);

  if (loc->mloc == NULL || loc->factory == NULL)
    {
      g_clear_pointer (&loc, loc_store_free);
    }

  return loc;
}

/* Возвращает (или создает и возврщает) прожектор для галса и источника. */
HyScanProjector *
get_projector (GHashTable       *locstores,
               gchar            *track,
               HyScanSourceType  source,
               HyScanmLoc       **mloc,
               HyScanAmplitude  **_amp)
{
  LocStore * store;
  HyScanProjector * pj;
  HyScanAmplitude * amp;
  gpointer pj_key = GINT_TO_POINTER (source);
  gpointer amp_key = GINT_TO_POINTER (10007 * source);

  /* Ищем хранилище проекторов для галса. */
  store = g_hash_table_lookup (locstores, track);

  if (store == NULL)
    return NULL;

  /* Ищем мини локейшн. */
  if (mloc != NULL)
    *mloc = store->mloc;

  /* Ищем проектор по источнику. */
  pj = g_hash_table_lookup (store->projectors, pj_key);

  /* Если не нашли, создаем с помощью фабрики. */
  if (pj == NULL)
    {
      amp = hyscan_factory_amplitude_produce (store->factory, store->track, source);

      if (amp == NULL)
        {
          return NULL;
        }

      pj = hyscan_projector_new (amp);
      if (pj == NULL)
        {
          g_object_unref (amp);
          return NULL;
        }

      /* Закидываем проектор и амплитуддата в таблицу. */
      g_hash_table_insert (store->projectors, pj_key, pj);
      g_hash_table_insert (store->projectors, amp_key, amp);
    }

  amp = g_hash_table_lookup (store->projectors, amp_key);

  if (_amp != NULL)
    *_amp = amp;

  return pj;
}

/* Освобождение структуры LocStore */
void
loc_store_free (gpointer data)
{
  LocStore *loc = data;

  if (loc == NULL)
    return;

  g_clear_pointer (&loc->track, g_free);
  g_clear_object  (&loc->mloc);
  g_clear_object  (&loc->factory);
  g_clear_pointer (&loc->projectors, g_hash_table_unref);
}

/* Создание структуры MarkAndLocation */
MarkAndLocation *
mark_and_location_new (HyScanMarkWaterfall *mark,
                       gboolean             ok,
                       gdouble              lat,
                       gdouble              lon)
{
  MarkAndLocation *dst = g_new (MarkAndLocation, 1);
  dst->mark = hyscan_mark_waterfall_copy (mark);
  dst->ok  = ok;
  dst->lat = ok ? lat : 0.0;
  dst->lon = ok ? lon : 0.0;

  return dst;
}

/* Копирование структуры MarkAndLocation */
MarkAndLocation *
mark_and_location_copy (gpointer data)
{
  MarkAndLocation *src = data;
  MarkAndLocation *dst = g_new (MarkAndLocation, 1);
  dst->mark = hyscan_mark_waterfall_copy (src->mark);
  dst->ok  = src->ok;
  dst->lat = src->lat;
  dst->lon = src->lon;

  return dst;
}

/* Освобождение структуры MarkAndLocation */
void
mark_and_location_free (gpointer data)
{
  MarkAndLocation *loc = data;
  hyscan_mark_waterfall_free (loc->mark);
  g_free (loc);
}

void
remove_mark_update (Global *global)
{
  if (global->marks.request_update_tag == 0)
    return;

  g_source_remove (global->marks.request_update_tag);
  global->marks.request_update_tag = 0;
}

gboolean
mark_update (Global *global)
{
  mark_model_changed (global->marks.model, global);
  global->marks.request_update_tag = 0;
  return G_SOURCE_REMOVE;
}

gboolean
add_mark_update (Global *global)
{
  remove_mark_update (global);

  global->marks.request_update_tag = g_timeout_add (1000, (GSourceFunc)mark_update, global);

  return TRUE;
}

/* Функция определяет координаты метки. */
MarkAndLocation *
get_mark_coords (GHashTable          * locstores,
                 HyScanMarkWaterfall * mark,
                 Global              * global)
{
  HyScanSourceType source;
  gdouble across;
  HyScanProjector *pj = NULL;
  HyScanAmplitude *amp = NULL;
  HyScanAntennaOffset apos;
  gint64 time;
  guint32 n;
  HyScanGeoGeodetic position;
  HyScanmLoc *mloc;

  if (mark->track == NULL)
    {
      return NULL;
    }

  /* Определяем название галса по айди. */
  if (!g_hash_table_contains (locstores, mark->track))
    {
      LocStore *ls;
      gchar * track_name = NULL;

      track_name = get_track_name_by_id (global->db_info, mark->track);
      if (track_name == NULL)
        {
          add_mark_update (global);
          return NULL;
        }

      ls = loc_store_new (global->db, global->cache, global->project_name, track_name);

      if (ls == NULL)
        {
          add_mark_update (global);
          return NULL;
        }

      g_hash_table_insert (locstores, g_strdup (mark->track), ls);
    }

  source = hyscan_source_get_type_by_id (mark->source);
  pj = get_projector (locstores, mark->track, source, &mloc, &amp);

  if (pj == NULL || amp == NULL)
    return NULL;

  // hyscan_projector_index_to_coord (pj, mark->index0, &along);
  hyscan_projector_count_to_coord (pj, mark->count, &across, 0);

  apos = hyscan_amplitude_get_offset (amp);
  hyscan_amplitude_get_amplitude (amp, NULL, mark->index, &n, &time, NULL);

  if (source == HYSCAN_SOURCE_SIDE_SCAN_PORT)
    across *= -1;

  if (!hyscan_mloc_get (mloc, NULL, time, &apos, 0, across, 0, &position))
    return NULL;

  return mark_and_location_new (mark, TRUE, position.lat, position.lon);
}

/* Возвращает таблицу меток с координатами. */
GHashTable *
make_marks_with_coords (HyScanObjectModel *model,
                        Global            *global)
{
  GHashTable *pure_marks;
  GHashTable *marks;
  GHashTableIter iter;
  MarkAndLocation *mark_ll;
  gpointer key, value;

  /* Достаем голенькие метки. */
  if ((pure_marks = hyscan_object_model_get (model)) == NULL)
    return NULL;

  /* Заводим таблицу под метки. */
  marks = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)mark_and_location_free);

  /* Теперь приделываем к ним координаты. */
  g_hash_table_iter_init (&iter, pure_marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      HyScanMarkWaterfall *wfmark = value;
      mark_ll = get_mark_coords (global->marks.loc_storage, wfmark, global);

      if (mark_ll != NULL)
        g_hash_table_insert (marks, g_strdup (key), mark_ll);
    }

  return marks;
}

void
mark_model_changed (HyScanObjectModel *model,
                    Global            *global)
{
  GtkTreeIter tree_iter;
  GHashTable *marks;
  GHashTableIter iter;
  gpointer key, value;
  GtkListStore *ls;

  hyscan_gtk_project_viewer_clear (HYSCAN_GTK_PROJECT_VIEWER (global->gui.mark_view));

  /* Получаем крутецкий список меток с координатами. */
  marks = make_marks_with_coords (model, global);

  ls = hyscan_gtk_project_viewer_get_liststore (HYSCAN_GTK_PROJECT_VIEWER (global->gui.mark_view));

  /* Проходим по всем существующим меткам... */
  g_hash_table_iter_init (&iter, marks);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const gchar *mark_id = key;
      MarkAndLocation *mark_ll = value;
      GDateTime *mtime;
      gchar *mtime_str;

      mtime = g_date_time_new_from_unix_local (mark_ll->mark->mtime / 1e6);
      mtime_str = g_date_time_format (mtime, "%d.%m %H:%M");

      gtk_list_store_append (ls, &tree_iter);
      gtk_list_store_set (ls, &tree_iter,
                          0, mark_id,
                          1, mark_ll->mark->name,
                          2, mtime_str,
                          3, mark_ll->mark->mtime,
                          -1);

      g_free (mtime_str);
      g_date_time_unref (mtime);
    }

  g_clear_pointer (&global->marks.current, g_hash_table_unref);
  global->marks.current = marks;

  /* Сравниваем с предыдущими метками. Если надо, отсылаем наружу. */
  mark_sync_func (marks, global);
}

void
mark_modified (HyScanGtkMarkEditor *med,
               Global              *global)
{
  gchar *mark_id = NULL;
  GHashTable *marks;
  HyScanMarkWaterfall *mark;

  hyscan_gtk_mark_editor_get_mark (med, &mark_id, NULL, NULL, NULL);

  if ((marks = hyscan_object_model_get (global->marks.model)) == NULL)
    return;

  if ((mark = g_hash_table_lookup (marks, mark_id)) != NULL)
    {
      HyScanMarkWaterfall *modified_mark = hyscan_mark_waterfall_copy (mark);
      HyScanMarkWaterfall *wfmark = modified_mark;

      g_clear_pointer (&wfmark->name, g_free);
      g_clear_pointer (&wfmark->operator_name, g_free);
      g_clear_pointer (&wfmark->description, g_free);

      hyscan_gtk_mark_editor_get_mark (med,
                                       NULL,
                                       &wfmark->name,
                                       &wfmark->operator_name,
                                       &wfmark->description);

      hyscan_object_model_modify_object (global->marks.model, mark_id, (const HyScanObject *) modified_mark);

      hyscan_mark_waterfall_free (modified_mark);
    }

  g_free (mark_id);
  g_hash_table_unref (marks);
}

/* Функция прокручивает список галсов. */
gboolean
track_scroll (GtkWidget *widget,
              GdkEvent  *event,
              Global    *global)
{
  gdouble position;
  gdouble step_x, step_y;
  gint size_x, size_y;

  position = gtk_adjustment_get_value (global->gui.track.scroll);
  gdk_event_get_scroll_deltas (event, &step_x, &step_y);
  gtk_tree_view_convert_bin_window_to_widget_coords (global->gui.track.tree, 0, 0, &size_x, &size_y);
  position += step_y * size_y;

  gtk_adjustment_set_value (global->gui.track.scroll, position);

  return TRUE;
}

gchar *
get_active_track (HyScanDBInfo *db_info)
{
  GHashTable *tracks;
  HyScanTrackInfo *current;
  GHashTableIter iter;
  gpointer key, val;
  gchar * active = NULL;

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&iter, tracks);
  while (g_hash_table_iter_next (&iter, &key, &val))
    {
      current = val;

      if (current->record)
        {
          active = g_strdup (current->name);
          break;
        }

    }

  g_hash_table_unref (tracks);
  return active;
}

gboolean
track_is_active (HyScanDBInfo *db_info,
                 const gchar  *name)
{
  GHashTable *tracks;
  HyScanTrackInfo *current;
  GHashTableIter iter;
  gpointer key, val;
  gboolean active = FALSE;

  tracks = hyscan_db_info_get_tracks (db_info);
  g_hash_table_iter_init (&iter, tracks);
  while (g_hash_table_iter_next (&iter, &key, &val))
    {
      current = val;

      if (g_str_equal (current->name, name))
        {
          active = current->record;
          break;
        }

    }

  g_hash_table_unref (tracks);
  return active;
}

/* Обработчик изменения галса. */
void
track_changed (GtkTreeView *list,
               Global      *global)
{
  GValue value = G_VALUE_INIT;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  GHashTableIter htiter;
  const gchar *track_name;
  gpointer k, v;
  gboolean on_air;

  /* Определяем название нового галса. */
  gtk_tree_view_get_cursor (list, &path, NULL);
  if (path == NULL)
    return;

  if (!gtk_tree_model_get_iter (global->gui.track.list, &iter, path))
    return;

  g_clear_pointer (&global->track_name, g_free);

  /* Открываем новый галс. */
  gtk_tree_model_get_value (global->gui.track.list, &iter, FNN_TRACK_COLUMN, &value);
  track_name = g_value_get_string (&value);
  global->track_name = g_strdup (track_name);

  {
    GHashTable *tracks = hyscan_db_info_get_tracks (global->db_info);
    HyScanTrackInfo *info = g_hash_table_lookup(tracks, track_name);
    update_panels (global, info);
    g_hash_table_unref (tracks);
  }

  /* Выясняем, ведется ли в него запись. */
  on_air = track_is_active (global->db_info, track_name);

  g_hash_table_iter_init (&htiter, global->panels);
  while (g_hash_table_iter_next (&htiter, &k, &v))
    {
      VisualWF *wf;
      VisualFL *fl;
      FnnPanel *panel = v;

      switch (panel->type)
        {
        case FNN_PANEL_WATERFALL:
        case FNN_PANEL_ECHO:
        case FNN_PANEL_PROFILER:
          wf = (VisualWF*) (panel->vis_gui);
          hyscan_gtk_waterfall_state_set_track (HYSCAN_GTK_WATERFALL_STATE (wf->wf),
                                                global->db, global->project_name, track_name);

          /* Записываемый галс включаем в режиме автопрокрутки. */
          hyscan_gtk_waterfall_automove (HYSCAN_GTK_WATERFALL (wf->wf), on_air);

          break;

        case FNN_PANEL_FORWARDLOOK:
          fl = (VisualFL*) (panel->vis_gui);
          hyscan_forward_look_player_open (fl->player, global->db, global->cache,
                                           global->project_name, track_name);
          hyscan_fl_coords_set_project (fl->coords, global->db, global->project_name, track_name);

          if (on_air)
            hyscan_forward_look_player_real_time (fl->player);
          else
            hyscan_forward_look_player_pause (fl->player);

          break;
        }
    }

  if (global->override.track_changed != NULL)
    global->override.track_changed (global, track_name);

  g_value_unset (&value);
}

gdouble
fl_current_scale (HyScanGtkForwardLook *fl)
{
  gdouble from_y, to_y, min_y, max_y, scale;
  GtkCifroArea *carea;

  carea = GTK_CIFRO_AREA (fl);

  gtk_cifro_area_get_view (carea, NULL, NULL, &from_y, &to_y);
  gtk_cifro_area_get_limits (carea, NULL, NULL, &min_y, &max_y);

  if (max_y == min_y)
    return -1;

  scale = 100.0 * (fabs (to_y - from_y) / fabs (max_y - min_y));

  if (isnan (scale) || isinf (scale))
    return -1;

  return scale;
}

/* Функция устанавливает масштаб отображения. */
void
zoom_changed (HyScanGtkWaterfall *wfall,
              gint                index,
              gdouble             gost,
              gint                panelx)
{
  gchar *text = NULL;
  FnnPanel *panel = get_panel (tglobal, panelx);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
    case FNN_PANEL_ECHO:
    case FNN_PANEL_PROFILER:
      break;

    case FNN_PANEL_FORWARDLOOK:
    default:
      g_warning ("zoom_changed: wrong panel type!");
      return;
    }

  text = g_strdup_printf ("<small><b>1:%.0f</b></small>", gost);
  gtk_label_set_markup (panel->vis_gui->scale_value, text);
  g_free (text);
  return;
}

/* Функция устанавливает масштаб отображения. */
gboolean
scale_set (Global   *global,
           gboolean  scale_up,
           gint      panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gchar *text = NULL;
  const gdouble *scales;
  gint n_scales, i_scale;
  gdouble scale;
  FnnPanel *panel = get_panel (global, panelx);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
    case FNN_PANEL_ECHO:
    case FNN_PANEL_PROFILER:
      wf = (VisualWF*)panel->vis_gui;
      hyscan_gtk_waterfall_control_zoom (wf->wf_ctrl, scale_up);

      i_scale = hyscan_gtk_waterfall_get_scale (HYSCAN_GTK_WATERFALL (wf->wf),
                                                &scales, &n_scales);
      scale = scales[i_scale];
      text = g_strdup_printf ("<small><b>1:%.0f</b></small>", scale);
      break;

    case FNN_PANEL_FORWARDLOOK:
      {
        GtkCifroArea *carea;
        GtkCifroAreaZoomType zoom_dir;

        fl = (VisualFL*)panel->vis_gui;
        carea = GTK_CIFRO_AREA (fl->fl);

        scale = fl_current_scale (fl->fl);

        if (scale_up && (scale < 5.0))
          return FALSE;

        if (!scale_up && (scale > 100.0))
          return FALSE;

        zoom_dir = (scale_up) ? GTK_CIFRO_AREA_ZOOM_IN : GTK_CIFRO_AREA_ZOOM_OUT;
        gtk_cifro_area_zoom (carea, zoom_dir, zoom_dir, 0.0, 0.0);

        scale = fl_current_scale (fl->fl);
        if (scale == -1)
          text = g_strdup_printf ("<small><b>N/A</b></small>");
        else
          text = g_strdup_printf ("<small><b>%.0f%%</b></small>", scale);
      }
      break;

    default:
      g_warning ("scale_set: wrong panel type!");
    }

  gtk_label_set_markup (panel->vis_gui->scale_value, text);
  g_free (text);
  return TRUE;
}

#define INCR(color,next) if (incr_##color)   \
                    {                        \
                      if (++color == 255)      \
                        {incr_##color = FALSE;\
                         next = TRUE; continue;} \
                    }
#define DECR(color,next) if (decr_##color)   \
                    {                        \
                      if (--color == 0)      \
                        {decr_##color = FALSE;\
                         next = TRUE; continue;} \
                    }
guint32*
hyscan_tile_color_compose_colormap_pf (guint *length)
{
  guint32 *out;
  guint len = 1022;
  len = 1120; //m_edition
  guint i;
  gboolean incr_g = FALSE;
  gboolean decr_b = FALSE;
  gboolean incr_r = FALSE;
  gboolean decr_g = FALSE;
  gboolean decr_r = TRUE;
  guchar r = 0, g = 0, b = 255;
  out = g_malloc0 (len * sizeof (guint32));

  out[0] = hyscan_tile_color_converter_c2i (255, 255, 255, 255);

  r=99;
  g=0;
  b=255;

  for (i = 1; i < len; ++i)
    {
      out[i] = hyscan_tile_color_converter_c2i (r, g, b, 255);
      DECR (r, incr_g);
      INCR (g, decr_b);
      DECR (b, incr_r);
      INCR (r, decr_g);
      DECR (g, decr_g);
    }

  if (length != NULL)
    *length = len;
  return out;
}

/* Функция устанавливает новую палитру. */
gboolean
color_map_set (Global *global,
               guint   desired_cmap,
               gint    panelx)
{
  VisualWF *wf;
  gchar   *text;
  FnnColormap *colormap;
  FnnPanel *panel = get_panel (global, panelx);

  /* Проверяем тип панели. */
  if (panel->type != FNN_PANEL_WATERFALL &&
      panel->type != FNN_PANEL_PROFILER &&
      panel->type != FNN_PANEL_ECHO)
    {
      g_warning ("color_map_set: wrong panel type");
      return FALSE;
    }

  wf = (VisualWF*)panel->vis_gui;

  if (desired_cmap >= wf->colormaps->len)
    return FALSE;

  colormap = g_array_index (wf->colormaps, FnnColormap*, desired_cmap);

  hyscan_gtk_waterfall_set_colormap_for_all (wf->wf,
                                             colormap->colors,
                                             colormap->len,
                                             colormap->bg);
  hyscan_gtk_waterfall_set_substrate (wf->wf, colormap->bg);

  /* Проверяем, существует ли вообще виджет. */
  if (wf->common.colormap_value != NULL)
    {
      text = g_strdup_printf ("<small><b>%s</b></small>", colormap->name);
      gtk_label_set_markup (wf->common.colormap_value, text);
      g_free (text);
    }

  return TRUE;
}

void
color_map_up (GtkWidget *widget,
              gint       panelx)
{
  guint desired_cmap;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired_cmap = panel->vis_current.colormap + 1;
  if (color_map_set (tglobal, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_up failed");
}

void
color_map_down (GtkWidget *widget,
                gint       panelx)
{
  guint desired_cmap;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired_cmap = panel->vis_current.colormap - 1;
  if (color_map_set (tglobal, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_down failed");
}

void
color_map_cyclic (GtkWidget *widget,
                  gint       panelx)
{
  guint desired_cmap;
  VisualWF *wf;
  FnnPanel *panel = get_panel (tglobal, panelx);

  if (panel->type != FNN_PANEL_WATERFALL &&
      panel->type != FNN_PANEL_PROFILER &&
      panel->type != FNN_PANEL_ECHO)
    {
      g_warning ("color_map_cyclic: wrong panel type");
      return;
    }

  wf = (VisualWF*)panel->vis_gui;
  desired_cmap = panel->vis_current.colormap + 1;
  if (desired_cmap >= wf->colormaps->len)
    desired_cmap = 0;

  if (color_map_set (tglobal, desired_cmap, panelx))
    panel->vis_current.colormap = desired_cmap;
  else
    g_warning ("color_map_cyclic failed");
}

/* Функция устанавливает порог чувствительности. */
void
sensitivity_label (FnnPanel *panel,
                   gdouble   sens)
{
  VisualFL *fl = (VisualFL*)panel->vis_gui;
  gchar *text;

  if (fl->common.sensitivity_value == NULL)
    return;

  text = g_strdup_printf ("<small><b>%.0f</b></small>", sens);
  gtk_label_set_markup (fl->common.sensitivity_value, text);
  g_free (text);
}

gboolean
sensitivity_set (Global  *global,
                 gdouble  desired_sensitivity,
                 guint    panelx)
{
  VisualFL *fl;
  FnnPanel *panel = get_panel (global, panelx);

  if (desired_sensitivity < 1.0)
   return FALSE;
  if (desired_sensitivity > 10.0)
   return FALSE;

  if (panel->type != FNN_PANEL_FORWARDLOOK)
    {
      g_warning ("sensitivity_set: wrong panel type!");
      return FALSE;
    }


  fl = (VisualFL*)panel->vis_gui;
  hyscan_gtk_forward_look_set_sensitivity (fl->fl, desired_sensitivity);
  gtk_widget_queue_draw (GTK_WIDGET (fl->fl));

  sensitivity_label (panel, desired_sensitivity);

  return TRUE;
}

void
sensitivity_up (GtkWidget *widget,
                gint       panelx)
{
  gdouble desired_sensitivity;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired_sensitivity = panel->vis_current.sensitivity + 1;
  if (sensitivity_set (tglobal, desired_sensitivity, panelx))
    panel->vis_current.sensitivity = desired_sensitivity;
  else
    g_warning ("sens_up failed");
}

void
sensitivity_down (GtkWidget *widget,
                  gint       panelx)
{
  gdouble desired_sensitivity;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired_sensitivity = panel->vis_current.sensitivity - 1;
  if (sensitivity_set (tglobal, desired_sensitivity, panelx))
    panel->vis_current.sensitivity = desired_sensitivity;
  else
    g_warning ("sens_down failed");
}

const HyScanDataSchemaEnumValue *
signal_finder (Global           *global,
               FnnPanel         *panel,
               HyScanSourceType  source,
               gint              n)
{
  HyScanSonarInfoSource * info;
  GList *link;

  /* Информация об источнике. */
  info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
  hyscan_return_val_if_fail (info != NULL && info->presets != NULL, NULL);

  /* Ищем сигнал. */
  link = g_list_nth (info->presets, n);
  if (link == NULL)
    return NULL;

  return (HyScanDataSchemaEnumValue*) link->data;
}

void
signal_label (FnnPanel    *panel,
              const gchar *name)
{
  gchar *text;

  if (panel->gui.signal_value != NULL)
    {
      text = g_strdup_printf ("<small><b>%s</b></small>", name);
      gtk_label_set_markup (panel->gui.signal_value, text);
      g_free (text);
    }
}

gboolean
rec_disable (Global   *global,
             gint      panelx)
{
  HyScanSourceType *iter;
  FnnPanel *panel = get_panel_quiet (global, panelx);

  g_message ("rec_disable: %s (%i)", panel->name, panelx);

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      source_informer ("  disabling receiver", *iter);
      status = hyscan_sonar_receiver_disable (HYSCAN_SONAR (global->sonar_model), *iter);

      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  g_message ("  success");
  return TRUE;
}

gboolean
gen_disable (Global   *global,
             gint      panelx)
{
  HyScanSourceType *iter;
  FnnPanel *panel = get_panel_quiet (global, panelx);

  g_message ("gen_disable: %s (%i)", panel->name, panelx);

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSourceType source = *iter;

      source_informer ("  disabling generator", source);
      status = hyscan_sonar_generator_disable (HYSCAN_SONAR (global->sonar_model), source);

      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  g_message ("  success");
  return TRUE;
}

/* Функция устанавливает излучаемый сигнал. */
gboolean
signal_set (Global *global,
            gint    sig_num,
            gint    panelx)
{
  HyScanSourceType *iter;
  const HyScanDataSchemaEnumValue *sig = NULL, *prev_sig = NULL;
  FnnPanel *panel = get_panel_quiet (global, panelx);

  g_message ("signal_set: %s (%i), Signal %i", panel->name, panelx, sig_num);

  if (global->dry)
    {
      g_warning ("  signal_set: dry mode!");
      return FALSE;
    }

  if (panel == NULL)
    {
      g_warning ("  signal_set: panel %i not found!", panelx);
      return FALSE;
    }

  if (sig_num < 0)
    return FALSE;

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSourceType source = *iter;

      source_informer ("  setting signal", source);
      if (source == HYSCAN_SOURCE_PROFILER_ECHO)
        {
          source_informer ("  skipping", source);
          continue;
        }

      sig = signal_finder (global, panel, source, sig_num);
      hyscan_return_val_if_fail (sig != NULL, FALSE);

      if (prev_sig != NULL && !g_str_equal (sig->name, prev_sig->name))
        {
          g_warning ("  signal name not equal to previous!");
          g_warning ("  current signal: <%s>, previous signal: <%s>", sig->name, prev_sig->name);
        }

      prev_sig = sig;

      /* Устанавливаем. */
      status = hyscan_sonar_generator_set_preset (HYSCAN_SONAR (global->sonar_model), source, sig->value);

      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  if (sig != NULL)
    signal_label (panel, sig->name);

  g_message ("  success");
  return TRUE;
}

void
signal_up (GtkWidget *widget,
           gint       panelx)
{
  gint desired_signal;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired_signal = panel->current.signal + 1;
  if (signal_set (tglobal, desired_signal, panelx))
    panel->current.signal = desired_signal;
  else
    g_warning ("signal_up failed");
}

void
signal_down (GtkWidget *widget,
             gint       panelx)
{
  gint desired_signal;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired_signal = panel->current.signal - 1;
  if (signal_set (tglobal, desired_signal, panelx))
    panel->current.signal = desired_signal;
  else
    g_warning ("signal_down failed");
}

void
tvg_printer (GtkLabel    *label,
             gdouble      value,
             const gchar *units)
{
  gchar *text;

  if (label == NULL)
    return;

  text = g_strdup_printf ("<small><b>%.1f%s%s</b></small>", value,
                          units != NULL ? " " : "", // space
                          units != NULL ? units : ""); // units

  gtk_label_set_markup (label, text);
  g_free (text);
}

void
tvg_label (FnnPanel *panel,
           gdouble   gain0,
           gdouble   step)
{
  tvg_printer (panel->gui.tvg0_value, gain0, "dB");
  tvg_printer (panel->gui.tvg_value, step, "dB");
}

void
auto_tvg_label (FnnPanel *panel,
                gdouble   level,
                gdouble   sensitivity)
{
  tvg_printer (panel->gui.tvg_level_value, level, NULL);
  tvg_printer (panel->gui.tvg_sens_value, sensitivity, NULL);
}

void
log_tvg_label (FnnPanel *panel,
               gdouble   gain0,
               gdouble   beta,
               gdouble   alpha)
{
  tvg_printer (panel->gui.logtvg_gain0_value, gain0, "dB");
  tvg_printer (panel->gui.logtvg_beta_value, beta, "dB");
  tvg_printer (panel->gui.logtvg_alpha_value, alpha, "dB/m");
}

void
const_tvg_label (FnnPanel *panel,
                 gdouble   gain0)
{
  tvg_printer (panel->gui.consttvg_value, gain0, "dB");
}


gboolean
log_tvg_set (Global  *global,
             gdouble *gain0,
             gdouble  beta,
             gdouble  alpha,
             gint     panelx)
{
  HyScanSourceType *iter;
  FnnPanel *panel = get_panel_quiet (global, panelx);
  g_message ("log_tvg_set: %s (%i), gain0 %f, beta %f, alpha %f", panel->name, panelx, *gain0, beta, alpha);

  if (panel == NULL)
    {
      g_warning ("  log_tvg_set: panel %i not found!", panelx);
      return FALSE;
    }

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSonarInfoSource *info;
      HyScanSourceType source = *iter;

      source_informer ("  setting TVG", source);

      /* Проверяем gain0. */
      info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
      hyscan_return_val_if_fail (info != NULL && info->tvg != NULL, TRUE); // TODO do something
      *gain0 = CLAMP (*gain0, info->tvg->min_gain,info->tvg->max_gain);

      status = hyscan_sonar_tvg_set_logarithmic (HYSCAN_SONAR (global->sonar_model), source, *gain0, beta, alpha);
      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  log_tvg_label (panel, *gain0, beta, alpha);
  g_message ("  success");
  panel->current.mode = HYSCAN_SONAR_TVG_MODE_LOGARITHMIC;
  return TRUE;
}

gboolean
const_tvg_set (Global  *global,
               gdouble *gain0,
               gint     panelx)
{
  HyScanSourceType *iter;
  FnnPanel *panel = get_panel_quiet (global, panelx);
  g_message ("const_tvg_set: %s (%i), gain0 %f", panel->name, panelx, *gain0);

  if (panel == NULL)
    {
      g_warning ("  const_tvg_set: panel %i not found!", panelx);
      return FALSE;
    }

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSonarInfoSource *info;
      HyScanSourceType source = *iter;

      source_informer ("  setting TVG", source);

      /* Проверяем gain0. */
      info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
      hyscan_return_val_if_fail (info != NULL && info->tvg != NULL, TRUE); // TODO do something
      *gain0 = CLAMP (*gain0, info->tvg->min_gain,info->tvg->max_gain);

      status = hyscan_sonar_tvg_set_constant (HYSCAN_SONAR (global->sonar_model), source, *gain0);
      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  const_tvg_label (panel, *gain0);
  g_message ("  success");
  panel->current.mode = HYSCAN_SONAR_TVG_MODE_CONSTANT;
  return TRUE;
}

/* Функция устанавливает параметры ВАРУ. */
gboolean
lin_tvg_set (Global  *global,
             gdouble *gain0,
             gdouble  step,
             gint     panelx)
{
  HyScanSourceType *iter;
  FnnPanel *panel = get_panel_quiet (global, panelx);
  g_message ("lin_tvg_set: %s (%i), gain0 %f, step %f", panel->name, panelx, *gain0, step);

  if (panel == NULL)
    {
      g_warning ("  lin_tvg_set: panel %i not found!", panelx);
      return FALSE;
    }

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      gboolean status;
      HyScanSonarInfoSource *info;
      HyScanSourceType source = *iter;

      if (source == HYSCAN_SOURCE_PROFILER_ECHO)
        {
          g_message ("  setting TVG for profiler-echo (hardcoded 10)");
          status = hyscan_sonar_tvg_set_constant (HYSCAN_SONAR (global->sonar_model), source, 10);
          if (!status)
            {
              g_message ("  failure!");
              return FALSE;
            }

          continue;
        }

      source_informer ("  setting TVG", source);

      /* Проверяем gain0. */
      info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (source));
      hyscan_return_val_if_fail (info != NULL && info->tvg != NULL, TRUE); // TODO do something
      *gain0 = CLAMP (*gain0, info->tvg->min_gain,info->tvg->max_gain);

      status = hyscan_sonar_tvg_set_linear_db (HYSCAN_SONAR (global->sonar_model), source, *gain0, step);
      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  tvg_label (panel, *gain0, step);
  g_message ("  success");
  panel->current.mode = HYSCAN_SONAR_TVG_MODE_LINEAR_DB;
  return TRUE;
}

/* Функция устанавливает параметры автоВАРУ. */
gboolean
auto_tvg_set (Global   *global,
              gdouble   level,
              gdouble   sensitivity,
              gint      panelx)
{
  gboolean status;
  FnnPanel *panel = get_panel_quiet (global, panelx);

  HyScanSourceType *iter;

  if (panel == NULL)
    {
      g_warning ("  auto_tvg_set: panel %i not found!", panelx);
      return FALSE;
    }

  g_message ("auto_tvg_set: %s (%i), level %f, sensitivity %f", panel->name, panelx, level, sensitivity);

  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      source_informer ("  setting auto-tvg", *iter);
      status = hyscan_sonar_tvg_set_auto (HYSCAN_SONAR (global->sonar_model), *iter, level, sensitivity);
      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  /* Теперь печатаем что и куда надо. */
  auto_tvg_label (panel, level, sensitivity);
  g_message ("  success");
  panel->current.mode = HYSCAN_SONAR_TVG_MODE_AUTO;
  return TRUE;
}

gboolean
tvg_set_preferred_mode (Global                 *global,
                        HyScanSonarTVGModeType  mode,
                        gint                    panelx)
{
  FnnPanel *panel = get_panel (tglobal, panelx);
  panel->current.mode = mode;
  return TRUE;
}

#define CONST_TVG_FUNC(fname, sign, value, from_to)                             \
TVG_FUNC_DEF(fname)                                                             \
{                                                                               \
  gdouble desired;                                                              \
  FnnPanel *panel = get_panel (tglobal, panelx);                                \
                                                                                \
  desired = from_to sign value;                                                 \
  desired = desired - fmod (desired, value);                                    \
                                                                                \
  if (const_tvg_set (tglobal, &desired, panelx))                                \
    from_to = desired;                                                          \
  else                                                                          \
    g_warning ("%s failed", __FUNCTION__);                                      \
}

CONST_TVG_FUNC (consttvg_up, +, 1, panel->current.const_gain0);
CONST_TVG_FUNC (consttvg_down, -, 1, panel->current.const_gain0);

#define LOG_TVG_FUNC(fname, sign, value, from_to, param1, param2, param3)       \
TVG_FUNC_DEF(fname)                                                             \
{                                                                               \
  gdouble desired;                                                              \
  FnnPanel *panel = get_panel (tglobal, panelx);                                \
                                                                                \
  desired = from_to sign value;                                                 \
  desired = desired - fmod (desired, value);                                    \
                                                                                \
  if (log_tvg_set (tglobal, param1, param2, param3, panelx))                    \
    from_to = desired;                                                          \
  else                                                                          \
    g_warning ("%s failed", __FUNCTION__);                                      \
}

LOG_TVG_FUNC (logtvg_gain0_up,   +, 1, panel->current.log_gain0, &desired, panel->current.log_beta, panel->current.log_alpha)
LOG_TVG_FUNC (logtvg_gain0_down, -, 1, panel->current.log_gain0, &desired, panel->current.log_beta, panel->current.log_alpha)

LOG_TVG_FUNC (logtvg_beta_up,    +, 1, panel->current.log_beta, &panel->current.log_gain0, desired, panel->current.log_alpha)
LOG_TVG_FUNC (logtvg_beta_down,  -, 1, panel->current.log_beta, &panel->current.log_gain0, desired, panel->current.log_alpha)

LOG_TVG_FUNC (logtvg_alpha_up,   +, 1, panel->current.log_alpha, &panel->current.log_gain0, panel->current.log_beta, desired)
LOG_TVG_FUNC (logtvg_alpha_down, -, 1, panel->current.log_alpha, &panel->current.log_gain0, panel->current.log_beta, desired)


#define LIN_TVG_FUNC(fname, sign, value, from_to, param1, param2)               \
TVG_FUNC_DEF(fname)                                                             \
{                                                                               \
  gdouble desired;                                                              \
  FnnPanel *panel = get_panel (tglobal, panelx);                                \
                                                                                \
  desired = from_to sign value;                                                 \
  desired = desired - fmod (desired, value);                                    \
                                                                                \
  if (lin_tvg_set (tglobal, param1, param2, panelx))                            \
    from_to = desired;                                                          \
  else                                                                          \
    g_warning ("%s failed", __FUNCTION__);                                      \
}

/* Use gcc -E to test. */
LIN_TVG_FUNC (tvg0_up,   +, 1, panel->current.gain0, &desired, panel->current.gain_step); /* tvg0_up*/
LIN_TVG_FUNC (tvg0_down, -, 1, panel->current.gain0, &desired, panel->current.gain_step); /* tvg0_down*/

LIN_TVG_FUNC (tvg_up,    +, 1, panel->current.gain_step, &panel->current.gain0, desired); /* tvg_up*/
LIN_TVG_FUNC (tvg_down,  -, 1, panel->current.gain_step, &panel->current.gain0, desired); /* tvg0_own*/


#define AUTO_TVG_FUNC(fname, sign, value, from_to, param1, param2)              \
TVG_FUNC_DEF(fname)                                                             \
{                                                                               \
  gdouble desired;                                                              \
  FnnPanel *panel = get_panel (tglobal, panelx);                                \
                                                                                \
  desired = from_to sign value;                                                 \
  desired = CLAMP (desired, 0.0, 1.0);                                          \
                                                                                \
  if (auto_tvg_set (tglobal, param1, param2, panelx))                           \
    from_to = desired;                                                          \
  else                                                                          \
    g_warning ("%s failed", __FUNCTION__);                                      \
}

AUTO_TVG_FUNC (tvg_level_up,   +, 0.1, panel->current.level, desired, panel->current.sensitivity);
AUTO_TVG_FUNC (tvg_level_down, -, 0.1, panel->current.level, desired, panel->current.sensitivity);

AUTO_TVG_FUNC (tvg_sens_up,    +, 0.1, panel->current.sensitivity, panel->current.level, desired);
AUTO_TVG_FUNC (tvg_sens_down,  -, 0.1, panel->current.sensitivity, panel->current.level, desired);

void
distance_label (FnnPanel *panel,
                gdouble   distance)
{
  gchar *text;
  if (panel->gui.distance_value != NULL)
  {
    text = g_strdup_printf ("<small><b>%.0f м</b></small>", distance);
    gtk_label_set_markup (panel->gui.distance_value, text);
    g_free (text);
  }
}

/* Функция устанавливает рабочую дистанцию. */
gboolean
distance_set (Global  *global,
              gdouble *meters,
              gint     panelx)
{
  gdouble receive_time, wait_time;
  gboolean status;
  HyScanSourceType *iter;
  FnnPanel *panel = get_panel_quiet (global, panelx);

  if (panel == NULL)
    {
      g_warning ("  distance_set: panel %i not found!", panelx);
      return FALSE;
    }

  g_message ("distance_set: %s (%i), distance %f", panel->name, panelx, *meters);

  if (*meters < 1.0)
    {
      g_message ("  failure!");
      return FALSE;
    }

  receive_time = *meters / (global->sound_velocity / 2.0);
  for (iter = panel->sources; *iter != HYSCAN_SOURCE_INVALID; ++iter)
    {
      HyScanSonarInfoSource *info;

      source_informer ("  setting distance", *iter);
      info = g_hash_table_lookup (global->infos, GINT_TO_POINTER (*iter));
      hyscan_return_val_if_fail (info != NULL, FALSE);

      if (receive_time > info->receiver->max_time || receive_time < info->receiver->min_time)
        {
          g_message ("    receive time %f (%4.2fm) is out of range [%f; %f]",
                     receive_time, *meters,
                     info->receiver->min_time,
                     info->receiver->max_time);
          receive_time = CLAMP (receive_time, info->receiver->min_time, info->receiver->max_time);
          *meters = receive_time * (global->sound_velocity/2.0);
          g_message ("    receive time set to %f (%4.2fm)", receive_time, *meters);
        }

      wait_time = 0;
      if (panelx == X_PROFILER)
        {
          gdouble ss_time = 0, fl_time = 0, master_time;
          gdouble requested_time = receive_time;
          gdouble full_time;
          FnnPanel *ss, *fl;

          ss = get_panel_quiet (tglobal, X_SIDESCAN);
          if (ss != NULL)
            ss_time = ss->current.distance / (global->sound_velocity / 2.0);
          fl = get_panel_quiet (tglobal, X_FORWARDL);
          if (fl != NULL)
            fl_time = fl->current.distance / (global->sound_velocity / 2.0);

          master_time = MAX (fl_time, ss_time);

          if (ss != NULL || fl != NULL)
            {
              g_message ("    distance: PF synced");
              full_time = floor (0.333 / master_time) * master_time;
              receive_time = master_time / 3.0;

              receive_time = MIN (receive_time, requested_time);
              receive_time = CLAMP (receive_time, info->receiver->min_time, info->receiver->max_time);

              if (requested_time != receive_time)
                *meters = receive_time * (global->sound_velocity/2.0);

              wait_time = full_time - receive_time - (master_time / 2.0);
            }
          else
            {
              g_message ("    distance: PF unsynced");
              if (receive_time >= 0.333)
                wait_time = receive_time;
              else
                wait_time = 0.333 - receive_time;
            }
        }

      g_message ("    %s: %4.1f m, r/w time: %f %f",
                 hyscan_source_get_id_by_type (*iter), *meters, receive_time, wait_time);

      status = hyscan_sonar_receiver_set_time (HYSCAN_SONAR (global->sonar_model), *iter, receive_time, wait_time);
      if (!status)
        {
          g_message ("  failure!");
          return FALSE;
        }
    }

  /* Специальный случай. */
  distance_label (panel, *meters);
  g_message ("  success");
  return TRUE;
}

static gfloat fnn_distances[] = {0., 10., 20., 30., 50., 75., 100., 150., };
static gint n_fnn_distances = (gint)G_N_ELEMENTS (fnn_distances);

void
distance_up (GtkWidget *widget,
             gint       panelx)
{
  gdouble desired;
  gint i;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.distance + 1.0;

  for (i = 0; i < n_fnn_distances; ++i)
    {
      if (desired <= fnn_distances[i])
        {
          desired = fnn_distances[i];
          break;
        }
    }
  if (i == n_fnn_distances)
    {
      desired = panel->current.distance + 50.0;
    }

  // desired = desired - fmod (desired, 5);

  if (distance_set (tglobal, &desired, panelx))
    panel->current.distance = desired;
  else
    g_warning ("distance_up failed");
}

void
distance_down (GtkWidget *widget,
               gint       panelx)
{
  gdouble desired;
  gint i;
  FnnPanel *panel = get_panel (tglobal, panelx);

  desired = panel->current.distance - 1.0;

  if (desired > fnn_distances[n_fnn_distances - 1])
    {
      desired = panel->current.distance - 50.0;
    }
  else
    {
      for (i = (gint)n_fnn_distances - 2; i >= 0 ; --i)
        {
          if (desired >= fnn_distances[i])
            {
              desired = fnn_distances[i];
              break;
            }
        }
      if (i == -1)
        {
          desired = panel->current.distance - 50.0;
        }
    }

  if (distance_set (tglobal, &desired, panelx))
    panel->current.distance = desired;
  else
    g_warning ("distance_down failed");
}

void
void_callback (gpointer data)
{
  g_message ("This is a void callback. ");
}

void
levels_printer (FnnPanel    *panel,
                const gchar *text_black,
                const gchar *text_gamma,
                const gchar *text_white)
{
  if (panel->vis_gui->black_value != NULL && text_black != NULL)
    gtk_label_set_markup (panel->vis_gui->black_value, text_black);
  if (panel->vis_gui->gamma_value != NULL && text_gamma != NULL)
    gtk_label_set_markup (panel->vis_gui->gamma_value, text_gamma);
  if (panel->vis_gui->white_value != NULL && text_white != NULL)
    gtk_label_set_markup (panel->vis_gui->white_value, text_white);
}

/* Функция устанавливает уровни. */
gboolean
levels_set (Global  *global,
            gdouble  new_black,
            gdouble  new_gamma,
            gdouble  new_white,
            gint     panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gchar *text_black;
  gchar *text_gamma;
  gchar *text_white;
  gdouble b, g, w;
  FnnPanel *panel;

  if (global->override.levels_set != NULL)
    return global->override.levels_set (global, new_black, new_gamma, new_white, panelx);

  panel = get_panel (global, panelx);

  if (new_white <= 0.0)
    return FALSE;
  if (new_white > 100.0)
    return FALSE;
  if (new_gamma < 0.0)
    return FALSE;
  if (new_black < 0.0)
    return FALSE;
  if (new_black > 100.0)
    return FALSE;

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
    case FNN_PANEL_ECHO:
      b = new_black / 100.0;
      w = new_white / 100.0;
      g = new_gamma; // 100.0;

      wf = (VisualWF*)panel->vis_gui;
      if (!hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w))
        return FALSE;
      break;

    case FNN_PANEL_PROFILER:
      b = new_black / 400000;
      w = (new_white / 100.0) * (1 - b);
      w *= w;
      g = 1;

      g_message ("SET: (%f %f %f)-> (%f %f %f)", new_black,new_gamma,new_white, b,g,w);
      if (b >= w)
        {
          g_message ("BBC error");
          return FALSE;
        }

      wf = (VisualWF*)panel->vis_gui;
      if (!hyscan_gtk_waterfall_set_levels_for_all (HYSCAN_GTK_WATERFALL (wf->wf), b, g, w))
        return FALSE;
      break;

    case FNN_PANEL_FORWARDLOOK:
      fl = (VisualFL*)panel->vis_gui;
      hyscan_gtk_forward_look_set_brightness (fl->fl, new_white);
      break;

    default:
      g_warning ("levels_set: wrong panel type!");
    }

  text_black = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_black);
  text_gamma = g_strdup_printf ("<small><b>%.2f</b></small>", new_gamma);
  text_white = g_strdup_printf ("<small><b>%.0f%%</b></small>", new_white);
  levels_printer (panel, text_black, text_gamma, text_white);
  g_free (text_black);
  g_free (text_gamma);
  g_free (text_white);

  return TRUE;
}

void
white_up (GtkWidget *widget,
          gint       panelx)
{
  gdouble new_white;
  gdouble step;
  FnnPanel *panel = get_panel (tglobal, panelx);

  new_white = panel->vis_current.white;

  switch (panel->type)
    {
      case FNN_PANEL_WATERFALL:
      case FNN_PANEL_PROFILER:
      case FNN_PANEL_ECHO:
        {
          // if (new_white < 50.0)
            // step = 10.0;
          // else if (new_white < 90.0)
            // step = 5.0;
          // else
            step = 5.0;
        }
        break;

      case FNN_PANEL_FORWARDLOOK:
        {
          if (new_white < 10.0)
            step = 1.0;
          else if (new_white < 30.0)
            step = 2.0;
          else if (new_white < 50.0)
            step = 5.0;
          else
            step = 10.0;
        }
        break;

      default:
        g_warning ("white_up: wrong panel type");
        return;
    }

  new_white = new_white + step;
  new_white = new_white - fmod (new_white, ABS (step));
  new_white = CLAMP (new_white, 0.0, 100.0);

  if (levels_set (tglobal, panel->vis_current.black, panel->vis_current.gamma, new_white, panelx))
    panel->vis_current.white = new_white;
  else
    g_warning ("white_up failed");
}

void
white_down (GtkWidget *widget,
            gint        panelx)
{
  gdouble new_white, step;
  FnnPanel *panel = get_panel (tglobal, panelx);

  new_white = panel->vis_current.white;

  switch (panel->type)
    {
      case FNN_PANEL_WATERFALL:
      case FNN_PANEL_PROFILER:
      case FNN_PANEL_ECHO:
        {
          // if (new_white > 90.0)
            step = -5.0;
          // else if (new_white > 50.0)
            // step = -5.0;
          // else
            // step = -10.0;
        }
        break;

      case FNN_PANEL_FORWARDLOOK:
        {
          if (new_white <= 10.0)
            step = -1.0;
          else if (new_white <= 30.0)
            step = -2.0;
          else if (new_white <= 50.0)
            step = -5.0;
          else
            step = -10.0;
        }
        break;

      default:
        g_warning ("white_down: wrong panel type");
        return;
    }

  new_white = new_white + step;
  new_white = new_white - fmod (new_white, ABS (step));
  new_white = CLAMP (new_white, 0.0, 100.0);

  if (levels_set (tglobal, panel->vis_current.black, panel->vis_current.gamma, new_white, panelx))
    panel->vis_current.white = new_white;
  else
    g_warning ("white_down failed");
}

void
black_up (GtkWidget *widget,
          gint       panelx)
{
  gdouble new_black, step;
  FnnPanel *panel = get_panel (tglobal, panelx);

  new_black = panel->vis_current.black;

  switch (panel->type)
    {
      case FNN_PANEL_WATERFALL:
      case FNN_PANEL_PROFILER:
      case FNN_PANEL_ECHO:
        {
          // if (new_black < 50.0)
            // step = 10.0;
          // else if (new_black < 90.0)
            // step = 5.0;
          // else
            step = 5.0;
        }
        break;

      case FNN_PANEL_FORWARDLOOK:
        {
          if (new_black < 10.0)
            step = 1.0;
          else if (new_black < 30.0)
            step = 2.0;
          else if (new_black < 50.0)
            step = 5.0;
          else
            step = 10.0;
        }
        break;

      default:
        g_warning ("black_up: wrong panel type");
        return;
    }

  new_black = new_black + step;
  new_black = new_black - fmod (new_black, ABS (step));
  new_black = CLAMP (new_black, 0.0, 100.0);

  if (levels_set (tglobal, new_black, panel->vis_current.gamma, panel->vis_current.white, panelx))
    panel->vis_current.black = new_black;
  else
    g_warning ("black_up failed");
}

void
black_down (GtkWidget *widget,
            gint       panelx)
{
  gdouble new_black, step;
  FnnPanel *panel = get_panel (tglobal, panelx);

  new_black = panel->vis_current.black;

  switch (panel->type)
    {
      case FNN_PANEL_WATERFALL:
      case FNN_PANEL_PROFILER:
      case FNN_PANEL_ECHO:
        {
          // if (new_black > 90.0)
            step = -5.0;
          // else if (new_black > 50.0)
            // step = -5.0;
          // else
            // step = -10.0;
        }
        break;

      case FNN_PANEL_FORWARDLOOK:
        {
          if (new_black <= 10.0)
            step = -1.0;
          else if (new_black <= 30.0)
            step = -2.0;
          else if (new_black <= 50.0)
            step = -5.0;
          else
            step = -10.0;
        }
        break;

      default:
        g_warning ("black_down: wrong panel type");
        return;
    }

  new_black = new_black + step;
  new_black = new_black - fmod (new_black, ABS (step));
  new_black = CLAMP (new_black, 0.0, 100.0);

  if (levels_set (tglobal, new_black, panel->vis_current.gamma, panel->vis_current.white, panelx))
    panel->vis_current.black = new_black;
  else
    g_warning ("black_down failed");
}

void
gamma_up (GtkWidget *widget,
          gint       panelx)
{
  gdouble new_gamma, step;
  FnnPanel *panel = get_panel (tglobal, panelx);

  new_gamma = panel->vis_current.gamma;

  step = .05;

  new_gamma = new_gamma + step;
  new_gamma = CLAMP (new_gamma, 0.001, 2.0);

  if (levels_set (tglobal, panel->vis_current.black, new_gamma, panel->vis_current.white, panelx))
    panel->vis_current.gamma = new_gamma;
  else
    g_warning ("gamma_up failed");
}

void
gamma_down (GtkWidget *widget,
            gint       panelx)
{
  gdouble new_gamma, step;
  FnnPanel *panel = get_panel (tglobal, panelx);
  new_gamma = panel->vis_current.gamma;

  step = -.05;
  new_gamma = new_gamma + step;
  new_gamma = CLAMP (new_gamma, 0.001, 2.0);

  if (levels_set (tglobal, panel->vis_current.black, new_gamma, panel->vis_current.white, panelx))
    panel->vis_current.gamma = new_gamma;
  else
    g_warning ("gamma_down failed");
}

void
scale_up (GtkWidget *widget,
          gint       panelx)
{
  scale_set (tglobal, TRUE, panelx);
}

void
scale_down (GtkWidget *widget,
            gint       panelx)
{
  scale_set (tglobal, FALSE, panelx);
}

/* Функция включает и выключает борт
 * Попрошу заметить, возвращается ЛОЖЬ для того, чтобы свитчер нормально переключился. */
gboolean
disable_changed (GtkWidget *widget,
                 gboolean   disabled,
                 gint       panelx)
{
  FnnPanel *panel = get_panel_quiet (tglobal, panelx);

  if (panel == NULL)
    {
      g_warning ("  disable_changed: panel %i not found!", panelx);
      return TRUE;
    }

  g_message ("disable_changed: %s (%i) %s", panel->name, panelx, disabled ? "OFF" : "ON");

  panel->current.disabled = disabled;

  if (!panel_turn_on_off (tglobal, tglobal->on_air, panel))
    {
      g_warning ("  failure!");
      return TRUE;
    }

  g_message ("  success");
  return FALSE;
}

/* Функция переключает режим отображения виджета ВС.
 * Попрошу заметить, возвращается ЛОЖЬ для того, чтобы свитчер нормально переключился. */
gboolean
mode_changed (GtkWidget *widget,
              gboolean   state,
              gint       panelx)
{
  VisualFL *fl;
  HyScanGtkForwardLookViewType type;

  FnnPanel *panel = get_panel (tglobal, panelx);

  if (panel->type != FNN_PANEL_FORWARDLOOK)
    {
      g_message ("mode_changed: wrong panel type!");
      return TRUE;
    }

  fl = (VisualFL*)panel->vis_gui;
  type = state ? HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET : HYSCAN_GTK_FORWARD_LOOK_VIEW_SURFACE;
  hyscan_gtk_forward_look_set_type (fl->fl, type);
  return FALSE;
}

/* Вкл и выкл спецобработки.
 * Попрошу заметить, возвращается ЛОЖЬ для того, чтобы свитчер нормально переключился. */
gboolean
pf_special (GtkWidget  *widget,
            gboolean    state,
            gint        panelx)
{
  VisualWF *wf;
  HyScanTileFlags flags = 0;
  FnnPanel *panel = get_panel (tglobal, panelx);

  if (panel->type != FNN_PANEL_PROFILER)
    {
      g_message ("pf_special: wrong panel type!");
      return TRUE;
    }

  wf = (VisualWF*)panel->vis_gui;
  flags = hyscan_gtk_waterfall_state_get_tile_flags (HYSCAN_GTK_WATERFALL_STATE (wf->wf));

  if (state)
    flags |= HYSCAN_TILE_PROFILER;
  else
    flags &= ~HYSCAN_TILE_PROFILER;

  hyscan_gtk_waterfall_state_set_tile_flags (HYSCAN_GTK_WATERFALL_STATE (wf->wf), flags);

  return FALSE;
}

/* Функция переключает автосдвижку. */
gboolean
live_view (GtkWidget  *widget,
           gboolean    state,
           gint        panelx)
{
  VisualWF *wf;
  VisualFL *fl;
  gboolean current;
  FnnPanel *panel = get_panel (tglobal, panelx);

  switch (panel->type)
    {
    case FNN_PANEL_WATERFALL:
    case FNN_PANEL_PROFILER:
    case FNN_PANEL_ECHO:
      wf = (VisualWF*)panel->vis_gui;
      current = hyscan_gtk_waterfall_automove (wf->wf, state);
      break;

    case FNN_PANEL_FORWARDLOOK:

      fl = (VisualFL*)panel->vis_gui;
      if (state)
        hyscan_forward_look_player_real_time (fl->player);
      else
        hyscan_forward_look_player_pause (fl->player);
      current = state;
      break;

    default:
      g_warning ("live_view: wrong panel type!");
    }

  button_set_active (G_OBJECT (panel->vis_gui->live_view), current);

  return TRUE;
}

gboolean
automove_switched (GtkWidget  *widget,
                   gboolean    state)
{
  static gboolean now_switching = FALSE;

  if (now_switching)
    return TRUE;

  now_switching = TRUE;
  automove_state_changed (widget, state, tglobal);
  now_switching = FALSE;

  return FALSE;
}

void
automove_state_changed (GtkWidget  *widget,
                        gboolean    state,
                        Global     *global)
{
  GHashTableIter iter;
  gpointer k, v;

  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, &k, &v))
    live_view (NULL, state, GPOINTER_TO_INT (k));
}

gboolean
panel_turn_on_off (Global   *global,
                   gboolean  on_air,
                   FnnPanel *panel)
{
  gint panelx = panel->panelx;
  gboolean status = TRUE;

  /* Не настраиваем те панели, которых нет в ГЛ. */
  if (!panel_sources_are_in_sonar (global, panel))
    return TRUE;

  /* Отключенные панели не включаем (но ведем себя так, будто они включены) */
  if (panel->current.disabled)
    {
      status  = gen_disable (global, panelx);
      status &= rec_disable (global, panelx);
      return status;
    }

  if (!on_air)
    return TRUE;

  status &= distance_set (global, &panel->current.distance, panelx);

  /* Излучение НЕ в режиме сух. пов. */
  if (global->dry)
    status &= gen_disable (global, panelx);
  else
    status &= signal_set (global, panel->current.signal, panelx);


  /* TVG */
  switch (panel->current.mode)
    {
      case HYSCAN_SONAR_TVG_MODE_LOGARITHMIC:
        status &= log_tvg_set (global,
                               &panel->current.log_gain0,
                               panel->current.log_beta,
                               panel->current.log_alpha,
                               panelx);
        break;

      case HYSCAN_SONAR_TVG_MODE_CONSTANT:
        status &= const_tvg_set (global, &panel->current.const_gain0, panelx);
        break;

      case HYSCAN_SONAR_TVG_MODE_LINEAR_DB:
        status &= lin_tvg_set (global, &panel->current.gain0, panel->current.gain_step, panelx);
        break;

      case HYSCAN_SONAR_TVG_MODE_AUTO:
        status &= auto_tvg_set (global, panel->current.level, panel->current.sensitivity, panelx);
        break;

      case HYSCAN_SONAR_TVG_MODE_NONE:
      default:
        g_warning ("start_stop: no tvg mode set");
    }

  return status;
}

gboolean
panels_turn_on (Global *global)
{
  GHashTableIter iter;
  gpointer v;

  /* Проходим по всем страницам и включаем системы локаторов. */
  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, NULL, &v))
    {
      FnnPanel *panel = v;

      /* Если не удалось запустить какую-либо подсистему. */
      if (!panel_turn_on_off (global, TRUE, panel))
        return FALSE;
    }

  return TRUE;
}

static gchar *
make_track_name (Global *global)
{
  gint track_num = 1; /* Число галсов в проекте. */
  gint number;
  gint32 project_id;
  gchar **tracks;
  gchar **strs;

  project_id = hyscan_db_project_open (global->db, global->project_name);
  tracks = hyscan_db_track_list (global->db, project_id);

  for (strs = tracks; strs != NULL && *strs != NULL; strs++)
    {
      /* Ищем галс с самым большим номером в названии. */
      number = g_ascii_strtoll (*strs, NULL, 10);
      if (number >= track_num)
        track_num = number + 1;
    }

  hyscan_db_close (global->db, project_id);
  g_free (tracks);

  return g_strdup_printf ("%d%s", track_num, global->dry ? DRY_TRACK_SUFFIX : "");
}

/* Обработчик сигнала "start-stop" модели ГЛ. Устанавливает состояние global по факту старта и остановки ГЛ. */
void
sonar_state_changed (Global *global)
{
  GHashTableIter iter;
  gpointer k;
  gchar *track_name;

  global->on_air = hyscan_sonar_state_get_start (HYSCAN_SONAR_STATE (global->sonar_model),
                                                 NULL, &track_name, NULL, NULL);
  if (global->on_air)
    {
      g_free (global->track_name);
      global->track_name = track_name;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (global->gui.track.tree), !global->on_air);

  /* Устанавливаем live_view для всех виджетов. */
  g_hash_table_iter_init (&iter, global->panels);
  while (g_hash_table_iter_next (&iter, &k, NULL))
    live_view (NULL, global->on_air, GPOINTER_TO_INT (k));
}

/* Обработчик сигнала "before-start" модели ГЛ.
 * Отменяет запись (возвращает TRUE), если не удалось включить какую-то из панелей. */
gboolean
before_start (HyScanSonarModel *model,
              Global           *global)
{
  if (panels_turn_on (global))
    return FALSE;

  /* Что-то пошло не так, отменяем запуск ГЛ. */
  g_message ("Failed to turn on some panels. Cancel sonar start");

  return TRUE;
}

/* Функция включает/выключает излучение. */
gboolean
start_stop (Global                *global,
            const HyScanTrackPlan *track_plan,
            gboolean               state)
{
  if (global->control == NULL)
    {
      g_message ("I ain't got no sonar.");
      return FALSE;
    }

  /* Включаем излучение. */
  if (state)
    {
      gboolean status;
      gchar *track_name;

      g_message ("Start sonars. Dry is %s", global->dry ? "ON" : "OFF");

      /* Закрываем текущий открытый галс. */
      g_clear_pointer (&global->track_name, g_free);

      /* Включаем запись нового галса. */
      track_name = make_track_name (global);

      /* Перед реальным запуском ГЛ будет отправлен сигнал "before-start",
       * который вызывает before_start(). */
      status = hyscan_sonar_start (HYSCAN_SONAR (global->sonar_model), global->project_name,
                                   track_name, HYSCAN_TRACK_SURVEY, track_plan);

      g_free (track_name);

      if (!status)
        {
          g_message ("Sonar startup failed @ %i",__LINE__);
          return FALSE;
        }

      g_message ("Sonar started");
    }

  /* Выключаем излучение и блокируем режим онлайн. */
  else
    {
      g_message ("Sonar stopped");
      hyscan_sonar_stop (HYSCAN_SONAR (global->sonar_model));
    }

  /* По факту запуска или остановки ГЛ будет отправлен сигнал "before-start",
   * который вызывает sonar_state_changed(). */

  return TRUE;
}

/* Функция включает/выключает сухую поверку. */
gboolean
set_dry (Global    *global,
         gboolean   state)
{
  global->dry = state;

  hyscan_sonar_recorder_set_suffix (global->recorder, global->dry ? "-dry" : "", FALSE);

  return TRUE;
}

GtkWidget*
make_layer_btn (HyScanGtkLayer *layer,
                GtkWidget      *from)
{
  GtkWidget *button;
  const gchar *icon;

  icon = hyscan_gtk_layer_get_icon_name (layer);
  button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (from));

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (hyscan_gtk_layer_grab_input),
                            layer);

  return button;
}

void
fl_coords_callback (HyScanFlCoords *coords,
                    GtkLabel       *label)
{
  gboolean status;
  gdouble lat;
  gdouble lon;
  gchar *text;

  status = hyscan_fl_coords_get_coords (coords, &lat, &lon);

  text = g_strdup_printf ("Широта: %f; Долгота: %f", lat, lon);
  gtk_widget_set_visible (GTK_WIDGET (label), status);
  gtk_label_set_text (label, text);
}

GtkWidget *
make_overlay (HyScanGtkWaterfall           *wf,
              HyScanGtkWaterfallGrid      **_grid,
              HyScanGtkWaterfallControl   **_ctrl,
              HyScanGtkWaterfallMark      **_mark,
              HyScanGtkWaterfallMeter     **_meter,
              HyScanGtkWaterfallPlayer    **_player,
              HyScanGtkWaterfallMagnifier **_magnifier,
              HyScanGtkWaterfallShadowm   **_shadow,
              HyScanGtkWaterfallCoord     **_coord,
              HyScanObjectModel            *mark_model)
{
  GtkWidget * container = gtk_grid_new ();

  HyScanGtkWaterfallGrid *grid = hyscan_gtk_waterfall_grid_new ();
  HyScanGtkWaterfallControl *ctrl = hyscan_gtk_waterfall_control_new ();
  HyScanGtkWaterfallMark *mark = hyscan_gtk_waterfall_mark_new (mark_model);
  HyScanGtkWaterfallMeter *meter = hyscan_gtk_waterfall_meter_new ();
  HyScanGtkWaterfallPlayer *player = hyscan_gtk_waterfall_player_new ();
  HyScanGtkWaterfallMagnifier *magnifier = hyscan_gtk_waterfall_magnifier_new ();
  HyScanGtkWaterfallShadowm *shadow = hyscan_gtk_waterfall_shadowm_new ();
  HyScanGtkWaterfallCoord *coord = hyscan_gtk_waterfall_coord_new ();

  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (grid), "grid");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (ctrl), "ctrl");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (mark), "mark");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (meter), "meter");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (player), "player");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (shadow), "shadow");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (coord), "coordinates");
  hyscan_gtk_layer_container_add (HYSCAN_GTK_LAYER_CONTAINER (wf), HYSCAN_GTK_LAYER (magnifier), "magnifier");

  GtkWidget *hscroll = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL,
                                           hyscan_gtk_waterfall_get_hadjustment (wf));
  GtkWidget *vscroll = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
                                           hyscan_gtk_waterfall_get_vadjustment (wf));
  gtk_range_set_inverted (GTK_RANGE (vscroll), TRUE);
  hyscan_gtk_waterfall_control_set_wheel_behaviour (ctrl, TRUE);

  if (_grid != NULL)
    *_grid = grid;
  if (_ctrl != NULL)
    *_ctrl = ctrl;
  if (_mark != NULL)
    *_mark = mark;
  if (_meter != NULL)
    *_meter = meter;
  if (_player != NULL)
    *_player = player;
  if (_magnifier != NULL)
    *_magnifier = magnifier;
  if (_shadow != NULL)
    *_shadow = shadow;
  if (_coord != NULL)
    *_coord = coord;

  gtk_grid_attach (GTK_GRID (container), GTK_WIDGET (wf), 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (container), hscroll, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (container), vscroll, 1, 0, 1, 1);

  gtk_widget_set_hexpand (GTK_WIDGET (wf), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (wf), TRUE);

  return GTK_WIDGET (container);
}


void
fnn_colormap_free (gpointer data)
{
  FnnColormap *cmap = *(FnnColormap**)data;
  if (cmap == NULL)
    return;

  g_clear_pointer (&cmap->colors, g_free);
  g_clear_pointer (&cmap->name, g_free);

  g_free (cmap);
}

GArray *
fnn_make_color_maps (gboolean profiler)
{
  guint32 kolors[2];
  GArray * colormaps;
  FnnColormap *new_map;
  #include "colormaps.h"

  colormaps = g_array_new (FALSE, TRUE, sizeof (FnnColormap*));
  g_array_set_clear_func (colormaps, fnn_colormap_free);

  if (profiler)
    {
      new_map = g_new (FnnColormap, 1);
      new_map->name = g_strdup (_("Profiler"));
      new_map->colors = hyscan_tile_color_compose_colormap_pf (&new_map->len);
      new_map->bg = WHITE_BG;
      g_array_append_vals (colormaps, &new_map, 1);

      return colormaps;
    }

  new_map = g_new (FnnColormap, 1);
  new_map->name = g_strdup (_("Yellow"));
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 0.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (FnnColormap, 1);
  new_map->name = g_strdup (_("Brown"));
  new_map->colors = g_memdup (orange, 256 * sizeof (guint32));
  new_map->len = 256;
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (FnnColormap, 1);
  new_map->name = g_strdup (_("Sepia"));
  new_map->colors = g_memdup (sepia, 256 * sizeof (guint32));
  new_map->len = 256;
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (FnnColormap, 1);
  new_map->name = g_strdup (_("White"));
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  new_map = g_new (FnnColormap, 1);
  new_map->name = g_strdup (_("Inverted"));
  kolors[0] = hyscan_tile_color_converter_d2i (1.0, 1.0, 1.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);new_map = g_new (FnnColormap, 1);

  new_map = g_new (FnnColormap, 1);
  new_map->name = g_strdup (_("Green"));
  kolors[0] = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
  kolors[1] = hyscan_tile_color_converter_d2i (0.2, 1.0, 0.2, 1.0);
  new_map->colors = hyscan_tile_color_compose_colormap (kolors, 2, &new_map->len);
  new_map->bg = BLACK_BG;
  g_array_append_vals (colormaps, &new_map, 1);

  return colormaps;
}

gboolean
panel_sources_are_in_sonar (Global   *global,
                            FnnPanel *panel)
{
  HyScanSourceType *i;

  /* Если в ГЛ нет хотя бы одного источника, который есть на панели,
   * считаем, что всё пропало. */
  for (i = panel->sources; *i != HYSCAN_SOURCE_INVALID; ++i)
    {
      if (!g_hash_table_contains (global->infos, GINT_TO_POINTER(*i)))
        return FALSE;
    }

  return TRUE;
}

gchar **
va_to_strv (int           n,
            const gchar * first,
            va_list       args)
{
  gchar ** strv = NULL;
  const gchar * str = NULL;
  int count = 0;

  strv = g_realloc (strv, (sizeof (gchar*)) * (count + 1));
  strv[count] = g_strdup (first);
  ++count;

  do
    {
      str = va_arg (args, gchar*);
      strv = g_realloc (strv, (sizeof (gchar*)) * (count + 1));
      strv[count] = g_strdup (str);
      ++count;
    }
  while (str != NULL);

  return strv;
}

#ifdef G_OS_WIN32
gchar *
win32_build_path (int n, ...)
{
  static gchar *exec_dir = NULL;
  gchar *path = NULL;
  gchar *utf8_path;
  va_list args;
  gchar ** args_str;

  /* ... If that directory's last component is "bin" or "lib",
   * its parent directory is returned, otherwise the directory itself. */
  if (exec_dir == NULL)
    {
      exec_dir = g_win32_get_package_installation_directory_of_module (NULL);
      if (exec_dir == NULL)
        {
          g_warning ("Something is totally wrong");
          return NULL;
        }

      g_message("installation dir: %s", exec_dir);
    }

  va_start (args, n);
  args_str = va_to_strv (n, exec_dir, args);
  va_end (args);

  utf8_path = g_build_filenamev (args_str);

  g_message ("Path: %s", utf8_path);
  return utf8_path;
}
#endif

const gchar *
get_locale_dir (void)
{
  static gchar *locale_dir = NULL;

  if (locale_dir == NULL)
    {
      #ifdef G_OS_WIN32
        gchar *utf8_path;
        utf8_path = win32_build_path (2, "share", "locale", NULL);
        locale_dir = g_win32_locale_filename_from_utf8 (utf8_path);
        g_free (utf8_path);
      #else
        locale_dir = FNN_LOCALE_DIR;
      #endif

      g_message ("locale_dir: <%s>", locale_dir);
    }

  return locale_dir;
}

gchar **
get_profile_dir (void)
{
  static gchar **dirs = NULL;
  gint dc = 0;

  if (dirs == NULL)
    {
      gint i;

      #ifdef FNN_PROFILE_STANDALONE
        dirs = g_realloc (dirs, ++dc * sizeof (*dirs));
        dirs[dc - 1] = g_strdup (g_get_user_config_dir ());
      #endif

      dirs = g_realloc (dirs, ++dc * sizeof (*dirs));

      #ifdef G_OS_WIN32
        dirs[dc - 1] = win32_build_path (1, FNN_PROFILE_DIR, NULL);
      #else
        dirs[dc - 1] = FNN_PROFILE_DIR;
      #endif

      /* NULL-termination */
      dirs = g_realloc (dirs, ++dc * sizeof (*dirs));
      dirs[dc - 1] = NULL;

      for (i = 0; dirs != NULL && dirs[i] != NULL; ++i)
        g_message ("profile_dirs: %i. %s", i, dirs[i]);
    }

  return dirs;
}

void
fnn_init (Global *ext_global)
{
  tglobal = ext_global;
}

void
fnn_deinit (Global *ext_global)
{
  GKeyFile *settings = ext_global->settings;

  if (settings != NULL)
    {
      HyScanUnitsGeo geo_units;

      if (ext_global->panels != NULL)
        {
          GHashTableIter iter;
          gpointer k, v;
          g_hash_table_iter_init (&iter, ext_global->panels);
          while (g_hash_table_iter_next (&iter, &k, &v))
            {
              FnnPanel *panel = v;
              keyfile_double_write_helper (settings, panel->name, "sonar.distance",    panel->current.distance);
              keyfile_double_write_helper (settings, panel->name, "sonar.signal",      panel->current.signal);
              keyfile_double_write_helper (settings, panel->name, "sonar.gain0",       panel->current.gain0);
              keyfile_double_write_helper (settings, panel->name, "sonar.gain_step",   panel->current.gain_step);
              keyfile_double_write_helper (settings, panel->name, "sonar.level",       panel->current.level);
              keyfile_double_write_helper (settings, panel->name, "sonar.sensitivity", panel->current.sensitivity);
              keyfile_double_write_helper (settings, panel->name, "sonar.log_gain0",   panel->current.log_gain0);
              keyfile_double_write_helper (settings, panel->name, "sonar.log_beta",    panel->current.log_beta);
              keyfile_double_write_helper (settings, panel->name, "sonar.log_alpha",   panel->current.log_alpha);
              keyfile_double_write_helper (settings, panel->name, "sonar.const_gain0", panel->current.const_gain0);
              keyfile_double_write_helper (settings, panel->name, "sonar.tvg_mode",    panel->current.mode);

              keyfile_double_write_helper (settings, panel->name, "cur_black2",            panel->vis_current.black);
              keyfile_double_write_helper (settings, panel->name, "cur_gamma2",            panel->vis_current.gamma);
              keyfile_double_write_helper (settings, panel->name, "cur_white2",            panel->vis_current.white);
              keyfile_double_write_helper (settings, panel->name, "cur_color_map2",        panel->vis_current.colormap);
              keyfile_double_write_helper (settings, panel->name, "cur_sensitivity2",      panel->vis_current.sensitivity);
            }
        }

      if (ext_global->project_name != NULL)
        keyfile_string_write_helper (settings, "common", "project", ext_global->project_name);

      geo_units = hyscan_units_get_geo (ext_global->units);
      keyfile_string_write_helper (settings, "units", "geo", hyscan_units_id_by_geo (geo_units));
    }

  tglobal = NULL;
}
