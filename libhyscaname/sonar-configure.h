#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <glib.h>

gchar **       keyfile_strv_read_helper   (GKeyFile    *config,
                                           const gchar *group,
                                           const gchar *key);

gboolean       keyfile_bool_read_helper (GKeyFile    *config,
                                         const gchar *group,
                                         const gchar *key);
gchar *        keyfile_string_read_helper (GKeyFile    *config,
                                           const gchar *group,
                                           const gchar *key);

void           keyfile_string_write_helper (GKeyFile    *config,
                                            const gchar *group,
                                            const gchar *key,
                                            const gchar *string);
gdouble        keyfile_double_read_helper   (GKeyFile                      *config,
                                             const gchar                   *group,
                                             const gchar                   *key,
                                             gdouble                        by_default);

void          keyfile_double_write_helper   (GKeyFile                      *config,
                                             const gchar                   *group,
                                             const gchar                   *key,
                                             gdouble                        value);
#endif /* __SENSORS_H__ */
