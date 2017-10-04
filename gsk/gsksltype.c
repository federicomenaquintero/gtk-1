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
#include "gskslpreprocessorprivate.h"
#include "gskslscopeprivate.h"
#include "gskslvalueprivate.h"
#include "gskspvwriterprivate.h"

#include <string.h>

#define N_SCALAR_TYPES 6

typedef struct _GskSlTypeMember GskSlTypeMember;
typedef struct _GskSlTypeClass GskSlTypeClass;

struct _GskSlTypeMember {
  GskSlType *type;
  char *name;
  gsize offset;
};


struct _GskSlType
{
  const GskSlTypeClass *class;

  int ref_count;
};

struct _GskSlTypeClass {
  void                  (* free)                                (GskSlType           *type);

  const char *          (* get_name)                            (GskSlType           *type);
  GskSlScalarType       (* get_scalar_type)                     (GskSlType           *type);
  GskSlType *           (* get_index_type)                      (GskSlType           *type);
  gsize                 (* get_index_stride)                    (GskSlType           *type);
  guint                 (* get_length)                          (GskSlType           *type);
  gsize                 (* get_size)                            (GskSlType           *type);
  guint                 (* get_n_members)                       (GskSlType           *type);
  const GskSlTypeMember * (* get_member)                        (GskSlType           *type,
                                                                 guint                n);
  gboolean              (* can_convert)                         (GskSlType           *target,
                                                                 GskSlType           *source);
  guint32               (* write_spv)                           (const GskSlType     *type,
                                                                 GskSpvWriter        *writer);
  void                  (* print_value)                         (const GskSlType     *type,
                                                                 GString             *string,
                                                                 gpointer             value);
  guint32               (* write_value_spv)                     (GskSlType           *type,
                                                                 GskSpvWriter        *writer,
                                                                 gpointer             value);
};

static void
print_void (GString *string,
            gpointer value)
{
  g_assert_not_reached ();
}

static guint32
write_void_spv (GskSpvWriter *writer, gpointer value)
{
  g_assert_not_reached ();
}

static void
print_float (GString *string,
             gpointer value)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  gfloat *f = value;
      
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, *f);
  g_string_append (string, buf);
  if (strchr (buf, '.') == NULL)
    g_string_append (string, ".0");
}

static guint32
write_float_spv (GskSpvWriter *writer, gpointer value)
{
  guint32 type_id, result_id;

  type_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_scalar (GSK_SL_FLOAT));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      4, GSK_SPV_OP_CONSTANT,
                      (guint32[3]) { type_id,
                                     result_id,
                                     *(guint32 *) value });

  return result_id;
}

static void
print_double (GString *string,
              gpointer value)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble *d = value;
      
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, *d);
  g_string_append (string, buf);
  if (strchr (buf, '.') == NULL)
    g_string_append (string, ".0");
  g_string_append (string, "lf");
}

static guint32
write_double_spv (GskSpvWriter *writer, gpointer value)
{
  guint32 type_id, result_id;

  type_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_scalar (GSK_SL_DOUBLE));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      5, GSK_SPV_OP_CONSTANT,
                      (guint32[4]) { type_id,
                                     result_id,
                                     *(guint32 *) value,
                                     *(((guint32 *) value) + 1) });

  return result_id;
}

static void
print_int (GString *string,
           gpointer value)
{
  gint32 *i = value;

  g_string_append_printf (string, "%i", (gint) *i);
}

static guint32
write_int_spv (GskSpvWriter *writer, gpointer value)
{
  guint32 type_id, result_id;

  type_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_scalar (GSK_SL_INT));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      4, GSK_SPV_OP_CONSTANT,
                      (guint32[3]) { type_id,
                                     result_id,
                                     *(guint32 *) value });

  return result_id;
}

static void
print_uint (GString *string,
            gpointer value)
{
  guint32 *u = value;
  
  g_string_append_printf (string, "%uu", (guint) *u);
}

static guint32
write_uint_spv (GskSpvWriter *writer, gpointer value)
{
  guint32 type_id, result_id;

  type_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_scalar (GSK_SL_UINT));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      4, GSK_SPV_OP_CONSTANT,
                      (guint32[3]) { type_id,
                                     result_id,
                                     *(guint32 *) value });

  return result_id;
}

static void
print_bool (GString *string,
            gpointer value)
{
  guint32 *u = value;
  
  g_string_append_printf (string, *u ? "true" : "false");
}

static guint32
write_bool_spv (GskSpvWriter *writer, gpointer value)
{
  guint32 type_id, result_id;

  type_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_scalar (GSK_SL_BOOL));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      3, *(guint32 *) value ? GSK_SPV_OP_CONSTANT_TRUE : GSK_SPV_OP_CONSTANT_FALSE,
                      (guint32[2]) { type_id,
                                     result_id });
  
  return result_id;
}

#define SIMPLE_CONVERSION(source_name, target_name, source_type, target_type) \
static void \
source_name ## _to_ ## target_name (gpointer target, gconstpointer source) \
{ \
  *(target_type *) target = *(const source_type *) source; \
}

SIMPLE_CONVERSION(float, float, float, float);
SIMPLE_CONVERSION(float, double, float, double);
SIMPLE_CONVERSION(float, int, float, gint32);
SIMPLE_CONVERSION(float, uint, float, guint32);
static void
float_to_bool (gpointer target, gconstpointer source)
{
  *(guint32 *) target = *(const float *) source ? TRUE : FALSE;
}

SIMPLE_CONVERSION(double, float, double, float);
SIMPLE_CONVERSION(double, double, double, double);
SIMPLE_CONVERSION(double, int, double, gint32);
SIMPLE_CONVERSION(double, uint, double, guint32);
static void
double_to_bool (gpointer target, gconstpointer source)
{
  *(guint32 *) target = *(const double *) source ? TRUE : FALSE;
}

SIMPLE_CONVERSION(int, float, gint32, float);
SIMPLE_CONVERSION(int, double, gint32, double);
SIMPLE_CONVERSION(int, int, gint32, gint32);
SIMPLE_CONVERSION(int, uint, gint32, guint32);
static void
int_to_bool (gpointer target, gconstpointer source)
{
  *(guint32 *) target = *(const gint32 *) source ? TRUE : FALSE;
}

