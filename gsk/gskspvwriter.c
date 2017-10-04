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

#include "gskspvwriterprivate.h"

#include "gskslpointertypeprivate.h"
#include "gsksltypeprivate.h"
#include "gskslvalueprivate.h"
#include "gskslvariableprivate.h"

struct _GskSpvWriter
{
  int ref_count;

  guint32 last_id;
  GArray *code[GSK_SPV_WRITER_N_SECTIONS];

  guint32 entry_point;
  GHashTable *types;
  GHashTable *pointer_types;
  GHashTable *values;
  GHashTable *variables;
};

GskSpvWriter *
gsk_spv_writer_new (void)
{
  GskSpvWriter *writer;
  guint i;
  
  writer = g_slice_new0 (GskSpvWriter);
  writer->ref_count = 1;

  for (i = 0; i < GSK_SPV_WRITER_N_SECTIONS; i++)
    {
      writer->code[i] = g_array_new (FALSE, FALSE, sizeof (guint32));
    }

  writer->types = g_hash_table_new_full (gsk_sl_type_hash, gsk_sl_type_equal,
                                         (GDestroyNotify) gsk_sl_type_unref, NULL);
  writer->pointer_types = g_hash_table_new_full (gsk_sl_pointer_type_hash, gsk_sl_pointer_type_equal,
                                                 (GDestroyNotify) gsk_sl_pointer_type_unref, NULL);
  writer->values = g_hash_table_new_full (gsk_sl_value_hash, gsk_sl_value_equal,
                                          (GDestroyNotify) gsk_sl_value_free, NULL);
  writer->variables = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                             (GDestroyNotify) gsk_sl_variable_unref, NULL);

  return writer;
}

GskSpvWriter *
gsk_spv_writer_ref (GskSpvWriter *writer)
{
  g_return_val_if_fail (writer != NULL, NULL);

  writer->ref_count += 1;

  return writer;
}

void
gsk_spv_writer_unref (GskSpvWriter *writer)
{
  guint i;

  if (writer == NULL)
    return;

  writer->ref_count -= 1;
  if (writer->ref_count > 0)
    return;

  for (i = 0; i < GSK_SPV_WRITER_N_SECTIONS; i++)
    {
      g_array_free (writer->code[i], TRUE);
    }

  g_hash_table_destroy (writer->pointer_types);
  g_hash_table_destroy (writer->types);
  g_hash_table_destroy (writer->values);
  g_hash_table_destroy (writer->variables);

  g_slice_free (GskSpvWriter, writer);
}

#define STRING(s, offset) ((guint32) ((s)[offset + 0] | ((s)[offset + 1] << 8) | ((s)[offset + 2] << 16) | ((s)[offset + 3] << 24)))
static void
gsk_spv_writer_write_header (GskSpvWriter *writer)
{
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_HEADER,
                      2, GSK_SPV_OP_CAPABILITY,
                      (guint32[1]) { GSK_SPV_CAPABILITY_SHADER });
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_HEADER,
                      3, GSK_SPV_OP_MEMORY_MODEL,
                      (guint32[2]) { GSK_SPV_ADDRESSING_LOGICAL,
                                     GSK_SPV_MEMORY_GLSL450 });
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_HEADER,
                      5, GSK_SPV_OP_ENTRY_POINT,
                      (guint32[4]) { GSK_SPV_EXECUTION_MODEL_FRAGMENT,
                                     writer->entry_point,
                                     STRING ("main", 0),
                                     0 });
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_HEADER,
                      3, GSK_SPV_OP_EXECUTION_MODE,
                      (guint32[4]) { writer->entry_point,
                                     GSK_SPV_EXECUTION_MODE_ORIGIN_UPPER_LEFT });
}

static void
gsk_spv_writer_clear_header (GskSpvWriter *writer)
{
  g_array_set_size (writer->code[GSK_SPV_WRITER_SECTION_HEADER], 0);
}

GBytes *
gsk_spv_writer_write (GskSpvWriter *writer)
{
  GArray *array;
  gsize size;
  guint i;

  gsk_spv_writer_write_header (writer);

  array = g_array_new (FALSE, FALSE, sizeof (guint32));

  g_array_append_val (array, (guint32) { GSK_SPV_MAGIC_NUMBER });
  g_array_append_val (array, (guint32) { (GSK_SPV_VERSION_MAJOR << 16) | (GSK_SPV_VERSION_MINOR << 8) });
  g_array_append_val (array, (guint32) { GSK_SPV_GENERATOR });
  g_array_append_val (array, (guint32) { writer->last_id + 1 });
  g_array_append_val (array, (guint32) { writer->last_id + 1 });
  
  for (i = 0; i < GSK_SPV_WRITER_N_SECTIONS; i++)
    {
      g_array_append_vals (array, writer->code[i]->data, writer->code[i]->len);
    }

  gsk_spv_writer_clear_header (writer);

  size = array->len * sizeof (guint32);
  return g_bytes_new_take (g_array_free (array, FALSE), size);
}

