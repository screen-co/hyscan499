#ifndef __HYSCAN_AME_FIXED_H__
#define __HYSCAN_AME_FIXED_H__

#include <hyscan-ame-button.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_AME_FIXED             (hyscan_ame_fixed_get_type ())
#define HYSCAN_AME_FIXED(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_AME_FIXED, HyScanAmeFixed))
#define HYSCAN_IS_AME_FIXED(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_AME_FIXED))
#define HYSCAN_AME_FIXED_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_AME_FIXED, HyScanAmeFixedClass))
#define HYSCAN_IS_AME_FIXED_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_AME_FIXED))
#define HYSCAN_AME_FIXED_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_AME_FIXED, HyScanAmeFixedClass))

typedef struct _HyScanAmeFixed HyScanAmeFixed;
typedef struct _HyScanAmeFixedPrivate HyScanAmeFixedPrivate;
typedef struct _HyScanAmeFixedClass HyScanAmeFixedClass;

struct _HyScanAmeFixed
{
  GtkFixed parent_instance;

  HyScanAmeFixedPrivate *priv;
};

struct _HyScanAmeFixedClass
{
  GtkFixedClass parent_class;
};

HYSCAN_API
GType                  hyscan_ame_fixed_get_type         (void);

HYSCAN_API
GtkWidget             *hyscan_ame_fixed_new              (void);

HYSCAN_API
void                   hyscan_ame_fixed_pack             (HyScanAmeFixed *self,
                                                          guint           position,
                                                          GtkWidget      *child);

HYSCAN_API
void                   hyscan_ame_fixed_activate         (HyScanAmeFixed *self,
                                                          guint           position);

HYSCAN_API
void                   hyscan_ame_fixed_set_state        (HyScanAmeFixed *self,
                                                          guint           position,
                                                          gboolean        state);

G_END_DECLS

#endif /* __HYSCAN_AME_FIXED_H__ */
