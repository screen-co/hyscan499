#ifndef __HYSCAN_FNN_FIXED_H__
#define __HYSCAN_FNN_FIXED_H__

#include <hyscan-fnn-button.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_FNN_FIXED             (hyscan_fnn_fixed_get_type ())
#define HYSCAN_FNN_FIXED(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FNN_FIXED, HyScanFnnFixed))
#define HYSCAN_IS_FNN_FIXED(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FNN_FIXED))
#define HYSCAN_FNN_FIXED_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FNN_FIXED, HyScanFnnFixedClass))
#define HYSCAN_IS_FNN_FIXED_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FNN_FIXED))
#define HYSCAN_FNN_FIXED_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FNN_FIXED, HyScanFnnFixedClass))

typedef struct _HyScanFnnFixed HyScanFnnFixed;
typedef struct _HyScanFnnFixedPrivate HyScanFnnFixedPrivate;
typedef struct _HyScanFnnFixedClass HyScanFnnFixedClass;

struct _HyScanFnnFixed
{
  GtkFixed parent_instance;

  HyScanFnnFixedPrivate *priv;
};

struct _HyScanFnnFixedClass
{
  GtkFixedClass parent_class;
};

HYSCAN_API
GType                  hyscan_fnn_fixed_get_type         (void);

HYSCAN_API
GtkWidget             *hyscan_fnn_fixed_new              (void);

HYSCAN_API
void                   hyscan_fnn_fixed_pack             (HyScanFnnFixed *self,
                                                          guint           position,
                                                          GtkWidget      *child);

HYSCAN_API
void                   hyscan_fnn_fixed_activate         (HyScanFnnFixed *self,
                                                          guint           position);

HYSCAN_API
void                   hyscan_fnn_fixed_set_state        (HyScanFnnFixed *self,
                                                          guint           position,
                                                          gboolean        state);

G_END_DECLS

#endif /* __HYSCAN_FNN_FIXED_H__ */
