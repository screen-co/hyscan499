#include "hyscan-gtk-forward-look.h"

#include <string.h>
#include <math.h>

struct _HyScanGtkForwardLookPrivate
{
  HyScanForwardLookPlayer             *player;         /* Объект управления просмотром. */

  GtkAdjustment                       *adjustment;     /* Прокрутка отображения. */
  guint32                              first_index;    /* Первый индекс данных. */
  guint32                              last_index;     /* Последний индекс данных. */
  guint32                              cur_index;      /* Текущий индекс данных. */
  gboolean                             self_changed;   /* Признак того, что параметры поменяли мы сами. */

  gboolean                             view_init;      /* Признак необходимости установки начальных пределов отображения. */
  HyScanGtkForwardLookViewType         type;           /* Тип отображения целей. */
  GdkRGBA                              color;          /* Цвет целей. */
  gdouble                              brightness;     /* Коэффициент яркости. */
  gdouble                              sensitivity;    /* Коэффициент чувствительности. */

  gdouble                              alpha;          /* Сектор обзора локатора в горизонтальной плоскости [-alpha +alpha], рад. */
  gdouble                              distance;       /* Максимальная дистанция обзора, м. */
  GArray                              *doa;            /* Массив целей. */
};

static void            hyscan_gtk_forward_look_object_constructed      (GObject                          *object);
static void            hyscan_gtk_forward_look_object_finalize         (GObject                          *object);

static gboolean        hyscan_gtk_forward_look_get_rotate              (GtkCifroArea                     *carea);

static void            hyscan_gtk_forward_look_get_swap                (GtkCifroArea                     *carea,
                                                                        gboolean                         *swap_x,
                                                                        gboolean                         *swap_y);

static void            hyscan_gtk_forward_look_get_stick               (GtkCifroArea                     *carea,
                                                                        GtkCifroAreaStickType            *stick_x,
                                                                        GtkCifroAreaStickType            *stick_y);

static void            hyscan_gtk_forward_look_get_border              (GtkCifroArea                     *carea,
                                                                        guint                            *top,
                                                                        guint                            *bottom,
                                                                        guint                            *left,
                                                                        guint                            *right);

static void            hyscan_gtk_forward_look_get_limits              (GtkCifroArea                     *carea,
                                                                        gdouble                          *min_x,
                                                                        gdouble                          *max_x,
                                                                        gdouble                          *min_y,
                                                                        gdouble                          *max_y);

static void            hyscan_gtk_forward_look_check_scale             (GtkCifroArea                     *carea,
                                                                        gdouble                          *scale_x,
                                                                        gdouble                          *scale_y);

static void            hyscan_gtk_forward_look_index_changed           (GtkAdjustment                    *adjustment,
                                                                        HyScanGtkForwardLook             *fl);

static void            hyscan_gtk_forward_look_range                   (HyScanForwardLookPlayer          *player,
                                                                        guint32                           first_index,
                                                                        guint32                           last_index,
                                                                        HyScanGtkForwardLook             *fl);

static void            hyscan_gtk_forward_look_data                    (HyScanForwardLookPlayer          *player,
                                                                        HyScanForwardLookPlayerInfo      *info,
                                                                        HyScanAntennaOffset              *position,
                                                                        HyScanDOA                        *doa,
                                                                        guint32                           n_doa,
                                                                        HyScanGtkForwardLook             *fl);

static void            hyscan_gtk_forward_look_visible_draw            (GtkWidget                        *widget,
                                                                        cairo_t                          *cairo);

static void            hyscan_gtk_forward_look_sector_draw             (GtkWidget                        *widget,
                                                                        cairo_t                          *cairo);

static void            hyscan_gtk_forward_look_azimuth_grid_draw       (GtkWidget                        *widget,
                                                                        cairo_t                          *cairo);

static void            hyscan_gtk_forward_look_distance_grid_draw      (GtkWidget                        *widget,
                                                                        cairo_t                          *cairo);

static void            hyscan_gtk_forward_look_doa_draw                (GtkWidget                        *widget,
                                                                        cairo_t                          *cairo);


