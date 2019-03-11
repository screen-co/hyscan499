/* evo-gtk-datetime.h
 *
 * Copyright 2018 Screen LLC, Alexander Dmitriev <m1n7@yandex.ru>
 *
 * This file is part of EvoGui.
 *
 * EvoGui is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EvoGui is distributed in the hope that it will be useful,
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

/* EvoGui имеет двойную лицензию.
 *
 * Во-первых, вы можете распространять EvoGui на условиях Стандартной
 * Общественной Лицензии GNU версии 3, либо по любой более поздней версии
 * лицензии (по вашему выбору). Полные положения лицензии GNU приведены в
 * <http://www.gnu.org/licenses/>.
 *
 * Во-вторых, этот программный код можно использовать по коммерческой
 * лицензии. Для этого свяжитесь с ООО Экран - <info@screen-co.ru>.
 */

#ifndef __EVO_SETTINGS_H__
#define __EVO_SETTINGS_H__

#include <hyscan-api.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EVO_TYPE_SETTINGS             (evo_settings_get_type ())
#define EVO_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVO_TYPE_SETTINGS, EvoSettings))
#define EVO_IS_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVO_TYPE_SETTINGS))
#define EVO_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), EVO_TYPE_SETTINGS, EvoSettingsClass))
#define EVO_IS_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVO_TYPE_SETTINGS))
#define EVO_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), EVO_TYPE_SETTINGS, EvoSettingsClass))

typedef struct _EvoSettings EvoSettings;
typedef struct _EvoSettingsPrivate EvoSettingsPrivate;
typedef struct _EvoSettingsClass EvoSettingsClass;

struct _EvoSettings
{
  GtkToggleButton parent_instance;

  EvoSettingsPrivate *priv;
};

struct _EvoSettingsClass
{
  GtkToggleButtonClass parent_class;
};

HYSCAN_API
GType                  evo_settings_get_type         (void);

HYSCAN_API
GtkWidget *            evo_settings_new              (void);

HYSCAN_API
GtkWidget *            evo_settings_get_grid         (EvoSettings    *self);

G_END_DECLS

#endif /* __EVO_SETTINGS_H__ */
