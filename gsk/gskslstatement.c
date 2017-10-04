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

#include "gskslstatementprivate.h"

#include "gskslexpressionprivate.h"
#include "gskslfunctionprivate.h"
#include "gskslpointertypeprivate.h"
#include "gskslpreprocessorprivate.h"
#include "gskslprinterprivate.h"
#include "gskslscopeprivate.h"
#include "gsksltokenizerprivate.h"
#include "gsksltypeprivate.h"
#include "gskslvalueprivate.h"
#include "gskslvariableprivate.h"
#include "gskspvwriterprivate.h"

#include <string.h>

typedef struct _GskSlStatementClass GskSlStatementClass;

struct _GskSlStatement {
  const GskSlStatementClass *class;
  guint ref_count;
};

struct _GskSlStatementClass {
  void                  (* free)                                (GskSlStatement         *statement);

  void                  (* print)                               (const GskSlStatement   *statement,
                                                                 GskSlPrinter           *printer);
  void                  (* write_spv)                           (const GskSlStatement   *statement,
                                                                 GskSpvWriter           *writer);
};

static GskSlStatement *
gsk_sl_statement_alloc (const GskSlStatementClass *klass,
                        gsize                      size)
{
  GskSlStatement *statement;

  statement = g_slice_alloc0 (size);

  statement->class = klass;
  statement->ref_count = 1;

  return statement;
}
#define gsk_sl_statement_new(_name, _klass) ((_name *) gsk_sl_statement_alloc ((_klass), sizeof (_name)))

/* EMPTY */

typedef struct _GskSlStatementEmpty GskSlStatementEmpty;

struct _GskSlStatementEmpty {
  GskSlStatement parent;
};

static void
gsk_sl_statement_empty_free (GskSlStatement *statement)
{
  GskSlStatementEmpty *empty = (GskSlStatementEmpty *) statement;

  g_slice_free (GskSlStatementEmpty, empty);
}

static void
gsk_sl_statement_empty_print (const GskSlStatement *statement,
                              GskSlPrinter         *printer)
{
  gsk_sl_printer_append (printer, ";");
}

static void
gsk_sl_statement_empty_write_spv (const GskSlStatement *statement,
                                  GskSpvWriter         *writer)
{
}

static const GskSlStatementClass GSK_SL_STATEMENT_EMPTY = {
  gsk_sl_statement_empty_free,
  gsk_sl_statement_empty_print,
  gsk_sl_statement_empty_write_spv
};

/* COMPOUND */

typedef struct _GskSlStatementCompound GskSlStatementCompound;

struct _GskSlStatementCompound {
  GskSlStatement parent;

  GskSlScope *scope;
  GSList *statements;
};

static void
gsk_sl_statement_compound_free (GskSlStatement *statement)
{
  GskSlStatementCompound *compound = (GskSlStatementCompound *) statement;

  g_slist_free_full (compound->statements, (GDestroyNotify) gsk_sl_statement_unref);
  if (compound->scope)
    gsk_sl_scope_unref (compound->scope);

  g_slice_free (GskSlStatementCompound, compound);
}

static void
gsk_sl_statement_compound_print (const GskSlStatement *statement,
                                 GskSlPrinter         *printer)
{
  GskSlStatementCompound *compound = (GskSlStatementCompound *) statement;
  GSList *l;

  gsk_sl_printer_append (printer, "{");
  gsk_sl_printer_push_indentation (printer);
  for (l = compound->statements; l; l = l->next)
    {
      gsk_sl_printer_newline (printer);
      gsk_sl_statement_print (l->data, printer);
    }
  gsk_sl_printer_pop_indentation (printer);
  gsk_sl_printer_newline (printer);
  gsk_sl_printer_append (printer, "}");
}

static void
gsk_sl_statement_compound_write_spv (const GskSlStatement *statement,
                                     GskSpvWriter         *writer)
{
  GskSlStatementCompound *compound = (GskSlStatementCompound *) statement;
  GSList *l;

  for (l = compound->statements; l; l = l->next)
    {
      gsk_sl_statement_write_spv (l->data, writer);
    }
}

static const GskSlStatementClass GSK_SL_STATEMENT_COMPOUND = {
  gsk_sl_statement_compound_free,
  gsk_sl_statement_compound_print,
  gsk_sl_statement_compound_write_spv
};

/* DECLARATION */

typedef struct _GskSlStatementDeclaration GskSlStatementDeclaration;

struct _GskSlStatementDeclaration {
  GskSlStatement parent;

  GskSlVariable *variable;
  GskSlExpression *initial;
};

