/**
 * \file hyscan-gtk-project-viewer.c
 *
 * \brief Исходный файл виджета просмотра проекта.
 * \author Vladimir Maximov (vmakxs@gmail.com)
 * \date 2018
 * \license Проприетарная лицензия ООО "Экран"
 *
 */

#include "hyscan-gtk-project-viewer.h"
#include <fnn-types-common.h>

enum
{
  SIGNAL_ITEM_CHANGED,
  SIGNAL_LAST
};

static guint hyscan_gtk_project_viewer_signals[SIGNAL_LAST] = {0};

enum
{
  COLUMN_ID,
  COLUMN_NFNN,
  COLUMN_DATE,
  COLUMN_SORT,
  COLUMN_COLOR,
  N_COLUMNS
};

struct _HyScanGtkProjectViewerPrivate
{
  GtkListStore    *liststore;
  GtkWidget       *treeview;
  GtkWidget       *scroll_wnd;
  GtkAdjustment   *range_adj;

  gchar           *selected_item;
};

static void   hyscan_gtk_project_viewer_constructed    (GObject                  *object);
static void   hyscan_gtk_project_viewer_finalize       (GObject                  *object);

static void   hyscan_gtk_project_viewer_item_changed   (HyScanGtkProjectViewer   *project_viewer,
                                                        GtkTreeView              *tree_view);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanGtkProjectViewer, hyscan_gtk_project_viewer, GTK_TYPE_GRID)

static void
hyscan_gtk_project_viewer_class_init (HyScanGtkProjectViewerClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->constructed = hyscan_gtk_project_viewer_constructed;
  G_OBJECT_CLASS (klass)->finalize = hyscan_gtk_project_viewer_finalize;

  hyscan_gtk_project_viewer_signals[SIGNAL_ITEM_CHANGED] =
    g_signal_new ("item-changed", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/ame/gtk/hyscan-gtk-project-viewer.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkProjectViewer, liststore);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkProjectViewer, treeview);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkProjectViewer, scroll_wnd);
  gtk_widget_class_bind_template_child_private (widget_class, HyScanGtkProjectViewer, range_adj);
  gtk_widget_class_bind_template_callback_full (widget_class, "item_changed",
                                                G_CALLBACK (hyscan_gtk_project_viewer_item_changed));
}

static void
hyscan_gtk_project_viewer_init (HyScanGtkProjectViewer *project_viewer)
{
  project_viewer->priv = hyscan_gtk_project_viewer_get_instance_private (project_viewer);

  gtk_widget_init_template (GTK_WIDGET (project_viewer));
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (project_viewer->priv->liststore),
                                        COLUMN_SORT, GTK_SORT_DESCENDING);
}

static void
hyscan_gtk_project_viewer_constructed (GObject *object)
{
  G_OBJECT_CLASS (hyscan_gtk_project_viewer_parent_class)->constructed (object);
}

static void
hyscan_gtk_project_viewer_finalize (GObject *object)
{
  G_OBJECT_CLASS (hyscan_gtk_project_viewer_parent_class)->finalize (object);
}

static void
hyscan_gtk_project_viewer_item_changed (HyScanGtkProjectViewer *project_viewer,
                                        GtkTreeView            *tree_view)
{
  HyScanGtkProjectViewerPrivate *priv = project_viewer->priv;
  GValue value = G_VALUE_INIT;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  gchar *selected_item;

  gtk_tree_view_get_cursor (tree_view, &path, NULL);
  if (path == NULL)
    return;

  if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->liststore), &iter, path))
    return;

  gtk_tree_model_get_value (GTK_TREE_MODEL (priv->liststore), &iter, COLUMN_ID, &value);
  selected_item = g_value_dup_string (&value);
  g_value_unset (&value);

  if (g_strcmp0 (priv->selected_item, selected_item))
    {
      g_free (priv->selected_item);
      priv->selected_item = selected_item;
      g_signal_emit (project_viewer, hyscan_gtk_project_viewer_signals[SIGNAL_ITEM_CHANGED], 0, NULL);
    }
  else
    {
      g_free (selected_item);
    }
}

GtkWidget *
hyscan_gtk_project_viewer_new (void)
{
  return g_object_new (HYSCAN_TYPE_GTK_PROJECT_VIEWER, NULL);
}

void
hyscan_gtk_project_viewer_clear (HyScanGtkProjectViewer *project_viewer)
{
  HyScanGtkProjectViewerPrivate *priv;
  GtkTreePath *tree_path;
  GtkTreeIter tree_iter;

  g_return_if_fail (HYSCAN_IS_GTK_PROJECT_VIEWER (project_viewer));

  priv = project_viewer->priv;

  /* Сбросить курсор. */
  tree_path = gtk_tree_path_new ();
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->treeview), tree_path, NULL, FALSE);
  gtk_tree_path_free (tree_path);

  /* Очистить. */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->liststore), &tree_iter))
    while (gtk_list_store_remove (priv->liststore, &tree_iter));
}

void
hyscan_gtk_project_viewer_set_selected_item (HyScanGtkProjectViewer *project_viewer,
                                             const gchar            *selected_item)
{
  HyScanGtkProjectViewerPrivate *priv;
  GtkTreeIter tree_iter;

  g_return_if_fail (HYSCAN_IS_GTK_PROJECT_VIEWER (project_viewer));

  priv = project_viewer->priv;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->liststore), &tree_iter))
    return;

  if (selected_item == NULL)
    {
      /* Сбросить курсор. */
      GtkTreePath *tree_path = gtk_tree_path_new ();
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->treeview), tree_path, NULL, FALSE);
      gtk_tree_path_free (tree_path);

      g_clear_pointer (&priv->selected_item, g_free);
      g_signal_emit (project_viewer, hyscan_gtk_project_viewer_signals[SIGNAL_ITEM_CHANGED], 0, NULL);
      return;
    }

  do
    {
      const gchar *tree_item;
      GValue value = G_VALUE_INIT;

      gtk_tree_model_get_value (GTK_TREE_MODEL (priv->liststore), &tree_iter, COLUMN_ID, &value);
      tree_item = g_value_get_string (&value);

      if (g_strcmp0 (selected_item, tree_item) == 0)
        {
          GtkTreePath *tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->liststore), &tree_iter);
          gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->treeview), tree_path, NULL, FALSE);
          gtk_tree_path_free (tree_path);

          g_free (priv->selected_item);
          priv->selected_item = g_strdup (selected_item);
        }

      g_value_unset (&value);
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->liststore), &tree_iter));
}

const gchar *
hyscan_gtk_project_viewer_get_selected_item (HyScanGtkProjectViewer *project_viewer)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_PROJECT_VIEWER (project_viewer), NULL);

  return project_viewer->priv->selected_item;
}

GtkListStore *
hyscan_gtk_project_viewer_get_liststore (HyScanGtkProjectViewer *project_viewer)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_PROJECT_VIEWER (project_viewer), NULL);

  return project_viewer->priv->liststore;
}

GtkTreeView*
hyscan_gtk_project_viewer_get_tree_view (HyScanGtkProjectViewer *project_viewer)
{
  g_return_val_if_fail (HYSCAN_IS_GTK_PROJECT_VIEWER (project_viewer), NULL);

  return GTK_TREE_VIEW (project_viewer->priv->treeview);
}
