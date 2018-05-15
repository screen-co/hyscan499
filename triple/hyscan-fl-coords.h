/**
 * \file hyscan-gtk-waterfall-grid.h
 *
 * \brief Координатная сетка для виджета водопад
 *
 * \author Dmitriev Alexander (m1n7@yandex.ru)
 * \date 2017
 * \license Проприетарная лицензия ООО "Экран"
 * \defgroup HyScanFlCoords HyScanFlCoords - координатная сетка для водопада
 *
 * Виджет создается методом #hyscan_fl_coords_new.
 *
 * Виджет обрабатывает сигнал "visible-draw" от \link GtkCifroArea \endlink.
 * В этом обработчике он рисует координатную сетку для отображаемой области.
 *
 * Методы этого класса предоставляют богатые возможности по настройке отображения.
 *
 * - #hyscan_fl_coords_show_grid - включение и выключение сетки;
 * - #hyscan_fl_coords_show_position - включение и выключение линеек местоположения;
 * - #hyscan_fl_coords_show_info - включение и выключение информационного окна;
 * - #hyscan_fl_coords_set_info_position - задает местоположение информационного окна;
 * - #hyscan_fl_coords_set_grid_step - шаг сетки;
 * - #hyscan_fl_coords_set_grid_color - цвет сетки;
 * - #hyscan_fl_coords_set_main_color - цвет подписей.
 *
 */

#ifndef __HYSCAN_FL_COORDS_H__
#define __HYSCAN_FL_COORDS_H__

#include "hyscan-gtk-forward-look.h"
#include "hyscan-gtk-forward-look.h"

G_BEGIN_DECLS

#define HYSCAN_TYPE_FL_COORDS             (hyscan_fl_coords_get_type ())
#define HYSCAN_FL_COORDS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FL_COORDS, HyScanFlCoords))
#define HYSCAN_IS_FL_COORDS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FL_COORDS))
#define HYSCAN_FL_COORDS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FL_COORDS, HyScanFlCoordsClass))
#define HYSCAN_IS_FL_COORDS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FL_COORDS))
#define HYSCAN_FL_COORDS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FL_COORDS, HyScanFlCoordsClass))

typedef struct _HyScanFlCoords HyScanFlCoords;
typedef struct _HyScanFlCoordsPrivate HyScanFlCoordsPrivate;
typedef struct _HyScanFlCoordsClass HyScanFlCoordsClass;

struct _HyScanFlCoords
{
  GObject parent_instance;

  HyScanFlCoordsPrivate *priv;
};

struct _HyScanFlCoordsClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType           hyscan_fl_coords_get_type              (void);

HYSCAN_API
HyScanFlCoords *hyscan_fl_coords_new                   (HyScanGtkForwardLook *fl);

void            hyscan_fl_coords_set_cache             (HyScanFlCoords       *flc,
                                                        HyScanCache          *cache);
HYSCAN_API
void            hyscan_fl_coords_set_project           (HyScanFlCoords       *self,
                                                        HyScanDB             *db,
                                                        const gchar          *project,
                                                        const gchar          *track);

HYSCAN_API
gboolean        hyscan_fl_coords_get_coords            (HyScanFlCoords       *self,
                                                        gdouble              *lat,
                                                        gdouble              *lon);

G_END_DECLS

#endif /* __HYSCAN_FL_COORDS_H__ */
