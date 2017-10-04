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

#include "gskslscopeprivate.h"

#include "gsksltypeprivate.h"
#include "gskslfunctionprivate.h"
#include "gskslvariableprivate.h"

#include <string.h>

struct _GskSlScope
{
  int ref_count;

  GskSlScope *parent;
  GskSlType *return_type;
  
  GHashTable *variables;
  GHashTable *functions;
};

GskSlScope *
gsk_sl_scope_new (GskSlScope *parent,
                  GskSlType  *return_type)
{
  GskSlScope *scope;
  
  scope = g_slice_new0 (GskSlScope);
  scope->ref_count = 1;

  if (parent)
    scope->parent = gsk_sl_scope_ref (parent);
  if (return_type)
    scope->return_type = gsk_sl_type_ref (return_type);
  scope->variables = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) gsk_sl_variable_unref);
  scope->functions = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) gsk_sl_function_unref);

  return scope;
}

GskSlScope *
gsk_sl_scope_ref (GskSlScope *scope)
{
  g_return_val_if_fail (scope != NULL, NULL);

  scope->ref_count += 1;

  return scope;
}

void
gsk_sl_scope_unref (GskSlScope *scope)
{
  if (scope == NULL)
    return;

  scope->ref_count -= 1;
  if (scope->ref_count > 0)
    return;

  g_hash_table_unref (scope->variables);
  g_hash_table_unref (scope->functions);

  if (scope->parent)
    gsk_sl_scope_unref (scope->parent);
  if (scope->return_type)
    gsk_sl_type_unref (scope->return_type);

  g_slice_free (GskSlScope, scope);
}

GskSlType *
gsk_sl_scope_get_return_type (const GskSlScope *scope)
{
  return scope->return_type;
}

void
gsk_sl_scope_add_variable (GskSlScope    *scope,
                           GskSlVariable *variable)
{
  g_hash_table_replace (scope->variables, gsk_sl_variable_get_name (variable), gsk_sl_variable_ref (variable));
}

GskSlVariable *
gsk_sl_scope_lookup_variable (GskSlScope *scope,
                              const char *name)
{
  GskSlVariable *result;

  for (;
       scope != NULL;
       scope = scope->parent)
    {
      result = g_hash_table_lookup (scope->variables, name);
      if (result)
        return result;
    }

  return NULL;
}

void
gsk_sl_scope_add_function (GskSlScope    *scope,
                           GskSlFunction *function)
{
  g_hash_table_replace (scope->functions, gsk_sl_function_get_name (function), gsk_sl_function_ref (function));
}

GskSlFunction *
gsk_sl_scope_lookup_function (GskSlScope *scope,
                              const char *name)
{
  GskSlFunction *result;

  for (;
       scope != NULL;
       scope = scope->parent)
    {
      result = g_hash_table_lookup (scope->functions, name);
      if (result)
        return result;
    }

  return NULL;
}