G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkForwardLook, hyscan_gtk_forward_look, GTK_TYPE_CIFRO_AREA_CONTROL)

static void
hyscan_gtk_forward_look_class_init (HyScanGtkForwardLookClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkCifroAreaClass *carea_class = GTK_CIFRO_AREA_CLASS (klass);

  object_class->constructed = hyscan_gtk_forward_look_object_constructed;
  object_class->finalize = hyscan_gtk_forward_look_object_finalize;

  carea_class->get_rotate = hyscan_gtk_forward_look_get_rotate;
  carea_class->get_swap = hyscan_gtk_forward_look_get_swap;
  carea_class->get_stick = hyscan_gtk_forward_look_get_stick;
  carea_class->get_border = hyscan_gtk_forward_look_get_border;
  carea_class->get_limits = hyscan_gtk_forward_look_get_limits;
  carea_class->check_scale = hyscan_gtk_forward_look_check_scale;
}

static void
hyscan_gtk_forward_look_init (HyScanGtkForwardLook *fl)
{
  fl->priv = hyscan_gtk_forward_look_get_instance_private (fl);
}

static void
hyscan_gtk_forward_look_object_constructed (GObject *object)
{
  HyScanGtkForwardLook *fl = HYSCAN_GTK_FORWARD_LOOK (object);
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  G_OBJECT_CLASS (hyscan_gtk_forward_look_parent_class)->constructed (object);

  priv->view_init = TRUE;

  priv->type = HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET;
  priv->color.red = 0.33;
  priv->color.green = 1.0;
  priv->color.blue = 0.33;
  priv->brightness = 1.0;
  priv->sensitivity = 1.0;

  priv->adjustment = g_object_ref_sink (gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 1.0));

  /* Буфер целей. */
  priv->doa = g_array_new (FALSE, FALSE, sizeof (HyScanDOA));

  /* Объект управления просмотром. */
  priv->player = hyscan_forward_look_player_new ();

  /* Обработчики сигналов. */
  g_signal_connect (fl, "visible-draw", G_CALLBACK (hyscan_gtk_forward_look_visible_draw), NULL);
  g_signal_connect (priv->adjustment, "value-changed", G_CALLBACK (hyscan_gtk_forward_look_index_changed), fl);
  g_signal_connect (priv->player, "range", G_CALLBACK (hyscan_gtk_forward_look_range), fl);
  g_signal_connect (priv->player, "data", G_CALLBACK (hyscan_gtk_forward_look_data), fl);
}

static void
hyscan_gtk_forward_look_object_finalize (GObject *object)
{
  HyScanGtkForwardLook *fl = HYSCAN_GTK_FORWARD_LOOK (object);
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  g_object_unref (priv->player);
  g_object_unref (priv->adjustment);
  g_array_unref (priv->doa);

  G_OBJECT_CLASS (hyscan_gtk_forward_look_parent_class)->finalize (object);
}

/* Виртуальная функция для определения разрешения поворота изображения. */
static gboolean
hyscan_gtk_forward_look_get_rotate (GtkCifroArea *carea)
{
  return FALSE;
}

/* Виртуальная функция для определения необходимости отражения изображения. */
static void
hyscan_gtk_forward_look_get_swap (GtkCifroArea *carea,
                                  gboolean     *swap_x,
                                  gboolean     *swap_y)
{
  (swap_x != NULL) ? *swap_x = FALSE : 0;
  (swap_y != NULL) ? *swap_y = FALSE : 0;
}

/* Виртуальная функция определения центрирования изображения. */
static void
hyscan_gtk_forward_look_get_stick (GtkCifroArea          *carea,
                                   GtkCifroAreaStickType *stick_x,
                                   GtkCifroAreaStickType *stick_y)
{
  (stick_x != NULL) ? *stick_x = GTK_CIFRO_AREA_STICK_CENTER : 0;
  (stick_y != NULL) ? *stick_y = GTK_CIFRO_AREA_STICK_BOTTOM : 0;
}

