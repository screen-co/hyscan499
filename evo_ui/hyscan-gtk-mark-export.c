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
#include <locale.h>

#define GETTEXT_PACKAGE "hyscanfnn-evoui"
#include <glib/gi18n-lib.h>

/* @brief Функция hyscan_gtk_mark_export_save_as_csv сохраняет список меток в формате CSV.
 * @param window - указатель на окно, которое вызывает диалог выбора файла для сохранения данных;
 * @param ml_model - указатель на модель водопадных меток с данными о положении в пространстве;
 * @param mark_geo_model - указатель на модель геометок;
 * @param projrct_name - указатель на название проекта. Используется как имя файла по умолчанию.
 * */
void
hyscan_gtk_mark_export_save_as_csv       (GtkWindow          *window,
                                          HyScanMarkLocModel *ml_model,
                                          HyScanObjectModel  *mark_geo_model,
                                          gchar              *project_name)
{
	gchar *filename = NULL;
  GtkWidget *dialog;
  GtkFileChooser *chooser;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
  GHashTable *wf_marks, *geo_marks;
  gint res;

  wf_marks = hyscan_mark_loc_model_get (ml_model);
  geo_marks = hyscan_object_model_get (mark_geo_model);

  if (wf_marks == NULL || geo_marks == NULL)
    return;

  dialog = gtk_file_chooser_dialog_new (_("Save file"),
                                        window,
                                        action,
                                        _("Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        _("Save"),
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  chooser = GTK_FILE_CHOOSER (dialog);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  filename = g_strdup_printf ("%s.csv", project_name);

  gtk_file_chooser_set_current_name (chooser, filename);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (chooser);
      FILE *file = fopen (filename, "w");
      if (file)
        {
          GHashTableIter hash_iter;
          gchar *mark_id = NULL;

          HyScanMarkGeo *geo_mark;
          HyScanMarkLocation *location;

          gchar *str = "LAT,LON,NAME,DESCRIPTION,COMMENT,NOTES,DATE,TIME\n";

          fwrite(str , sizeof (gchar), g_utf8_strlen (str, -1) , file);
          /* Водопадные метки. */
          if (wf_marks)
            {
              g_hash_table_iter_init (&hash_iter, wf_marks);

              while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &location))
                {
                  GDateTime *local;
                  gchar *lat, *lon, *name, *description, *comment, *notes, *date, *time, *delimiter;

                  delimiter = localeconv()->decimal_point;
                  lat = g_strdup_printf ("%.6f", location->mark_geo.lat);
                  g_strdelimit(lat, delimiter, '.');
                  lon = g_strdup_printf ("%.6f", location->mark_geo.lon);
                  g_strdelimit(lon, delimiter, '.');
                  local  = g_date_time_new_from_unix_local (1e-6 * location->mark->ctime);
                  date = g_date_time_format (local, "%m/%d/%Y");
                  time = g_date_time_format (local, "%H:%M:%S");
                  name = g_strdup (location->mark->name);
                  description = g_strdup (location->mark->description);
                  comment = g_strdup ("");
                  notes = g_strdup ("");

                  if (location->mark->type == HYSCAN_MARK_WATERFALL)
                    {
                      str = "";
                      str = g_strconcat (str, lat, ",", lon, ",", name, ",", description, ",",
                                         comment, ",", notes, ",", date, ",", time, "\n", (gchar*) NULL);
                      fwrite(str , sizeof (gchar), g_utf8_strlen(str, -1) , file);
                    }

                  g_free (lon);
                  g_free (lat);
                  g_free (name);
                  g_free (description);
                  g_free (comment);
                  g_free (notes);
                  g_free (date);
                  g_free (time);
                  g_free (str);

                  lon = lat = name = description = comment = notes = date = time = str = NULL;

                  g_date_time_unref (local);
                }

              g_hash_table_unref (wf_marks);
            }
          /* Гео-метки. */
          if (geo_marks)
            {
              g_hash_table_iter_init (&hash_iter, geo_marks);

              while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &geo_mark))
	              {
                  GDateTime *local;
                  gchar *lat, *lon, *name, *description, *comment, *notes, *date, *time, *delimiter;

                  delimiter = localeconv()->decimal_point;
                  lat = g_strdup_printf ("%.6f", geo_mark->center.lat);
                  g_strdelimit(lat, delimiter, '.');
                  lon = g_strdup_printf ("%.6f", geo_mark->center.lon);
                  g_strdelimit(lon, delimiter, '.');
                  local  = g_date_time_new_from_unix_local (1e-6 * geo_mark->ctime);
                  date = g_date_time_format (local, "%m/%d/%Y");
                  time = g_date_time_format (local, "%H:%M:%S");
                  name = g_strdup (geo_mark->name);
                  description = g_strdup (geo_mark->description);
                  comment = g_strdup ("");
                  notes = g_strdup ("");

                  if (geo_mark->type == HYSCAN_MARK_GEO)
                    {
                      str = "";
                      str = g_strconcat (str, lat, ",", lon, ",", name, ",", description, ",",
                                         comment, ",", notes, ",", date, ",", time, "\n", (gchar*) NULL);
                      fwrite(str , sizeof (gchar), g_utf8_strlen(str, -1) , file);
                    }

                  g_free (lon);
                  g_free (lat);
                  g_free (name);
                  g_free (description);
                  g_free (comment);
                  g_free (notes);
                  g_free (date);
                  g_free (time);
                  g_free (str);

                  lon = lat = name = description = comment = notes = date = time = str = NULL;

	                g_date_time_unref (local);
                }
              g_hash_table_unref (geo_marks);
            }

          fclose (file);
        }
      g_free (filename);
    }
  gtk_widget_destroy (dialog);
}

