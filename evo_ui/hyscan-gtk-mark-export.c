/*
 * hyscan-gtk-mark-export.c
 *
 *  Created on: 20 нояб. 2019 г.
 *      Author: Andrey Zakharov <zaharov@screen-co.ru>
 */
#include "hyscan-gtk-mark-export.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <hyscan-mark.h>

#define GETTEXT_PACKAGE "hyscanfnn-evoui"
#include <glib/gi18n-lib.h>

static gchar hyscan_gtk_mark_export_header[] = "LAT,LON,NAME,DESCRIPTION,COMMENT,NOTES,DATE,TIME\n";

/* Функция генерации строки. */
static gchar *
hyscan_gtk_mark_export_formatter (gdouble      lat,
                                  gdouble      lon,
                                  gint64       unixtime,
                                  const gchar *name,
                                  const gchar *description,
                                  const gchar *comment,
                                  const gchar *notes)
{
  gchar *str, *date, *time;
  gchar lat_str[128];
  gchar lon_str[128];
  GDateTime *dt = NULL;

  dt = g_date_time_new_from_unix_local (1e-6 * unixtime);

  if (dt == NULL)
    return NULL;

  date = g_date_time_format (dt, "%m/%d/%Y");
  time = g_date_time_format (dt, "%H:%M:%S");

  g_ascii_dtostr (lat_str, sizeof (lat_str), lat);
  g_ascii_dtostr (lon_str, sizeof (lon_str), lon);

  (name == NULL) ? name = "" : 0;
  (description == NULL) ? description = "" : 0;
  (comment == NULL) ? comment = "" : 0;
  (notes == NULL) ? notes = "" : 0;

  /* LAT,LON,NAME,DESCRIPTION,COMMENT,NOTES,DATE,TIME\n */
  str = g_strdup_printf ("%s,%s,%s,%s,%s,%s,%s,%s\n",
                         lat_str, lon_str,
                         name, description, comment, notes,
                         date, time);
  g_date_time_unref (dt);

  return str;
}

/* Функция печати всех меток. */
static gchar *
hyscan_gtk_mark_export_print_marks (GHashTable *wf_marks,
                                    GHashTable *geo_marks)
{
  GHashTableIter iter;
  gchar *mark_id = NULL;
  GString *str = g_string_new (NULL);

  /* Водопадные метки. */
  if (wf_marks != NULL)
    {
      HyScanMarkLocation *location;

      g_hash_table_iter_init (&iter, wf_marks);
      while (g_hash_table_iter_next (&iter, (gpointer *) &mark_id, (gpointer *) &location))
        {
          gchar *line;

          if (location->mark->type != HYSCAN_MARK_WATERFALL)
            continue;

          line = hyscan_gtk_mark_export_formatter (location->mark_geo.lat,
                                                   location->mark_geo.lon,
                                                   location->mark->ctime,
                                                   location->mark->name,
                                                   location->mark->description,
                                                   NULL, NULL);

          g_string_append (str, line);
          g_free (line);
        }
    }
  /* Гео-метки. */
  if (geo_marks != NULL)
    {
      HyScanMarkGeo *geo_mark;

      g_hash_table_iter_init (&iter, geo_marks);
      while (g_hash_table_iter_next (&iter, (gpointer *) &mark_id, (gpointer *) &geo_mark))
        {
          gchar *line;

          if (geo_mark->type != HYSCAN_MARK_GEO)
            continue;

          line = hyscan_gtk_mark_export_formatter (geo_mark->center.lat,
                                                   geo_mark->center.lon,
                                                   geo_mark->ctime,
                                                   geo_mark->name,
                                                   geo_mark->description,
                                                   NULL, NULL);

          g_string_append (str, line);
          g_free (line);
        }
    }

  /* Возвращаю строку в чистом виде. */
  return g_string_free (str, FALSE);
}

/**
 * hyscan_gtk_mark_export_save_as_csv:
 * @window: указатель на окно, которое вызывает диалог выбора файла для сохранения данных
 * @ml_model: указатель на модель водопадных меток с данными о положении в пространстве
 * @mark_geo_model: указатель на модель геометок
 * @projrct_name: указатель на название проекта. Используется как имя файла по умолчанию
 *
 * сохраняет список меток в формате CSV.
 */
void
hyscan_gtk_mark_export_save_as_csv (GtkWindow          *window,
                                    HyScanMarkLocModel *ml_model,
                                    HyScanObjectModel  *mark_geo_model,
                                    gchar              *project_name)
{
  FILE *file = NULL;
  gchar *filename = NULL;
  GtkWidget *dialog;
  GHashTable *wf_marks, *geo_marks;
  gint res;
  gchar *marks;

  wf_marks = hyscan_mark_loc_model_get (ml_model);
  geo_marks = hyscan_object_model_get (mark_geo_model);

  if (wf_marks == NULL || geo_marks == NULL)
    return;

  dialog = gtk_file_chooser_dialog_new (_("Save file"),
                                        window,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        _("Save"),
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  filename = g_strdup_printf ("%s.csv", project_name);

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
#ifdef G_OS_WIN32
  fopen_s (&file, filename, "w");
#else
  file = fopen (filename, "w");
#endif
  if (file != NULL)
    {
      fwrite (hyscan_gtk_mark_export_header, sizeof (gchar), g_utf8_strlen (hyscan_gtk_mark_export_header, -1), file);

      marks = hyscan_gtk_mark_export_print_marks (wf_marks, geo_marks);
      fwrite (marks, sizeof (gchar), g_utf8_strlen (marks, -1), file);

      fclose (file);
    }

  g_hash_table_unref (wf_marks);
  g_hash_table_unref (geo_marks);
  g_free (filename);
  gtk_widget_destroy (dialog);
}

/**
 * hyscan_gtk_mark_export_copy_to_clipboard:
 * @ml_model: указатель на модель водопадных меток с данными о положении в пространстве
 * @mark_geo_model: указатель на модель геометок
 * @projrct_name: указатель на назване проекта. Добавляется в буфер обмена как описание содержимого
 *
 * копирует список меток в буфер обмена
 */
void
hyscan_gtk_mark_export_copy_to_clipboard (HyScanMarkLocModel *ml_model,
                                          HyScanObjectModel  *mark_geo_model,
                                          gchar              *project_name)
{
  GtkClipboard *clipboard;
  GHashTable *wf_marks, *geo_marks;
  GDateTime *local;
  gchar *str, *marks;

  wf_marks = hyscan_mark_loc_model_get (ml_model);
  geo_marks = hyscan_object_model_get (mark_geo_model);

  if (wf_marks == NULL || geo_marks == NULL)
    return;

  local = g_date_time_new_now_local ();

  marks = hyscan_gtk_mark_export_print_marks (wf_marks, geo_marks);
  str = g_strdup_printf (_("%s\nProject: %s\n%s%s"),
                         g_date_time_format (local, "%A %B %e %T %Y"),
                         project_name, hyscan_gtk_mark_export_header, marks);

  g_hash_table_unref (wf_marks);
  g_hash_table_unref (geo_marks);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, str, -1);

  g_free (str);
  g_free (marks);
}