SIMPLE_CONVERSION(uint, float, guint32, float);
SIMPLE_CONVERSION(uint, double, guint32, double);
SIMPLE_CONVERSION(uint, int, guint32, gint32);
SIMPLE_CONVERSION(uint, uint, guint32, guint32);
static void
uint_to_bool (gpointer target, gconstpointer source)
{
  *(guint32 *) target = *(const guint32 *) source ? TRUE : FALSE;
}

static void
bool_to_float (gpointer target, gconstpointer source)
{
  *(float *) target = *(const guint32 *) source ? 1.0 : 0.0;
}

static void
bool_to_double (gpointer target, gconstpointer source)
{
  *(double *) target = *(const guint32 *) source ? 1.0 : 0.0;
}

static void
bool_to_int (gpointer target, gconstpointer source)
{
  *(gint32 *) target = *(const guint32 *) source ? 1 : 0;
}

static void
bool_to_uint (gpointer target, gconstpointer source)
{
  *(guint32 *) target = *(const guint32 *) source ? 1 : 0;
}

static void
bool_to_bool (gpointer target, gconstpointer source)
{
  *(guint32 *) target = *(const guint32 *) source;
}

#define CONVERSIONS(name) { NULL, name ## _to_float, name ## _to_double, name ## _to_int, name ## _to_uint, name ## _to_bool }
struct {
  char *name;
  gsize size;
  void (* print_value) (GString *string, gpointer value);
  void (* convert_value[N_SCALAR_TYPES]) (gpointer target, gconstpointer source);
  guint32 (* write_value_spv) (GskSpvWriter *writer, gpointer value);
} scalar_infos[] = {
  [GSK_SL_VOID] =   { "void",   0, print_void,   { NULL, },            write_void_spv, },
  [GSK_SL_FLOAT] =  { "float",  4, print_float,  CONVERSIONS (float),  write_float_spv },
  [GSK_SL_DOUBLE] = { "double", 8, print_double, CONVERSIONS (double), write_double_spv },
  [GSK_SL_INT] =    { "int",    4, print_int,    CONVERSIONS (int),    write_int_spv },
  [GSK_SL_UINT] =   { "uint",   4, print_uint,   CONVERSIONS (uint),   write_uint_spv },
  [GSK_SL_BOOL] =   { "bool",   4, print_bool,   CONVERSIONS (bool),   write_bool_spv }
};
#undef SIMPLE_CONVERSION
#undef CONVERSIONS

static GskSlType *
gsk_sl_type_alloc (const GskSlTypeClass *klass,
                   gsize                 size)
{
  GskSlType *type;

  type = g_slice_alloc0 (size);

  type->class = klass;
  type->ref_count = 1;

  return type;
}
#define gsk_sl_type_new(_name, _klass) ((_name *) gsk_sl_type_alloc ((_klass), sizeof (_name)))

/* SCALAR */

typedef struct _GskSlTypeScalar GskSlTypeScalar;

struct _GskSlTypeScalar {
  GskSlType parent;

  GskSlScalarType scalar;
};

static void
gsk_sl_type_scalar_free (GskSlType *type)
{
  g_assert_not_reached ();
}

static const char *
gsk_sl_type_scalar_get_name (GskSlType *type)
{
  GskSlTypeScalar *scalar = (GskSlTypeScalar *) type;

  return scalar_infos[scalar->scalar].name;
}

static GskSlScalarType
gsk_sl_type_scalar_get_scalar_type (GskSlType *type)
{
  GskSlTypeScalar *scalar = (GskSlTypeScalar *) type;

  return scalar->scalar;
}

static GskSlType *
gsk_sl_type_scalar_get_index_type (GskSlType *type)
{
  return NULL;
}

static gsize
gsk_sl_type_scalar_get_index_stride (GskSlType *type)
{
  return 0;
}

static guint
gsk_sl_type_scalar_get_length (GskSlType *type)
{
  return 0;
}

static gsize
gsk_sl_type_scalar_get_size (GskSlType *type)
{
  GskSlTypeScalar *scalar = (GskSlTypeScalar *) type;

  return scalar_infos[scalar->scalar].size;
}

static guint
gsk_sl_type_scalar_get_n_members (GskSlType *type)
{
  return 0;
}

static const GskSlTypeMember *
gsk_sl_type_scalar_get_member (GskSlType *type,
                               guint      n)
{
  return NULL;
}

static gboolean
gsk_sl_type_scalar_can_convert (GskSlType *target,
                                GskSlType *source)
{
  GskSlTypeScalar *target_scalar = (GskSlTypeScalar *) target;
  GskSlTypeScalar *source_scalar = (GskSlTypeScalar *) source;

  if (target->class != source->class)
    return FALSE;
  
  return gsk_sl_scalar_type_can_convert (target_scalar->scalar, source_scalar->scalar);
}

static guint32
gsk_sl_type_scalar_write_spv (const GskSlType *type,
                              GskSpvWriter    *writer)
{
  GskSlTypeScalar *scalar = (GskSlTypeScalar *) type;
  guint32 result;

  switch (scalar->scalar)
  {
    case GSK_SL_VOID:
      result = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_DECLARE,
                          2, GSK_SPV_OP_TYPE_VOID,
                          (guint32[1]) { result });
      break;

    case GSK_SL_FLOAT:
      result = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_DECLARE,
                          3, GSK_SPV_OP_TYPE_FLOAT,
                          (guint32[2]) { result,
                                         32 });
      break;

    case GSK_SL_DOUBLE:
      result = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_DECLARE,
                          3, GSK_SPV_OP_TYPE_FLOAT,
                          (guint32[2]) { result,
                                         64 });
      break;

    case GSK_SL_INT:
      result = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_DECLARE,
                          4, GSK_SPV_OP_TYPE_INT,
                          (guint32[3]) { result,
                                         32,
                                         1 });
      break;

    case GSK_SL_UINT:
      result = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_DECLARE,
                          4, GSK_SPV_OP_TYPE_INT,
                          (guint32[3]) { result,
                                         32,
                                         0 });
      break;

    case GSK_SL_BOOL:
      result = gsk_spv_writer_next_id (writer);
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_DECLARE,
                          2, GSK_SPV_OP_TYPE_BOOL,
                          (guint32[1]) { result });
      break;

    default:
      g_assert_not_reached ();
      break;
  }

  return result;
}