/* Виртуальная функция для определения размеров окантовки. */
static void
hyscan_gtk_forward_look_get_border (GtkCifroArea *carea,
                                    guint        *top,
                                    guint        *bottom,
                                    guint        *left,
                                    guint        *right)
{
  (top != NULL) ? *top = 0 : 0;
  (bottom != NULL) ? *bottom = 0 : 0;
  (left != NULL) ? *left = 0 : 0;
  (right != NULL) ? *right = 0 : 0;
}

/* Виртуальная функция для определения текущих значений пределов перемещения изображения. */
static void
hyscan_gtk_forward_look_get_limits (GtkCifroArea *carea,
                                    gdouble      *min_x,
                                    gdouble      *max_x,
                                    gdouble      *min_y,
                                    gdouble      *max_y)
{
  HyScanGtkForwardLookPrivate *priv;
  HyScanGtkForwardLook *fl;
  gdouble distance_limit;

  fl = HYSCAN_GTK_FORWARD_LOOK (carea);
  priv = fl->priv;

  gtk_cifro_area_get_view (carea, NULL, NULL, NULL, &distance_limit);
  distance_limit = CLAMP (distance_limit, 1.0, priv->distance);

  (min_x != NULL) ? *min_x = -distance_limit * tan (priv->alpha) : 0;
  (max_x != NULL) ? *max_x =  distance_limit * tan (priv->alpha) : 0;
  (min_y != NULL) ? *min_y = 0.0 : 0;
  (max_y != NULL) ? *max_y = priv->distance : 0;
}

/* Виртуальная функция ограничения масштабов. */
static void
hyscan_gtk_forward_look_check_scale (GtkCifroArea *carea,
                                     gdouble      *scale_x,
                                     gdouble      *scale_y)
{
  *scale_x = CLAMP (*scale_y, 1e-3, 1e3);
  *scale_y = CLAMP (*scale_y, 1e-3, 1e3);
}

/* Функция обработки изменения текущего индекса. */
static void
hyscan_gtk_forward_look_index_changed (GtkAdjustment        *adjustment,
                                       HyScanGtkForwardLook *fl)
{
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  if (!priv->self_changed)
    {
      guint32 index;
      index = gtk_adjustment_get_value (adjustment);
      hyscan_forward_look_player_seek (priv->player, index);
    }
}

/* Функция обработки диапазонов отображения. */
static void
hyscan_gtk_forward_look_range (HyScanForwardLookPlayer *player,
                               guint32                  first_index,
                               guint32                  last_index,
                               HyScanGtkForwardLook    *fl)
{
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  priv->first_index = first_index;
  priv->last_index = last_index;

  priv->self_changed = TRUE;
  gtk_adjustment_configure (priv->adjustment, priv->cur_index,
                            priv->first_index, priv->last_index,
                            1.0, 1.0, 1.0);
  priv->self_changed = FALSE;
}

/* Функция получения данных для отображения. */
static void
hyscan_gtk_forward_look_data (HyScanForwardLookPlayer     *control,
                              HyScanForwardLookPlayerInfo *info,
                              HyScanAntennaOffset         *position,
                              HyScanDOA                   *doa,
                              guint32                      n_doa,
                              HyScanGtkForwardLook        *fl)
{
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  if (info == NULL)
    {
      priv->cur_index = 0;
      g_array_set_size (priv->doa, 0);
    }
  else
    {
      priv->distance = info->distance;
      priv->alpha = info->alpha;
      priv->cur_index = info->index;

      if (priv->view_init)
        {
          gtk_cifro_area_set_view (GTK_CIFRO_AREA (fl),
                                   -priv->distance * sin (priv->alpha),
                                   priv->distance * sin (priv->alpha),
                                   0.0,
                                   priv->distance);

          priv->view_init = FALSE;
        }

      g_array_set_size (priv->doa, n_doa);
      memcpy (priv->doa->data, doa, n_doa * sizeof (HyScanDOA));
    }

  priv->self_changed = TRUE;
  gtk_adjustment_configure (priv->adjustment, priv->cur_index,
                            priv->first_index, priv->last_index,
                            1.0, 1.0, 1.0);
  priv->self_changed = FALSE;

  gtk_widget_queue_draw (GTK_WIDGET (fl));
}