/* @brief Функция hyscan_gtk_mark_export_copy_to_clipboard копирует список меток в буфер обмена.
 * @param ml_model - указатель на модель водопадных меток с данными о положении в пространстве;
 * @param mark_geo_model - указатель на модель геометок;
 * @param projrct_name - указатель на название проекта. Добавляется в буфер обмена как описание содержимого.
 * */
void
hyscan_gtk_mark_export_copy_to_clipboard (HyScanMarkLocModel *ml_model,
                                          HyScanObjectModel  *mark_geo_model,
                                          gchar              *project_name)
{
  GtkClipboard* clipboard;
  GHashTableIter hash_iter;
  HyScanMarkGeo *geo_mark;
  HyScanMarkLocation *location;
  GHashTable *wf_marks, *geo_marks;
  time_t nt;
  gchar *str = NULL, *mark_id = NULL;

  wf_marks = hyscan_mark_loc_model_get (ml_model);
  geo_marks = hyscan_object_model_get (mark_geo_model);

  if (wf_marks == NULL || geo_marks == NULL)
    return;

  nt = time(NULL);
  str = g_strdup_printf (_("%sProject: %s\n%s\n"),
                            ctime (&nt),
                            project_name,
                            "LAT,LON,NAME,DESCRIPTION,COMMENT,NOTES,DATE,TIME");
  /* Водопадные метки. */
  if (wf_marks)
    {
      g_hash_table_iter_init (&hash_iter, wf_marks);

      while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &location))
        {
          GDateTime *local;
          gchar *lat, *lon, *name, *description, *comment, *notes, *date, *time, *delimiter;

          delimiter = localeconv()->decimal_point;
          lat = g_strdup_printf ("%.6f", location->mark_geo.lat);
          g_strdelimit(lat, delimiter, '.');
          lon = g_strdup_printf ("%.6f", location->mark_geo.lon);
          g_strdelimit(lon, delimiter, '.');
          local  = g_date_time_new_from_unix_local (1e-6 * location->mark->ctime);
          date = g_date_time_format (local, "%m/%d/%Y");
          time = g_date_time_format (local, "%H:%M:%S");
          name = g_strdup (location->mark->name);
          description = g_strdup (location->mark->description);
          comment = g_strdup ("");
          notes = g_strdup ("");

          if (location->mark->type == HYSCAN_MARK_WATERFALL)
            {
              str = g_strconcat (str, lat, ",", lon, ",", name, ",", description, ",",
                                 comment, ",", notes, ",", date, ",", time, "\n", (gchar*) NULL);
            }

          g_free (lon);
          g_free (lat);
          g_free (name);
          g_free (description);
          g_free (comment);
          g_free (notes);
          g_free (date);
          g_free (time);

          lon = lat = name = description = comment = notes = date = time = NULL;

          g_date_time_unref (local);
        }

      g_hash_table_unref (wf_marks);
    }
  /* Гео-метки. */
  if (geo_marks)
    {
      g_hash_table_iter_init (&hash_iter, geo_marks);

      while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &geo_mark))
        {
          GDateTime *local;
          gchar *lat, *lon, *name, *description, *comment, *notes, *date, *time, *delimiter;

          delimiter = localeconv()->decimal_point;
          lat = g_strdup_printf ("%.6f", geo_mark->center.lat);
          g_strdelimit(lat, delimiter, '.');
          lon = g_strdup_printf ("%.6f", geo_mark->center.lon);
          g_strdelimit(lon, delimiter, '.');
          local  = g_date_time_new_from_unix_local (1e-6 * geo_mark->ctime);
          date = g_date_time_format (local, "%m/%d/%Y");
          time = g_date_time_format (local, "%H:%M:%S");
          name = g_strdup (geo_mark->name);
          description = g_strdup (geo_mark->description);
          comment = g_strdup ("");
          notes = g_strdup ("");

          if (geo_mark->type == HYSCAN_MARK_GEO)
            {
              str = g_strconcat (str, lat, ",", lon, ",", name, ",", description, ",",
                                 comment, ",", notes, ",", date, ",", time, "\n", (gchar*) NULL);
            }

          g_free (lon);
          g_free (lat);
          g_free (name);
          g_free (description);
          g_free (comment);
          g_free (notes);
          g_free (date);
          g_free (time);

          lon = lat = name = description = comment = notes = date = time = NULL;

          g_date_time_unref (local);
        }
      g_hash_table_unref (geo_marks);
    }
  str[g_utf8_strlen(str, -1) - 1] = 0;
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text(clipboard, str, -1);
}