guint32
gsk_spv_writer_get_id_for_type (GskSpvWriter *writer,
                                GskSlType    *type)
{
  guint32 result;

  result = GPOINTER_TO_UINT (g_hash_table_lookup (writer->types, type));
  if (result != 0)
    return result;

  result = gsk_sl_type_write_spv (type, writer);
  g_hash_table_insert (writer->types, gsk_sl_type_ref (type), GUINT_TO_POINTER (result));
  return result;
}

guint32
gsk_spv_writer_get_id_for_pointer_type (GskSpvWriter       *writer,
                                        GskSlPointerType   *type)
{
  guint32 result;

  result = GPOINTER_TO_UINT (g_hash_table_lookup (writer->pointer_types, type));
  if (result != 0)
    return result;

  result = gsk_sl_pointer_type_write_spv (type, writer);
  g_hash_table_insert (writer->pointer_types, gsk_sl_pointer_type_ref (type), GUINT_TO_POINTER (result));
  return result;
}

guint32
gsk_spv_writer_get_id_for_value (GskSpvWriter *writer,
                                 GskSlValue   *value)
{
  guint32 result;

  result = GPOINTER_TO_UINT (g_hash_table_lookup (writer->values, value));
  if (result != 0)
    return result;

  result = gsk_sl_value_write_spv (value, writer);
  g_hash_table_insert (writer->values, gsk_sl_value_copy (value), GUINT_TO_POINTER (result));
  return result;
}

guint32
gsk_spv_writer_get_id_for_variable (GskSpvWriter  *writer,
                                    GskSlVariable *variable)
{
  guint32 result;

  result = GPOINTER_TO_UINT (g_hash_table_lookup (writer->variables, variable));
  if (result != 0)
    return result;

  result = gsk_sl_variable_write_spv (variable, writer);
  g_hash_table_insert (writer->variables, gsk_sl_variable_ref (variable), GUINT_TO_POINTER (result));
  return result;
}

guint32
gsk_spv_writer_next_id (GskSpvWriter *writer)
{
  writer->last_id++;

  return writer->last_id;
}

void
gsk_spv_writer_set_entry_point (GskSpvWriter *writer,
                                guint32       entry_point)
{
  writer->entry_point = entry_point;
}

void
gsk_spv_writer_add (GskSpvWriter        *writer,
                    GskSpvWriterSection  section,
                    guint16              word_count,
                    guint16              opcode,
                    guint32             *words)
{
  guint32 word;

  word = word_count << 16 | opcode;
  g_array_append_val (writer->code[section], word);
  g_array_append_vals (writer->code[section], words, word_count - 1);
}

static void
copy_4_bytes (gpointer dest, gpointer src)
{
  memcpy (dest, src, 4);
}

static void
copy_8_bytes (gpointer dest, gpointer src)
{
  memcpy (dest, src, 8);
}

