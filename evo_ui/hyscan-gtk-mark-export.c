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
#include <sys/stat.h>
#include <hyscan-mark.h>

#define GETTEXT_PACKAGE "hyscanfnn-evoui"
#include <glib/gi18n-lib.h>

/* Структура для передачи данных в поток сохранения меток в формате HTML. */
typedef struct _DataForHTML
{
  GHashTable  *wf_marks,      /* "Водопадные" метки. */
              *geo_marks;     /* Гео-метки. */
  Global      *global;        /* Структура с "глобальными" параметрами. */
  gchar       *folder;        /* Папка для экспорта. */
}DataForHTML;

/* Структура для передачи данных для сохранения тайла после генерации. */
typedef struct _package
{
  GMutex       mutex;           /* Мьютекс для защиты данных во сохранения тайла. */
  HyScanCache *cache;           /* Указатель на кэш. */
  GdkRGBA      color;           /* Цветовая схема. */
  guint        counter;         /* Счётчик тайлов. */

}Package;

static gchar hyscan_gtk_mark_export_header[] = "LAT,LON,NAME,DESCRIPTION,COMMENT,NOTES,DATE,TIME\n";

static gchar *empty = N_("Empty");

static void          hyscan_gtk_mark_export_tile_loaded         (Package             *package,
                                                                 HyScanTile          *tile,
                                                                 gfloat              *img,
                                                                 gint                 size,
                                                                 gulong               hash);

static void          hyscan_gtk_mark_export_save_tile_as_png    (HyScanTile          *tile,
                                                                 Package             *package,
                                                                 gfloat              *img,
                                                                 gint                 size,
                                                                 const gchar         *file_name,
                                                                 gboolean             echo);

static void          hyscan_gtk_mark_export_generate_tile       (HyScanMarkLocation  *location,
                                                                 HyScanTileQueue     *tile_queue,
                                                                 guint               *counter);

static void          hyscan_gtk_mark_export_save_tile           (HyScanMarkLocation  *location,
                                                                 HyScanTileQueue     *tile_queue,
                                                                 const gchar         *image_folder,
                                                                 const gchar         *media,
                                                                 FILE                *file,
                                                                 Package             *package);

static void          hyscan_gtk_mark_export_init_tile           (HyScanTile          *tile,
                                                                 HyScanTileCacheable *tile_cacheable,
                                                                 HyScanMarkLocation  *location);

static gpointer      hyscan_gtk_mark_export_save_as_html_thread (gpointer             user_data);

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


gchar*
hyscan_gtk_mark_export_to_str (HyScanMarkLocModel *ml_model,
                               HyScanObjectModel  *mark_geo_model,
                               gchar              *project_name)
{
  GHashTable *wf_marks, *geo_marks;
  GDateTime *local;
  gchar *str, *marks;

  wf_marks = hyscan_mark_loc_model_get (ml_model);
  geo_marks = hyscan_object_model_get (mark_geo_model);

  if (wf_marks == NULL || geo_marks == NULL)
    return NULL;

  local = g_date_time_new_now_local ();

  marks = hyscan_gtk_mark_export_print_marks (wf_marks, geo_marks);
  str = g_strdup_printf (_("%s\nProject: %s\n%s%s"),
                         g_date_time_format (local, "%A %B %e %T %Y"),
                         project_name, hyscan_gtk_mark_export_header, marks);

  g_hash_table_unref (wf_marks);
  g_hash_table_unref (geo_marks);

  g_free (marks);

  return str;
}

/* Функция-обработчик завершения гененрации тайла.
 * По завершению генерации уменьшает счётчик герируемых тайлов.
 * */
void
hyscan_gtk_mark_export_tile_loaded    (Package           *package, /* Пакет дополнительных данных. */
                                       HyScanTile        *tile,    /* Тайл. */
                                       gfloat            *img,     /* Акустическое изображение. */
                                       gint               size,    /* Размер акустического изображения в байтах. */
                                       gulong             hash)    /* Хэш состояния тайла. */
{
  package->counter--;
}

/*
 * функция ищет тайл в кэше и если не находит, то запускает процесс генерации тайла.
 * */