static void
gsk_sl_type_scalar_print_value (const GskSlType *type,
                                GString         *string,
                                gpointer         value)
{
  GskSlTypeScalar *scalar = (GskSlTypeScalar *) type;

  scalar_infos[scalar->scalar].print_value (string, value);
}

static guint32
gsk_sl_type_scalar_write_value_spv (GskSlType    *type,
                                    GskSpvWriter *writer,
                                    gpointer      value)
{
  GskSlTypeScalar *scalar = (GskSlTypeScalar *) type;

  return scalar_infos[scalar->scalar].write_value_spv (writer, value);
}

static const GskSlTypeClass GSK_SL_TYPE_SCALAR = {
  gsk_sl_type_scalar_free,
  gsk_sl_type_scalar_get_name,
  gsk_sl_type_scalar_get_scalar_type,
  gsk_sl_type_scalar_get_index_type,
  gsk_sl_type_scalar_get_index_stride,
  gsk_sl_type_scalar_get_length,
  gsk_sl_type_scalar_get_size,
  gsk_sl_type_scalar_get_n_members,
  gsk_sl_type_scalar_get_member,
  gsk_sl_type_scalar_can_convert,
  gsk_sl_type_scalar_write_spv,
  gsk_sl_type_scalar_print_value,
  gsk_sl_type_scalar_write_value_spv
};

/* VECTOR */

typedef struct _GskSlTypeVector GskSlTypeVector;

struct _GskSlTypeVector {
  GskSlType parent;

  const char *name;
  GskSlScalarType scalar;
  guint length;
};

static void
gsk_sl_type_vector_free (GskSlType *type)
{
  g_assert_not_reached ();
}

static const char *
gsk_sl_type_vector_get_name (GskSlType *type)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;

  return vector->name;
}

static GskSlScalarType
gsk_sl_type_vector_get_scalar_type (GskSlType *type)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;

  return vector->scalar;
}

static GskSlType *
gsk_sl_type_vector_get_index_type (GskSlType *type)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;

  return gsk_sl_type_get_scalar (vector->scalar);
}

static gsize
gsk_sl_type_vector_get_index_stride (GskSlType *type)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;

  return scalar_infos[vector->scalar].size;
}

static guint
gsk_sl_type_vector_get_length (GskSlType *type)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;

  return vector->length;
}

static gsize
gsk_sl_type_vector_get_size (GskSlType *type)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;

  return vector->length * scalar_infos[vector->scalar].size;
}

static guint
gsk_sl_type_vector_get_n_members (GskSlType *type)
{
  return 0;
}

static const GskSlTypeMember *
gsk_sl_type_vector_get_member (GskSlType *type,
                               guint      n)
{
  return NULL;
}

static gboolean
gsk_sl_type_vector_can_convert (GskSlType *target,
                                GskSlType *source)
{
  GskSlTypeVector *target_vector = (GskSlTypeVector *) target;
  GskSlTypeVector *source_vector = (GskSlTypeVector *) source;

  if (target->class != source->class)
    return FALSE;
  
  if (target_vector->length != source_vector->length)
    return FALSE;

  return gsk_sl_scalar_type_can_convert (target_vector->scalar, source_vector->scalar);
}

static guint32
gsk_sl_type_vector_write_spv (const GskSlType *type,
                              GskSpvWriter    *writer)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;
  guint32 result_id, scalar_id;

  scalar_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_scalar (vector->scalar));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      4, GSK_SPV_OP_TYPE_VECTOR,
                      (guint32[3]) { result_id,
                                     scalar_id,
                                     vector->length });
  
  return result_id;
}

static void
gsk_sl_type_vector_print_value (const GskSlType *type,
                                GString         *string,
                                gpointer         value)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;
  guint i;
  guchar *data;

  data = value;

  g_string_append (string, vector->name);
  g_string_append (string, "(");
  for (i = 0; i < vector->length; i++)
    {
      if (i > 0)
        g_string_append (string, ", ");
      scalar_infos[vector->scalar].print_value (string, data);
      data += scalar_infos[vector->scalar].size;
    }

  g_string_append (string, ")");
}

static guint32
gsk_sl_type_vector_write_value_spv (GskSlType    *type,
                                    GskSpvWriter *writer,
                                    gpointer      value)
{
  GskSlTypeVector *vector = (GskSlTypeVector *) type;
  guint32 ids[vector->length + 2];
  GskSlType *scalar_type;
  GskSlValue *v;
  guchar *data;
  guint i;

  data = value;
  scalar_type = gsk_sl_type_get_scalar (vector->scalar);

  ids[0] = gsk_spv_writer_get_id_for_type (writer, type);
  for (i = 0; i < vector->length; i++)
    {
      v = gsk_sl_value_new_for_data (scalar_type, data, NULL, NULL);
      ids[2 + i] = gsk_spv_writer_get_id_for_value (writer, v);
      gsk_sl_value_free (v);
      data += scalar_infos[vector->scalar].size;
    }
  ids[1] = gsk_spv_writer_next_id (writer);

  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      3 + vector->length,
                      GSK_SPV_OP_CONSTANT_COMPOSITE,
                      ids);
  
  return ids[1];
}

static const GskSlTypeClass GSK_SL_TYPE_VECTOR = {
  gsk_sl_type_vector_free,
  gsk_sl_type_vector_get_name,
  gsk_sl_type_vector_get_scalar_type,
  gsk_sl_type_vector_get_index_type,
  gsk_sl_type_vector_get_index_stride,
  gsk_sl_type_vector_get_length,
  gsk_sl_type_vector_get_size,
  gsk_sl_type_vector_get_n_members,
  gsk_sl_type_vector_get_member,
  gsk_sl_type_vector_can_convert,
  gsk_sl_type_vector_write_spv,
  gsk_sl_type_vector_print_value,
  gsk_sl_type_vector_write_value_spv
};

