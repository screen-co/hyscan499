#ifndef EVO_OVERRIDES_H
#define EVO_OVERRIDES_H

#include <triple-types.h>

/* Функция устанавливает яркость отображения. */
gboolean
evo_brightness_set_override (Global  *global,
                             gdouble  new_brightness,
                             gdouble  new_black,
                             gint     panelx);

#endif /* EVO_OVERRIDES_H */