void
hyscan_gtk_mark_export_generate_tile (HyScanMarkLocation *location,   /* Метка. */
                                      HyScanTileQueue    *tile_queue, /* Очередь для работы с акустическими
                                                                       * изображениями. */
                                      guint              *counter)    /* Счётчик генерируемых тайлов. */
{
  if (location->mark->type == HYSCAN_MARK_WATERFALL)
    {
      HyScanTile *tile = NULL;
      HyScanTileCacheable tile_cacheable;

      tile = hyscan_tile_new (location->track_name);
      tile->info.source = hyscan_source_get_type_by_id (location->mark->source);
      if (tile->info.source != HYSCAN_SOURCE_INVALID)
        {
          hyscan_gtk_mark_export_init_tile (tile, &tile_cacheable, location);

          if (hyscan_tile_queue_check (tile_queue, tile, &tile_cacheable, NULL) == FALSE)
            {
              HyScanCancellable *cancellable;
              cancellable = hyscan_cancellable_new ();
              /* Добавляем тайл в очередь на генерацию. */
              hyscan_tile_queue_add (tile_queue, tile, cancellable);
              /* Увеличиваем счётчик тайлов. */
              (*counter)++;
              g_object_unref (cancellable);
            }
        }
      g_object_unref (tile);
    }
}

/*
 * функция сохраняет метку в файл и добавляет запись в файл index.html.
 */
void
hyscan_gtk_mark_export_save_tile (HyScanMarkLocation *location,     /* Метка. */
                                  HyScanTileQueue    *tile_queue,   /* Очередь для работы с акустическими изображениями. */
                                  const gchar        *image_folder, /* Полный путь до папки с изображениями. */
                                  const gchar        *media,        /* Папка для сохранения изображений. */
                                  FILE               *file,         /* Дескриптор файла для записи данных в index.html. */
                                  Package            *package)      /* Пакет дополнительных данных. */
{
  if (location->mark->type == HYSCAN_MARK_WATERFALL)
    {
      HyScanTile *tile = NULL;
      HyScanTileCacheable tile_cacheable;
      GRand *rand = g_rand_new ();

      tile = hyscan_tile_new (location->track_name);
      tile->info.source = hyscan_source_get_type_by_id (location->mark->source);
      if (tile->info.source != HYSCAN_SOURCE_INVALID)
        {
          hyscan_gtk_mark_export_init_tile (tile, &tile_cacheable, location);

          if (hyscan_tile_queue_check (tile_queue, tile, &tile_cacheable, NULL))
            {
              gfloat *image = NULL;
              guint32 size = 0;
              if (hyscan_tile_queue_get (tile_queue, tile, &tile_cacheable, &image, &size))
                {
                  gboolean echo = (location->direction == HYSCAN_MARK_LOCATION_BOTTOM)? TRUE : FALSE;
                  gint id = g_rand_int_range (rand, 0, INT32_MAX);
                  GDateTime *local = NULL;
                  gchar *lat, *lon, *name, *description, *comment,
                        *notes, *date, *time, *content, *file_name,
                        *format  = "\t\t\t<p><strong>%s</strong></p>\n"
                                   "\t\t\t\t<img src=\"%s/%i.png\" alt=\"%s\" title=\"%s\">\n"
                                   "\t\t\t\t<p>Date: %s</p>\n"
                                   "\t\t\t\t<p>Time: %s</p>\n"
                                   "\t\t\t\t<p>Location: %s, %s</p>\n"
                                   "\t\t\t\t<p>Description: %s</p>\n"
                                   "\t\t\t\t<p>Comment: %s</p>\n"
                                   "\t\t\t\t<p>Notes: %s</p>\n"
                                   "\t\t\t<br style=\"page-break-before: always\"/>\n";

                  lat = g_strdup_printf ("%.6f°", location->mark_geo.lat);
                  lon = g_strdup_printf ("%.6f°", location->mark_geo.lon);

                  local  = g_date_time_new_from_unix_local (1e-6 * location->mark->ctime);
                  if (local == NULL)
                    return;

                  date = g_date_time_format (local, "%m/%d/%Y");
                  time = g_date_time_format (local, "%H:%M:%S");

                  name = (location->mark->name == NULL) ? empty : g_strdup (location->mark->name);
                  description = (location->mark->description == NULL) ? empty : g_strdup (location->mark->description);

                  comment = g_strdup (empty);
                  notes   = g_strdup (empty);

                  content = g_strdup_printf (format, name, media, id, name, name, date,
                                             time, lat, lon, description, comment, notes);

                  fwrite (content, sizeof (gchar), strlen (content), file);

                  g_free (content);
                  g_free (lat);
                  g_free (lon);
                  g_free (name);
                  g_free (description);
                  g_free (comment);
                  g_free (notes);
                  g_free (date);
                  g_free (time);

                  lat = lon = name = description = comment = notes = date = time = NULL;

                  g_date_time_unref (local);

                  tile->cacheable = tile_cacheable;
                  file_name = g_strdup_printf ("%s/%i.png", image_folder, id);

                  hyscan_gtk_mark_export_save_tile_as_png (tile,
                                                           package,
                                                           image,
                                                           size,
                                                           file_name,
                                                           echo);
                  g_free (file_name);
                }
            }
        }
      g_object_unref (tile);
      g_free (rand);
    }
}
/*
 * функция сохраняет тайл в формате PNG.
 * */
