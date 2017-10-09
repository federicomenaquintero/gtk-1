/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-indentation-style: gnu -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechooserview.h: Folder view interface for the file chooser
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

#include "gtkfilechooserview.h"

typedef GtkFileChooserViewIface GtkFileChooserViewInterface;

static void
gtk_file_chooser_view_default_init (GtkFileChooserViewInterface *iface)
{
}

G_DEFINE_INTERFACE (GtkFileChooserView, gtk_file_chooser_view, G_TYPE_OBJECT)

void
gtk_file_chooser_view_set_settings (GtkFileChooserView *view, GtkFileChooserSettings *settings)
{
  GtkFileChooserViewIface *iface;

  g_assert (GTK_IS_FILE_CHOOSER_VIEW (view));
  g_assert (settings != NULL);

  iface = GTK_FILE_CHOOSER_VIEW_GET_IFACE (view);

  g_assert (iface->set_settings != NULL);
  (* iface->set_settings) (view, settings);
}

void
gtk_file_chooser_view_set_model (GtkFileChooserView *view, GtkFileSystemModel *model)
{
  GtkFileChooserViewIface *iface;

  g_assert (GTK_IS_FILE_CHOOSER_VIEW (view));
  g_assert (model == NULL || GTK_IS_FILE_SYSTEM_MODEL (model));

  iface = GTK_FILE_CHOOSER_VIEW_GET_IFACE (view);

  g_assert (iface->set_model != NULL);
  (* iface->set_model) (view, model);
}

void
gtk_file_chooser_view_unselect_all (GtkFileChooserView *view)
{
  GtkFileChooserViewIface *iface;

  g_assert (GTK_IS_FILE_CHOOSER_VIEW (view));

  iface = GTK_FILE_CHOOSER_VIEW_GET_IFACE (view);

  g_assert (iface->unselect_all != NULL);
  (* iface->unselect_all) (view);
}

void
gtk_file_chooser_view_set_iter_selection (GtkFileChooserView *view, GtkTreeIter *iter, gboolean do_select)
{
  GtkFileChooserViewIface *iface;

  g_assert (GTK_IS_FILE_CHOOSER_VIEW (view));
  g_assert (iter != NULL);

  iface = GTK_FILE_CHOOSER_VIEW_GET_IFACE (view);

  g_assert (iface->set_iter_selection != NULL);
  (* iface->set_iter_selection) (view, iter, do_select);
}

void
gtk_file_chooser_view_selected_foreach (GtkFileChooserView          *view,
                                        GtkTreeSelectionForeachFunc  func,
                                        gpointer                     data)
{
  GtkFileChooserViewIface *iface;

  g_assert (GTK_IS_FILE_CHOOSER_VIEW (view));
  g_assert (func != NULL);

  iface = GTK_FILE_CHOOSER_VIEW_GET_IFACE (view);

  g_assert (iface->selected_foreach != NULL);
  (* iface->selected_foreach) (view, func, data);
}
