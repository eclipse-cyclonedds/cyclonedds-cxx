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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include "idlcxx/backendCpp11Utils.h"
#include "idlcxx/backendCpp11Type.h"
#include "idl/string.h"

typedef struct cpp11_member_state_s
{
  const idl_node_t *member_type_node;
  const idl_declarator_t *declarator_node;
  char *name;
  char *type_name;
} cpp11_member_state;

typedef struct cpp11_struct_context_s
{
  cpp11_member_state *members;
  uint32_t member_count;
  char *name;
  char *base_type;
} cpp11_struct_context;

static idl_retcode_t
cpp11_scope_walk(idl_backend_ctx ctx, const idl_node_t *node);

static idl_retcode_t
count_declarators(idl_backend_ctx ctx, const idl_node_t *node)
{
  uint32_t *nr_members = (uint32_t *) idl_get_custom_context(ctx);
  (void)node;
  ++(*nr_members);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
count_members(idl_backend_ctx ctx, const idl_node_t *node)
{
  const idl_member_t *member = (const idl_member_t *) node;
  const idl_node_t *declarators = (const idl_node_t *) member->declarators;
  return idl_walk_node_list(ctx, declarators, count_declarators, IDL_DECLARATOR);
}

static char *
get_cpp11_declarator_array_expr(idl_backend_ctx ctx, const idl_node_t *node, const char *member_type)
{
  idl_node_t *next_const_expr = node->next;
  char *element_expr, *const_expr, *array_expr = NULL;

  if (next_const_expr) {
    element_expr = get_cpp11_declarator_array_expr(ctx, next_const_expr, member_type);
  } else {
    element_expr = idl_strdup(member_type);
  }
  const_expr = get_cpp11_const_value((const idl_constval_t *)node);
  idl_asprintf(&array_expr, CPP11_ARRAY_TEMPLATE(element_expr, const_expr));
  free(const_expr);
  free(element_expr);
  return array_expr;
}

static idl_retcode_t
get_cpp11_declarator_data(idl_backend_ctx ctx, const idl_node_t *node)
{
  cpp11_struct_context *struct_ctx = (cpp11_struct_context *) idl_get_custom_context(ctx);
  cpp11_member_state *member_data = &struct_ctx->members[struct_ctx->member_count];
  const idl_declarator_t *declarator = (const idl_declarator_t *) node;

  member_data->member_type_node = ((const idl_member_t *) node->parent)->type_spec;
  member_data->declarator_node = declarator;
  member_data->name = get_cpp11_name(idl_identifier(declarator));
  member_data->type_name = get_cpp11_type(member_data->member_type_node);
  /* Check if the declarator contains also an array expression... */
  if (idl_declarator_is_array(declarator))
  {
    char *array_expr = get_cpp11_declarator_array_expr(ctx, declarator->const_expr, member_data->type_name);
    free(member_data->type_name);
    member_data->type_name = array_expr;
  }
  ++(struct_ctx->member_count);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
get_cpp11_member_data(idl_backend_ctx ctx, const idl_node_t *node)
{
  const idl_member_t *member = (const idl_member_t *) node;
  const idl_node_t *declarators = (const idl_node_t *) member->declarators;
  return idl_walk_node_list(ctx, declarators, get_cpp11_declarator_data, IDL_DECLARATOR);
}

static void
struct_generate_attributes(idl_backend_ctx ctx)
{
  cpp11_struct_context *struct_ctx = (cpp11_struct_context *) idl_get_custom_context(ctx);

  idl_file_out_printf(ctx, "private:\n");
  /* Declare all the member attributes. */
  idl_indent_incr(ctx);
  for (uint32_t i = 0; i < struct_ctx->member_count; ++i)
  {
    idl_file_out_printf(ctx, "%s %s_;\n",
        struct_ctx->members[i].type_name,
        struct_ctx->members[i].name);
  }
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "\n");
}

static void
struct_generate_constructors_and_operators(idl_backend_ctx ctx)
{
  cpp11_struct_context *struct_ctx = (cpp11_struct_context *) idl_get_custom_context(ctx);
  bool def_value_present = false;

  /* Start building default (empty) constructor. */
  idl_file_out_printf(ctx, "public:\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "%s()", struct_ctx->name);

  /* Make double indent for member initialization list */
  idl_indent_double_incr(ctx);
  for (uint32_t i = 0; i < struct_ctx->member_count; ++i)
  {
    const idl_node_t *member_type_node = struct_ctx->members[i].member_type_node;
    const idl_declarator_t *declarator_node = struct_ctx->members[i].declarator_node;
    char *defValue = get_default_value(ctx, member_type_node);
    if (defValue && !idl_declarator_is_array((const idl_declarator_t *) declarator_node)) {
      if (!def_value_present)
      {
        idl_file_out_printf_no_indent(ctx, " :\n");
        def_value_present = true;
      }
      else
      {
        idl_file_out_printf_no_indent(ctx, ",\n");
      }
      idl_file_out_printf(ctx, "%s_(%s)", struct_ctx->members[i].name, defValue);
    }
    if (defValue)
      free(defValue);
  }
  idl_file_out_printf_no_indent(ctx, " {}\n\n");
  idl_indent_double_decr(ctx);

  /* Check if the struct has members. A struct may extend from another but have no members of its own. */
  if (struct_ctx->member_count > 0)
  {
    /* Start building constructor that inits all parameters explicitly. */
    idl_file_out_printf(ctx, "explicit %s(\n", struct_ctx->name);
    idl_indent_double_incr(ctx);
    for (uint32_t i = 0; i < struct_ctx->member_count; ++i)
    {
      bool is_primitive = idl_declarator_is_primitive(struct_ctx->members[i].declarator_node);
      idl_file_out_printf(ctx, "%s%s%s %s%s",
          is_primitive ? "" : "const ",
          struct_ctx->members[i].type_name,
          is_primitive ? "" : "&",
          struct_ctx->members[i].name,
          (i == (struct_ctx->member_count - 1) ? ") :\n" : ",\n"));
    }
    idl_indent_double_incr(ctx);
    for (uint32_t i = 0; i < struct_ctx->member_count; ++i)
    {
      idl_file_out_printf(ctx, "%s_(%s)%s",
          struct_ctx->members[i].name,
          struct_ctx->members[i].name,
          (i == (struct_ctx->member_count - 1) ? " {}\n\n" : ",\n"));
    }
    idl_indent_double_decr(ctx);
    idl_indent_double_decr(ctx);
  }
  idl_indent_decr(ctx);
}

static void
struct_generate_getters_setters(idl_backend_ctx ctx)
{
  cpp11_struct_context *struct_ctx = (cpp11_struct_context *) idl_get_custom_context(ctx);

  /* Start building the getter/setter methods for each attribute. */
  idl_indent_incr(ctx);
  for (uint32_t i = 0; i < struct_ctx->member_count; ++i)
  {
    const cpp11_member_state *member = &struct_ctx->members[i];
    bool is_primitive = idl_declarator_is_primitive(member->declarator_node);
    idl_file_out_printf(ctx, "%s%s%s %s() const { return this->%s_; }\n",
        is_primitive ? "" : "const ",
        member->type_name,
        is_primitive ? "" : "&",
        member->name,
        member->name);
    idl_file_out_printf(ctx, "%s& %s() { return this->%s_; }\n",
        member->type_name,
        member->name,
        member->name);
    idl_file_out_printf(ctx, "void %s(%s%s%s _val_) { this->%s_ = _val_; }\n",
        member->name,
        is_primitive ? "": "const ",
        member->type_name,
        is_primitive ? "" : "&",
        member->name);
    if (!is_primitive) {
      idl_file_out_printf(ctx, "void %s(%s&& _val_) { this->%s_ = _val_; }\n",
          member->name,
          member->type_name,
          member->name);
    }
  }
  idl_indent_decr(ctx);
  idl_file_out_printf_no_indent(ctx, "\n");
}

static idl_retcode_t
generate_streamer_interfaces(idl_backend_ctx ctx)
{
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "size_t write_struct(void* data, size_t position) const;\n");
  idl_file_out_printf(ctx, "size_t write_size(size_t offset) const;\n");
  idl_file_out_printf(ctx, "size_t read_struct(const void* data, size_t position);\n");
  idl_file_out_printf(ctx, "size_t key_size(size_t position) const;\n");
  idl_file_out_printf(ctx, "size_t key_max_size(size_t position) const;\n");
  idl_file_out_printf(ctx, "size_t key_write(void* data, size_t position) const;\n");
  idl_file_out_printf(ctx, "size_t key_read(const void* data, size_t position);\n");
  idl_file_out_printf(ctx, "bool key(ddsi_keyhash_t& hash) const;\n");
  idl_indent_decr(ctx);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
struct_generate_body(idl_backend_ctx ctx, const idl_struct_t *struct_node)
{
  idl_retcode_t result = IDL_RETCODE_OK;

  uint32_t nr_members = 0;
  cpp11_struct_context struct_ctx;
  const idl_node_t *members = (const idl_node_t *) struct_node->members;
  result = idl_set_custom_context(ctx, &nr_members);
  if (result) return result;
  idl_walk_node_list(ctx, members, count_members, IDL_MEMBER);
  idl_reset_custom_context(ctx);

  struct_ctx.members = malloc(sizeof(cpp11_member_state) * nr_members);
  struct_ctx.member_count = 0;
  struct_ctx.name = get_cpp11_name(idl_identifier(struct_node));
  if (struct_node->base_type)
  {
    const idl_node_t *base_node = (const idl_node_t *) struct_node->base_type;
    struct_ctx.base_type = get_cpp11_fully_scoped_name(base_node);
  }
  else
  {
    struct_ctx.base_type = NULL;
  }
  result = idl_set_custom_context(ctx, &struct_ctx);
  if (result) return result;
  idl_walk_node_list(ctx, members, get_cpp11_member_data, IDL_MEMBER);

  /* Create class declaration. */
  if (struct_ctx.base_type) {
    idl_file_out_printf(ctx, "class %s : %s\n", struct_ctx.name, struct_ctx.base_type);
  } else {
    idl_file_out_printf(ctx, "class %s\n", struct_ctx.name);
  }
  idl_file_out_printf(ctx, "{\n");

  /* Create (private) struct attributes. */
  struct_generate_attributes(ctx);

  /* Create constructors and operators. */
  struct_generate_constructors_and_operators(ctx);

  /* Create the getters and setters. */
  struct_generate_getters_setters(ctx);

  /* Generate the streamer interfaces. */
  generate_streamer_interfaces(ctx);

  idl_file_out_printf(ctx, "};\n\n");

  idl_reset_custom_context(ctx);
  for (uint32_t i = 0; i < nr_members; ++i)
  {
    free(struct_ctx.members[i].name);
    free(struct_ctx.members[i].type_name);
  }
  free(struct_ctx.members);
  if (struct_ctx.base_type) free(struct_ctx.base_type);
  free(struct_ctx.name);

  return result;
}

typedef struct cpp11_case_state_s
{
  const idl_node_t *typespec_node;
  const idl_declarator_t *declarator_node;
  char *name;
  char *type_name;
  uint32_t label_count;
  char **labels;
} cpp11_case_state;

typedef struct cpp11_union_context_s
{
  cpp11_case_state *cases;
  uint32_t case_count;
  uint64_t total_label_count, nr_unused_discr_values;
  const idl_node_t *discr_node;
  char *discr_type;
  const char *name;
  const cpp11_case_state *default_case;
  char *default_label;
  bool has_impl_default;
} cpp11_union_context;

static idl_retcode_t
count_labels(idl_backend_ctx ctx, const idl_node_t *node)
{
  uint32_t *nr_labels = (uint32_t *) idl_get_custom_context(ctx);
  (void)node;
  ++(*nr_labels);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
count_cases(idl_backend_ctx ctx, const idl_node_t *node)
{
  uint32_t *nr_cases = (uint32_t *) idl_get_custom_context(ctx);
  (void)node;
  ++(*nr_cases);
  return IDL_RETCODE_OK;
}

static char *
get_label_value(const idl_case_label_t *label)
{
  char *label_value;

  if (label->const_expr->mask & IDL_ENUMERATOR) {
    label_value = get_cpp11_fully_scoped_name(label->const_expr);
  } else {
    label_value = get_cpp11_const_value((const idl_constval_t *) label->const_expr);
  }
  return label_value;
}

static idl_retcode_t
get_cpp11_labels(idl_backend_ctx ctx, const idl_node_t *node)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);
  const idl_case_label_t *label = (const idl_case_label_t *) node;
  cpp11_case_state *case_data = &union_ctx->cases[union_ctx->case_count];
  /* Check if there is a label: if not it represents the default case. */
  if (label->const_expr) {
    case_data->labels[case_data->label_count] = get_label_value(label);
  } else {
    /* Assert that there can only be one default case */
    assert(union_ctx->default_case == NULL);
    union_ctx->default_case = case_data;
    case_data->labels[case_data->label_count] = NULL;
  }
  ++(case_data->label_count);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
get_cpp11_case_data(idl_backend_ctx ctx, const idl_node_t *node)
{
  idl_retcode_t result;
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);
  cpp11_case_state *case_data = &union_ctx->cases[union_ctx->case_count];
  const idl_case_t *case_node = (const idl_case_t *) node;
  const idl_node_t *case_labels = (const idl_node_t *) case_node->case_labels;
  uint32_t label_count = 0;

  idl_reset_custom_context(ctx);
  result = idl_set_custom_context(ctx, &label_count);
  if (result) return result;
  idl_walk_node_list(ctx, case_labels, count_labels, IDL_CASE_LABEL);
  union_ctx->total_label_count += label_count;
  idl_reset_custom_context(ctx);
  result = idl_set_custom_context(ctx, union_ctx);
  if (result) return result;

  case_data->typespec_node = case_node->type_spec;
  case_data->declarator_node = case_node->declarator;
  case_data->name = get_cpp11_name(idl_identifier(case_node->declarator));
  case_data->label_count = 0;
  if (label_count > 0) {
    case_data->labels = malloc(sizeof(char *) * label_count);
  } else {
    case_data->labels = NULL;
  }
  case_data->type_name = get_cpp11_type(case_node->type_spec);
  /* Check if the declarator contains also an array expression... */
  if (idl_declarator_is_array(case_data->declarator_node))
  {
    char *array_expr = get_cpp11_declarator_array_expr(
        ctx, case_data->declarator_node->const_expr, case_data->type_name);
    free(case_data->type_name);
    case_data->type_name = array_expr;
  }
  idl_walk_node_list(ctx, case_labels, get_cpp11_labels, IDL_CASE_LABEL);

  ++(union_ctx->case_count);
  return IDL_RETCODE_OK;
}

static uint64_t
get_potential_nr_discr_values(const idl_union_t *union_node)
{
  uint64_t nr_discr_values = 0;
  const idl_node_t *node = union_node->switch_type_spec;

  switch (node->mask & (IDL_BASE_TYPE | IDL_CONSTR_TYPE))
  {
  case IDL_BASE_TYPE:
    switch (node->mask & IDL_BASE_TYPE_MASK)
    {
    case IDL_INTEGER_TYPE:
      switch(node->mask & IDL_INTEGER_MASK_IGNORE_SIGN)
      {
      case IDL_INT8:
        nr_discr_values = UINT8_MAX;
        break;
      case IDL_INT16:
        nr_discr_values = UINT16_MAX;
        break;
      case IDL_INT32:
        nr_discr_values = UINT32_MAX;
        break;
      case IDL_INT64:
        nr_discr_values = UINT64_MAX;
        break;
      default:
        assert(0);
        break;
      }
      break;
    default:
      switch(node->mask & IDL_BASE_OTHERS_MASK)
      {
      case IDL_CHAR:
      case IDL_OCTET:
        nr_discr_values = UINT8_MAX;
        break;
      case IDL_WCHAR:
        nr_discr_values = UINT16_MAX;
        break;
      case IDL_BOOL:
        nr_discr_values = 2;
        break;
      default:
        assert(0);
        break;
      }
      break;
    }
    break;
  case IDL_CONSTR_TYPE:
    switch (node->mask & IDL_ENUM)
    {
    case IDL_ENUM:
    {
      /* Pick the first of the available enumerators. */
      const idl_enum_t *enumeration = (const idl_enum_t *) node;
      const idl_node_t *enumerator = (const idl_node_t *) enumeration->enumerators;
      nr_discr_values = 0;
      while(enumerator)
      {
        ++nr_discr_values;
        enumerator = enumerator->next;
      }
      break;
    }
    default:
      assert(0);
      break;
    }
    break;
  default:
    assert(0);
    break;
  }
  return nr_discr_values;
}

static idl_constval_t
get_min_value(const idl_node_t *node)
{
  idl_constval_t result;
  static const idl_mask_t mask = (IDL_BASE_TYPE|(IDL_BASE_TYPE-1));

  result.node = *node;
  result.node.mask &= mask;
  switch (node->mask & mask)
  {
  case IDL_BOOL:
    result.value.bln = false;
    break;
  case IDL_OCTET:
    result.value.oct = 0;
    break;
  case IDL_INT8:
    result.value.int8 = INT8_MIN;
    break;
  case IDL_UINT8:
    result.value.uint8 = 0;
    break;
  case IDL_INT16:
    result.value.int16 = INT16_MIN;
    break;
  case IDL_UINT16:
    result.value.uint16 = 0;
    break;
  case IDL_INT32:
    result.value.int32 = INT32_MIN;
    break;
  case IDL_UINT32:
    result.value.uint32 = 0;
    break;
  case IDL_INT64:
    result.value.int64 = INT64_MIN;
    break;
  case IDL_UINT64:
    result.value.uint64 = 0ULL;
    break;
  default:
    assert(0);
    break;
  }
  result.node.mask |= IDL_CONST;
  return result;
}

static void *
enumerator_incr_value(void *val)
{
  const idl_enumerator_t *enum_val = (const idl_enumerator_t *) val;
  return enum_val->node.next;
}

static void *
constval_incr_value(void *val)
{
  idl_constval_t *cv = (idl_constval_t *)val;
  static const idl_mask_t mask = (IDL_BASE_TYPE|(IDL_BASE_TYPE-1));

  switch (cv->node.mask & mask)
  {
  case IDL_BOOL:
    cv->value.bln = true;
    break;
  case IDL_INT8:
    cv->value.int8++;
    break;
  case IDL_UINT8:
    cv->value.uint8++;
    break;
  case IDL_INT16:
    cv->value.int16++;
    break;
  case IDL_UINT16:
    cv->value.uint16++;
    break;
  case IDL_INT32:
    cv->value.int32++;
    break;
  case IDL_UINT32:
    cv->value.uint32++;
    break;
  case IDL_INT64:
    cv->value.int64++;
    break;
  case IDL_UINT64:
    cv->value.uint64++;
    break;
  default:
    assert(0);
    break;
  }
  return cv;
}

typedef enum { IDL_LT, IDL_EQ, IDL_GT} idl_equality_t;
typedef idl_equality_t (*idl_comparison_fn) (const void *element1, const void *element2);
typedef void *(*idl_incr_element_fn) (void *element1);

static idl_equality_t
compare_enum_elements(const void *element1, const void *element2)
{
  const idl_enumerator_t *enumerator1 = (const idl_enumerator_t *) element1;
  const idl_enumerator_t *enumerator2 = (const idl_enumerator_t *) element2;
  idl_equality_t result = IDL_EQ;

  if (enumerator1->value < enumerator2->value) result = IDL_LT;
  if (enumerator1->value > enumerator2->value) result = IDL_GT;
  return result;
}

static idl_equality_t
compare_const_elements(const void *element1, const void *element2)
{
#define EQ(a,b) ((a<b) ? IDL_LT : ((a>b) ? IDL_GT : IDL_EQ))
  const idl_constval_t *cv1 = (const idl_constval_t *) element1;
  const idl_constval_t *cv2 = (const idl_constval_t *) element2;
  static const idl_mask_t mask = IDL_BASE_TYPE|(IDL_BASE_TYPE-1);

  assert((cv1->node.mask & mask) == (cv2->node.mask & mask));
  switch (cv1->node.mask & mask)
  {
  case IDL_BOOL:
    return EQ(cv1->value.bln, cv2->value.bln);
  case IDL_INT8:
    return EQ(cv1->value.int8, cv2->value.int8);
  case IDL_UINT8:
    return EQ(cv1->value.uint8, cv2->value.uint8);
  case IDL_INT16:
    return EQ(cv1->value.int16, cv2->value.int16);
  case IDL_UINT16:
    return EQ(cv1->value.uint16, cv2->value.uint16);
  case IDL_INT32:
    return EQ(cv1->value.int32, cv2->value.int32);
  case IDL_UINT32:
    return EQ(cv1->value.uint32, cv2->value.uint32);
  case IDL_INT64:
    return EQ(cv1->value.int64, cv2->value.int64);
  case IDL_UINT64:
    return EQ(cv1->value.uint64, cv2->value.uint64);
  default:
    assert(0);
    break;
  }
  return IDL_EQ;
#undef EQ
}

static void
swap(void **element1, void **element2)
{
  void *tmp = *element1;
  *element1 = *element2;
  *element2 = tmp;
}

static uint64_t
manage_pivot (void **array, uint64_t low, uint64_t high, idl_comparison_fn compare_elements)
{
  uint64_t i = low;
  void *pivot = array[high];

  for (uint64_t j = low; j <= high- 1; j++)
  {
    if (compare_elements(&array[j], &pivot) == IDL_LT)
    {
      swap(&array[i++], &array[j]);
    }
  }
  swap(&array[i + 1], &array[high]);
  return (i + 1);
}

static void quick_sort(void **array, uint64_t low, uint64_t high, idl_comparison_fn compare_elements)
{
  uint64_t pivot_index;

  if (low < high)
  {
    pivot_index = manage_pivot(array, low, high, compare_elements);
    quick_sort(array, low, pivot_index - 1, compare_elements);
    quick_sort(array, pivot_index + 1, high, compare_elements);
  }
}

static char *
get_first_unused_discr_value(
    void **array,
    void *min_value,
    void *max_value,
    idl_comparison_fn compare_elements,
    idl_incr_element_fn incr_element)
{

  uint32_t i = 0;
  do
  {
    if (compare_elements(min_value, array[i]) == IDL_LT)
    {
      return get_cpp11_const_value(min_value);
    }
    min_value = incr_element(min_value);
    if (array[i] != max_value) ++i;
  } while (compare_elements(min_value, array[i]) != IDL_GT);
  return get_cpp11_const_value(min_value);
}

static char *
get_default_discr_value(idl_backend_ctx ctx, const idl_union_t *union_node)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);
  char *def_value = NULL;
  union_ctx->nr_unused_discr_values =
      get_potential_nr_discr_values(union_node) - union_ctx->total_label_count;
  idl_constval_t min_const_value;
  void *min_value;
  idl_comparison_fn compare_elements;
  idl_incr_element_fn incr_element;

  if (union_ctx->nr_unused_discr_values) {
    /* find the smallest available unused discr_value. */
    void **all_labels = malloc(sizeof(void *) * (size_t)union_ctx->total_label_count);
    uint32_t i = 0;
    const idl_case_t *case_data = union_node->cases;
    while (case_data)
    {
      const idl_case_label_t *label = case_data->case_labels;
      while (label)
      {
        all_labels[i++] = label->const_expr;
        label = (const idl_case_label_t *) label->node.next;
      }
      case_data = (const idl_case_t *) case_data->node.next;
    }
    if (union_node->switch_type_spec->mask & IDL_ENUM) {
      compare_elements = compare_enum_elements;
      incr_element = enumerator_incr_value;
      min_value = ((const idl_enum_t *)union_ctx->discr_node)->enumerators;
    } else {
      compare_elements = compare_const_elements;
      incr_element = constval_incr_value;
      min_const_value = get_min_value(union_node->switch_type_spec);
      min_value = &min_const_value;
    }
    quick_sort(all_labels, 0, union_ctx->total_label_count - 1, compare_elements);
    def_value = get_first_unused_discr_value(
        all_labels,
        min_value,
        all_labels[union_ctx->total_label_count - 1],
        compare_elements,
        incr_element);
    if (!union_ctx->default_case) union_ctx->has_impl_default = true;
    free(all_labels);
  } else {
    /* Pick the first available literal value from the first available case. */
    const idl_const_expr_t *const_expr = union_node->cases->case_labels->const_expr;
    def_value = get_cpp11_const_value((const idl_constval_t *) const_expr);
  }

  return def_value;
}

