#ifndef __TRIPLE_BUILDER__
#define __TRIPLE_BUILDER__

#include "triple-types.h"
#include "hyscan-ame-fixed.h"
#include "hyscan-ame-button.h"

GtkWidget *
make_item (AmePageItem *item)
{
  gboolean is_toggle = item->toggle != TOGGLE_NONE;
  gboolean state = item->toggle == TOGGLE_ON;

  GtkWidget *button = hyscan_ame_button_new (item->icon_name,
                                             item->title,
                                             is_toggle,
                                             state);

  g_signal_connect (button, is_toggle ? "ame-toggled" : "ame-activated",
                    G_CALLBACK (item->callback), item->user_data);

  if (item->value != NULL)
    *item->value = hyscan_ame_button_create_value (HYSCAN_AME_BUTTON (button),
                                                   item->value_default);
  if (item->place != NULL)
    *item->place = button;
  
  return button;
}

void build_page (AmePage *page,
                 GtkStack *lstack,
                 GtkStack *rstack)
{
  gint i;
  GtkWidget *left, *right;

  /* Разбираемся, есть эти страницы в стеках или нет. */
  left = gtk_stack_get_child_by_name (lstack, page->path);
  right = gtk_stack_get_child_by_name (rstack, page->path);

  /* Если нет, то создаем и пихаем их в стеки. */
  if (left == NULL)
    {
      left = hyscan_ame_fixed_new ();
      gtk_stack_add_titled (lstack, left, page->path, page->path);
    }
  if (right == NULL)
    {
      right = hyscan_ame_fixed_new ();
      gtk_stack_add_titled (rstack, right, page->path, page->path);
    }

  for (i = 0; i <= AME_N_BUTTONS; ++i)
    {
      GtkWidget *button;
      HyScanAmeFixed *dest;
      AmePageItem *item = &page->items[i];

      if (item->side == L)
        dest = (HyScanAmeFixed*)left;
      else if (item->side == R)
        dest = (HyScanAmeFixed*)right;
      else if (item->side == END)
        break;
      else
        continue;

      button = make_item (item);
      hyscan_ame_fixed_pack (dest, item->position, button);
    }

}

void build_all (AmePage  *page,
                GtkStack *lstack,
                GtkStack *rstack)
{
  for (; page->path != NULL; ++page)
    build_page (page, lstack, rstack);
}

#endif /* __TRIPLE_BUILDER__ */
