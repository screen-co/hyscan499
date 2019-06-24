#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <glib.h>
#include <hyscan-api.h>

HYSCAN_API
guint          keyfile_uint_read_helper   (GKeyFile    *config,
                                           const gchar *group,
                                           const gchar *key,
                                           guint        def);

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

HYSCAN_API
void          keyfile_bool_write_helper     (GKeyFile                *config,
                                             const gchar             *group,
                                             const gchar             *key,
                                             gboolean                 setting);
#endif /* __SENSORS_H__ */