void
hyscan_gtk_mark_export_save_tile_as_png (HyScanTile  *tile,       /* Тайл. */
                                         Package     *package,    /* Пакет дополнительных данных. */
                                         gfloat      *img,        /* Акустическое изображение. */
                                         gint         size,       /* Размер акустического изображения в байтах. */
                                         const gchar *file_name,  /* Имя файла. */
                                         gboolean     echo)       /* Флаг "эхолотной" метки, которую нужно
                                                                   * повернуть на 90 градусов и отразить
                                                                   * по горизонтали. */
{
  cairo_surface_t   *surface    = NULL;
  HyScanTileSurface  tile_surface;
  HyScanTileColor   *tile_color = NULL;
  guint32            background,
                     colors[2],
                    *colormap   = NULL;
  guint              cmap_len;

  if (file_name != NULL)
    {
      tile_color = hyscan_tile_color_new (package->cache);

      background = hyscan_tile_color_converter_d2i (0.15, 0.15, 0.15, 1.0);
      colors[0]  = hyscan_tile_color_converter_d2i (0.0, 0.0, 0.0, 1.0);
      colors[1]  = hyscan_tile_color_converter_d2i (package->color.red,
                                                    package->color.green,
                                                    package->color.blue,
                                                    1.0);

      colormap   = hyscan_tile_color_compose_colormap (colors, 2, &cmap_len);

      hyscan_tile_color_set_colormap_for_all (tile_color, colormap, cmap_len, background);

      /* 1.0 / 2.2 = 0.454545... */
      hyscan_tile_color_set_levels (tile_color, tile->info.source, 0.0, 0.454545, 1.0);

      tile_surface.width  = tile->cacheable.w;
      tile_surface.height = tile->cacheable.h;
      tile_surface.stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32,
                                                           tile_surface.width);
      tile_surface.data   = g_malloc0 (tile_surface.height * tile_surface.stride);

      hyscan_tile_color_add (tile_color, tile, img, size, &tile_surface);

      if (hyscan_tile_color_check (tile_color, tile, &tile->cacheable))
        {
          gpointer buffer = NULL;

          if (echo == TRUE)
            {
              gint   i, j,
                     height = tile_surface.width,
                     width  = tile_surface.height,
                     stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);
              guint *dest,
                    *src;

              buffer = g_malloc0 (tile_surface.height * tile_surface.stride);

              hyscan_tile_color_get (tile_color, tile, &tile->cacheable, &tile_surface);

              dest = (guint*)buffer,
              src  = (guint*)tile_surface.data;
              /* Поворот на 90 градусов */
              for (j = 0; j < tile_surface.height; j++)
                {
                  for (i = 0; i < tile_surface.width; i++)
                    {
                      dest[i * tile_surface.height + j] = src[j * tile_surface.width + i];
                    }
                }
              /* Отражение по горизонтали. */
              for (j = 0; j <= (height - 1); j++)
                {
                  for (i = 0; i <= (width >> 1); i++)
                    {
                      gint  from = j * width + i,
                            to   = j * width + width - 1 - i;
                      guint tmp = dest[from];

                      dest[from] = dest[to];
                      dest[to] = tmp;
                   }
                }

              surface = cairo_image_surface_create_for_data ( (guchar*)buffer,
                                                              CAIRO_FORMAT_ARGB32,
                                                              width,
                                                              height,
                                                              stride);
            }
          else
            {
              hyscan_tile_color_get (tile_color, tile, &tile->cacheable, &tile_surface);
              surface = cairo_image_surface_create_for_data ( (guchar*)tile_surface.data,
                                                              CAIRO_FORMAT_ARGB32,
                                                              tile_surface.width,
                                                              tile_surface.height,
                                                              tile_surface.stride);
            }
          cairo_surface_write_to_png (surface, file_name);
          if (buffer != NULL)
            g_free (buffer);
        }
      if (tile_surface.data != NULL)
        g_free (tile_surface.data);
      if (tile_color != NULL)
        g_object_unref (tile_color);
      if (surface != NULL)
        cairo_surface_destroy (surface);
    }
}

