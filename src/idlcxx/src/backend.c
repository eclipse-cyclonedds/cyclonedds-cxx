/*
 * Copyright(c) 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "idlcxx/backend.h"

#ifdef _WIN32
#pragma warning(disable : 4996)
#endif

struct idl_backend_ctx_s
{
  idl_file_out    output;
  uint32_t        indent_level;
  uint32_t        indent_size;
  idl_ostream_t * ostream;
  const char    * target_file;
  void *          custom_context;
};

idl_backend_ctx
idl_backend_context_new(uint32_t indent_size, const char *target_file, void *custom_context)
{
  idl_backend_ctx ctx = malloc(sizeof(struct idl_backend_ctx_s));
  ctx->output = NULL;
  ctx->indent_level = 0;
  ctx->indent_size = indent_size;
  ctx->ostream = create_idl_ostream(NULL);
  ctx->target_file = target_file;
  ctx->custom_context = custom_context;
  return ctx;
}

idl_retcode_t
idl_backend_context_free(idl_backend_ctx ctx)
{
  assert(!ctx->custom_context);
  if (ctx->ostream) destruct_idl_ostream(ctx->ostream);
  if (ctx->output) idl_file_out_close(ctx);
  free(ctx);
  return IDL_RETCODE_OK;
}

idl_retcode_t
idl_file_out_new(idl_backend_ctx ctx, const char *file_name)
{
  idl_retcode_t result = IDL_RETCODE_OK;

  if (ctx->output)
  {
    result = IDL_RETCODE_CANNOT_OPEN_FILE;
  }
  else
  {
    ctx->output = malloc (sizeof(struct idl_file_out_s));
    ctx->output->file = fopen (file_name, "w");

    if (ctx->output->file == NULL)
    {
        free(ctx->output);
        ctx->output = NULL;
        result = IDL_RETCODE_CANNOT_OPEN_FILE;
    }
  }
  return result;
}

void
idl_file_out_close(idl_backend_ctx ctx)
{
  if (ctx->output)
  {
    fclose (ctx->output->file);
    free(ctx->output);
    ctx->output = NULL;
  }
}

idl_ostream_t *
idl_get_output_stream(idl_backend_ctx ctx)
{
  return ctx->ostream;
}

void
idl_indent_incr(idl_backend_ctx ctx)
{
  ++ctx->indent_level;
}

void
idl_indent_double_incr(idl_backend_ctx ctx)
{
  ctx->indent_level += 2;
}

void
idl_indent_decr(idl_backend_ctx ctx)
{
  --ctx->indent_level;
}

void
idl_indent_double_decr(idl_backend_ctx ctx)
{
  ctx->indent_level -= 2;
}

void *
idl_get_custom_context(idl_backend_ctx ctx)
{
  return ctx->custom_context;
}

void
idl_reset_custom_context(idl_backend_ctx ctx)
{
  ctx->custom_context = NULL;
}

idl_retcode_t
idl_set_custom_context(idl_backend_ctx ctx, void *custom_context)
{
  assert(!ctx->custom_context);
  ctx->custom_context = custom_context;
  return IDL_RETCODE_OK;
}

void
idl_file_out_printf (
    idl_backend_ctx ctx,
    const char *format,
    ...)
{
  va_list args;
  uint32_t indentation = ctx->indent_level * ctx->indent_size;

  assert(ctx->ostream);
  va_start (args, format);
  if (indentation) format_ostream(ctx->ostream, "%*c", indentation, ' ');
  format_ostream_va_args (ctx->ostream, format, args);
  va_end (args);
}

void
idl_file_out_printf_no_indent (
    idl_backend_ctx ctx,
    const char *format,
    ...)
{
  va_list args;

  assert(ctx->ostream);
  va_start (args, format);
  format_ostream_va_args(ctx->ostream, format, args);
  va_end (args);
}

bool
idl_is_reference(const idl_node_t *node)
{
  bool result = false;
  if (node->mask & IDL_TEMPL_TYPE)
  {
    switch(node->mask & IDL_TEMPL_TYPE_MASK)
    {
    case IDL_SEQUENCE:
    case IDL_STRING:
    case IDL_WSTRING:
      result = true;
      break;
    default:
      result = false;
      break;
    }
  }
  return result;
}

bool
idl_declarator_is_array(const idl_declarator_t *declarator)
{
  assert(declarator->node.mask & IDL_DECLARATOR);
  return (declarator->const_expr) ? true : false;
}

bool
idl_declarator_is_primitive(const idl_declarator_t *declarator)
{
  assert(declarator->node.mask & IDL_DECLARATOR);
  const idl_member_t *member = (const idl_member_t *) declarator->node.parent;
  const idl_node_t *member_type = member->type_spec;
  /* Unwind any typedefs to their original type. */
  while (member_type->mask & IDL_TYPEDEF)
  {
    const idl_typedef_t *typedef_node = (const idl_typedef_t *) member_type;
    /* If this is a typedef to an array (no matter its type), then it is not a primitive. */
    if (typedef_node->declarators->const_expr) {
      return false;
    } else {
        member_type = typedef_node->type_spec;
    }
  }
  bool is_primitive = (member_type->mask & (IDL_BASE_TYPE | IDL_ENUM));
  return (is_primitive & !idl_declarator_is_array(declarator));
}

