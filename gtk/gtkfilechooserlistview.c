/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-indentation-style: gnu -*- */
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
  int dummy;
} GtkFileChooserListViewPrivate;

static void view_iface_init (GtkFileChooserViewIface *iface);

static void
gtk_file_chooser_list_view_init (GtkFileChooserListView *view)
{
}

static void
gtk_file_chooser_list_view_class_init (GtkFileChooserListViewClass *view)
{
}

G_DEFINE_TYPE_WITH_CODE (GtkFileChooserListView, gtk_file_chooser_list_view, GTK_TYPE_TREE_VIEW,
                         G_ADD_PRIVATE (GtkFileChooserListView)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_FILE_CHOOSER_VIEW,
                                                view_iface_init));

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
  iface->set_settings       = set_settings;
  iface->set_model          = set_model;
  iface->unselect_all       = unselect_all;
  iface->set_iter_selection = set_iter_selection;
  iface->selected_foreach   = selected_foreach;
}
