#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <glib.h>
#include <hyscan-api.h>

HYSCAN_API
gchar **       keyfile_strv_read_helper   (GKeyFile    *config,
                                           const gchar *group,
                                           const gchar *key);

HYSCAN_API
gboolean       keyfile_bool_read_helper (GKeyFile    *config,
                                         const gchar *group,
                                         const gchar *key);
HYSCAN_API
gchar *        keyfile_string_read_helper (GKeyFile    *config,
                                           const gchar *group,
                                           const gchar *key);

HYSCAN_API
void           keyfile_string_write_helper (GKeyFile    *config,
                                            const gchar *group,
                                            const gchar *key,
                                            const gchar *string);
HYSCAN_API
gdouble        keyfile_double_read_helper   (GKeyFile                      *config,
                                             const gchar                   *group,
                                             const gchar                   *key,
                                             gdouble                        by_default);

HYSCAN_API
void          keyfile_double_write_helper   (GKeyFile                      *config,
                                             const gchar                   *group,
                                             const gchar                   *key,
                                             gdouble                        value);
#endif /* __SENSORS_H__ */