/* Функция отображения. */
static void
hyscan_gtk_forward_look_visible_draw (GtkWidget *widget,
                                      cairo_t   *cairo)
{
  /* Фон. */
  cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
  cairo_paint (cairo);

  /* Сетка. */
  hyscan_gtk_forward_look_sector_draw (widget, cairo);
  hyscan_gtk_forward_look_azimuth_grid_draw (widget, cairo);
  hyscan_gtk_forward_look_distance_grid_draw (widget, cairo);

  /* Цели. */
  hyscan_gtk_forward_look_doa_draw (widget, cairo);
}

/* Функция отображения сектора обзора. */
static void
hyscan_gtk_forward_look_sector_draw (GtkWidget *widget,
                                     cairo_t   *cairo)
{
  GtkCifroArea *carea = GTK_CIFRO_AREA (widget);
  HyScanGtkForwardLook *fl = HYSCAN_GTK_FORWARD_LOOK (carea);
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  gdouble x, y, r;
  guint i;

  cairo_set_line_width (cairo, 1.0);
  cairo_set_source_rgb (cairo, 0.6, 0.2, 0.2);

  for (i = 0; i < 2; i++)
    {
      gtk_cifro_area_visible_value_to_point (carea, &x, &y, 0, 0);
      x = gtk_cifro_area_point_to_cairo (x);
      y = gtk_cifro_area_point_to_cairo (y);
      cairo_move_to (cairo, x, y);

      x = priv->distance * sinf (-priv->alpha + 2 * i * priv->alpha);
      y = priv->distance * cosf (-priv->alpha + 2 * i * priv->alpha);
      gtk_cifro_area_visible_value_to_point (carea, &x, &y, x, y);
      x = gtk_cifro_area_point_to_cairo (x);
      y = gtk_cifro_area_point_to_cairo (y);
      cairo_line_to (cairo, x, y);
      cairo_stroke (cairo);
    }

  gtk_cifro_area_visible_value_to_point (carea, &x, &y, 0, 0);
  x = gtk_cifro_area_point_to_cairo (x);
  y = gtk_cifro_area_point_to_cairo (y);
  gtk_cifro_area_visible_value_to_point (carea, NULL, &r, 0, priv->distance);
  r = fabs (y - r);
  cairo_arc (cairo, x, y, r, -G_PI_2 - priv->alpha, -G_PI_2 + priv->alpha);
  cairo_stroke (cairo);
}

/* Функция отображения координатной сетки по азимут. */
static void
hyscan_gtk_forward_look_azimuth_grid_draw (GtkWidget *widget,
                                           cairo_t   *cairo)
{
  GtkCifroArea *carea = GTK_CIFRO_AREA (widget);
  HyScanGtkForwardLook *fl = HYSCAN_GTK_FORWARD_LOOK (carea);
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  cairo_text_extents_t extents;
  gdouble scale, from_y, to_y;
  gdouble x0, y0, x, y, r;
  gdouble angle, step;
  gdouble text_height;
  gchar text[1024];

  cairo_set_line_width (cairo, 1.0);
  cairo_set_source_rgb (cairo, 0.6, 0.6, 0.6);
  cairo_set_font_size (cairo, 12.0);

  gtk_cifro_area_get_view (carea, NULL, NULL, &from_y, &to_y);
  gtk_cifro_area_get_scale (carea, NULL, &scale);

  g_snprintf (text, sizeof (text), "99°");
  cairo_text_extents (cairo, text, &extents);
  text_height = extents.height * scale;
  step = 5.0 * (G_PI / 180.0);

  gtk_cifro_area_visible_value_to_point (carea, &x0, &y0, 0, 0);
  x0 = gtk_cifro_area_point_to_cairo (x0);
  y0 = gtk_cifro_area_point_to_cairo (y0);

  for (angle = 0; angle > -priv->alpha; angle -= step);
  if (angle < -priv->alpha)
    angle += step;

  while (angle <= priv->alpha)
    {
      r = (to_y + from_y) / 2 + 0.47 * (to_y - from_y);

      x = (r - text_height) * sin (angle);
      y = (r - text_height) * cos (angle);
      gtk_cifro_area_visible_value_to_point (carea, &x, &y, x, y);
      x = gtk_cifro_area_point_to_cairo (x);
      y = gtk_cifro_area_point_to_cairo (y);

      cairo_move_to (cairo, x0, y0);
      cairo_line_to (cairo, x, y);

      g_snprintf (text, sizeof (text), "%.0f°", (180.0 / G_PI) * angle);
      x = x - (extents.width / 2 + extents.x_bearing);
      y = y - 5;
      cairo_move_to (cairo, x, y);
      cairo_show_text (cairo, text);

      x = (r + text_height) * sin (angle);
      y = (r + text_height) * cos (angle);
      gtk_cifro_area_visible_value_to_point (carea, &x, &y, x, y);
      cairo_move_to (cairo, x, y);

      x = priv->distance * sin (angle);
      y = priv->distance * cos (angle);
      gtk_cifro_area_visible_value_to_point (carea, &x, &y, x, y);
      x = gtk_cifro_area_point_to_cairo (x);
      y = gtk_cifro_area_point_to_cairo (y);
      cairo_line_to (cairo, x, y);
      cairo_stroke (cairo);
      angle += step;
    }
}