/* MATRIX */

typedef struct _GskSlTypeMatrix GskSlTypeMatrix;

struct _GskSlTypeMatrix {
  GskSlType parent;

  const char *name;
  GskSlScalarType scalar;
  guint columns;
  guint rows;
};

static void
gsk_sl_type_matrix_free (GskSlType *type)
{
  g_assert_not_reached ();
}

static const char *
gsk_sl_type_matrix_get_name (GskSlType *type)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;

  return matrix->name;
}

static GskSlScalarType
gsk_sl_type_matrix_get_scalar_type (GskSlType *type)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;

  return matrix->scalar;
}

static GskSlType *
gsk_sl_type_matrix_get_index_type (GskSlType *type)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;

  return gsk_sl_type_get_vector (matrix->scalar, matrix->rows);
}

static gsize
gsk_sl_type_matrix_get_index_stride (GskSlType *type)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;

  return scalar_infos[matrix->scalar].size * matrix->rows;
}

static guint
gsk_sl_type_matrix_get_length (GskSlType *type)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;

  return matrix->columns;
}

static gsize
gsk_sl_type_matrix_get_size (GskSlType *type)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;

  return matrix->columns * matrix->rows * scalar_infos[matrix->scalar].size;
}

static guint
gsk_sl_type_matrix_get_n_members (GskSlType *type)
{
  return 0;
}

static const GskSlTypeMember *
gsk_sl_type_matrix_get_member (GskSlType *type,
                               guint      n)
{
  return NULL;
}

static gboolean
gsk_sl_type_matrix_can_convert (GskSlType *target,
                                GskSlType *source)
{
  GskSlTypeMatrix *target_matrix = (GskSlTypeMatrix *) target;
  GskSlTypeMatrix *source_matrix = (GskSlTypeMatrix *) source;

  if (target->class != source->class)
    return FALSE;
  
  if (target_matrix->rows != source_matrix->rows ||
      target_matrix->columns != source_matrix->columns)
    return FALSE;

  return gsk_sl_scalar_type_can_convert (target_matrix->scalar, source_matrix->scalar);
}

static guint32
gsk_sl_type_matrix_write_spv (const GskSlType *type,
                              GskSpvWriter    *writer)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;
  guint32 result_id, vector_id;

  vector_id = gsk_spv_writer_get_id_for_type (writer, gsk_sl_type_get_index_type (type));
  result_id = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      4, GSK_SPV_OP_TYPE_MATRIX,
                      (guint32[3]) { result_id,
                                     vector_id,
                                     matrix->columns });
  
  return result_id;
}

static void
gsk_sl_type_matrix_print_value (const GskSlType *type,
                                GString         *string,
                                gpointer         value)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;
  guint i;
  guchar *data;

  data = value;

  g_string_append (string, matrix->name);
  g_string_append (string, "(");
  for (i = 0; i < matrix->rows * matrix->columns; i++)
    {
      if (i > 0)
        g_string_append (string, ", ");
      scalar_infos[matrix->scalar].print_value (string, data);
      data += scalar_infos[matrix->scalar].size;
    }

  g_string_append (string, ")");
}

static guint32
gsk_sl_type_matrix_write_value_spv (GskSlType    *type,
                                    GskSpvWriter *writer,
                                    gpointer      value)
{
  GskSlTypeMatrix *matrix = (GskSlTypeMatrix *) type;
  guint32 ids[matrix->rows + 2];
  GskSlType *vector_type;
  GskSlValue *v;
  guchar *data;
  guint i;

  data = value;
  vector_type = gsk_sl_type_get_index_type (type);

  ids[0] = gsk_spv_writer_get_id_for_type (writer, type);
  for (i = 0; i < matrix->columns; i++)
    {
      v = gsk_sl_value_new_for_data (vector_type, data, NULL, NULL);
      ids[2 + i] = gsk_spv_writer_get_id_for_value (writer, v);
      gsk_sl_value_free (v);
      data += gsk_sl_type_get_size (vector_type);
    }
  ids[1] = gsk_spv_writer_next_id (writer);

  ids[1] = gsk_spv_writer_next_id (writer);
  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      3 + matrix->columns,
                      GSK_SPV_OP_CONSTANT_COMPOSITE,
                      ids);
  
  return ids[1];
}

static const GskSlTypeClass GSK_SL_TYPE_MATRIX = {
  gsk_sl_type_matrix_free,
  gsk_sl_type_matrix_get_name,
  gsk_sl_type_matrix_get_scalar_type,
  gsk_sl_type_matrix_get_index_type,
  gsk_sl_type_matrix_get_index_stride,
  gsk_sl_type_matrix_get_length,
  gsk_sl_type_matrix_get_size,
  gsk_sl_type_matrix_get_n_members,
  gsk_sl_type_matrix_get_member,
  gsk_sl_type_matrix_can_convert,
  gsk_sl_type_matrix_write_spv,
  gsk_sl_type_matrix_print_value,
  gsk_sl_type_matrix_write_value_spv
};

/* STRUCT */

typedef struct _GskSlTypeStruct GskSlTypeStruct;

struct _GskSlTypeStruct {
  GskSlType parent;

  char *name;
  gsize size;

  GskSlTypeMember *members;
  guint n_members;
};

static void
gsk_sl_type_struct_free (GskSlType *type)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;
  guint i;

  for (i = 0; i < struc->n_members; i++)
    {
      gsk_sl_type_unref (struc->members[i].type);
      g_free (struc->members[i].name);
    }

  g_free (struc->members);
  g_free (struc->name);

  g_slice_free (GskSlTypeStruct, struc);
}

static const char *
gsk_sl_type_struct_get_name (GskSlType *type)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;

  return struc->name;
}

static GskSlScalarType
gsk_sl_type_struct_get_scalar_type (GskSlType *type)
{
  return GSK_SL_VOID;
}

