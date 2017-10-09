/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechoosericonview.h: Icon view mode for the file chooser
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

#ifndef __GTK_FILE_CHOOSER_ICON_VIEW_H__
#define __GTK_FILE_CHOOSER_ICON_VIEW_H__

#include "gtkfilechooserview.h"

G_BEGIN_DECLS

#define GTK_TYPE_FILE_CHOOSER_ICON_VIEW             (gtk_file_chooser_icon_view_get_type ())
#define GTK_FILE_CHOOSER_ICON_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_CHOOSER_ICON_VIEW, GtkFileChooserIconView))
#define GTK_FILE_CHOOSER_ICON_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_CHOOSER_ICON_VIEW, GtkFileChooserIconViewClass))
#define GTK_IS_FILE_CHOOSER_ICON_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_CHOOSER_ICON_VIEW))
#define GTK_IS_FILE_CHOOSER_ICON_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_CHOOSER_ICON_VIEW))
#define GTK_FILE_CHOOSER_ICON_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_CHOOSER_ICON_VIEW, GtkFileChooserIconViewClass))

typedef struct _GtkFileChooserIconView GtkFileChooserIconView;
typedef struct _GtkFileChooserIconViewClass GtkFileChooserIconViewClass;

G_GNUC_INTERNAL
GType gtk_file_chooser_icon_view_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GTK_FILE_CHOOSER_ICON_VIEW_H__ */
