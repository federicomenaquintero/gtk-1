/* GTK - The GIMP Toolkit
 *   
 * Copyright © 2017 Benjamin Otte <otte@gnome.org>
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

#include "gsksltypeprivate.h"

#include "gsksltokenizerprivate.h"
#include "gskslpreprocessorprivate.h"

#include <string.h>

struct _GskSlType
{
  int ref_count;

  GskSlScalarType scalar;
};

#define N_SCALAR_TYPES 6

static GskSlType
builtin_types[N_SCALAR_TYPES] = {
  [GSK_SL_VOID] = { 1, GSK_SL_VOID },
  [GSK_SL_FLOAT] = { 1, GSK_SL_FLOAT },
  [GSK_SL_DOUBLE] = { 1, GSK_SL_DOUBLE },
  [GSK_SL_INT] = { 1, GSK_SL_INT },
  [GSK_SL_UINT] = { 1, GSK_SL_UINT },
  [GSK_SL_BOOL] = { 1, GSK_SL_BOOL },
};

GskSlType *
gsk_sl_type_new_parse (GskSlPreprocessor *stream)
{
  GskSlType *type;
  const GskSlToken *token;

  token = gsk_sl_preprocessor_get (stream);

  switch (token->type)
  {
    case GSK_SL_TOKEN_VOID:
      type = gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_VOID));
      break;
    case GSK_SL_TOKEN_FLOAT:
      type = gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_FLOAT));
      break;
    case GSK_SL_TOKEN_DOUBLE:
      type = gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_DOUBLE));
      break;
    case GSK_SL_TOKEN_INT:
      type = gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_INT));
      break;
    case GSK_SL_TOKEN_UINT:
      type = gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_UINT));
      break;
    case GSK_SL_TOKEN_BOOL:
      type = gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_BOOL));
      break;
    default:
      gsk_sl_preprocessor_error (stream, "Expected type specifier");
      return NULL;
  }

  gsk_sl_preprocessor_consume (stream, NULL);

  return type;
}

GskSlType *
gsk_sl_type_get_scalar (GskSlScalarType scalar)
{
  g_assert (scalar < N_SCALAR_TYPES);

  return &builtin_types[scalar];
}

GskSlType *
gsk_sl_type_ref (GskSlType *type)
{
  g_return_val_if_fail (type != NULL, NULL);

  type->ref_count += 1;

  return type;
}

void
gsk_sl_type_unref (GskSlType *type)
{
  if (type == NULL)
    return;

  type->ref_count -= 1;
  if (type->ref_count > 0)
    return;

  type->class->free (type);
}

void
gsk_sl_type_print (const GskSlType *type,
                   GString         *string)
{
  switch (type->scalar)
  {
    case GSK_SL_VOID:
      g_string_append (string, "void");
      break;
    case GSK_SL_FLOAT:
      g_string_append (string, "float");
      break;
    case GSK_SL_DOUBLE:
      g_string_append (string, "double");
      break;
    case GSK_SL_INT:
      g_string_append (string, "int");
      break;
    case GSK_SL_UINT:
      g_string_append (string, "uint");
      break;
    case GSK_SL_BOOL:
      g_string_append (string, "bool");
      break;
    /* add more above */
    default:
      g_assert_not_reached ();
      break;
  }
}

char *
gsk_sl_type_to_string (const GskSlType *type)
{
  GString *string;

  string = g_string_new (NULL);
  gsk_sl_type_print (type, string);
  return g_string_free (string, FALSE);
}

gboolean
gsk_sl_type_can_convert (const GskSlType *target,
                         const GskSlType *source)
{
  if (target->scalar == source->scalar)
    return TRUE;

  switch (source->scalar)
  {
    case GSK_SL_INT:
      return target->scalar == GSK_SL_UINT
          || target->scalar == GSK_SL_FLOAT
          || target->scalar == GSK_SL_DOUBLE;
    case GSK_SL_UINT:
      return target->scalar == GSK_SL_FLOAT
          || target->scalar == GSK_SL_DOUBLE;
    case GSK_SL_FLOAT:
      return target->scalar == GSK_SL_DOUBLE;
    default:
      return FALSE;
  }
}
