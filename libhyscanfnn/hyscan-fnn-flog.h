#ifndef __HYSCAN_FNN_FLOG_H__
#define __HYSCAN_FNN_FLOG_H__

#include <hyscan-api.h>
#include <glib.h>

G_BEGIN_DECLS

HYSCAN_API
gboolean        hyscan_fnn_flog_open               (const gchar *component,
                                                    guint        file_size);

HYSCAN_API
void            hyscan_fnn_flog_close              (void);

G_END_DECLS

#endif  /* __HYSCAN_FNN_FLOG_H__ */