static GskSlType *
gsk_sl_type_struct_get_index_type (GskSlType *type)
{
  return NULL;
}

static gsize
gsk_sl_type_struct_get_index_stride (GskSlType *type)
{
  return 0;
}

static guint
gsk_sl_type_struct_get_length (GskSlType *type)
{
  return 0;
}

static gsize
gsk_sl_type_struct_get_size (GskSlType *type)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;

  return struc->size;
}

static guint
gsk_sl_type_struct_get_n_members (GskSlType *type)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;

  return struc->n_members;
}

static const GskSlTypeMember *
gsk_sl_type_struct_get_member (GskSlType *type,
                               guint      n)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;

  return &struc->members[n];
}

static gboolean
gsk_sl_type_struct_can_convert (GskSlType *target,
                                GskSlType *source)
{
  return gsk_sl_type_equal (target, source);
}

static guint32
gsk_sl_type_struct_write_spv (const GskSlType *type,
                              GskSpvWriter    *writer)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;
  guint32 ids[struc->n_members + 1];
  guint i;

  ids[0] = gsk_spv_writer_next_id (writer);

  for (i = 0; i < struc->n_members; i++)
    {
      ids[i + 1] = gsk_spv_writer_get_id_for_type (writer, struc->members[i].type);
    }

  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      2 + struc->n_members, GSK_SPV_OP_TYPE_STRUCT,
                      ids);
  
  return ids[0];
}

static void
gsk_sl_type_struct_print_value (const GskSlType *type,
                                GString         *string,
                                gpointer         value)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;
  guint i;

  g_string_append (string, struc->name);
  g_string_append (string, "(");

  for (i = 0; i < struc->n_members; i++)
    {
      if (i > 0)
        g_string_append (string, ", ");
      gsk_sl_type_print_value (struc->members[i].type,
                               string,
                               (guchar *) value + struc->members[i].offset);
    }

  g_string_append (string, ")");
}

static guint32
gsk_sl_type_struct_write_value_spv (GskSlType    *type,
                                    GskSpvWriter *writer,
                                    gpointer      value)
{
  GskSlTypeStruct *struc = (GskSlTypeStruct *) type;
  guint32 ids[struc->n_members + 2];
  GskSlType *vector_type;
  GskSlValue *v;
  guchar *data;
  guint i;

  data = value;
  vector_type = gsk_sl_type_get_index_type (type);

  ids[0] = gsk_spv_writer_get_id_for_type (writer, type);
  for (i = 0; i < struc->n_members; i++)
    {
      v = gsk_sl_value_new_for_data (struc->members[i].type,
                                     (guchar *) value + struc->members[i].offset,
                                     NULL, NULL);
      ids[2 + i] = gsk_spv_writer_get_id_for_value (writer, v);
      gsk_sl_value_free (v);
      data += gsk_sl_type_get_size (vector_type);
    }

  ids[1] = gsk_spv_writer_next_id (writer);

  gsk_spv_writer_add (writer,
                      GSK_SPV_WRITER_SECTION_DECLARE,
                      3 + struc->n_members,
                      GSK_SPV_OP_CONSTANT_COMPOSITE,
                      ids);
  
  return ids[1];
}

static const GskSlTypeClass GSK_SL_TYPE_STRUCT = {
  gsk_sl_type_struct_free,
  gsk_sl_type_struct_get_name,
  gsk_sl_type_struct_get_scalar_type,
  gsk_sl_type_struct_get_index_type,
  gsk_sl_type_struct_get_index_stride,
  gsk_sl_type_struct_get_length,
  gsk_sl_type_struct_get_size,
  gsk_sl_type_struct_get_n_members,
  gsk_sl_type_struct_get_member,
  gsk_sl_type_struct_can_convert,
  gsk_sl_type_struct_write_spv,
  gsk_sl_type_struct_print_value,
  gsk_sl_type_struct_write_value_spv
};

/* API */

static GskSlType *
gsk_sl_type_parse_struct (GskSlScope        *scope,
                          GskSlPreprocessor *preproc)
{
  GskSlType *type;
  const GskSlToken *token;
  GskSlTypeBuilder *builder;
  gboolean add_type = FALSE;

  /* the struct token */
  gsk_sl_preprocessor_consume (preproc, NULL);

  token = gsk_sl_preprocessor_get (preproc);
  if (gsk_sl_token_is (token, GSK_SL_TOKEN_IDENTIFIER))
    {    
      if (gsk_sl_scope_is_global (scope))
        {
          add_type = TRUE;
          builder = gsk_sl_type_builder_new_struct (token->str);
        }
      else
        {
          builder = gsk_sl_type_builder_new_struct (NULL);
        }
      gsk_sl_preprocessor_consume (preproc, NULL);
    }
  else
    {
      builder = gsk_sl_type_builder_new_struct (NULL);
    }

  token = gsk_sl_preprocessor_get (preproc);
  if (!gsk_sl_token_is (token, GSK_SL_TOKEN_LEFT_BRACE))
    {
      gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected opening \"{\" after struct declaration.");
      goto out;
    }
  gsk_sl_preprocessor_consume (preproc, NULL);

  for (token = gsk_sl_preprocessor_get (preproc);
       !gsk_sl_token_is (token, GSK_SL_TOKEN_RIGHT_BRACE) && !gsk_sl_token_is (token, GSK_SL_TOKEN_EOF);
       token = gsk_sl_preprocessor_get (preproc))
    {
      type = gsk_sl_type_new_parse (scope, preproc);

      while (TRUE)
        {
          token = gsk_sl_preprocessor_get (preproc);
          if (!gsk_sl_token_is (token, GSK_SL_TOKEN_IDENTIFIER))
            {
              gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected identifier for type name.");
              break;
            }
          if (gsk_sl_type_builder_has_member (builder, token->str))
            gsk_sl_preprocessor_error (preproc, DECLARATION, "struct already has a member named \"%s\".", token->str);
          else
            gsk_sl_type_builder_add_member (builder, type, token->str);
          gsk_sl_preprocessor_consume (preproc, NULL);

          token = gsk_sl_preprocessor_get (preproc);
          if (!gsk_sl_token_is (token, GSK_SL_TOKEN_COMMA))
            break;

          gsk_sl_preprocessor_consume (preproc, NULL);
        }
      gsk_sl_type_unref (type);

      if (!gsk_sl_token_is (token, GSK_SL_TOKEN_SEMICOLON))
        gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected semicolon after struct member declaration.");
      else
        gsk_sl_preprocessor_consume (preproc, NULL);
    }

  if (!gsk_sl_token_is (token, GSK_SL_TOKEN_RIGHT_BRACE))
    gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected closing \"}\" after struct declaration.");
  else
    gsk_sl_preprocessor_consume (preproc, NULL);
  
