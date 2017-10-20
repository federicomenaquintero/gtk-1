/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechooserstate.h: State object for the file chooser
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

#ifndef __GTK_FILE_CHOOSER_STATE_H__
#define __GTK_FILE_CHOOSER_STATE_H__

#include "gtkfilechooser.h"

typedef enum {
  OPERATION_MODE_BROWSE,
  OPERATION_MODE_SEARCH,
  OPERATION_MODE_ENTER_LOCATION,
  OPERATION_MODE_OTHER_LOCATIONS,
  OPERATION_MODE_RECENT
} OperationMode;

typedef struct
{
  GtkFileChooserAction  action;
  gboolean              select_multiple;
  OperationMode         operation_mode;
} GtkFileChooserState;

/*
typedef struct
{
  enum action {
    Open(Vec<selected_files>),
    Save(current_name | selected_file)
    SelectFolder(Vec<selected_files>),
    CreateFolder(current_name | selected_file)
  };

  enum operation_mode {
    Browse(current_folder),
    Search(),
    Recent(),
    EnterLocation(initial_string),
    OtherLocations
  };

  // choices?
} GtkFileChooserState;
*/

G_GNUC_INTERNAL
void gtk_file_chooser_state_discard (GtkFileChooserState *state);

G_GNUC_INTERNAL
void gtk_file_chooser_state_copy (const GtkFileChooserState *src, GtkFileChooserState *dst);

#endif /* __GTK_FILE_CHOOSER_STATE_H__ */
