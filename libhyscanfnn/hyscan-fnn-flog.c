#include "hyscan-fnn-flog.h"
#include <string.h>
#include <stdio.h>
#include <glib/gstdio.h>
#include <hyscan-config.h>

typedef struct
{
  FILE      *file;

  GMutex     lock;
  guint      size;
  guint      max_size;
  gchar     *filename;
  gchar     *filename_old;
#ifdef G_OS_WIN32
  gunichar2 *wfilename;
  gunichar2 *wfilename_old;
#endif
  gint64     start_time;
} HyScanFnnFlog;

static HyScanFnnFlog flog;

static void
hyscan_fnn_flog_write (const gchar *log_domain,
                       const gchar *message)
{
  gchar *log_entry;

  if (flog.file == NULL)
    return;

  log_entry = g_strdup_printf ("[%9.3f] %s: %s\n",
                               1e-6 * (gdouble) (g_get_monotonic_time () - flog.start_time),
                               log_domain, message);
  flog.size += strlen (log_entry);
  fputs (log_entry, flog.file);
  fflush (flog.file);
}

static void
hyscan_fnn_flog_rotate (void)
{
  if (flog.size > flog.max_size)
    {
      g_clear_pointer (&flog.file, fclose);

      g_unlink (flog.filename_old);
      g_rename (flog.filename, flog.filename_old);
      flog.size = 0;
    }

  if (flog.file == NULL)
    {
#ifdef G_OS_WIN32
      _wfopen_s (&flog.file, flog.wfilename, L"a+");
#else
      flog.file = g_fopen (flog.filename, "a+");
#endif
    }
}

/* Обработчик вывода лог сообщений в файл. */
static void
hyscan_fnn_flog_handler (const gchar    *log_domain,
                         GLogLevelFlags  log_level,
                         const gchar    *message,
                         gpointer        user_data)
{
  if (log_level & G_LOG_LEVEL_DEBUG)
    return;

  g_mutex_lock (&flog.lock);
  hyscan_fnn_flog_write (log_domain, message);
  hyscan_fnn_flog_rotate ();
  g_mutex_unlock (&flog.lock);
}

static void
hyscan_fnn_flog_open_file ()
{
  FILE *file = NULL;

  /* Открываем файл в бинарном режиме, чтобы узнать его размер в байтах. */
#ifdef G_OS_WIN32
  _wfopen_s (&file, flog.wfilename, L"rb");
#else
  file = g_fopen (flog.filename, "rb");
#endif
  if (file != NULL)
    {
      /* Проверяем размер лога. */
      fseek (file, 0, SEEK_END);
      flog.size = ftell (file);

      /* Закрываем файл. */
      fclose (file);
    }

  hyscan_fnn_flog_rotate ();
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
  gchar *log_dir;
  gchar *base_name, *base_name_old;
  gboolean result = FALSE;

  g_mutex_init (&flog.lock);

  base_name = g_strdup_printf ("%s.log", component);
  base_name_old = g_strdup_printf ("%s.0.log", component);

  flog.start_time = g_get_monotonic_time ();
  flog.max_size = file_size;

  /* Создаём папку для записи логов. */
  // log_dir = g_build_path (G_DIR_SEPARATOR_S, hyscan_config_get_user_dir (), "log", NULL);
  log_dir = g_build_path (G_DIR_SEPARATOR_S, hyscan_config_get_log_dir (), NULL);
  g_message ("HyScanFnnFlog: logs are written to %s", log_dir);
  g_mkdir_with_parents (log_dir, 0755);

  /* Открываем файл. */
  flog.filename = g_build_path (G_DIR_SEPARATOR_S, log_dir, base_name, NULL);
  flog.filename_old = g_build_path (G_DIR_SEPARATOR_S, log_dir, base_name_old, NULL);
#ifdef G_OS_WIN32
  flog.wfilename = g_utf8_to_utf16 (flog.filename, -1, NULL, NULL, NULL);
  flog.wfilename_old = g_utf8_to_utf16 (flog.filename_old, -1, NULL, NULL, NULL);
#endif

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

  g_mutex_clear (&flog.lock);
  g_clear_pointer (&flog.file, fclose);
  g_free (flog.filename);
  g_free (flog.filename_old);
#ifdef G_OS_WIN32
  g_free (flog.wfilename);
  g_free (flog.wfilename_old);
#endif
}
