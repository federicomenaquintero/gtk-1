/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechooserutils.h: Private utility functions useful for
 *                        implementing a GtkFileChooser interface
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __GTK_FILE_CHOOSER_UTILS_H__
#define __GTK_FILE_CHOOSER_UTILS_H__

#include "gtkfilechooserprivate.h"

G_BEGIN_DECLS

#define GTK_FILE_CHOOSER_DELEGATE_QUARK	  (_gtk_file_chooser_delegate_get_quark ())

typedef enum {
  GTK_FILE_CHOOSER_PROP_FIRST                  = 0x1000,
  GTK_FILE_CHOOSER_PROP_ACTION                 = GTK_FILE_CHOOSER_PROP_FIRST,
  GTK_FILE_CHOOSER_PROP_FILTER,
  GTK_FILE_CHOOSER_PROP_LOCAL_ONLY,
  GTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET,
  GTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE,
  GTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL,
  GTK_FILE_CHOOSER_PROP_EXTRA_WIDGET,
  GTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
  GTK_FILE_CHOOSER_PROP_SHOW_HIDDEN,
  GTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION,
  GTK_FILE_CHOOSER_PROP_CREATE_FOLDERS,
  GTK_FILE_CHOOSER_PROP_LAST                   = GTK_FILE_CHOOSER_PROP_CREATE_FOLDERS
} GtkFileChooserProp;

void _gtk_file_chooser_install_properties (GObjectClass *klass);

void _gtk_file_chooser_delegate_iface_init (GtkFileChooserIface *iface);
void _gtk_file_chooser_set_delegate        (GtkFileChooser *receiver,
					    GtkFileChooser *delegate);

GQuark _gtk_file_chooser_delegate_get_quark (void) G_GNUC_CONST;

GList *_gtk_file_chooser_extract_recent_folders (GList *infos);

GSettings *_gtk_file_chooser_get_settings_for_widget (GtkWidget *widget);

gchar * _gtk_file_chooser_label_for_file (GFile *file);

typedef enum {
  /* the first 3 must be these due to settings caching sort column */
  MODEL_COL_NAME,
  MODEL_COL_SIZE,
  MODEL_COL_TIME,
  MODEL_COL_FILE,
  MODEL_COL_NAME_COLLATED,
  MODEL_COL_IS_FOLDER,
  MODEL_COL_IS_SENSITIVE,
  MODEL_COL_SURFACE,
  MODEL_COL_SIZE_TEXT,
  MODEL_COL_DATE_TEXT,
  MODEL_COL_TIME_TEXT,
  MODEL_COL_LOCATION_TEXT,
  MODEL_COL_ELLIPSIZE,
  MODEL_COL_NUM_COLUMNS
} GtkFileChooserModelCol;

typedef enum {
  STARTUP_MODE_RECENT,
  STARTUP_MODE_CWD
} StartupMode;

typedef enum {
  CLOCK_FORMAT_24,
  CLOCK_FORMAT_12
} ClockFormat;

typedef enum {
  DATE_FORMAT_REGULAR,
  DATE_FORMAT_WITH_TIME
} DateFormat;

typedef struct
{
  gint        sort_column;
  GtkSortType sort_order;
  StartupMode startup_mode;
  gint        sidebar_width;
  DateFormat  date_format;
  ClockFormat clock_format;

  guint show_hidden            : 1;
  guint show_size_column       : 1;
  guint sort_directories_first : 1;
} GtkFileChooserSettings;

GtkFileChooserSettings _gtk_file_chooser_read_settings (GSettings *settings);
GtkFileChooserSettings _gtk_file_chooser_get_default_settings (void);

G_END_DECLS

#endif /* __GTK_FILE_CHOOSER_UTILS_H__ */