idl_retcode_t
idl_walk_node_list(
    idl_backend_ctx ctx,
    const idl_node_t *target_node,
    idl_walkAction action,
    idl_mask_t mask)
{
  idl_retcode_t result = IDL_RETCODE_OK;

  while (target_node && result == IDL_RETCODE_OK) {
    bool matches_target_file;

    /* For top-level nodes, only walk over that match the specified file.                 */
    /* (Lower level nodes are typically contained in the same file as their parent node,  */
    /* and may contain valid references to types contained in another file.               */
    if (ctx->target_file && !target_node->parent) {
      matches_target_file = (strcmp(ctx->target_file, target_node->location.first.file) == 0);
    } else {
      matches_target_file = true;
    }

    if ((target_node->mask & mask) && matches_target_file)
    {
      result = action(ctx, target_node);
    }
    target_node = target_node->next;
  }
  return result;
}

idl_retcode_t
idl_walk_tree(
    idl_backend_ctx ctx,
    const idl_node_t *target_node,
    idl_walkAction action,
    idl_mask_t mask)
{
  idl_retcode_t result = IDL_RETCODE_OK;
  const idl_node_t *sub_nodes;

  while (target_node && result == IDL_RETCODE_OK) {
    bool matches_target_file;

    /* For top-level nodes, only walk over that match the specified file.                 */
    /* (Lower level nodes are typically contained in the same file as their parent node,  */
    /* and may contain valid references to types contained in another file.               */
    if (ctx->target_file && !target_node->parent) {
      matches_target_file = (strcmp(ctx->target_file, target_node->location.first.file) == 0);
    } else {
      matches_target_file = true;
    }

    if ((target_node->mask & mask) && matches_target_file)
    {
      result = action(ctx, target_node);
      switch (target_node->mask & IDL_CATEGORY_MASK)
      {
      case IDL_MODULE:
        sub_nodes = ((const idl_module_t *) target_node)->definitions;
        break;
      case IDL_STRUCT:
        sub_nodes = (const idl_node_t *)(((const idl_struct_t *) target_node)->members);
        break;
      case IDL_UNION:
        result = action(ctx, ((const idl_union_t *) target_node)->switch_type_spec);
        if (result == IDL_RETCODE_OK) {
          sub_nodes = (const idl_node_t *)(((const idl_union_t *) target_node)->cases);
        } else {
          return result;
        }
        break;
      case IDL_ENUM:
        sub_nodes = (const idl_node_t *)(((const idl_enum_t *) target_node)->enumerators);
        break;
      case IDL_TYPEDEF:
        sub_nodes = ((const idl_typedef_t *) target_node)->type_spec;
        break;
      case IDL_CONST:
        sub_nodes = ((const idl_const_t *) target_node)->type_spec;
        break;
      case IDL_MEMBER:
        result = action(ctx, ((const idl_member_t *) target_node)->type_spec);
        if (result == IDL_RETCODE_OK) {
          sub_nodes = (const idl_node_t *)(((const idl_member_t *) target_node)->declarators);
        } else {
          return result;
        }
        break;
      case IDL_CASE:
        result = action(ctx, ((const idl_case_t *) target_node)->type_spec);
        if (result == IDL_RETCODE_OK) {
          sub_nodes = (const idl_node_t *)(((const idl_case_t *) target_node)->declarator);
        } else {
          return result;
        }
        break;
      default:
        sub_nodes = NULL;
        break;
      }
      if (sub_nodes) {
        result = idl_walk_tree(ctx, sub_nodes, action, IDL_MASK_ALL);
      }
    }

    target_node = target_node->next;
  }
  return result;
}

static idl_retcode_t
prune_include_list(idl_backend_ctx ctx, const idl_node_t *node)
{
  idl_retcode_t result = IDL_RETCODE_OK;

  /* If you find a dependency to another file than your own, prune it form the include list. */
  if (strcmp(node->location.first.file, ctx->target_file) != 0)
  {
    idl_file_t **cur_file = (idl_file_t **) ctx->custom_context;
    while (*cur_file != NULL) {
      if (strcmp((*cur_file)->name, node->location.first.file) == 0)
      {
        idl_file_t *tmp = *cur_file;
        *cur_file = tmp->next;
        free(tmp);
        break;
      }
      cur_file = &(*cur_file)->next;
    }
  }

  return result;
}

idl_file_t *
idl_get_include_list(idl_backend_ctx ctx, const idl_tree_t *tree)
{
  const char *org_target_file = ctx->target_file;

  /* Clone the list of included files. */
  idl_file_t *include_list_head = malloc(sizeof(idl_file_t));
  idl_file_t *include_list_tail = include_list_head;
  idl_file_t **index;

  /* Make shallow copy of each file, and append to list. */
  const idl_file_t *cur_tree_file = tree->files;
  include_list_tail->name = cur_tree_file->name;
  include_list_tail->next = NULL;
  cur_tree_file = cur_tree_file->next;
  while (cur_tree_file) {
    include_list_tail->next = malloc(sizeof(idl_file_t));
    include_list_tail = include_list_tail->next;
    include_list_tail->name = cur_tree_file->name;
    include_list_tail->next = NULL;
    cur_tree_file = cur_tree_file->next;
  }

  /* First prune the target file. */
  index = &include_list_head->next;
  ctx->custom_context = index;
  while (*index) {
    ctx->target_file = (*index)->name;
    idl_walk_tree(ctx, tree->root, prune_include_list, IDL_MASK_ALL);
    index = &(*index)->next;
  }

  ctx->target_file = org_target_file;
  ctx->custom_context = NULL;

  /* Remove the first element (current file) from the include list. */
  include_list_tail = include_list_head->next;
  free(include_list_head);

  return include_list_tail;
}