/*
 * Функция инициализирует структуры HyScanTile и HyScanTileCacheable
 * используя данные метки из location.
 * */
void
hyscan_gtk_mark_export_init_tile (HyScanTile          *tile,
                                  HyScanTileCacheable *tile_cacheable,
                                  HyScanMarkLocation  *location)
{
  gfloat ppi = 1.0f;
  gdouble width  = location->mark->width,
          height = location->mark->height;

  if (location->direction == HYSCAN_MARK_LOCATION_BOTTOM)
    {
      /* Если метка "эхолотная", то умножаем её габариты на 10.
       * 10 это скорость движения при которой генерируются тайлы в Echosounder-е,
       * но метка сохраняется без учёта этого коэфициента масштабирования.
       * И меняем ширину и высоту местами, т.к. у Echosounder-а другая система координат.*/
      width =  10.0 * location->mark->height;
      height = 10.0 * location->mark->width;
    }

  /* Для левого борта тайл надо отразить по оси X. */
  if (location->direction == HYSCAN_MARK_LOCATION_PORT)
    {
      /* Поэтому значения отрицательные, и start и end меняются местами. */
      tile->info.across_start = -round (
            (location->across + width) * 1000.0);
      tile->info.across_end   = -round (
            (location->across - width) * 1000.0);
      if (tile->info.across_start < 0 && tile->info.across_end > 0)
        {
          tile->info.across_end = 0;
        }
    }
  else
    {
      tile->info.across_start = round (
            (location->across - width) * 1000.0);
      tile->info.across_end   = round (
            (location->across + width) * 1000.0);
      if (tile->info.across_start < 0 && tile->info.across_end > 0)
        {
          tile->info.across_start = 0;
        }
    }

  tile->info.along_start = round (
        (location->along - height) * 1000.0);
  tile->info.along_end   = round (
        (location->along + height) * 1000.0);

  /* Нормировка тайлов по ширине в 600 пикселей. */
  ppi = 600.0 / ( (2.0 * 100.0 * width) / 2.54);
  if (location->direction == HYSCAN_MARK_LOCATION_BOTTOM)
    {
      /* Если метка "эхолотная", то нормируем по высоте в 600 пикселей,
       * т.к. сгенерированный тайл будет повёрнут на 90 градусов. */
      ppi = 600.0 / ( (2.0 * 100.0 * height) / 2.54);
    }

  tile->info.scale    = 1.0f;
  tile->info.ppi      = ppi;
  tile->info.upsample = 2;
  tile->info.rotate   = FALSE;
  tile->info.flags    = HYSCAN_TILE_GROUND;

  if (tile->info.source == HYSCAN_SOURCE_ECHOSOUNDER)
    {
      tile->info.rotate   = FALSE;
      tile->info.flags    = HYSCAN_TILE_PROFILER;
    }

  tile_cacheable->w =      /* Будет заполнено генератором. */
  tile_cacheable->h = 0;   /* Будет заполнено генератором. */
  tile_cacheable->finalized = FALSE;   /* Будет заполнено генератором. */
}

/*
 * Потоковая функция сохранения меток в формате HTML.
 * */
