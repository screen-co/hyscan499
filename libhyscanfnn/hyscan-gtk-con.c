/* hyscan-gtk-con.h
 *
 * Copyright 2019 Screen LLC, Alexander Dmitriev <m1n7@yandex.ru>
 *
 * This file is part of HyScanGtk.
 *
 * HyScanGtk is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HyScanGtk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Alternatively, you can license this code under a commercial license.
 * Contact the Screen LLC in this case - <info@screen-co.ru>.
 */

/* HyScanGtk имеет двойную лицензию.
 *
 * Во-первых, вы можете распространять HyScanGtk на условиях Стандартной
 * Общественной Лицензии GNU версии 3, либо по любой более поздней версии
 * лицензии (по вашему выбору). Полные положения лицензии GNU приведены в
 * <http://www.gnu.org/licenses/>.
 *
 * Во-вторых, этот программный код можно использовать по коммерческой
 * лицензии. Для этого свяжитесь с ООО Экран - <info@screen-co.ru>.
 */

#include "hyscan-gtk-con.h"
#include "hyscan-gtk-profile-db.h"
#include "hyscan-gtk-profile-hw.h"
#include "hyscan-gtk-profile-offset.h"
#include "hyscan-fnn-project.h"
#include "sonar-configure.h"
#include <hyscan-async.h>
#define GETTEXT_PACKAGE "libhyscanfnn"
#include <glib/gi18n-lib.h>

#define HYSCAN_GTK_CON_DB "db"
#define HYSCAN_GTK_CON_HW_CHK "hw_check"
#define HYSCAN_GTK_CON_HW "hw"
#define HYSCAN_GTK_CON_BIND "bind"
#define HYSCAN_GTK_CON_OF "offset"
enum
{
  PROP_0,
  PROP_DRIVERS,
  PROP_CONFIG,
  PROP_SYSFOLDER,
  PROP_DB,
};

struct _HyScanGtkConPrivate
{
  GKeyFile      *kf;
  gchar        **drivers;
  gchar         *sysfolder;
  HyScanDB      *db;

  GtkLabel      *project_label;
  GtkLabel      *hw_label;

  GtkWidget *hw_page;

  HyScanProfile *hw_profile;
  HyScanProfile *of_profile;

  struct
    {
      GtkWidget *bar;
      GtkWidget *label;
    } progress;

  gchar         *hw_profile_file;
  gchar         *of_profile_file;

  gint           connect_page;

  HyScanAsync   *async;
  HyScanControl *control;

  gboolean       result;
};

static void    hyscan_gtk_con_set_property             (GObject               *object,
                                                        guint                  prop_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
static void    hyscan_gtk_con_object_constructed       (GObject               *object);
static void    hyscan_gtk_con_object_finalize          (GObject               *object);

// static void    hyscan_gtk_con_prepare                  (HyScanGtkCon    *self,
                                                              // GtkWidget             *page);

static void    hyscan_gtk_con_make_intro_page          (HyScanGtkCon    *self);
// static void    hyscan_gtk_con_make_db_page             (HyScanGtkCon    *self);
static void    hyscan_gtk_con_make_hw_page             (HyScanGtkCon    *self);
// static void    hyscan_gtk_con_make_offset_page         (HyScanGtkCon    *self);
// static void    hyscan_gtk_con_make_confirm_page        (HyScanGtkCon    *self);
static void    hyscan_gtk_con_make_connect_page        (HyScanGtkCon    *self);

static void    hyscan_gtk_con_selected_hw              (HyScanGtkProfile      *page,
                                                        HyScanProfile         *profile,
                                                        HyScanGtkCon    *self);

static void    hyscan_gtk_con_apply                    (HyScanGtkCon    *self);

static void    hyscan_gtk_con_finished                 (HyScanGtkCon    *self,
                                                              gboolean               success);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkCon, hyscan_gtk_con, GTK_TYPE_ASSISTANT);