out:
  type = gsk_sl_type_builder_free (builder);
  if (add_type)
    {
      if (gsk_sl_scope_lookup_type (scope, gsk_sl_type_get_name (type)))
        gsk_sl_preprocessor_error (preproc, DECLARATION, "Redefinition of struct \"%s\".", gsk_sl_type_get_name (type));
      else if (gsk_sl_scope_lookup_function (scope, gsk_sl_type_get_name (type)))
        gsk_sl_preprocessor_error (preproc, DECLARATION, "Constructor name \"%s\" would override function of same name.", gsk_sl_type_get_name (type));
      else
        gsk_sl_scope_add_type (scope, type);
    }
  return type;
}

GskSlType *
gsk_sl_type_new_parse (GskSlScope        *scope,
                       GskSlPreprocessor *preproc)
{
  GskSlType *type;
  const GskSlToken *token;

  token = gsk_sl_preprocessor_get (preproc);

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
    case GSK_SL_TOKEN_BVEC2:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_BOOL, 2));
      break;
    case GSK_SL_TOKEN_BVEC3:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_BOOL, 3));
      break;
    case GSK_SL_TOKEN_BVEC4:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_BOOL, 4));
      break;
    case GSK_SL_TOKEN_IVEC2:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_INT, 2));
      break;
    case GSK_SL_TOKEN_IVEC3:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_INT, 3));
      break;
    case GSK_SL_TOKEN_IVEC4:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_INT, 4));
      break;
    case GSK_SL_TOKEN_UVEC2:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_UINT, 2));
      break;
    case GSK_SL_TOKEN_UVEC3:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_UINT, 3));
      break;
    case GSK_SL_TOKEN_UVEC4:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_UINT, 4));
      break;
    case GSK_SL_TOKEN_VEC2:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_FLOAT, 2));
      break;
    case GSK_SL_TOKEN_VEC3:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_FLOAT, 3));
      break;
    case GSK_SL_TOKEN_VEC4:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_FLOAT, 4));
      break;
    case GSK_SL_TOKEN_DVEC2:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_DOUBLE, 2));
      break;
    case GSK_SL_TOKEN_DVEC3:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_DOUBLE, 3));
      break;
    case GSK_SL_TOKEN_DVEC4:
      type = gsk_sl_type_ref (gsk_sl_type_get_vector (GSK_SL_DOUBLE, 4));
      break;
    case GSK_SL_TOKEN_MAT2:
    case GSK_SL_TOKEN_MAT2X2:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 2, 2));
      break;
    case GSK_SL_TOKEN_MAT2X3:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 2, 3));
      break;
    case GSK_SL_TOKEN_MAT2X4:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 2, 4));
      break;
    case GSK_SL_TOKEN_MAT3X2:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 3, 2));
      break;
    case GSK_SL_TOKEN_MAT3:
    case GSK_SL_TOKEN_MAT3X3:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 3, 3));
      break;
    case GSK_SL_TOKEN_MAT3X4:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 3, 4));
      break;
    case GSK_SL_TOKEN_MAT4X2:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 4, 2));
      break;
    case GSK_SL_TOKEN_MAT4X3:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 4, 3));
      break;
    case GSK_SL_TOKEN_MAT4:
    case GSK_SL_TOKEN_MAT4X4:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_FLOAT, 4, 4));
      break;
    case GSK_SL_TOKEN_DMAT2:
    case GSK_SL_TOKEN_DMAT2X2:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 2, 2));
      break;
    case GSK_SL_TOKEN_DMAT2X3:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 2, 3));
      break;
    case GSK_SL_TOKEN_DMAT2X4:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 2, 4));
      break;
    case GSK_SL_TOKEN_DMAT3X2:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 3, 2));
      break;
    case GSK_SL_TOKEN_DMAT3:
    case GSK_SL_TOKEN_DMAT3X3:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 3, 3));
      break;
    case GSK_SL_TOKEN_DMAT3X4:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 3, 4));
      break;
    case GSK_SL_TOKEN_DMAT4X2:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 4, 2));
      break;
    case GSK_SL_TOKEN_DMAT4X3:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 4, 3));
      break;
    case GSK_SL_TOKEN_DMAT4:
    case GSK_SL_TOKEN_DMAT4X4:
      type = gsk_sl_type_ref (gsk_sl_type_get_matrix (GSK_SL_DOUBLE, 4, 4));
      break;
    case GSK_SL_TOKEN_STRUCT:
      return gsk_sl_type_parse_struct (scope, preproc);
    case GSK_SL_TOKEN_IDENTIFIER:
      {
        type = gsk_sl_scope_lookup_type (scope, token->str);

        if (type)
          {
            type = gsk_sl_type_ref (type);
            break;
          }
      }
      /* fall through */
    default:
      gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected type specifier");
      return gsk_sl_type_ref (gsk_sl_type_get_scalar (GSK_SL_FLOAT));
  }

  gsk_sl_preprocessor_consume (preproc, NULL);

  return type;
}

