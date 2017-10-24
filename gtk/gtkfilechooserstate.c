/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-file-style: "gnu" -*- */
/* GTK - The GIMP Toolkit
 * gtkfilechooserstate.c: State object for the file chooser
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
#include "gtkfilechooserstate.h"

G_GNUC_WARN_UNUSED_RESULT
static GPtrArray *
free_files (GPtrArray *files)
{
  if (files == NULL)
    {
      return NULL;
    }

  g_ptr_array_free (files, TRUE);
  return NULL;
}

G_GNUC_WARN_UNUSED_RESULT
static GPtrArray *
new_files_array (guint len)
{
  return g_ptr_array_new_full (len, (GDestroyNotify) g_object_unref);
}

G_GNUC_WARN_UNUSED_RESULT
static GPtrArray *
copy_files (GPtrArray *files)
{
  GPtrArray *dst;
  guint len;
  guint i;

  if (files == NULL)
    {
      return NULL;
    }

  len = files->len;
  dst = new_files_array (len);

  for (i = 0; i < len; i++)
    {
      GFile *file;

      file = g_ptr_array_index (files, i);
      g_ptr_array_add (dst, g_object_ref (file));
    }

  return dst;
}

void
gtk_file_chooser_state_discard (GtkFileChooserState *state)
{
  state->selected_files = free_files (state->selected_files);
}

void
gtk_file_chooser_state_copy (const GtkFileChooserState *src, GtkFileChooserState *dst)
{
  gtk_file_chooser_state_discard (dst);

  dst->action          = src->action;
  dst->select_multiple = src->select_multiple;
  dst->operation_mode  = src->operation_mode;
  dst->create_folders  = src->create_folders;
  dst->selected_files  = copy_files (src->selected_files);
}

void
gtk_file_chooser_state_set_action (GtkFileChooserState *state, GtkFileChooserAction action)
{
  gtk_file_chooser_state_set_selected_files (state, NULL);
  state->action = action;
}

void
gtk_file_chooser_state_set_needs_focus_widget (GtkFileChooserState *state, gboolean needs_focus_widget)
{
  state->needs_focus_widget = needs_focus_widget;
}

void
gtk_file_chooser_state_set_create_folders (GtkFileChooserState *state, gboolean create_folders)
{
  state->create_folders = create_folders;
}

/* Takes ownership of the files array */
void
gtk_file_chooser_state_set_selected_files (GtkFileChooserState *state, GPtrArray *files)
{
  state->selected_files = free_files (state->selected_files);
  state->selected_files = files;
}

GPtrArray *
gtk_file_chooser_state_new_file_array (void)
{
  return new_files_array (0);
}
