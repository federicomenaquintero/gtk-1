/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechoosericonview.c: Icon view mode for the file chooser
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
#include "gtkfilechoosericonview.h"
#include "gtkiconview.h"

struct _GtkFileChooserIconView
{
  GtkIconView parent;
};

struct _GtkFileChooserIconViewClass
{
  GtkIconViewClass parent_class;
};

typedef struct
{
  int dummy;
} GtkFileChooserIconViewPrivate;

static void view_iface_init (GtkFileChooserViewIface *iface);

static void
gtk_file_chooser_icon_view_init (GtkFileChooserIconView *view)
{
}

static void
gtk_file_chooser_icon_view_class_init (GtkFileChooserIconViewClass *view)
{
}

G_DEFINE_TYPE_WITH_CODE (GtkFileChooserIconView, gtk_file_chooser_icon_view, GTK_TYPE_ICON_VIEW,
                         G_ADD_PRIVATE (GtkFileChooserIconView)
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
  GtkIconView *icon_view;

  icon_view = GTK_ICON_VIEW (view);

  gtk_icon_view_unselect_all (icon_view);
}

static void
set_iter_selection (GtkFileChooserView *view, GtkTreeIter *iter, gboolean do_select)
{
  GtkIconView *icon_view;
  GtkTreeModel *model;
  GtkTreePath *path;

  icon_view = GTK_ICON_VIEW (view);

  model = gtk_icon_view_get_model (icon_view);
  path = gtk_tree_model_get_path (model, iter);

  if (do_select)
    {
      gtk_icon_view_select_path (icon_view, path);
    }
  else
    {
      gtk_icon_view_unselect_path (icon_view, path);
    }

  gtk_tree_path_free (path);
}

struct selected_foreach_closure {
  GtkTreeSelectionForeachFunc func;
  gpointer data;
};

static void
selected_foreach_cb (GtkIconView *icon_view,
		     GtkTreePath *path,
		     gpointer     data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct selected_foreach_closure *closure = data;

  model = gtk_icon_view_get_model (icon_view);
  
  g_assert (gtk_tree_model_get_iter (model, &iter, path));

  (* closure->func) (model, path, &iter, closure->data);
}

static void
selected_foreach (GtkFileChooserView          *view,
		  GtkTreeSelectionForeachFunc  func,
		  gpointer                     data)
{
  GtkIconView *icon_view;
  struct selected_foreach_closure closure;

  icon_view = GTK_ICON_VIEW (view);

  closure.func = func;
  closure.data = data;

  gtk_icon_view_selected_foreach (icon_view, selected_foreach_cb, &closure);
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
