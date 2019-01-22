#include "sonar-configure.h"

gchar **
keyfile_strv_read_helper (GKeyFile    *config,
                          const gchar *group,
                          const gchar *key)
{
  GError *error = NULL;
  gchar ** keys;

  /* Список ключей. */
  keys = g_key_file_get_string_list(config, group, key, NULL, &error);

  if (error == NULL)
    return keys;

  g_error_free (error);

  return NULL;
}

gboolean
keyfile_bool_read_helper (GKeyFile    *config,
                          const gchar *group,
                          const gchar *key)
{
  GError *error = NULL;
  gboolean read_val;

  if (config == NULL)
    return FALSE;

  read_val = g_key_file_get_boolean (config, group, key, &error);

  if (error == NULL)
    return read_val;

  g_error_free (error);

  return FALSE;
}

gchar *
keyfile_string_read_helper (GKeyFile    *config,
                            const gchar *group,
                            const gchar *key)
{
  GError *error = NULL;
  gchar * text;

  if (config == NULL)
    return NULL;

  text = g_key_file_get_string (config, group, key, &error);

  if (error == NULL)
    return text;

  g_error_free (error);

  return NULL;
}

void
keyfile_string_write_helper (GKeyFile    *config,
                             const gchar *group,
                             const gchar *key,
                             const gchar *string)
{
  if (config == NULL)
    return;

  g_key_file_set_string (config, group, key, string);
}

gdouble
keyfile_double_read_helper (GKeyFile    *config,
                            const gchar *group,
                            const gchar *key,
                            gdouble      by_default)
{
  GError *error = NULL;
  gdouble value;

  if (config == NULL)
    return by_default;

  value = g_key_file_get_double (config, group, key, &error);

  if (error == NULL)
    return value;

  g_error_free (error);

  return by_default;
}


void
keyfile_double_write_helper (GKeyFile    *config,
                             const gchar *group,
                             const gchar *key,
                             gdouble      value)
{
  if (config == NULL)
    return;

  g_key_file_set_double (config, group, key, value);

}