static void
union_generate_attributes(idl_backend_ctx ctx)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);

  idl_file_out_printf(ctx, "private:\n");
  idl_indent_incr(ctx);

  /* Declare a union attribute representing the discriminator. */
  idl_file_out_printf(ctx, "%s m__d;\n", union_ctx->discr_type);

  /* Declare a union attribute comprising of all the branch types. */
  idl_file_out_printf(ctx, CPP11_UNION_TEMPLATE "<\n");
  idl_indent_double_incr(ctx);
  for (uint32_t i = 0; i < union_ctx->case_count; ++i) {
    idl_file_out_printf(
        ctx,
        "%s%s\n",
        union_ctx->cases[i].type_name,
        i == (union_ctx->case_count - 1) ? "" : ",");
  }
  idl_indent_double_decr(ctx);
  idl_file_out_printf(ctx, ">");
  for (uint32_t i = 0; i < union_ctx->case_count; ++i) {
    idl_file_out_printf_no_indent(
        ctx,
        " %s%s",
        union_ctx->cases[i].name,
        (i ==  (union_ctx->case_count - 1)) ? ";" : ",");
  }
  idl_file_out_printf_no_indent(ctx, "\n\n");
  idl_indent_decr(ctx);
}

static void
union_generate_constructor(idl_backend_ctx ctx)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);

  idl_file_out_printf(ctx, "public:\n");
  idl_indent_incr(ctx);

  /* Start building default (empty) constructor. */
  idl_file_out_printf(ctx, "%s() :\n", union_ctx->name);
  idl_indent_double_incr(ctx);
  idl_file_out_printf(ctx, "m__d(%s)", union_ctx->default_label);
  if (!union_ctx->has_impl_default)
  {
    idl_file_out_printf_no_indent(ctx, ",\n");
    idl_file_out_printf(ctx, "%s()", union_ctx->default_case->name);
  }
  idl_file_out_printf_no_indent(ctx, " {}\n\n");
  idl_indent_double_decr(ctx);
  idl_indent_decr(ctx);
}
static void
union_generate_discr_getter_setter(idl_backend_ctx ctx)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);

  idl_indent_incr(ctx);
  /* Generate a getter for the discriminator. */
  idl_file_out_printf(ctx, "%s _d() const\n", union_ctx->discr_type);
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return m__d;\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");

  /* Generate a setter for the discriminator. */
  idl_file_out_printf(ctx, "void _d(%s val)\n", union_ctx->discr_type);
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);

  if ((union_ctx->discr_node->mask & IDL_BOOL) == IDL_BOOL) {
    idl_file_out_printf(ctx, "bool valid = (val == m__d);\n\n");
  } else {
    idl_file_out_printf(ctx, "bool valid = true;\n");
    idl_file_out_printf(ctx, "switch (val) {\n");
    for (uint32_t i = 0; i < union_ctx->case_count; ++i) {
      if (&union_ctx->cases[i] == union_ctx->default_case)
      {
        continue;
      }
      else
      {
        for (uint32_t j = 0; j < union_ctx->cases[i].label_count; ++j) {
          idl_file_out_printf(ctx, "case %s:\n", union_ctx->cases[i].labels[j]);
        }
        idl_indent_incr(ctx);
        for (uint32_t j = 0; j < union_ctx->cases[i].label_count; ++j) {
          if (j == 0) {
            idl_file_out_printf(ctx, "if (m__d != %s", union_ctx->cases[i].labels[j]);
            idl_indent_double_incr(ctx);
          } else {
            idl_file_out_printf_no_indent(ctx, " &&\n");
            idl_file_out_printf(ctx, "m__d != %s", union_ctx->cases[i].labels[j]);
          }
        }
        idl_indent_decr(ctx);
        idl_file_out_printf_no_indent(ctx, ") {\n");
        idl_file_out_printf(ctx, "valid = false;\n");
        idl_indent_decr(ctx);
        idl_file_out_printf(ctx, "}\n");
        idl_file_out_printf(ctx, "break;\n");
        idl_indent_decr(ctx);
      }
    }
    if (union_ctx->default_case || union_ctx->has_impl_default) {
      idl_file_out_printf(ctx, "default:\n");
      idl_indent_incr(ctx);
      for (uint32_t i = 0; i < union_ctx->case_count; ++i) {
        for (uint32_t j = 0; j < union_ctx->cases[i].label_count; ++j) {
          if (i == 0 && j == 0) {
            idl_file_out_printf(ctx, "if (m__d == %s", union_ctx->cases[i].labels[j]);
            idl_indent_double_incr(ctx);
          }
          else
          {
            idl_file_out_printf_no_indent(ctx, " ||\n");
            idl_file_out_printf(ctx, "m__d == %s", union_ctx->cases[i].labels[j]);
          }
        }
      }
      idl_file_out_printf_no_indent(ctx, ") {\n");
      idl_indent_decr(ctx);
      idl_file_out_printf(ctx, "valid = false;\n");
      idl_indent_decr(ctx);
      idl_file_out_printf(ctx, "}\n");
      idl_file_out_printf(ctx, "break;\n");
    }

    idl_indent_decr(ctx);
    idl_file_out_printf(ctx, "}\n\n");
  }

  idl_file_out_printf(ctx, "if (!valid) {\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "throw dds::core::InvalidArgumentError(\"New discriminator value does not match current discriminator\");\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "m__d = val;\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_indent_decr(ctx);
}

static void
union_generate_getter_body(idl_backend_ctx ctx, uint32_t i)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);

  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  if (&union_ctx->cases[i] != union_ctx->default_case) {
    for (uint32_t j = 0; j < union_ctx->cases[i].label_count; ++j) {
      if (j == 0) {
        idl_file_out_printf(ctx, "if (m__d == %s", union_ctx->cases[i].labels[j]);
        idl_indent_double_incr(ctx);
      } else {
        idl_file_out_printf_no_indent(ctx, " || \n");
        idl_file_out_printf(ctx, "m__d == %s", union_ctx->cases[i].labels[j]);
      }
    }
    idl_file_out_printf_no_indent(ctx, ") {\n");
  } else {
    for (uint32_t j = 0; j < union_ctx->case_count; ++j) {
      for (uint32_t k = 0; k < union_ctx->cases[j].label_count; k++) {
        if (j == 0 && k == 0) {
          idl_file_out_printf(ctx, "if (m__d != %s", union_ctx->cases[j].labels[k]);
          idl_indent_double_incr(ctx);
        } else {
          idl_file_out_printf_no_indent(ctx, " && \n");
          idl_file_out_printf(ctx, "m__d != %s", union_ctx->cases[j].labels[k]);
        }
      }
    }
    idl_file_out_printf_no_indent(ctx, ") {\n");
    idl_indent_decr(ctx);
  }
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "return " CPP11_UNION_GETTER_TEMPLATE "<%s>(%s);\n", union_ctx->cases[i].type_name, union_ctx->cases[i].name);
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "} else {\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "throw dds::core::InvalidArgumentError(\"Requested branch does not match current discriminator\");\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
}