gpointer
hyscan_gtk_mark_export_save_as_html_thread (gpointer user_data)
{
  DataForHTML *data = (DataForHTML*)user_data;
  FILE        *file           = NULL;    /* Дескриптор файла. */
  GdkCursor  *cursor_watch    = NULL;
  GdkDisplay *display         = gdk_display_get_default ();
  gchar       *current_folder = NULL,   /* Полный путь до папки с проектом. */
              *media          = "media", /* Папка для сохранения изображений. */
              *image_folder   = NULL,    /* Полный путь до папки с изображениями. */
              *file_name      = NULL;    /* Полный путь до файла index.html */

  /* Создаём папку с названием проекта. */
  current_folder = g_strconcat (data->folder, "/", data->global->project_name, (gchar*) NULL);
  mkdir (current_folder, 0777);
  /* HTML-файл. */
  file_name = g_strdup_printf ("%s/index.html", current_folder);
  /* Создаём папку для сохранения тайлов. */
  image_folder = g_strdup_printf ("%s/%s", current_folder, media);
  mkdir (image_folder, 0777);

#ifdef G_OS_WIN32
  fopen_s (&file, file_name, "w");
#else
  file = fopen (file_name, "w");
#endif
  if (file != NULL)
    {
      HyScanFactoryAmplitude  *factory_amp;  /* Фабрика объектов акустических данных. */
      HyScanFactoryDepth      *factory_dpt;  /* Фабрика объектов глубины. */
      HyScanTileQueue         *tile_queue;   /* Очередь для работы с акустическими изображениями. */
      Package                  package;
      gchar *header = "<!DOCTYPE html>\n"
                      "<html lang=\"ru\">\n"
                      "\t<head>\n"
                      "\t\t<meta charset=\"utf-8\">\n"
                      "\t\t<meta name=\"generator\" content=\"HyScan5\">\n"
                      "\t\t<meta name=\"description\" content=\"description\">\n"
                      "\t\t<meta name=\"author\" content=\"operator_name\">\n"
                      "\t\t<meta name=\"document-state\" content=\"static\">\n"
                      "\t\t<title>Конвертация HTML с картинками в ODT, DOC, DOCX, RTF, PDF.</title>\n"
                      "\t</head>\n"
                      "\t<body>\n"
                      "\t\t<p>Далее будут размещены картинки. А вообще весь файл можно конвертировать"
                      " в ODT, DOC, DOCX, RTF, PDF. <a href=\"#more\">Подробнее...</a></p>\n"
                      "\t\t<p>Сгенерировано в <a href=\"http://screen-co.ru/\">HyScan5</a>.</p>\n"
                      "\t\t<br style=\"page-break-before: always\"/>\n",
            *footer = "\t\t<p><a name=\"more\"><strong>Microsoft Word (DOC, DOCX, RTF, PDF)</strong></a></p>\n"
                      "\t\t<p>Чтобы конвертировать этот файл в doc, необходимо открыть его в Microsoft"
                      " Word-е. Выполните Правый клик -> Контекстное меню -> Открыть с помощью ->"
                      " Microsoft Word. Если Word-a в открывшемся списке нет, то нужно \"Выбрать"
                      " программу...\" и там найти Word. Тогда файл откроется.</p>\n"
                      "\t\t<p>После того как файл открылся Выберете в меню Файл -> Сведения -> Связаные"
                      " документы -> Изменить связи с файлами. И в открывшемся окне для каждого файла"
                      " поставить галочку в Параметры связи -> Хранить в документе и нажать OK. Затем"
                      " выбрать Cохранить как и укажите нужный формат DOC, DOCX, RTF, PDF. Всё файл"
                      " сконвертирован.</p>\n"
                      "\t\t<br style=\"page-break-before: always\"/>\n"
                      "\t\t<p><strong>LibreOffice Writer (ODT, DOC, DOCX, RTF, PDF)</strong></p>\n"
                      "\t\t<p>Чтобы сконвертировать этот файл в odt, необходимо открыть его в LibreOffice"
                      " Writer. Выполните Правый клик -> Контекстное меню -> Открыть в программе ->"
                      " LibreOffice Writer. в меню выберите Правка -> Связи и для каждого файла нажать"
                      " Разорвать связи. Затем можно сохранять файл в нужном формате ODT, DOC, DOCX, RTF."
                      " Для сохранения в формате PDF Выберите в меню Файл -> Экспорт в PDF. Всё файл"
                      " сконвертирован.</p>\n"
                      "\t</body>\n"
                      "</html>";

      fwrite (header, sizeof (gchar), strlen (header), file);
      /* Создаём фабрику объектов доступа к данным амплитуд. */
      factory_amp = hyscan_factory_amplitude_new (data->global->cache);
      hyscan_factory_amplitude_set_project (factory_amp,
                                            data->global->db,
                                            data->global->project_name);
      /* Создаём фабрику объектов доступа к данным глубины. */
      factory_dpt = hyscan_factory_depth_new (data->global->cache);
      hyscan_factory_depth_set_project (factory_dpt,
                                        data->global->db,
                                        data->global->project_name);
      /* Создаём очередь для генерации тайлов. */
      tile_queue = hyscan_tile_queue_new (g_get_num_processors (),
                                          data->global->cache,
                                          factory_amp,
                                          factory_dpt);
      g_mutex_init (&package.mutex);
      package.cache = data->global->cache;
      gdk_rgba_parse (&package.color, "#FFFF00");
      package.counter = 0;
      /* Соединяем сигнал готовности тайла с функцией-обработчиком. */
      g_signal_connect_swapped (G_OBJECT (tile_queue), "tile-queue-image",
                                G_CALLBACK (hyscan_gtk_mark_export_tile_loaded), &package);

      /* Водопадные метки. */
      if (data->wf_marks != NULL)
        {
          HyScanMarkLocation *location   = NULL; /*  */
          GHashTableIter      hash_iter;
          gchar              *mark_id    = NULL; /* Идентификатор метки. */

          g_hash_table_iter_init (&hash_iter, data->wf_marks);

          while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &location))
            {
              hyscan_gtk_mark_export_generate_tile (location, tile_queue, &package.counter);
            }
        }
      if (data->geo_marks != NULL)
        {
          HyScanMarkGeo  *geo_mark   = NULL; /*  */
          GHashTableIter  hash_iter;
          gchar          *mark_id    = NULL, /* Идентификатор метки. */
                         *category = "\t\t<p><strong>Geo marks</strong></p>\n";

          fwrite (category, sizeof (gchar), strlen (category), file);

          g_hash_table_iter_init (&hash_iter, data->geo_marks);

          while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &geo_mark))
            {
              GDateTime *local;
              gchar *lat, *lon, *name, *description, *comment, *notes, *date, *time;

              lat = g_strdup_printf ("%.6f°", geo_mark->center.lat);
              lon = g_strdup_printf ("%.6f°", geo_mark->center.lon);

              local  = g_date_time_new_from_unix_local (1e-6 * geo_mark->ctime);
              if (local == NULL)
                {
                  date = g_strdup (empty);
                  time = g_strdup (empty);
                }
              else
                {
                  date = g_date_time_format (local, "%m/%d/%Y");
                  time = g_date_time_format (local, "%H:%M:%S");
                }

              name = (geo_mark->name == NULL) ? empty : g_strdup (geo_mark->name);
              description = (geo_mark->description == NULL) ? empty : g_strdup (geo_mark->description);

              comment = g_strdup (empty);
              notes = g_strdup (empty);

              if (geo_mark->type == HYSCAN_MARK_GEO)
                {
                   gchar *format  = "\t\t\t<p><strong>%s</strong></p>\n"
                                    "\t\t\t\t<p>Date: %s</p>\n"
                                    "\t\t\t\t<p>Time: %s</p>\n"
                                    "\t\t\t\t<p>Location: %s, %s</p>\n"
                                    "\t\t\t\t<p>Description: %s</p>\n"
                                    "\t\t\t\t<p>Comment: %s</p>\n"
                                    "\t\t\t\t<p>Notes: %s</p>\n"
                                    "\t\t\t<br style=\"page-break-before: always\"/>\n";
                   gchar *content = g_strdup_printf (format, name, date, time, lat, lon,
                                                     description, comment, notes);
                   fwrite (content, sizeof (gchar), strlen (content), file);
                   g_free (content);
                }

              g_free (lat);
              g_free (lon);
              g_free (name);
              g_free (description);
              g_free (comment);
              g_free (notes);
              g_free (date);
              g_free (time);

              lat = lon = name = description = comment = notes = date = time = NULL;

              g_date_time_unref (local);
            }
        }

      if (package.counter)
        {
          GRand *rand = g_rand_new ();  /* Идентификатор для TileQueue. */
          /* Запуск генерации тайлов. */
          hyscan_tile_queue_add_finished (tile_queue, g_rand_int_range (rand, 0, INT32_MAX));
          /* Устанавливаем курсор-часы. */
          cursor_watch = gdk_cursor_new_for_display (display, GDK_WATCH);
          gdk_window_set_cursor (gtk_widget_get_window (data->global->gui.window), cursor_watch);
          g_free (rand);
        }
      while (package.counter)
        {
          /* Ждём пока сгенерируются и сохранятся все тайлы. */
          /* g_usleep (1000); */
        }

      /* Водопадные метки. */
      if (data->wf_marks != NULL)
        {
          HyScanMarkLocation *location   = NULL; /*  */
          GHashTableIter  hash_iter;
          gchar          *mark_id    = NULL, /* Идентификатор метки. */
                         *category = "\t\t<p><strong>Waterfall marks</strong></p>\n";

          fwrite (category, sizeof (gchar), strlen (category), file);

          g_hash_table_iter_init (&hash_iter, data->wf_marks);

          while (g_hash_table_iter_next (&hash_iter, (gpointer *) &mark_id, (gpointer *) &location))
            {
              hyscan_gtk_mark_export_save_tile (location,
                                                tile_queue,
                                                image_folder,
                                                media,
                                                file,
                                                &package);
            }
        }

      g_mutex_clear (&package.mutex);

      g_object_unref (tile_queue);
      g_object_unref (factory_dpt);
      g_object_unref (factory_amp);

      fwrite (footer, sizeof (gchar), strlen (footer), file);
      fclose (file);
    }

  g_hash_table_unref (data->wf_marks);
  g_hash_table_unref (data->geo_marks);
  g_free (file_name);
  gdk_window_set_cursor (gtk_widget_get_window (data->global->gui.window), NULL);
  return NULL;
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
 * @project_name: указатель на назване проекта. Добавляется в буфер обмена как описание содержимого
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