static void
gsk_sl_statement_declaration_free (GskSlStatement *statement)
{
  GskSlStatementDeclaration *declaration = (GskSlStatementDeclaration *) statement;

  gsk_sl_variable_unref (declaration->variable);
  if (declaration->initial)
    gsk_sl_expression_unref (declaration->initial);

  g_slice_free (GskSlStatementDeclaration, declaration);
}

static void
gsk_sl_statement_declaration_print (const GskSlStatement *statement,
                                    GskSlPrinter         *printer)
{
  GskSlStatementDeclaration *declaration = (GskSlStatementDeclaration *) statement;

  gsk_sl_variable_print (declaration->variable, printer);
  if (declaration->initial)
    {
      gsk_sl_printer_append (printer, " = ");
      gsk_sl_expression_print (declaration->initial, printer);
    }
  gsk_sl_printer_append (printer, ";");
}

static void
gsk_sl_statement_declaration_write_spv (const GskSlStatement *statement,
                                        GskSpvWriter         *writer)
{
  GskSlStatementDeclaration *declaration = (GskSlStatementDeclaration *) statement;
  guint32 variable_id;

  variable_id = gsk_spv_writer_get_id_for_variable (writer, declaration->variable);
  
  if (declaration->initial && ! gsk_sl_variable_get_initial_value (declaration->variable))
    {
      gsk_spv_writer_add (writer,
                          GSK_SPV_WRITER_SECTION_CODE,
                          3, GSK_SPV_OP_STORE,
                          (guint32[2]) { variable_id,
                                         gsk_sl_expression_write_spv (declaration->initial, writer)});
    }
}

static const GskSlStatementClass GSK_SL_STATEMENT_DECLARATION = {
  gsk_sl_statement_declaration_free,
  gsk_sl_statement_declaration_print,
  gsk_sl_statement_declaration_write_spv
};

/* RETURN */

typedef struct _GskSlStatementReturn GskSlStatementReturn;

struct _GskSlStatementReturn {
  GskSlStatement parent;

  GskSlExpression *value;
};

static void
gsk_sl_statement_return_free (GskSlStatement *statement)
{
  GskSlStatementReturn *return_statement = (GskSlStatementReturn *) statement;

  if (return_statement->value)
    gsk_sl_expression_unref (return_statement->value);

  g_slice_free (GskSlStatementReturn, return_statement);
}

static void
gsk_sl_statement_return_print (const GskSlStatement *statement,
                               GskSlPrinter         *printer)
{
  GskSlStatementReturn *return_statement = (GskSlStatementReturn *) statement;

  gsk_sl_printer_append (printer, "return");
  if (return_statement->value)
    {
      gsk_sl_printer_append (printer, " ");
      gsk_sl_expression_print (return_statement->value, printer);
    }
  gsk_sl_printer_append (printer, ";");
}

static void
gsk_sl_statement_return_write_spv (const GskSlStatement *statement,
                                   GskSpvWriter         *writer)
{
  g_assert_not_reached ();
}

static const GskSlStatementClass GSK_SL_STATEMENT_RETURN = {
  gsk_sl_statement_return_free,
  gsk_sl_statement_return_print,
  gsk_sl_statement_return_write_spv
};

/* EXPRESSION */
 
typedef struct _GskSlStatementExpression GskSlStatementExpression;

struct _GskSlStatementExpression {
  GskSlStatement parent;

  GskSlExpression *expression;
};

static void
gsk_sl_statement_expression_free (GskSlStatement *statement)
{
  GskSlStatementExpression *expression_statement = (GskSlStatementExpression *) statement;
 
  gsk_sl_expression_unref (expression_statement->expression);
 
  g_slice_free (GskSlStatementExpression, expression_statement);
}
 
static void
gsk_sl_statement_expression_print (const GskSlStatement *statement,
                                   GskSlPrinter         *printer)
{
  GskSlStatementExpression *expression_statement = (GskSlStatementExpression *) statement;

  gsk_sl_expression_print (expression_statement->expression, printer);
  gsk_sl_printer_append (printer, ";");
}
 
static void
gsk_sl_statement_expression_write_spv (const GskSlStatement *statement,
                                       GskSpvWriter         *writer)
{
  GskSlStatementExpression *expression_statement = (GskSlStatementExpression *) statement;

  gsk_sl_expression_write_spv (expression_statement->expression, writer);
}
 
static const GskSlStatementClass GSK_SL_STATEMENT_EXPRESSION = {
  gsk_sl_statement_expression_free,
  gsk_sl_statement_expression_print,
  gsk_sl_statement_expression_write_spv
};