static GskSlTypeScalar
builtin_scalar_types[N_SCALAR_TYPES] = {
  [GSK_SL_VOID] = { { &GSK_SL_TYPE_SCALAR, 1 }, GSK_SL_VOID },
  [GSK_SL_FLOAT] = { { &GSK_SL_TYPE_SCALAR, 1 }, GSK_SL_FLOAT },
  [GSK_SL_DOUBLE] = { { &GSK_SL_TYPE_SCALAR, 1 }, GSK_SL_DOUBLE },
  [GSK_SL_INT] = { { &GSK_SL_TYPE_SCALAR, 1 }, GSK_SL_INT },
  [GSK_SL_UINT] = { { &GSK_SL_TYPE_SCALAR, 1 }, GSK_SL_UINT },
  [GSK_SL_BOOL] = { { &GSK_SL_TYPE_SCALAR, 1 }, GSK_SL_BOOL },
};

GskSlType *
gsk_sl_type_get_scalar (GskSlScalarType scalar)
{
  g_assert (scalar < N_SCALAR_TYPES);

  return &builtin_scalar_types[scalar].parent;
}

static GskSlTypeVector
builtin_vector_types[3][N_SCALAR_TYPES] = {
  {
    [GSK_SL_FLOAT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "vec2", GSK_SL_FLOAT, 2 },
    [GSK_SL_DOUBLE] = { { &GSK_SL_TYPE_VECTOR, 1 }, "dvec2", GSK_SL_DOUBLE, 2 },
    [GSK_SL_INT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "ivec2", GSK_SL_INT, 2 },
    [GSK_SL_UINT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "uvec2", GSK_SL_UINT, 2 },
    [GSK_SL_BOOL] = { { &GSK_SL_TYPE_VECTOR, 1 }, "bvec2", GSK_SL_BOOL, 2 },
  },
  {
    [GSK_SL_FLOAT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "vec3", GSK_SL_FLOAT, 3 },
    [GSK_SL_DOUBLE] = { { &GSK_SL_TYPE_VECTOR, 1 }, "dvec3", GSK_SL_DOUBLE, 3 },
    [GSK_SL_INT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "ivec3", GSK_SL_INT, 3 },
    [GSK_SL_UINT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "uvec3", GSK_SL_UINT, 3 },
    [GSK_SL_BOOL] = { { &GSK_SL_TYPE_VECTOR, 1 }, "bvec3", GSK_SL_BOOL, 3 },
  },
  {
    [GSK_SL_FLOAT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "vec4", GSK_SL_FLOAT, 4 },
    [GSK_SL_DOUBLE] = { { &GSK_SL_TYPE_VECTOR, 1 }, "dvec4", GSK_SL_DOUBLE, 4 },
    [GSK_SL_INT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "ivec4", GSK_SL_INT, 4 },
    [GSK_SL_UINT] = { { &GSK_SL_TYPE_VECTOR, 1 }, "uvec4", GSK_SL_UINT, 4 },
    [GSK_SL_BOOL] = { { &GSK_SL_TYPE_VECTOR, 1 }, "bvec4", GSK_SL_BOOL, 4 },
  }
};

GskSlType *
gsk_sl_type_get_vector (GskSlScalarType scalar,
                        guint           length)
{
  g_assert (scalar < N_SCALAR_TYPES);
  g_assert (scalar != GSK_SL_VOID);
  g_assert (length >= 2 && length <= 4);

  return &builtin_vector_types[length - 2][scalar].parent;
}

static GskSlTypeMatrix
builtin_matrix_types[3][3][2] = {
  {
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat2",    GSK_SL_FLOAT,  2, 2 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat2",   GSK_SL_DOUBLE, 2, 2 }
    },
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat2x3",  GSK_SL_FLOAT,  2, 3 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat2x3", GSK_SL_DOUBLE, 2, 3 }
    },
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat2x4",  GSK_SL_FLOAT,  2, 4 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat2x4", GSK_SL_DOUBLE, 2, 4 }
    },
  },
  {
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat3x2",  GSK_SL_FLOAT,  3, 2 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat3x2", GSK_SL_DOUBLE, 3, 2 }
    },
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat3",    GSK_SL_FLOAT,  3, 3 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat3",   GSK_SL_DOUBLE, 3, 3 }
    },
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat3x4",  GSK_SL_FLOAT,  3, 4 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat3x4", GSK_SL_DOUBLE, 3, 4 }
    },
  },
  {
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat4x2",  GSK_SL_FLOAT,  4, 2 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat4x2", GSK_SL_DOUBLE, 4, 2 }
    },
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat4x3",  GSK_SL_FLOAT,  4, 3 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat4x3", GSK_SL_DOUBLE, 4, 3 }
    },
    {
      { { &GSK_SL_TYPE_MATRIX, 1 }, "mat4",    GSK_SL_FLOAT,  4, 4 },
      { { &GSK_SL_TYPE_MATRIX, 1 }, "dmat4",   GSK_SL_DOUBLE, 4, 4 }
    },
  },
};

