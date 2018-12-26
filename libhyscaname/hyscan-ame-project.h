#ifndef __HYSCAN_AME_PROJECT_H__
#define __HYSCAN_AME_PROJECT_H__

#include <hyscan-db-info.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_AME_PROJECT             (hyscan_ame_project_get_type ())
#define HYSCAN_AME_PROJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_AME_PROJECT, HyScanAmeProject))
#define HYSCAN_IS_AME_PROJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_AME_PROJECT))
#define HYSCAN_AME_PROJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_AME_PROJECT, HyScanAmeProjectClass))
#define HYSCAN_IS_AME_PROJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_AME_PROJECT))
#define HYSCAN_AME_PROJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_AME_PROJECT, HyScanAmeProjectClass))

typedef struct _HyScanAmeProject HyScanAmeProject;
typedef struct _HyScanAmeProjectPrivate HyScanAmeProjectPrivate;
typedef struct _HyScanAmeProjectClass HyScanAmeProjectClass;

enum 
{
	HYSCAN_AME_PROJECT_CREATE = GTK_RESPONSE_DELETE_EVENT + 1,
	HYSCAN_AME_PROJECT_OPEN
};

struct _HyScanAmeProject
{
  GtkDialog parent_instance;

  HyScanAmeProjectPrivate *priv;
};

struct _HyScanAmeProjectClass
{
  GtkDialogClass parent_class;
};

GType                   hyscan_ame_project_get_type         (void);

GtkWidget              *hyscan_ame_project_new              (HyScanDB     *db,
                                                             HyScanDBInfo *info,
                                                             GtkWindow    *parent);

void                    hyscan_ame_project_get              (HyScanAmeProject *ampj,
                                                             gchar           **project,
                                                             gchar           **track);

G_END_DECLS

#endif /* __HYSCAN_AME_PROJECT_H__ */
