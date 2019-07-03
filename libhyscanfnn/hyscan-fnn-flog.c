#include "hyscan-fnn-flog.h"
#include <stdio.h>
#include <glib/gstdio.h>

typedef struct
{
  FILE *file;

  GMutex lock;
  guint  size;
  guint  max_size;
  gchar *filename;
  gchar *filename_old;
  gint64 start_time;
} HyScanFnnFlog;

static HyScanFnnFlog flog;

/* Обработчик вывода лог сообщений в файл. */
static void
hyscan_fnn_flog_handler (const gchar    *log_domain,
                         GLogLevelFlags  log_level,
                         const gchar    *message,
                         gpointer        user_data)
{
  if (log_level & G_LOG_LEVEL_DEBUG)
    return;

  fprintf (flog.file, "[%9.3f] %s: %s\n", 1e-6 * (g_get_monotonic_time () - flog.start_time), log_domain, message);
  fflush (flog.file);
}

static void
hyscan_fnn_flog_open_file ()
{
  /* Открываем файл в бинарном режиме, чтобы узнать его размер в байтах. */
  flog.file = g_fopen (flog.filename, "rb");
  if (flog.file != NULL)
    {
      gsize filesize;
      
      /* Проверяем размер лога. */
      fseek (flog.file, 0, SEEK_END);
      filesize = ftell (flog.file);
      if (filesize > flog.max_size)
        {
          fclose (flog.file);
          g_unlink (flog.filename_old);
          g_rename (flog.filename, flog.filename_old);
        }
      
      /* Закрываем файл. */
      fclose (flog.file);
    }

  flog.file = g_fopen (flog.filename, "a+");
}


/**
 * hyscan_fnn_flog_open:
 * @component
 * @file_size
 *
 * Устанавливает стандартный обработчик логов
 */
gboolean
hyscan_fnn_flog_open (const gchar *component,
                      gsize        file_size)
{
  const gchar *data_dir;
  gchar *log_dir;
  gchar *base_name, *base_name_old;
  gboolean result = FALSE;

  base_name = g_strdup_printf ("%s.log", component);
  base_name_old = g_strdup_printf ("%s.0.log", component);

  data_dir = g_get_user_data_dir ();

  flog.start_time = g_get_monotonic_time ();
  flog.max_size = file_size;

  /* Создаём папку для записи логов. */
  log_dir = g_build_path (G_DIR_SEPARATOR_S, data_dir, "hyscan", "log", NULL);
  g_message ("%s,, %s,", data_dir, log_dir);
  g_mkdir_with_parents (log_dir, 0755);

  /* Открываем файл. */
  flog.filename = g_build_path (G_DIR_SEPARATOR_S, log_dir, base_name, NULL);
  flog.filename_old = g_build_path (G_DIR_SEPARATOR_S, log_dir, base_name_old, NULL);

  hyscan_fnn_flog_open_file ();
  if (flog.file != NULL)
    {
      result = TRUE;
      g_log_set_default_handler (hyscan_fnn_flog_handler, NULL);
    }

  g_free (log_dir);
  g_free (base_name);
  g_free (base_name_old);

  return result;
}

/**
 * hyscan_fnn_flog_destroy:
 * @flog: указатель на #HyScanFnnFlog
 *
 * Закрывает лог-файл и освобождает использованную память.
 */
void
hyscan_fnn_flog_close (void)
{
  /* Возвращаем стандартный обработчик логов. */
  g_log_set_default_handler (g_log_default_handler, NULL);

  g_clear_pointer (&flog.file, fclose);
  g_free (flog.filename);
  g_free (flog.filename_old);
}
