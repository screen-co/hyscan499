/**
 * \file hyscan-gtk-mark-editor.h
 *
 * \brief Заголовочный файл виджета редактора меток.
 * \author Vladimir Maximov (vmakxs@gmail.com)
 * \date 2018
 * \license Проприетарная лицензия ООО "Экран"
 *
 * \defgroup HyScanGtkMarkEditor HyScanGtkMarkEditor - виджет редактора меток
 *
 */

#ifndef __HYSCAN_GTK_MARK_EDITOR_H__
#define __HYSCAN_GTK_MARK_EDITOR_H__

#include <gtk/gtk.h>
#include <hyscan-api.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_GTK_MARK_EDITOR            \
        (hyscan_gtk_mark_editor_get_type ())

#define HYSCAN_GTK_MARK_EDITOR(obj)            \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_GTK_MARK_EDITOR, HyScanGtkMarkEditor))

#define HYSCAN_IS_GTK_MARK_EDITOR(obj)         \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_GTK_MARK_EDITOR))

#define HYSCAN_GTK_MARK_EDITOR_CLASS(klass)    \
        (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_GTK_MARK_EDITOR, HyScanGtkMarkEditorClass))

#define HYSCAN_IS_GTK_MARK_EDITOR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_GTK_MARK_EDITOR))

#define HYSCAN_GTK_MARK_EDITOR_GET_CLASS(obj)  \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_GTK_MARK_EDITOR, HyScanGtkMarkEditorClass))

typedef struct _HyScanGtkMarkEditor HyScanGtkMarkEditor;
typedef struct _HyScanGtkMarkEditorPrivate HyScanGtkMarkEditorPrivate;
typedef struct _HyScanGtkMarkEditorClass HyScanGtkMarkEditorClass;

struct _HyScanGtkMarkEditor
{
  GtkGrid                      parent_instance;
  HyScanGtkMarkEditorPrivate  *priv;
};

struct _HyScanGtkMarkEditorClass
{
  GtkGridClass  parent_class;
};

HYSCAN_API
GType       hyscan_gtk_mark_editor_get_type  (void);

HYSCAN_API
GtkWidget*  hyscan_gtk_mark_editor_new       (void);

HYSCAN_API
void        hyscan_gtk_mark_editor_set_mark  (HyScanGtkMarkEditor   *mark_editor,
                                              const gchar           *id,
                                              const gchar           *title,
                                              const gchar           *operator_name,
                                              const gchar           *description);

HYSCAN_API
void        hyscan_gtk_mark_editor_get_mark  (HyScanGtkMarkEditor   *mark_editor,
                                              gchar                **id,
                                              gchar                **title,
                                              gchar                **operator_name,
                                              gchar                **description);


G_END_DECLS

#endif /* __HYSCAN_GTK_MARK_EDITOR_H__ */