guint32
gsk_spv_writer_add_conversion (GskSpvWriter    *writer,
                               guint32          id,
                               const GskSlType *type,
                               const GskSlType *new_type)
{
  GskSlScalarType scalar = gsk_sl_type_get_scalar_type (type);
  GskSlScalarType new_scalar = gsk_sl_type_get_scalar_type (new_type);

  if (scalar == new_scalar)
    return id;

  if (gsk_sl_type_is_scalar (type) ||
      gsk_sl_type_is_vector (type))
    {
      GskSlValue *value;
      guint32 result_type_id, result_id;
      guint32 true_id, false_id, zero_id;

      switch (new_scalar)
        {
        case GSK_SL_VOID:
        default:
          g_assert_not_reached ();
          return id;

        case GSK_SL_INT:
        case GSK_SL_UINT:
          switch (scalar)
            {
            case GSK_SL_INT:
            case GSK_SL_UINT:
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  4, GSK_SPV_OP_BITCAST,
                                  (guint32[3]) { result_type_id,
                                                 result_id,
                                                 id });
              return result_id;

            case GSK_SL_FLOAT:
            case GSK_SL_DOUBLE:
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  4, GSK_SPV_OP_CONVERT_F_TO_S,
                                  (guint32[3]) { result_type_id,
                                                 result_id,
                                                 id });
              return result_id;

            case GSK_SL_BOOL:
              value = gsk_sl_value_new (new_type);
              true_id = gsk_spv_writer_get_id_for_value (writer, value);
              gsk_sl_value_componentwise (value, copy_4_bytes, &(gint32) { 1 });
              false_id = gsk_spv_writer_get_id_for_value (writer, value);
              gsk_sl_value_free (value);
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  6, GSK_SPV_OP_SELECT,
                                  (guint32[5]) { result_type_id,
                                                 result_id,
                                                 id,
                                                 true_id,
                                                 false_id });
              return result_id;

            case GSK_SL_VOID:
            default:
              g_assert_not_reached ();
              return id;
            }
          g_assert_not_reached ();

        case GSK_SL_FLOAT:
        case GSK_SL_DOUBLE:
          switch (scalar)
            {
            case GSK_SL_INT:
            case GSK_SL_UINT:
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  4, GSK_SPV_OP_CONVERT_F_TO_S,
                                  (guint32[3]) { result_type_id,
                                                 result_id,
                                                 id });
              return result_id;

            case GSK_SL_FLOAT:
            case GSK_SL_DOUBLE:
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  4, GSK_SPV_OP_F_CONVERT,
                                  (guint32[3]) { result_type_id,
                                                 result_id,
                                                 id });
              return result_id;

            case GSK_SL_BOOL:
              value = gsk_sl_value_new (new_type);
              true_id = gsk_spv_writer_get_id_for_value (writer, value);
              if (scalar == GSK_SL_DOUBLE)
                gsk_sl_value_componentwise (value, copy_8_bytes, &(double) { 1 });
              else
                gsk_sl_value_componentwise (value, copy_4_bytes, &(float) { 1 });
              false_id = gsk_spv_writer_get_id_for_value (writer, value);
              gsk_sl_value_free (value);
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  6, GSK_SPV_OP_SELECT,
                                  (guint32[5]) { result_type_id,
                                                 result_id,
                                                 id,
                                                 true_id,
                                                 false_id });
              return result_id;

            case GSK_SL_VOID:
            default:
              g_assert_not_reached ();
              return id;
            }
          g_assert_not_reached ();

        case GSK_SL_BOOL:
          switch (scalar)
            {
            case GSK_SL_INT:
            case GSK_SL_UINT:
              value = gsk_sl_value_new (new_type);
              zero_id = gsk_spv_writer_get_id_for_value (writer, value);
              gsk_sl_value_free (value);
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  5, GSK_SPV_OP_I_NOT_EQUAL,
                                  (guint32[4]) { result_type_id,
                                                 result_id,
                                                 id,
                                                 zero_id });
              return result_id;

            case GSK_SL_FLOAT:
            case GSK_SL_DOUBLE:
              value = gsk_sl_value_new (new_type);
              zero_id = gsk_spv_writer_get_id_for_value (writer, value);
              gsk_sl_value_free (value);
              result_type_id = gsk_spv_writer_get_id_for_type (writer, new_type);
              result_id = gsk_spv_writer_next_id (writer);
              gsk_spv_writer_add (writer,
                                  GSK_SPV_WRITER_SECTION_CODE,
                                  5, GSK_SPV_OP_F_ORD_NOT_EQUAL,
                                  (guint32[4]) { result_type_id,
                                                 result_id,
                                                 id,
                                                 zero_id });
              return result_id;

            case GSK_SL_BOOL:
            case GSK_SL_VOID:
            default:
              g_assert_not_reached ();
              return id;
            }
          g_assert_not_reached ();

        }
    }
  else if (gsk_sl_type_is_matrix (type))
    {
      GskSlType *row_type, *new_row_type;
      guint i, n = gsk_sl_type_get_length (type);
      guint32 ids[n + 2], row_id;

      row_type = gsk_sl_type_get_index_type (type);
      new_row_type = gsk_sl_type_get_index_type (new_type);
      row_id = gsk_spv_writer_get_id_for_type (writer, row_type);
      for (i = 0; i < n; i++)
        {
          guint tmp_id = gsk_spv_writer_next_id (writer);
          gsk_spv_writer_add (writer,
                              GSK_SPV_WRITER_SECTION_CODE,
                              5, GSK_SPV_OP_COMPOSITE_EXTRACT,
                              (guint32[4]) { row_id,
                                             tmp_id,
                                             id,
                                             i });
          ids[i + 2] = gsk_spv_writer_add_conversion (writer, tmp_id, row_type, new_row_type);
        }

      ids[0] = gsk_spv_writer_get_id_for_type (writer, new_type);
      ids[1] = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_CODE,
                          3 + n, GSK_SPV_OP_COMPOSITE_CONSTRUCT,
                          ids);

      return ids[1];
    }
  else
    {
      g_return_val_if_reached (id);
    }
}
