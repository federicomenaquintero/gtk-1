/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
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

#ifndef __GTK_FILE_CHOOSER_VIEW_H__
#define __GTK_FILE_CHOOSER_VIEW_H__

#include "gtkfilechooserutils.h"
#include "gtktreeselection.h"

G_BEGIN_DECLS

#define GTK_TYPE_FILE_CHOOSER_VIEW            (gtk_file_chooser_view_get_type ())
#define GTK_FILE_CHOOSER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_CHOOSER_VIEW, GtkFileChooserView))
#define GTK_FILE_CHOOSER_VIEW_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), GTK_TYPE_FILE_CHOOSER_VIEW, GtkFileChooserViewIface))
#define GTK_IS_FILE_CHOOSER_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_CHOOSER_VIEW))
#define GTK_FILE_CHOOSER_VIEW_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTK_TYPE_FILE_CHOOSER_VIEW, GtkFileChooserViewIface))

typedef struct _GtkFileChooserView GtkFileChooserView;

typedef struct {
  GTypeInterface base_iface;

  /* Signals */
  void (* context_menu)        (GtkFileChooserView *view,
                                GdkRectangle       *relative_to);

  void (* set_settings)        (GtkFileChooserView *view, GtkFileChooserSettings *settings);
  void (* set_model)           (GtkFileChooserView *view, GtkFileSystemModel *model);
  void (* select_all)          (GtkFileChooserView *view);
  void (* unselect_all)        (GtkFileChooserView *view);
  void (* set_iter_selection)  (GtkFileChooserView *view, GtkTreeIter *iter, gboolean do_select);
  void (* get_region_for_path) (GtkFileChooserView *view, GtkTreePath *path, GdkRectangle *out_rect);

  void (* selected_foreach)    (GtkFileChooserView          *view,
                                GtkTreeSelectionForeachFunc  func,
                                gpointer                     data);
} GtkFileChooserViewIface;

G_GNUC_INTERNAL
GType gtk_file_chooser_view_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
void gtk_file_chooser_view_set_settings (GtkFileChooserView *view, GtkFileChooserSettings *settings);

G_GNUC_INTERNAL
void gtk_file_chooser_view_set_model (GtkFileChooserView *view, GtkFileSystemModel *model);

G_GNUC_INTERNAL
void gtk_file_chooser_view_select_all (GtkFileChooserView *view);

G_GNUC_INTERNAL
void gtk_file_chooser_view_unselect_all (GtkFileChooserView *view);

G_GNUC_INTERNAL
void gtk_file_chooser_view_set_iter_selection (GtkFileChooserView *view, GtkTreeIter *iter, gboolean do_select);

G_GNUC_INTERNAL
void gtk_file_chooser_view_get_region_for_path (GtkFileChooserView *view, GtkTreePath *path, GdkRectangle *out_rect);

G_GNUC_INTERNAL
void gtk_file_chooser_view_selected_foreach (GtkFileChooserView          *view,
                                             GtkTreeSelectionForeachFunc  func,
                                             gpointer                     data);

G_GNUC_INTERNAL
void gtk_file_chooser_view_emit_context_menu (GtkFileChooserView *view,
                                              GdkRectangle       *relative_to);

G_END_DECLS

#endif /* __GTK_FILE_CHOOSER_VIEW_H__ */