static void
union_generate_setter_body(idl_backend_ctx ctx, uint32_t i)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);

  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  /* Check if a setter with optional discriminant value is present (when more than 1 label). */
  if (union_ctx->cases[i].label_count < 2)
  {
    const char *new_discr_value;

    /* If not, check whether the current case is the default case. */
    if (&union_ctx->cases[i] != union_ctx->default_case) {
      /* If not the default case, pick the first available value. */
      new_discr_value = union_ctx->cases[i].labels[0];
    } else {
      /* If it is the default case, pick the default label. */
      new_discr_value = union_ctx->default_label;
    }
    idl_file_out_printf(ctx, "m__d = %s;\n", new_discr_value);
  }
  else
  {
    /* In case the discriminant is explicitly passed, check its validity and then take that value. */
    for (uint32_t j = 0; j < union_ctx->cases[i].label_count; ++j) {
      if (j == 0) {
        idl_file_out_printf(ctx, "if (m__d != %s", union_ctx->cases[i].labels[j]);
        idl_indent_double_incr(ctx);
      } else {
        idl_file_out_printf_no_indent(ctx, " &&\n");
        idl_file_out_printf(ctx, "m__d != %s", union_ctx->cases[i].labels[j]);
      }
    }
    idl_indent_decr(ctx);
    idl_file_out_printf_no_indent(ctx, ") {\n");
    idl_file_out_printf(ctx, "throw dds::core::InvalidArgumentError(\"Provided discriminator does not match selected branch\");\n");
    idl_indent_decr(ctx);
    idl_file_out_printf(ctx, "} else {\n");
    idl_indent_incr(ctx);
    idl_file_out_printf(ctx, "m__d = _d;\n");
    idl_indent_decr(ctx);
    idl_file_out_printf(ctx, "}\n");
  }
  idl_file_out_printf(ctx, "%s = val;\n", union_ctx->cases[i].name);
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
}