static void
hyscan_gtk_con_class_init (HyScanGtkConClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = hyscan_gtk_con_set_property;
  oclass->constructed = hyscan_gtk_con_object_constructed;
  oclass->finalize = hyscan_gtk_con_object_finalize;

  g_object_class_install_property (oclass, PROP_DRIVERS,
    g_param_spec_pointer ("drivers", "DriverPaths", "Where to look for drivert",
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (oclass, PROP_CONFIG,
    g_param_spec_pointer ("kf", "kf", "config kf",
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (oclass, PROP_SYSFOLDER,
    g_param_spec_string ("sysfolder", "SysFolder", "Folder with system profiles", NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (oclass, PROP_DB,
    g_param_spec_object ("db", "db", "db",
                         HYSCAN_TYPE_DB,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_gtk_con_init (HyScanGtkCon *gtk_con)
{
  gtk_con->priv = hyscan_gtk_con_get_instance_private (gtk_con);
}

static void
hyscan_gtk_con_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  HyScanGtkCon *self = HYSCAN_GTK_CON (object);

  switch (prop_id)
    {
    case PROP_DRIVERS:
      self->priv->drivers = g_strdupv (g_value_get_pointer (value));
      break;

    case PROP_CONFIG:
      self->priv->kf = g_key_file_ref (g_value_get_pointer (value));
      break;

    case PROP_SYSFOLDER:
      self->priv->sysfolder = g_value_dup_string (value);
      break;

    case PROP_DB:
      self->priv->db = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_gtk_con_object_constructed (GObject *object)
{
  HyScanGtkCon *self = HYSCAN_GTK_CON (object);
  HyScanGtkConPrivate *priv = self->priv;

  G_OBJECT_CLASS (hyscan_gtk_con_parent_class)->constructed (object);

  priv->async = hyscan_async_new ();
  hyscan_gtk_con_make_intro_page (self);

  hyscan_gtk_con_make_hw_page (self);
  hyscan_gtk_con_make_connect_page (self);

  g_signal_connect (self, "apply", G_CALLBACK (hyscan_gtk_con_apply), NULL);
}

static void
hyscan_gtk_con_object_finalize (GObject *object)
{
  HyScanGtkCon *self = HYSCAN_GTK_CON (object);
  HyScanGtkConPrivate *priv = self->priv;

  g_clear_object (&priv->async);
  g_clear_object (&priv->hw_profile);
  g_clear_object (&priv->of_profile);

  g_clear_object (&priv->db);
  g_clear_object (&priv->control);

  g_clear_pointer (&priv->kf, g_key_file_unref);
  g_clear_pointer (&priv->drivers, g_strfreev);
  g_clear_pointer (&priv->sysfolder, g_free);

  G_OBJECT_CLASS (hyscan_gtk_con_parent_class)->finalize (object);
}

// static void
// hyscan_gtk_con_prepare (HyScanGtkCon *self,
//                         GtkWidget    *page)
// {
//   GtkAssistant *assistant = GTK_ASSISTANT (self);
//   HyScanGtkConPrivate *priv = self->priv;
//   gint cur_page, prev_page;

//   cur_page = gtk_assistant_get_current_page (assistant);
//   prev_page = priv->previous_page;
//   priv->previous_page = cur_page;

//    Пропуск страницы с оффсетами, если не выбран профиль оборудования.
//   if (cur_page == priv->offset_page && priv->hw_profile == NULL)
//     {
//       if (prev_page < cur_page)
//         gtk_assistant_next_page (assistant);
//       else
//         gtk_assistant_previous_page (assistant);
//     }
//   if (cur_page == priv->connect_page)
//     gtk_assistant_commit (assistant);
// }
static void
hyscan_gtk_con_run_project_manager (GObject *emitter,
                                    HyScanGtkCon *self)
{
  HyScanGtkConPrivate *priv = self->priv;
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  HyScanDBInfo *info = hyscan_db_info_new (priv->db);
  GtkWidget *dialog;
  gint res;
  gchar *project_name = NULL;

  dialog = hyscan_fnn_project_new (priv->db, info, GTK_WINDOW (window));
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  hyscan_fnn_project_get (HYSCAN_FNN_PROJECT (dialog), &project_name, NULL);
  gtk_widget_destroy (dialog);
  gtk_widget_destroy (window);

  /* Если был нажат крестик - сорян, сваливаем отсюда. */
  if (res == HYSCAN_FNN_PROJECT_OPEN || res == HYSCAN_FNN_PROJECT_CREATE)
    {
      keyfile_string_write_helper (priv->kf, "common", "project", project_name);
      gtk_label_set_text (priv->project_label, project_name);
    }

  g_object_unref (info);
  g_free (project_name);
}

static void
hyscan_gtk_con_run_view (GObject *emitter,
                                    HyScanGtkCon *self)
{
  HyScanGtkConPrivate *priv = self->priv;

  g_clear_object (&priv->control);
  gtk_assistant_set_page_complete (GTK_ASSISTANT (self), priv->hw_page, TRUE);
  gtk_assistant_set_current_page (GTK_ASSISTANT (self), self->priv->connect_page);
  hyscan_gtk_con_finished (self, TRUE);
}

static void
hyscan_gtk_con_run_scan (GObject *emitter,
                         HyScanGtkCon *self)
{
  {
    gchar *pf_file;
    HyScanProfileHW *pfl;

    pf_file = keyfile_string_read_helper (self->priv->kf, "common", "hardware");

    if (pf_file != NULL)
      {

        pfl = hyscan_profile_hw_new (pf_file);
        hyscan_profile_hw_set_driver_paths(pfl, self->priv->drivers);
        hyscan_profile_read (HYSCAN_PROFILE (pfl));
        g_free (pf_file);

        hyscan_gtk_con_selected_hw (self->priv->hw_page, pfl, self);
        gtk_assistant_set_current_page (GTK_ASSISTANT (self), self->priv->connect_page);
      }
  }

  hyscan_gtk_con_apply (self);
}

static void
hyscan_gtk_con_make_intro_page (HyScanGtkCon *self)
{
  HyScanGtkConPrivate *priv = self->priv;
  GtkWidget *intro;
  GtkBuilder *b;
  b = gtk_builder_new_from_resource ("/org/libhyscanfnn/gtk/hyscan-gtk-con.ui");

  intro = GTK_WIDGET(gtk_builder_get_object (b, "intro"));
  priv->project_label = GTK_LABEL(gtk_builder_get_object (b, "pj_name"));
  priv->hw_label = GTK_LABEL(gtk_builder_get_object (b, "hw_name"));

  gtk_builder_add_callback_symbol (b, "hyscan_gtk_con_run_project_manager", hyscan_gtk_con_run_project_manager);
  gtk_builder_add_callback_symbol (b, "hyscan_gtk_con_run_view", hyscan_gtk_con_run_view);
  gtk_builder_add_callback_symbol (b, "hyscan_gtk_con_run_scan", hyscan_gtk_con_run_scan);
  gtk_builder_connect_signals (b, self);

  {
    gchar *pf_file;
    HyScanProfileHW *pfl;

    pf_file = keyfile_string_read_helper (priv->kf, "common", "hardware");

    if (pf_file != NULL)
      {

        pfl = hyscan_profile_hw_new (pf_file);
        g_free (pf_file);

        hyscan_profile_read (HYSCAN_PROFILE (pfl));
        gtk_label_set_text (priv->hw_label, hyscan_profile_get_name(HYSCAN_PROFILE (pfl)));
      }
    else
      {
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object (b, "scan")), FALSE);
      }
  }
  {
    gchar *pjct;
    pjct = keyfile_string_read_helper (priv->kf, "common", "project");
    if (pjct != NULL)
      {
        gtk_label_set_text(priv->project_label, pjct);
        g_free (pjct);
      }
  }

  gtk_assistant_append_page (GTK_ASSISTANT (self), intro);

  gtk_assistant_set_page_type (GTK_ASSISTANT (self), intro, GTK_ASSISTANT_PAGE_INTRO);
  gtk_assistant_set_page_title (GTK_ASSISTANT (self), intro, _("Intro"));
  gtk_assistant_set_page_complete (GTK_ASSISTANT (self), intro, TRUE);
}

// static void
// hyscan_gtk_con_make_db_page (HyScanGtkCon *self)
// {
//   GtkWidget *page = hyscan_gtk_profile_db_new (self->priv->sysfolder);

//   g_signal_connect (page, "selected",
//                     G_CALLBACK (hyscan_gtk_con_selected_db), self);
//   gtk_assistant_append_page (GTK_ASSISTANT (self), page);
//   gtk_assistant_set_page_title (GTK_ASSISTANT (self), page, _("Database"));
// }

static void
hyscan_gtk_con_make_hw_page (HyScanGtkCon *self)
{
  GtkWidget *page = hyscan_gtk_profile_hw_new (self->priv->sysfolder, self->priv->drivers);

  g_signal_connect (page, "selected", G_CALLBACK (hyscan_gtk_con_selected_hw), self);
  self->priv->hw_page = page;
  gtk_assistant_append_page (GTK_ASSISTANT (self), page);
  gtk_assistant_set_page_title (GTK_ASSISTANT (self), page, _("Hardware"));
  gtk_assistant_set_page_complete (GTK_ASSISTANT (self), page, TRUE);
  gtk_assistant_set_page_type (GTK_ASSISTANT (self), page, GTK_ASSISTANT_PAGE_CONFIRM);
}

// static void
// hyscan_gtk_con_make_offset_page (HyScanGtkCon *self)
// {
//   GtkWidget *page = hyscan_gtk_profile_offset_new (self->priv->sysfolder);

//   g_signal_connect (page, "selected",
//                     G_CALLBACK (hyscan_gtk_con_selected_offset), self);
//   self->priv->offset_page = gtk_assistant_append_page (GTK_ASSISTANT (self), page);
//   gtk_assistant_set_page_title (GTK_ASSISTANT (self), page, _("Antennas positions"));
//   gtk_assistant_set_page_complete (GTK_ASSISTANT (self), page, TRUE);
// }

// static void
// hyscan_gtk_con_make_confirm_page (HyScanGtkCon *self)
// {
//   GtkWidget *page = gtk_label_new ("Review your selections");

//   gtk_assistant_append_page (GTK_ASSISTANT (self), page);
//   gtk_assistant_set_page_type (GTK_ASSISTANT (self), page, GTK_ASSISTANT_PAGE_CONFIRM);
//   gtk_assistant_set_page_title (GTK_ASSISTANT (self), page, _("Confirm"));
//   gtk_assistant_set_page_complete (GTK_ASSISTANT (self), page, TRUE);
// }

static void
hyscan_gtk_con_make_connect_page (HyScanGtkCon *self)
{
  GtkWidget *box;
  HyScanGtkConPrivate *priv = self->priv;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  priv->progress.label = gtk_label_new (NULL);
  priv->progress.bar = gtk_progress_bar_new ();

  gtk_box_pack_start (GTK_BOX (box), priv->progress.label, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), priv->progress.bar, FALSE, FALSE, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);

  self->priv->connect_page = gtk_assistant_append_page (GTK_ASSISTANT (self), box);
  gtk_assistant_set_page_type (GTK_ASSISTANT (self), box, GTK_ASSISTANT_PAGE_SUMMARY);
  gtk_assistant_set_page_title (GTK_ASSISTANT (self), box, _("Connection"));
}


static void
hyscan_gtk_con_selected_hw (HyScanGtkProfile   *page,
                            HyScanProfile      *profile,
                            HyScanGtkCon       *self)
{
  g_clear_object (&self->priv->hw_profile);
  g_clear_pointer (&self->priv->hw_profile_file, g_free);

  gtk_assistant_set_page_complete (GTK_ASSISTANT (self), GTK_WIDGET (page), profile != NULL);
  if (profile != NULL)
    {
      self->priv->hw_profile = g_object_ref (profile);
      self->priv->hw_profile_file = g_strdup (hyscan_profile_get_file (profile));
      keyfile_string_write_helper (self->priv->kf, "common", "hardware", self->priv->hw_profile_file);
    }
}

static void
hyscan_gtk_con_apply (HyScanGtkCon *self)
{
  HyScanGtkConPrivate *priv = self->priv;

  gtk_label_set_text (GTK_LABEL (self->priv->progress.label), _("Connecting..."));

  if (!hyscan_profile_hw_check (priv->hw_profile))
    {
      hyscan_gtk_con_finished (self, FALSE);
      return;
    }

  priv->control = hyscan_profile_hw_connect (priv->hw_profile);

  if (priv->control == NULL)
    {
      hyscan_gtk_con_finished (self, FALSE);
      return;
    }

  hyscan_gtk_con_finished (self, TRUE);
}



static void
hyscan_gtk_con_finished (HyScanGtkCon *self,
                         gboolean      success)
{
  GtkAssistant *wizard = GTK_ASSISTANT (self);
  GtkWidget *current_page;
  GtkLabel *label = GTK_LABEL (self->priv->progress.label);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->priv->progress.bar), 1.0);

  current_page = gtk_assistant_get_nth_page (wizard, gtk_assistant_get_current_page (wizard));
  gtk_assistant_set_page_complete (wizard, current_page, TRUE);

  if (success)
    gtk_label_set_text (label, _("Connection completed. HyScan is ready to launch. "));
  else
    gtk_label_set_text (label, _("Connection failed. "));

  self->priv->result = success;
}

GtkWidget *
hyscan_gtk_con_new (const gchar  *sysfolder,
                    gchar       **drivers,
                    GKeyFile     *kf,
                    HyScanDB     *db)
{
  return g_object_new (HYSCAN_TYPE_GTK_CON,
                       "use-header-bar", TRUE,
                       "drivers", drivers,
                       "sysfolder", sysfolder,
                       "kf", kf,
                       "db", db,
                       NULL);
}

gboolean
hyscan_gtk_con_get_result (HyScanGtkCon *self)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_CON (self), NULL);

  return self->priv->result;
}

HyScanDB *
hyscan_gtk_con_get_db (HyScanGtkCon *self)
{
  HyScanGtkConPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_GTK_CON (self), NULL);
  priv = self->priv;

  return (priv->db != NULL) ? g_object_ref (priv->db) : NULL;
}

HyScanControl *
hyscan_gtk_con_get_control (HyScanGtkCon *self)
{
  HyScanGtkConPrivate *priv;

  g_return_val_if_fail (HYSCAN_IS_GTK_CON (self), NULL);
  priv = self->priv;

  return (priv->control != NULL) ? g_object_ref (priv->control) : NULL;
}


const gchar *
hyscan_gtk_con_get_hw_name (HyScanGtkCon *self)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_CON (self), NULL);

  return self->priv->hw_profile_file;
}
const gchar *
hyscan_gtk_con_get_offset_name (HyScanGtkCon *self)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_CON (self), NULL);

  return self->priv->of_profile_file;
}
