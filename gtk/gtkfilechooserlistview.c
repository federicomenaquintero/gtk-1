/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechooserlistview.c: List view mode for the file chooser
 * Copyright (C) 2017 Federico Mena-Quintero
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "gtkfilechooserlistview.h"
#include "gtkgesturelongpress.h"

struct _GtkFileChooserListView
{
  GtkTreeView parent;
};

struct _GtkFileChooserListViewClass
{
  GtkTreeViewClass parent_class;
};

typedef struct
{
  GtkGesture *long_press_gesture;
} GtkFileChooserListViewPrivate;

static void view_iface_init (GtkFileChooserViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (GtkFileChooserListView, gtk_file_chooser_list_view, GTK_TYPE_TREE_VIEW,
                         G_ADD_PRIVATE (GtkFileChooserListView)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_FILE_CHOOSER_VIEW,
                                                view_iface_init));

static GtkFileChooserListViewPrivate *
get_private (GtkFileChooserListView *view)
{
  return G_TYPE_INSTANCE_GET_PRIVATE (view,
                                      GTK_TYPE_FILE_CHOOSER_LIST_VIEW,
                                      GtkFileChooserListViewPrivate);
}

static void
context_menu_at_point (GtkFileChooserListView *view,
                       gint                    x,
                       gint                    y)
{
  GdkRectangle rect;
  GtkTreeSelection *selection;
  GList *list;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  list = gtk_tree_selection_get_selected_rows (selection, NULL);
  if (list)
    {
      GtkTreePath *path = list->data;

      gtk_tree_view_get_cell_area (GTK_TREE_VIEW (view), path, NULL, &rect);
      gtk_tree_view_convert_bin_window_to_widget_coords (GTK_TREE_VIEW (view),
                                                         rect.x, rect.y, &rect.x, &rect.y);
      rect.x = CLAMP (x - 20, 0, gtk_widget_get_allocated_width (GTK_WIDGET (view)) - 40);
      rect.width = 40;

      g_list_free_full (list, (GDestroyNotify) gtk_tree_path_free);
    }
  else
    {
      rect.x = x;
      rect.y = y;
      rect.width = 1;
      rect.height = 1;
    }

  gtk_file_chooser_view_emit_context_menu (GTK_FILE_CHOOSER_VIEW (view), &rect);
}

static void
long_press_cb (GtkGesture             *gesture,
               gdouble                 x,
               gdouble                 y,
               GtkFileChooserListView *view)
{
  context_menu_at_point (view, (gint) x, (gint) y);
}

static void
init_long_press_gesture (GtkFileChooserListView *view)
{
  GtkFileChooserListViewPrivate *priv = get_private (view);

  priv->long_press_gesture = gtk_gesture_long_press_new (GTK_WIDGET (view));
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (priv->long_press_gesture), TRUE);
  g_signal_connect (priv->long_press_gesture, "pressed",
                    G_CALLBACK (long_press_cb), view);
}

static void
gtk_file_chooser_list_view_dispose (GObject *object)
{
  GtkFileChooserListView *view = GTK_FILE_CHOOSER_LIST_VIEW (object);
  GtkFileChooserListViewPrivate *priv = get_private (view);

  g_clear_object (&priv->long_press_gesture);

  G_OBJECT_CLASS (gtk_file_chooser_list_view_parent_class)->dispose (object);
}

static void
gtk_file_chooser_list_view_init (GtkFileChooserListView *view)
{
  init_long_press_gesture (view);
}

static gboolean
popup_menu_cb (GtkWidget *widget,
               GtkFileChooserListView *view)
{
  context_menu_at_point (view,
                         (gint) (0.5 * gtk_widget_get_allocated_width (GTK_WIDGET (view))),
                         (gint) (0.5 * gtk_widget_get_allocated_height (GTK_WIDGET (view))));
  return TRUE;
}

static void
gtk_file_chooser_list_view_constructed (GObject *object)
{
  GtkFileChooserListView *view = GTK_FILE_CHOOSER_LIST_VIEW (object);

  g_signal_connect (view, "popup-menu",
                    G_CALLBACK (popup_menu_cb), view);
}

static void
gtk_file_chooser_list_view_class_init (GtkFileChooserListViewClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) klass;

  object_class->dispose     = gtk_file_chooser_list_view_dispose;
  object_class->constructed = gtk_file_chooser_list_view_constructed;
}

static void
set_settings (GtkFileChooserView *view, GtkFileChooserSettings *settings)
{
}

static void
set_model (GtkFileChooserView *view, GtkFileSystemModel *model)
{
  g_object_set (view, "model", model, NULL);
}

static void
select_all (GtkFileChooserView *view)
{
  GtkTreeView *tree_view;
  GtkTreeSelection *selection;

  tree_view = GTK_TREE_VIEW (view);

  selection = gtk_tree_view_get_selection (tree_view);
  gtk_tree_selection_select_all (selection);
}

static void
unselect_all (GtkFileChooserView *view)
{
  GtkTreeView *tree_view;
  GtkTreeSelection *selection;

  tree_view = GTK_TREE_VIEW (view);

  selection = gtk_tree_view_get_selection (tree_view);
  gtk_tree_selection_unselect_all (selection);
}

static void
set_iter_selection (GtkFileChooserView *view, GtkTreeIter *iter, gboolean do_select)
{
  GtkTreeView *tree_view;
  GtkTreeSelection *selection;

  tree_view = GTK_TREE_VIEW (view);

  selection = gtk_tree_view_get_selection (tree_view);

  if (do_select)
    {
      gtk_tree_selection_select_iter (selection, iter);
    }
  else
    {
      gtk_tree_selection_unselect_iter (selection, iter);
    }
}

static void
get_region_for_path (GtkFileChooserView *view, GtkTreePath *path, GdkRectangle *out_rect)
{
  GdkRectangle rect;

  gtk_tree_view_get_cell_area (GTK_TREE_VIEW (view), path, NULL, &rect);
  gtk_tree_view_convert_bin_window_to_widget_coords (GTK_TREE_VIEW (view),
                                                     rect.x, rect.y, &rect.x, &rect.y);
  /*
  rect.x = CLAMP (x - 20, 0, gtk_widget_get_allocated_width (GTK_WIDGET (view)) - 40);
  rect.width = 40;
  */

  *out_rect = rect;
}

static void
selected_foreach (GtkFileChooserView          *view,
		  GtkTreeSelectionForeachFunc  func,
		  gpointer                     data)
{
  GtkTreeView *tree_view;
  GtkTreeSelection *selection;

  tree_view = GTK_TREE_VIEW (view);

  selection = gtk_tree_view_get_selection (tree_view);

  gtk_tree_selection_selected_foreach (selection, func, data);
}

static void
view_iface_init (GtkFileChooserViewIface *iface)
{
  iface->set_settings        = set_settings;
  iface->set_model           = set_model;
  iface->select_all          = select_all;
  iface->unselect_all        = unselect_all;
  iface->set_iter_selection  = set_iter_selection;
  iface->get_region_for_path = get_region_for_path;
  iface->selected_foreach    = selected_foreach;
}