/* API */

static GskSlStatement *
gsk_sl_statement_parse_declaration (GskSlScope        *scope,
                                    GskSlPreprocessor *stream,
                                    GskSlDecorations  *decoration,
                                    GskSlType         *type)
{
  GskSlStatementDeclaration *declaration;
  GskSlPointerType *pointer_type;
  GskSlValue *value = NULL;
  const GskSlToken *token;
  char *name;

  declaration = gsk_sl_statement_new (GskSlStatementDeclaration, &GSK_SL_STATEMENT_DECLARATION);
  
  token = gsk_sl_preprocessor_get (stream);
  if (gsk_sl_token_is (token, GSK_SL_TOKEN_IDENTIFIER))
    {
      name = g_strdup (token->str);
      gsk_sl_preprocessor_consume (stream, (GskSlStatement *) declaration);

      token = gsk_sl_preprocessor_get (stream);
      if (gsk_sl_token_is (token, GSK_SL_TOKEN_EQUAL))
        {
          GskSlValue *unconverted;

          gsk_sl_preprocessor_consume (stream, (GskSlStatement *) declaration);
          declaration->initial = gsk_sl_expression_parse_assignment (scope, stream);
          if (!gsk_sl_type_can_convert (type, gsk_sl_expression_get_return_type (declaration->initial)))
            {
              gsk_sl_preprocessor_error (stream, TYPE_MISMATCH,
                                         "Cannot convert from initializer type %s to variable type %s",
                                         gsk_sl_type_get_name (gsk_sl_expression_get_return_type (declaration->initial)),
                                         gsk_sl_type_get_name (type));
              gsk_sl_expression_unref (declaration->initial);
              declaration->initial = NULL;
            }
          else
            {
              unconverted = gsk_sl_expression_get_constant (declaration->initial);
              if (unconverted)
                {
                  value = gsk_sl_value_new_convert (unconverted, type);
                  gsk_sl_value_free (unconverted);
                }
            }
        }
    }
  else
    {
      name = NULL;
      value = NULL;
    }

  pointer_type = gsk_sl_pointer_type_new (type, TRUE, decoration->values[GSK_SL_DECORATION_CALLER_ACCESS].value);
  declaration->variable = gsk_sl_variable_new (pointer_type, name, value, decoration->values[GSK_SL_DECORATION_CONST].set);
  gsk_sl_pointer_type_unref (pointer_type);
  gsk_sl_scope_add_variable (scope, declaration->variable);

  return (GskSlStatement *) declaration;
}

GskSlStatement *
gsk_sl_statement_parse_compound (GskSlScope        *scope,
                                 GskSlPreprocessor *preproc,
                                 gboolean           new_scope)
{
  GskSlStatementCompound *compound;
  const GskSlToken *token;

  compound = gsk_sl_statement_new (GskSlStatementCompound, &GSK_SL_STATEMENT_COMPOUND);
  if (new_scope)
    {
      compound->scope = gsk_sl_scope_new (scope, gsk_sl_scope_get_return_type (scope));
      scope = compound->scope;
    }

  token = gsk_sl_preprocessor_get (preproc);
  if (!gsk_sl_token_is (token, GSK_SL_TOKEN_LEFT_BRACE))
    {
      gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected an opening \"{\"");
      return (GskSlStatement *) compound;
    }
  gsk_sl_preprocessor_consume (preproc, compound);

  for (token = gsk_sl_preprocessor_get (preproc);
       !gsk_sl_token_is (token, GSK_SL_TOKEN_RIGHT_BRACE) && !gsk_sl_token_is (token, GSK_SL_TOKEN_EOF);
       token = gsk_sl_preprocessor_get (preproc))
    {
      GskSlStatement *statement;

      statement = gsk_sl_statement_parse (scope, preproc);
      compound->statements = g_slist_prepend (compound->statements, statement);
    }
  compound->statements = g_slist_reverse (compound->statements);

  if (!gsk_sl_token_is (token, GSK_SL_TOKEN_RIGHT_BRACE))
    {
      gsk_sl_preprocessor_error (preproc, SYNTAX, "Expected closing \"}\" at end of block.");
      gsk_sl_preprocessor_sync (preproc, GSK_SL_TOKEN_RIGHT_BRACE);
    }
  gsk_sl_preprocessor_consume (preproc, compound);
  
  return (GskSlStatement *) compound;
}