/**
 * hyscan_gtk_mark_export_save_as_html:
 * @ml_model: указатель на модель водопадных меток с данными о положении в пространстве
 * @mark_geo_model: указатель на модель геометок
 * @projrct_name: указатель на назване проекта. Добавляется в буфер обмена как описание содержимого
 * @global: указатель на структуру с "глобадьными" переменными
 * @export_folder: папка для экспорта
 *
 * Сохраняет метки в формате HTML.
 */
void
hyscan_gtk_mark_export_save_as_html (HyScanMarkLocModel *ml_model,
                                     HyScanObjectModel  *mark_geo_model,
                                     Global             *global,
                                     gchar              *export_folder)
{
  GtkWidget   *dialog = NULL;   /* Диалог выбора директории. */
  GThread     *thread = NULL;   /* Поток. */
  DataForHTML *data   = NULL;   /* Данные для потока. */
  gint         res;             /* Результат диалога. */

  dialog = gtk_file_chooser_dialog_new (_("Choose directory"),
                                        GTK_WINDOW (global->gui.window),
                                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                        _("Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        _("Choose"),
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), export_folder);

  res = gtk_dialog_run (GTK_DIALOG (dialog));

  if (res != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  data = g_malloc0 (sizeof (DataForHTML));
  data->wf_marks  = hyscan_mark_loc_model_get (ml_model);
  data->geo_marks = hyscan_object_model_get (mark_geo_model);

  if (data->wf_marks == NULL || data->geo_marks == NULL)
    return;

  data->global = global;
  data->folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));

  thread = g_thread_new ("save_sa_html", hyscan_gtk_mark_export_save_as_html_thread, (gpointer)data);

  g_thread_unref (thread);
  gtk_widget_destroy (dialog);
}
