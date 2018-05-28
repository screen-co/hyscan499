#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <hyscan-sonar-control.h>

/* Функция настраивает местоположение антенн датчиков и активирует приём данных. */
gboolean       setup_sensors           (HyScanSensorControl           *control,
                                        GKeyFile                      *config);

/* Функция настраивает местоположение антенн гидролокатора. */
gboolean       setup_sonar_antenna     (HyScanSonarControl            *control,
                                        HyScanSourceType               source,
                                        GKeyFile                      *config);

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