static void
union_generate_branch_getters_setters(idl_backend_ctx ctx)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);

  /* Start building the getter/setter methods for each attribute. */
  idl_indent_incr(ctx);
  for (uint32_t i = 0; i < union_ctx->case_count; ++i) {
    bool is_primitive = idl_declarator_is_primitive(union_ctx->cases[i].declarator_node);
    const cpp11_case_state *case_state = &union_ctx->cases[i];

    /* Build const-getter. */
    idl_file_out_printf(ctx, "%s%s%s %s() const\n",
        is_primitive ? "" : "const ",
        case_state->type_name,
        is_primitive ? "" : "&",
        case_state->name);
    union_generate_getter_body(ctx, i);

    /* Build ref-getter. */
    idl_file_out_printf(ctx, "%s& %s()\n",
        case_state->type_name, case_state->name);
    union_generate_getter_body(ctx, i);

    /* Build setter. */
    if (case_state->label_count < 2)
    {
      /* No need for optional discriminant parameter if there is only one applicable label. */
      idl_file_out_printf(ctx, "void %s(%s%s%s val)\n",
          case_state->name,
          is_primitive ? "": "const ",
          case_state->type_name,
          is_primitive ? "" : "&");
    }
    else
    {
      /* Use optional discriminant parameter if there is more than one applicable label. */
      idl_file_out_printf(ctx, "void %s(%s%s%s val, %s _d = %s)\n",
          case_state->name,
          is_primitive ? "": "const ",
          case_state->type_name,
          is_primitive ? "" : "&",
          union_ctx->discr_type,
          case_state->labels[0]);
    }
    union_generate_setter_body(ctx, i);

    /* When appropriate, build setter with move semantics. */
    if (!is_primitive) {
      if (case_state->label_count < 2)
      {
        idl_file_out_printf(ctx, "void %s(%s&& val)\n",
            case_state->name,
            case_state->type_name);
      }
      else
      {
        idl_file_out_printf(ctx, "void %s(%s&& val, %s _d = %s)\n",
            case_state->name,
            case_state->type_name,
            union_ctx->discr_type,
            case_state->labels[0]);
      }
      union_generate_setter_body(ctx, i);
    }
  }
  idl_indent_decr(ctx);
}

