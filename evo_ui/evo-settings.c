/* evo-gtk-datetime.c
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

/**
 * SECTION: evo-gtk-datetime
 * @Short_description: виджет даты и времени
 * @Title: EvoGtkDatetime
 *
 * Виджет отображает заданное время и дату или что-то одно.
 * Временная зона всегда UTC.
 */
#include "evo-settings.h"

struct _EvoSettingsPrivate
{
  GtkGrid *grid;
};

static void        evo_settings_object_constructed   (GObject     *object);

static void        evo_settings_start_edit           (EvoSettings *self,
                                                      GtkWidget   *popover);
static void        evo_settings_end_edit             (EvoSettings *self,
                                                      GtkWidget   *popover);

G_DEFINE_TYPE_WITH_PRIVATE (EvoSettings, evo_settings, GTK_TYPE_TOGGLE_BUTTON);

static void
evo_settings_class_init (EvoSettingsClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->constructed = evo_settings_object_constructed;
}

static void
evo_settings_init (EvoSettings *self)
{
  GtkWidget *popover, *scroll;
  EvoSettingsPrivate *priv = evo_settings_get_instance_private (self);

  self->priv = priv;

  priv->grid = GTK_GRID (gtk_grid_new ());
  popover = gtk_popover_new (GTK_WIDGET (self));
  gtk_popover_set_modal (GTK_POPOVER (popover), TRUE);

  scroll = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                         "vadjustment", NULL,
                         "hadjustment", NULL,
                         "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                         "hscrollbar-policy", GTK_POLICY_NEVER,
                         "width-request", 250,
                         "height-request", 250,
                         "child", priv->grid,
                         NULL);
  gtk_container_add (GTK_CONTAINER (popover), scroll);

  // gtk_widget_set_halign (popover, GTK_ALIGN_FILL);
  // gtk_widget_set_valign (popover, GTK_ALIGN_FILL);

  g_signal_connect (self, "toggled", G_CALLBACK (evo_settings_start_edit), popover);
  g_signal_connect_swapped (popover, "closed", G_CALLBACK (evo_settings_end_edit), self);
}

static void
evo_settings_object_constructed (GObject *object)
{
  EvoSettings *self = EVO_SETTINGS (object);
  GtkWidget *image;

  G_OBJECT_CLASS (evo_settings_parent_class)->constructed (object);

  image = gtk_image_new_from_icon_name ("open-menu-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image (GTK_BUTTON (self), image);

  gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
  gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top (GTK_WIDGET (self), 6);
  gtk_widget_set_margin_bottom (GTK_WIDGET (self), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (self), 6);
  gtk_widget_set_margin_end (GTK_WIDGET (self), 6);

}

/* Функция преднастраивает виджеты popover'а. */
static void
evo_settings_start_edit (EvoSettings *self,
                         GtkWidget   *popover)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self)))
    return;

  gtk_widget_show_all (popover);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), TRUE);
}

/* Функция вызывается, когда popover прячется. */
static void
evo_settings_end_edit (EvoSettings *self,
                       GtkWidget   *popover)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), FALSE);
}

/**
 * evo_settings_new:
 * @mode: режим отображения
 *
 * Функция создаёт новый виджет #EvoSettings, отображающий данные в
 * соответствии с заданным режимом
 *
 * Returns: #EvoSettings.
 */
GtkWidget *
evo_settings_new (void)
{
  return g_object_new (EVO_TYPE_SETTINGS, NULL);
}

GtkWidget *
evo_settings_get_grid (EvoSettings *self)
{
  g_return_val_if_fail (EVO_IS_SETTINGS (self), NULL);

  return GTK_WIDGET (self->priv->grid);
}
