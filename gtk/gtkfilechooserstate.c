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

void
gtk_file_chooser_state_discard (GtkFileChooserState *state)
{
}

void
gtk_file_chooser_state_copy (const GtkFileChooserState *src, GtkFileChooserState *dst)
{
  gtk_file_chooser_state_discard (dst);

  dst->action         = src->action;
  dst->operation_mode = src->operation_mode;
}