GskSlType *
gsk_sl_type_get_matrix (GskSlScalarType      scalar,
                        guint                columns,
                        guint                rows)
{
  g_assert (scalar == GSK_SL_FLOAT || scalar == GSK_SL_DOUBLE);
  g_assert (columns >= 2 && columns <= 4);
  g_assert (rows >= 2 && rows <= 4);

  return &builtin_matrix_types[columns - 2][rows - 2][scalar == GSK_SL_FLOAT ? 0 : 1].parent;
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

const char *
gsk_sl_type_get_name (const GskSlType *type)
{
  return type->class->get_name (type);
}

gboolean
gsk_sl_type_is_scalar (const GskSlType *type)
{
  return type->class == &GSK_SL_TYPE_SCALAR;
}

gboolean
gsk_sl_type_is_vector (const GskSlType *type)
{
  return type->class == &GSK_SL_TYPE_VECTOR;
}

gboolean
gsk_sl_type_is_matrix (const GskSlType *type)
{
  return type->class == &GSK_SL_TYPE_MATRIX;
}

gboolean
gsk_sl_type_is_struct (const GskSlType *type)
{
  return type->class == &GSK_SL_TYPE_STRUCT;
}

GskSlScalarType
gsk_sl_type_get_scalar_type (const GskSlType *type)
{
  return type->class->get_scalar_type (type);
}

GskSlType *
gsk_sl_type_get_index_type (const GskSlType *type)
{
  return type->class->get_index_type (type);
}

gsize
gsk_sl_type_get_index_stride (const GskSlType *type)
{
  return type->class->get_index_stride (type);
}

guint
gsk_sl_type_get_length (const GskSlType *type)
{
  return type->class->get_length (type);
}

gsize
gsk_sl_type_get_size (const GskSlType *type)
{
  return type->class->get_size (type);
}

guint
gsk_sl_type_get_n_members (const GskSlType *type)
{
  return type->class->get_n_members (type);
}

static const GskSlTypeMember *
gsk_sl_type_get_member (const GskSlType *type,
                        guint            n)
{
  return type->class->get_member (type, n);
}

GskSlType *
gsk_sl_type_get_member_type (const GskSlType *type,
                             guint            n)
{
  const GskSlTypeMember *member;

  member = gsk_sl_type_get_member (type, n);

  return member->type;
}

const char *
gsk_sl_type_get_member_name (const GskSlType *type,
                             guint            n)
{
  const GskSlTypeMember *member;

  member = gsk_sl_type_get_member (type, n);

  return member->name;
}

gsize
gsk_sl_type_get_member_offset (const GskSlType *type,
                               guint            n)
{
  const GskSlTypeMember *member;

  member = gsk_sl_type_get_member (type, n);

  return member->offset;
}

gboolean
gsk_sl_type_find_member (const GskSlType *type,
                         const char      *name,
                         guint           *out_index,
                         GskSlType      **out_type,
                         gsize           *out_offset)
{
  const GskSlTypeMember *member;
  guint i, n;
  
  n = gsk_sl_type_get_n_members (type);
  for (i = 0; i < n; i++)
    {
      member = gsk_sl_type_get_member (type, i);
      if (g_str_equal (member->name, name))
        {
          if (out_index)
            *out_index = i;
          if (out_type)
            *out_type = member->type;
          if (out_offset)
            *out_offset = member->offset;
          return TRUE;
        }
    }

  return FALSE;
}

gboolean
gsk_sl_scalar_type_can_convert (GskSlScalarType target,
                                GskSlScalarType source)
{
  if (target == source)
    return TRUE;

  switch (source)
  {
    case GSK_SL_INT:
      return target == GSK_SL_UINT
          || target == GSK_SL_FLOAT
          || target == GSK_SL_DOUBLE;
    case GSK_SL_UINT:
      return target == GSK_SL_FLOAT
          || target == GSK_SL_DOUBLE;
    case GSK_SL_FLOAT:
      return target == GSK_SL_DOUBLE;
    default:
      return FALSE;
  }
}

void
gsk_sl_scalar_type_convert_value (GskSlScalarType target_type,
                                  gpointer        target_value,
                                  GskSlScalarType source_type,
                                  gconstpointer   source_value)
{
  scalar_infos[source_type].convert_value[target_type] (target_value, source_value);
}

gboolean
gsk_sl_type_can_convert (const GskSlType *target,
                         const GskSlType *source)
{
  return target->class->can_convert (target, source);
}

gboolean
gsk_sl_type_equal (gconstpointer a,
                   gconstpointer b)
{
  return a == b;
}

guint
gsk_sl_type_hash (gconstpointer type)
{
  return GPOINTER_TO_UINT (type);
}

guint32
gsk_sl_type_write_spv (const GskSlType *type,
                       GskSpvWriter    *writer)
{
  return type->class->write_spv (type, writer);
}

void
gsk_sl_type_print_value (const GskSlType     *type,
                         GString             *string,
                         gpointer             value)
{
  type->class->print_value (type, string, value);
}

guint32
gsk_sl_type_write_value_spv (GskSlType       *type,
                             GskSpvWriter    *writer,
                             gpointer         value)
{
  return type->class->write_value_spv (type, writer, value);
}

struct _GskSlTypeBuilder {
  char *name;
  gsize size;
  GArray *members;
};

GskSlTypeBuilder *
gsk_sl_type_builder_new_struct (const char *name)
{
  GskSlTypeBuilder *builder;

  builder = g_slice_new0 (GskSlTypeBuilder);

  builder->name = g_strdup (name);
  builder->members = g_array_new (FALSE, FALSE, sizeof (GskSlTypeMember));

  return builder;
}

static char *
gsk_sl_type_builder_generate_name (GskSlTypeBuilder *builder)
{
  GString *string = g_string_new ("struct { ");
  guint i;

  for (i = 0; i < builder->members->len; i++)
    {
      GskSlTypeMember *m = &g_array_index (builder->members, GskSlTypeMember, i);
      g_string_append (string, gsk_sl_type_get_name (m->type));
      g_string_append (string, " ");
      g_string_append (string, m->name);
      g_string_append (string, "; ");
    }
  g_string_append (string, "}");

  return g_string_free (string, FALSE);
}

GskSlType *
gsk_sl_type_builder_free (GskSlTypeBuilder *builder)
{
  GskSlTypeStruct *result;

  result = gsk_sl_type_new (GskSlTypeStruct, &GSK_SL_TYPE_STRUCT);

  if (builder->name)
    result->name = builder->name;
  else
    result->name = gsk_sl_type_builder_generate_name (builder);
  result->size = builder->size;
  result->n_members = builder->members->len;
  result->members = (GskSlTypeMember *) g_array_free (builder->members, FALSE);

  g_slice_free (GskSlTypeBuilder, builder);

  return &result->parent;
}

void
gsk_sl_type_builder_add_member (GskSlTypeBuilder *builder,
                                GskSlType        *type,
                                const char       *name)
{
  g_array_append_vals (builder->members,
                       &(GskSlTypeMember) {
                           gsk_sl_type_ref (type),
                           g_strdup (name),
                           builder->size }, 1);
  builder->size += gsk_sl_type_get_size (type);
}

gboolean
gsk_sl_type_builder_has_member (GskSlTypeBuilder *builder,
                                const char       *name)
{
  guint i;

  for (i = 0; i < builder->members->len; i++)
    {
      if (g_str_equal (g_array_index (builder->members, GskSlTypeMember, i).name, name))
        return TRUE;
    }

  return FALSE;
}