static void
union_generate_implicit_default_setter(idl_backend_ctx ctx)
{
  cpp11_union_context *union_ctx = (cpp11_union_context *) idl_get_custom_context(ctx);
  const char *discr_value;

  /* Check if all possible discriminant values are covered. */
  if (union_ctx->has_impl_default)
  {
    idl_indent_incr(ctx);
    if (union_ctx->nr_unused_discr_values < 2)
    {
      idl_file_out_printf(ctx, "void _default()\n");
      discr_value = union_ctx->default_label;
    }
    else
    {
      idl_file_out_printf(ctx, "void _default(%s _d = %s)\n",
          union_ctx->discr_type,
          discr_value = union_ctx->default_label);
      discr_value = "_d";
    }
    idl_file_out_printf(ctx, "{\n");
    idl_indent_incr(ctx);
    idl_file_out_printf(ctx, "m__d = %s;\n", discr_value);
    idl_indent_decr(ctx);
    idl_file_out_printf(ctx, "}\n\n");
    idl_indent_decr(ctx);
  }
}

static idl_retcode_t
union_generate_body(idl_backend_ctx ctx, const idl_union_t *union_node)
{
  idl_retcode_t result = IDL_RETCODE_OK;
  char *cpp11Name = get_cpp11_name(idl_identifier(union_node));

  idl_file_out_printf(ctx, "class %s\n", cpp11Name);
  idl_file_out_printf(ctx, "{\n");

  uint32_t nr_cases = 0;
  const idl_node_t *cases = (const idl_node_t *) union_node->cases;
  cpp11_union_context union_ctx;
  result = idl_set_custom_context(ctx, &nr_cases);
  if (result) return result;
  idl_walk_node_list(ctx, cases, count_cases, IDL_CASE);
  idl_reset_custom_context(ctx);

  union_ctx.cases = malloc(sizeof(cpp11_case_state) * nr_cases);
  union_ctx.case_count = 0;
  union_ctx.discr_node = union_node->switch_type_spec;
  union_ctx.discr_type = get_cpp11_type(union_node->switch_type_spec);
  union_ctx.name = cpp11Name;
  union_ctx.default_case = NULL;
  union_ctx.has_impl_default = false;
  union_ctx.total_label_count = 0;
  result = idl_set_custom_context(ctx, &union_ctx);
  if (result) return result;
  idl_walk_node_list(ctx, cases, get_cpp11_case_data, IDL_CASE);
  union_ctx.default_label = get_default_discr_value(ctx, union_node);

  /* Generate the union content. */
  union_generate_attributes(ctx);
  union_generate_constructor(ctx);
  union_generate_discr_getter_setter(ctx);
  union_generate_branch_getters_setters(ctx);
  union_generate_implicit_default_setter(ctx);
  generate_streamer_interfaces(ctx);

  idl_file_out_printf(ctx, "};\n\n");
  idl_reset_custom_context(ctx);

  for (uint32_t i = 0; i < nr_cases; ++i)
  {
    free(union_ctx.cases[i].name);
    free(union_ctx.cases[i].type_name);
    for (uint32_t j = 0; j < union_ctx.cases[i].label_count; ++j)
    {
      free(union_ctx.cases[i].labels[j]);
    }
    free(union_ctx.cases[i].labels);
  }
  free(union_ctx.cases);
  free(union_ctx.discr_type);
  free(union_ctx.default_label);
  free(cpp11Name);

  return result;
}