/* Функция отображения координатной сетки по дальности. */
static void
hyscan_gtk_forward_look_distance_grid_draw (GtkWidget *widget,
                                            cairo_t   *cairo)
{
  GtkCifroArea *carea = GTK_CIFRO_AREA (widget);
  HyScanGtkForwardLook *fl = HYSCAN_GTK_FORWARD_LOOK (carea);
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  cairo_text_extents_t extents;
  gdouble scale, step;
  gdouble x, y, r, r1;
  gdouble x0, y0, r0;
  gdouble distance;
  gchar text[1024];

  cairo_set_line_width (cairo, 1.0);
  cairo_set_source_rgb (cairo, 0.6, 0.6, 0.6);
  cairo_set_font_size (cairo, 12.0);

  /* Минимальное расстояние между линиями сетки по дальности. */
  g_snprintf (text, sizeof (text), "0123456789 м");
  cairo_text_extents (cairo, text, &extents);

  /* Шаг сетки. */
  gtk_cifro_area_get_scale (carea, NULL, &scale);
  gtk_cifro_area_get_view (carea, NULL, NULL, &r0, NULL);
  gtk_cifro_area_get_axis_step (scale, 8.0 * extents.height, &r0, &step, NULL, NULL);
  gtk_cifro_area_get_view (carea, NULL, NULL, NULL, &distance);

  gtk_cifro_area_visible_value_to_point (carea, &x0, &y0, 0, 0);
  x0 = gtk_cifro_area_point_to_cairo (x0);
  y0 = gtk_cifro_area_point_to_cairo (y0);

  for (r = r0; r < distance; r += step)
    {
      g_snprintf (text, sizeof (text), "%.1f м", r);
      cairo_text_extents (cairo, text, &extents);
      gtk_cifro_area_visible_value_to_point (carea, NULL, &r1, 0, r);

      r1 = fabs (y0 - r1);
      cairo_arc (cairo, x0, y0, r1, -G_PI_2 - priv->alpha, -G_PI_2 + priv->alpha);
      cairo_stroke (cairo);

      x = r1 * sinf (-G_PI - priv->alpha) + x0;
      y = r1 * cosf (-G_PI - priv->alpha) + y0;
      y = y - (extents.height / 2 + extents.y_bearing);
      cairo_move_to (cairo, x, y);
      cairo_show_text (cairo, text);

      cairo_stroke (cairo);
    }
}

