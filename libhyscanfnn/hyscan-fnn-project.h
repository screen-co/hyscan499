#ifndef __HYSCAN_FNN_PROJECT_H__
#define __HYSCAN_FNN_PROJECT_H__

#include <hyscan-db-info.h>
#include <hyscan-api.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_FNN_PROJECT             (hyscan_fnn_project_get_type ())
#define HYSCAN_FNN_PROJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FNN_PROJECT, HyScanFnnProject))
#define HYSCAN_IS_FNN_PROJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FNN_PROJECT))
#define HYSCAN_FNN_PROJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FNN_PROJECT, HyScanFnnProjectClass))
#define HYSCAN_IS_FNN_PROJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FNN_PROJECT))
#define HYSCAN_FNN_PROJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FNN_PROJECT, HyScanFnnProjectClass))

typedef struct _HyScanFnnProject HyScanFnnProject;
typedef struct _HyScanFnnProjectPrivate HyScanFnnProjectPrivate;
typedef struct _HyScanFnnProjectClass HyScanFnnProjectClass;

enum 
{
	HYSCAN_FNN_PROJECT_CREATE = GTK_RESPONSE_DELETE_EVENT + 1,
	HYSCAN_FNN_PROJECT_OPEN
};

struct _HyScanFnnProject
{
  GtkDialog parent_instance;

  HyScanFnnProjectPrivate *priv;
};

struct _HyScanFnnProjectClass
{
  GtkDialogClass parent_class;
};

HYSCAN_API
GType                   hyscan_fnn_project_get_type         (void);

HYSCAN_API
GtkWidget              *hyscan_fnn_project_new              (HyScanDB     *db,
                                                             HyScanDBInfo *info,
                                                             GtkWindow    *parent);

HYSCAN_API
void                    hyscan_fnn_project_get              (HyScanFnnProject *ampj,
                                                             gchar           **project,
                                                             gchar           **track);

G_END_DECLS

#endif /* __HYSCAN_FNN_PROJECT_H__ */