static idl_retcode_t
enumerator_generate_identifier(idl_backend_ctx ctx, const idl_node_t *enumerator_node)
{
  const idl_enumerator_t *enumerator = (const idl_enumerator_t *) enumerator_node;
  char *cpp11Name = get_cpp11_name(idl_identifier(enumerator));
  idl_file_out_printf(ctx, "%s,\n", cpp11Name);
  free(cpp11Name);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
enum_generate_body(idl_backend_ctx ctx, const idl_enum_t *enum_node)
{
  idl_retcode_t result;
  char *cpp11Name = get_cpp11_name(idl_identifier(enum_node));
  const idl_node_t *enumerators = (const idl_node_t *) enum_node->enumerators;

  idl_file_out_printf(ctx, "enum class %s\n", cpp11Name);
  idl_file_out_printf(ctx, "{\n", cpp11Name);
  idl_indent_incr(ctx);
  result = idl_walk_node_list(ctx, enumerators, enumerator_generate_identifier, IDL_ENUMERATOR);
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "};\n\n");
  free(cpp11Name);
  return result;
}

static idl_retcode_t
typedef_generate_body(idl_backend_ctx ctx, const idl_typedef_t *typedef_node)
{
  char *cpp11Name = get_cpp11_name(idl_identifier(typedef_node->declarators));
  char *cpp11Type = get_cpp11_type(typedef_node->type_spec);

  idl_file_out_printf(ctx, "typedef %s %s;\n\n", cpp11Type, cpp11Name);

  free(cpp11Type);
  free(cpp11Name);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
module_generate_body(idl_backend_ctx ctx, const idl_module_t *module_node)
{
  idl_retcode_t result;
  char *cpp11Name = get_cpp11_name(idl_identifier(module_node));

  idl_file_out_printf(ctx, "namespace %s\n", cpp11Name);
  idl_file_out_printf(ctx, "{\n", cpp11Name);
  idl_indent_incr(ctx);
  result = idl_walk_node_list(ctx, module_node->definitions, cpp11_scope_walk, IDL_MASK_ALL);
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");

  free(cpp11Name);

  return result;
}

static idl_retcode_t
forward_decl_generate_body(idl_backend_ctx ctx, const idl_forward_t *forward_node)
{
  char *cpp11Name = get_cpp11_name(idl_identifier(forward_node));
  assert(forward_node->node.mask & (IDL_STRUCT | IDL_UNION));
  idl_file_out_printf(
      ctx,
      "%s %s;\n\n",
      (forward_node->node.mask & IDL_STRUCT) ? "struct" : "union",
      cpp11Name);
  free(cpp11Name);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
const_generate_body(idl_backend_ctx ctx, const idl_const_t *const_node)
{
  char *cpp11Name = get_cpp11_name(idl_identifier(const_node));
  char *cpp11Type = get_cpp11_type(const_node->type_spec);
  char *cpp11Value = get_cpp11_const_value((const idl_constval_t *) const_node->const_expr);
  idl_file_out_printf(
      ctx,
      "const %s %s = %s;\n\n",
      cpp11Type,
      cpp11Name,
      cpp11Value);
  free(cpp11Value);
  free(cpp11Type);
  free(cpp11Name);
  return IDL_RETCODE_OK;
}

static idl_retcode_t
cpp11_scope_walk(idl_backend_ctx ctx, const idl_node_t *node)
{
  idl_retcode_t result = IDL_RETCODE_OK;

  if (node->mask & IDL_FORWARD)
  {
    result = forward_decl_generate_body(ctx, (const idl_forward_t *) node);
  }
  else
  {
    switch (node->mask & IDL_CATEGORY_MASK)
    {
    case IDL_MODULE:
      result = module_generate_body(ctx, (const idl_module_t *) node);
      break;
    case IDL_STRUCT:
      result = struct_generate_body(ctx, (const idl_struct_t *) node);
      break;
    case IDL_UNION:
      result = union_generate_body(ctx, (const idl_union_t *) node);
      break;
    case IDL_ENUM:
      result = enum_generate_body(ctx, (const idl_enum_t *) node);
      break;
    case IDL_TYPEDEF:
      result = typedef_generate_body(ctx, (const idl_typedef_t *) node);
      break;
    case IDL_CONST:
      result = const_generate_body(ctx, (const idl_const_t *) node);
      break;
    default:
      assert(0);
      break;
    }
  }

  return result;
}

typedef enum
{
  idl_no_dep                = 0x0,
  idl_string_bounded_dep    = 0x01 << 0,
  idl_string_unbounded_dep  = 0x01 << 1,
  idl_array_dep             = 0x01 << 2,
  idl_vector_bounded_dep    = 0x01 << 3,
  idl_vector_unbounded_dep  = 0x01 << 4,
  idl_variant_dep           = 0x01 << 5,
  idl_optional_dep          = 0x01 << 6
} idl_include_dep;

static idl_retcode_t
get_util_dependencies(idl_backend_ctx ctx, const idl_node_t *node)
{
  idl_retcode_t result = IDL_RETCODE_OK;
  idl_include_dep *dependency_mask = (idl_include_dep *) idl_get_custom_context(ctx);

  switch (node->mask & IDL_CATEGORY_MASK)
  {
  case IDL_UNION:
    (*dependency_mask) |= idl_variant_dep;
    break;
  case IDL_TEMPL_TYPE:
    switch (node->mask & IDL_TEMPL_TYPE_MASK)
    {
    case IDL_SEQUENCE:
      if (((const idl_sequence_t*)node)->maximum)
        (*dependency_mask) |= idl_vector_bounded_dep;
      else
        (*dependency_mask) |= idl_vector_unbounded_dep;
      result = get_util_dependencies(ctx, ((const idl_sequence_t *)node)->type_spec);
      break;
    case IDL_STRING:
      if (((const idl_string_t*)node)->maximum)
        (*dependency_mask) |= idl_string_bounded_dep;
      else
        (*dependency_mask) |= idl_string_unbounded_dep;
      break;
    default:
      break;
    }
    break;
  case IDL_DECLARATOR:
    if (((const idl_declarator_t *)node)->const_expr) {
      (*dependency_mask) |= idl_array_dep;
    }
    break;
  default:
    break;
  }
  return result;
}

static void
idl_generate_include_statements(idl_backend_ctx ctx, const idl_tree_t *parse_tree)
{
  idl_include_dep util_depencencies = idl_no_dep;
  uint32_t nr_includes = 0;

  /* First determine the list of files included by our IDL file itself. */
  idl_include_t *include, *next;
  include = idl_get_include_list(ctx, parse_tree);
  for (; include; include = next, ++nr_includes) {
    char *file, *dot;
    file = include->file->name;
    dot = strrchr(file, '.');
    if (!dot) dot = file + strlen(file);
    if (!include->indirect)
      idl_file_out_printf(ctx, "#include \"%.*s.hpp\"\n", dot - file, file);
    next = include->next;
    free(include);
  }
  if (nr_includes == 0) {
    idl_file_out_printf(ctx, "#include <cstddef>\n");
    idl_file_out_printf(ctx, "#include <cstdint>\n\n");
    idl_file_out_printf(ctx, "#include \"dds/ddsi/ddsi_keyhash.h\"\n");
  }
  idl_file_out_printf(ctx, "\n");

  /* Next determine if we need to include any utility libraries... */
  idl_set_custom_context(ctx, &util_depencencies);
  idl_walk_tree(ctx, parse_tree->root, get_util_dependencies, IDL_MASK_ALL);
  idl_reset_custom_context(ctx);
  if (util_depencencies) {
    if (strcmp(CPP11_BOUNDED_SEQUENCE_INCLUDE, CPP11_SEQUENCE_INCLUDE))
    {
      if (util_depencencies & idl_vector_bounded_dep) {
        idl_file_out_printf(ctx, "#include " CPP11_BOUNDED_SEQUENCE_INCLUDE "\n");
      }
      if (util_depencencies & idl_vector_unbounded_dep) {
        idl_file_out_printf(ctx, "#include " CPP11_SEQUENCE_INCLUDE "\n");
      }
    }
    else
    {
      if (util_depencencies & (idl_vector_bounded_dep | idl_vector_unbounded_dep)) {
        idl_file_out_printf(ctx, "#include " CPP11_SEQUENCE_INCLUDE "\n");
      }
    }
    if (strcmp(CPP11_BOUNDED_STRING_INCLUDE, CPP11_STRING_INCLUDE))
    {
      if (util_depencencies & idl_string_bounded_dep) {
        idl_file_out_printf(ctx, "#include " CPP11_BOUNDED_STRING_INCLUDE "\n");
      }
      if (util_depencencies & idl_string_unbounded_dep) {
        idl_file_out_printf(ctx, "#include " CPP11_STRING_INCLUDE "\n");
      }
    }
    else
    {
      if (util_depencencies & (idl_string_bounded_dep | idl_string_unbounded_dep)) {
        idl_file_out_printf(ctx, "#include " CPP11_STRING_INCLUDE "\n");
      }
    }
    if (util_depencencies & idl_variant_dep) {
      idl_file_out_printf(ctx, "#include " CPP11_UNION_INCLUDE "\n");
    }
    if (util_depencencies & idl_array_dep) {
      idl_file_out_printf(ctx, "#include " CPP11_ARRAY_INCLUDE "\n");
    }
    idl_file_out_printf(ctx, "\n");
  }
}

idl_retcode_t
idl_backendGenerateType(idl_backend_ctx ctx, const idl_tree_t *parse_tree)
{
  /* If input comes from a file, generate appropriate include statements. */
  if (parse_tree->files) idl_generate_include_statements(ctx, parse_tree);

  /* Next, generate the C++ representation for all nodes in the list. */
  return idl_walk_node_list(ctx, parse_tree->root, cpp11_scope_walk, IDL_MASK_ALL);
}