/* Функция отображения целей. */
static void
hyscan_gtk_forward_look_doa_draw (GtkWidget *widget,
                                  cairo_t   *cairo)
{
  GtkCifroArea *carea = GTK_CIFRO_AREA (widget);
  HyScanGtkForwardLook *fl = HYSCAN_GTK_FORWARD_LOOK (carea);
  HyScanGtkForwardLookPrivate *priv = fl->priv;

  HyScanGtkForwardLookViewType type = priv->type;
  gdouble color_red = priv->color.red;
  gdouble color_green = priv->color.green;
  gdouble color_blue = priv->color.blue;
  gdouble brightness = priv->brightness;
  gdouble sensitivity = priv->sensitivity;
  HyScanDOA *doa = (gpointer)priv->doa->data;
  guint n_doa = priv->doa->len;

  gdouble amplitude;
  gdouble radius;
  gdouble scale_x;
  gdouble scale_y;
  gdouble doa_x;
  gdouble doa_y;
  gdouble x;
  gdouble y;
  guint i;

  gtk_cifro_area_get_scale (carea, &scale_x, &scale_y);

  for (i = 0; i < n_doa; i++)
    {
      amplitude = 100.0 * brightness * doa[i].amplitude;
      if (amplitude < (1.01 - (sensitivity / 10.0)))
        continue;

      radius = amplitude * 0.125 / sqrtf (scale_x);
      radius = CLAMP (radius, 1.0, 10.0);
      radius = (type == HYSCAN_GTK_FORWARD_LOOK_VIEW_TARGET) ? radius : 2.0;
      amplitude = CLAMP (amplitude, (sensitivity / 50.0), 1.0);

      doa_x = doa[i].distance * sin (doa[i].angle);
      doa_y = doa[i].distance * cos (doa[i].angle);
      gtk_cifro_area_visible_value_to_point (carea, &x, &y, doa_x, doa_y);
      x = gtk_cifro_area_point_to_cairo (x);
      y = gtk_cifro_area_point_to_cairo (y);

      cairo_set_source_rgb (cairo, color_red * amplitude, color_green * amplitude, color_blue * amplitude);
      cairo_arc (cairo, x, y, radius, 0.0, 2 * G_PI);
      cairo_fill (cairo);
    }
}

GtkWidget *
hyscan_gtk_forward_look_new (void)
{
  return g_object_new (HYSCAN_TYPE_GTK_FORWARD_LOOK, NULL);
}

HyScanForwardLookPlayer *
hyscan_gtk_forward_look_get_player (HyScanGtkForwardLook *fl)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_FORWARD_LOOK (fl), NULL);

  return fl->priv->player;
}

GtkAdjustment *
hyscan_gtk_forward_look_get_adjustment (HyScanGtkForwardLook *fl)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_FORWARD_LOOK (fl), NULL);

  return fl->priv->adjustment;
}

void
hyscan_gtk_forward_look_set_type (HyScanGtkForwardLook         *fl,
                                  HyScanGtkForwardLookViewType  type)
{
  g_return_if_fail (HYSCAN_IS_GTK_FORWARD_LOOK (fl));

  fl->priv->type = type;

  gtk_widget_queue_draw (GTK_WIDGET (fl));
}

void
hyscan_gtk_forward_look_set_color (HyScanGtkForwardLook *fl,
                                   GdkRGBA               color)
{
  g_return_if_fail (HYSCAN_IS_GTK_FORWARD_LOOK (fl));

  fl->priv->color = color;

  gtk_widget_queue_draw (GTK_WIDGET (fl));
}

void
hyscan_gtk_forward_look_set_brightness (HyScanGtkForwardLook *fl,
                                        gdouble               brightness)
{
  g_return_if_fail (HYSCAN_IS_GTK_FORWARD_LOOK (fl));

  fl->priv->brightness = brightness;

  gtk_widget_queue_draw (GTK_WIDGET (fl));
}

void
hyscan_gtk_forward_look_set_sensitivity (HyScanGtkForwardLook *fl,
                                         gdouble               sensitivity)
{
  g_return_if_fail (HYSCAN_IS_GTK_FORWARD_LOOK (fl));

  fl->priv->sensitivity = sensitivity;

  gtk_widget_queue_draw (GTK_WIDGET (fl));
}