GskSlStatement *
gsk_sl_statement_parse (GskSlScope        *scope,
                        GskSlPreprocessor *preproc)
{
  const GskSlToken *token;
  GskSlStatement *statement;

  token = gsk_sl_preprocessor_get (preproc);

  switch (token->type)
  {
    case GSK_SL_TOKEN_SEMICOLON:
      statement = (GskSlStatement *) gsk_sl_statement_new (GskSlStatementEmpty, &GSK_SL_STATEMENT_EMPTY);
      break;

    case GSK_SL_TOKEN_EOF:
      gsk_sl_preprocessor_error (preproc, SYNTAX, "Unexpected end of document");
      return (GskSlStatement *) gsk_sl_statement_new (GskSlStatementEmpty, &GSK_SL_STATEMENT_EMPTY);

    case GSK_SL_TOKEN_LEFT_BRACE:
      return gsk_sl_statement_parse_compound (scope, preproc, TRUE);

    case GSK_SL_TOKEN_CONST:
    case GSK_SL_TOKEN_IN:
    case GSK_SL_TOKEN_OUT:
    case GSK_SL_TOKEN_INOUT:
    case GSK_SL_TOKEN_INVARIANT:
    case GSK_SL_TOKEN_COHERENT:
    case GSK_SL_TOKEN_VOLATILE:
    case GSK_SL_TOKEN_RESTRICT:
    case GSK_SL_TOKEN_READONLY:
    case GSK_SL_TOKEN_WRITEONLY:
    case GSK_SL_TOKEN_VOID:
    case GSK_SL_TOKEN_FLOAT:
    case GSK_SL_TOKEN_DOUBLE:
    case GSK_SL_TOKEN_INT:
    case GSK_SL_TOKEN_UINT:
    case GSK_SL_TOKEN_BOOL:
    case GSK_SL_TOKEN_BVEC2:
    case GSK_SL_TOKEN_BVEC3:
    case GSK_SL_TOKEN_BVEC4:
    case GSK_SL_TOKEN_IVEC2:
    case GSK_SL_TOKEN_IVEC3:
    case GSK_SL_TOKEN_IVEC4:
    case GSK_SL_TOKEN_UVEC2:
    case GSK_SL_TOKEN_UVEC3:
    case GSK_SL_TOKEN_UVEC4:
    case GSK_SL_TOKEN_VEC2:
    case GSK_SL_TOKEN_VEC3:
    case GSK_SL_TOKEN_VEC4:
    case GSK_SL_TOKEN_DVEC2:
    case GSK_SL_TOKEN_DVEC3:
    case GSK_SL_TOKEN_DVEC4:
    case GSK_SL_TOKEN_MAT2:
    case GSK_SL_TOKEN_MAT3:
    case GSK_SL_TOKEN_MAT4:
    case GSK_SL_TOKEN_DMAT2:
    case GSK_SL_TOKEN_DMAT3:
    case GSK_SL_TOKEN_DMAT4:
    case GSK_SL_TOKEN_MAT2X2:
    case GSK_SL_TOKEN_MAT2X3:
    case GSK_SL_TOKEN_MAT2X4:
    case GSK_SL_TOKEN_MAT3X2:
    case GSK_SL_TOKEN_MAT3X3:
    case GSK_SL_TOKEN_MAT3X4:
    case GSK_SL_TOKEN_MAT4X2:
    case GSK_SL_TOKEN_MAT4X3:
    case GSK_SL_TOKEN_MAT4X4:
    case GSK_SL_TOKEN_DMAT2X2:
    case GSK_SL_TOKEN_DMAT2X3:
    case GSK_SL_TOKEN_DMAT2X4:
    case GSK_SL_TOKEN_DMAT3X2:
    case GSK_SL_TOKEN_DMAT3X3:
    case GSK_SL_TOKEN_DMAT3X4:
    case GSK_SL_TOKEN_DMAT4X2:
    case GSK_SL_TOKEN_DMAT4X3:
    case GSK_SL_TOKEN_DMAT4X4:
    case GSK_SL_TOKEN_STRUCT:
      {
        GskSlType *type;
        GskSlDecorations decoration;

its_a_type:
        gsk_sl_decoration_list_parse (scope,
                                      preproc,
                                      &decoration);

        type = gsk_sl_type_new_parse (scope, preproc);

        token = gsk_sl_preprocessor_get (preproc);

        if (token->type == GSK_SL_TOKEN_LEFT_PAREN)
          {
            GskSlStatementExpression *statement_expression;
            GskSlFunction *constructor;
                
            constructor = gsk_sl_function_new_constructor (type);
            statement_expression = gsk_sl_statement_new (GskSlStatementExpression, &GSK_SL_STATEMENT_EXPRESSION);
            if (gsk_sl_function_is_builtin_constructor (constructor))
              {
                statement_expression->expression = gsk_sl_expression_parse_function_call (scope, preproc, NULL, constructor);
              }
            else
              {
                GskSlFunctionMatcher matcher;
                gsk_sl_function_matcher_init (&matcher, g_list_prepend (NULL, constructor));
                statement_expression->expression = gsk_sl_expression_parse_function_call (scope, preproc, &matcher, constructor);
                gsk_sl_function_matcher_finish (&matcher);
              }
            statement = (GskSlStatement *) statement_expression;
            gsk_sl_function_unref (constructor);
          }
        else
          {
            statement = gsk_sl_statement_parse_declaration (scope, preproc, &decoration, type);
          }

        gsk_sl_type_unref (type);
      }
      break;

    case GSK_SL_TOKEN_RETURN:
      {
        GskSlStatementReturn *return_statement;
        GskSlType *return_type;

        return_statement = gsk_sl_statement_new (GskSlStatementReturn, &GSK_SL_STATEMENT_RETURN);
        gsk_sl_preprocessor_consume (preproc, (GskSlStatement *) return_statement);
        token = gsk_sl_preprocessor_get (preproc);
        if (!gsk_sl_token_is (token, GSK_SL_TOKEN_SEMICOLON))
          return_statement->value = gsk_sl_expression_parse (scope, preproc);

        return_type = gsk_sl_scope_get_return_type (scope);
        statement = (GskSlStatement *) return_statement;

        if (return_type == NULL)
          {
            gsk_sl_preprocessor_error (preproc, SCOPE, "Cannot return from here.");
          }
        else if (return_statement->value == NULL)
          {
            if (!gsk_sl_type_equal (return_type, gsk_sl_type_get_scalar (GSK_SL_VOID)))
              {
                gsk_sl_preprocessor_error (preproc, TYPE_MISMATCH,"Functions expectes a return value of type %s", gsk_sl_type_get_name (return_type));
              }
          }
        else
          {
            if (gsk_sl_type_equal (return_type, gsk_sl_type_get_scalar (GSK_SL_VOID)))
              {
                gsk_sl_preprocessor_error (preproc, TYPE_MISMATCH, "Cannot return a value from a void function.");
              }
            else if (!gsk_sl_type_can_convert (return_type, gsk_sl_expression_get_return_type (return_statement->value)))
              {
                gsk_sl_preprocessor_error (preproc, TYPE_MISMATCH,
                                           "Cannot convert type %s to return type %s.",
                                           gsk_sl_type_get_name (gsk_sl_expression_get_return_type (return_statement->value)),
                                           gsk_sl_type_get_name (return_type));
                break;
              }
            }
        }
      break;

    case GSK_SL_TOKEN_IDENTIFIER:
      if (gsk_sl_scope_lookup_type (scope, token->str))
        goto its_a_type;
      /* else fall through*/

    default:
      {
        GskSlStatementExpression *statement_expression;

        statement_expression = gsk_sl_statement_new (GskSlStatementExpression, &GSK_SL_STATEMENT_EXPRESSION);
        statement_expression->expression = gsk_sl_expression_parse (scope, preproc);

        statement = (GskSlStatement *) statement_expression;
      }
      break;
  }

  token = gsk_sl_preprocessor_get (preproc);
  if (!gsk_sl_token_is (token, GSK_SL_TOKEN_SEMICOLON))
    {
      gsk_sl_preprocessor_error (preproc, SYNTAX, "No semicolon at end of statement.");
      gsk_sl_preprocessor_sync (preproc, GSK_SL_TOKEN_SEMICOLON);
    }
  gsk_sl_preprocessor_consume (preproc, (GskSlStatement *) statement);

  return statement;
}

GskSlStatement *
gsk_sl_statement_ref (GskSlStatement *statement)
{
  g_return_val_if_fail (statement != NULL, NULL);

  statement->ref_count += 1;

  return statement;
}

void
gsk_sl_statement_unref (GskSlStatement *statement)
{
  if (statement == NULL)
    return;

  statement->ref_count -= 1;
  if (statement->ref_count > 0)
    return;

  statement->class->free (statement);
}

void
gsk_sl_statement_print (const GskSlStatement *statement,
                        GskSlPrinter         *printer)
{
  statement->class->print (statement, printer);
}

void
gsk_sl_statement_write_spv (const GskSlStatement *statement,
                            GskSpvWriter         *writer)
{
  statement->class->write_spv (statement, writer);
}

