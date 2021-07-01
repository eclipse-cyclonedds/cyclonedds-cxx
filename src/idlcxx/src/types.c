/*
 * Copyright(c) 2021 ADLINK Technology Limited and others
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
#include <inttypes.h>

#include "idl/string.h"
#include "idl/stream.h"
#include "idl/processor.h"
#include "idl/print.h"

#include "generator.h"

static idl_retcode_t
emit_member(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const idl_type_spec_t *type_spec;
  char *type, *value;
  const char *name, *fmt;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (idl_is_array(node))
    type_spec = node;
  else
    type_spec = idl_type_spec(node);

  name = get_cpp11_name(node);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_is_array(type_spec))
    value = "{ }";
  else if (!idl_is_enum(type_spec) && !idl_is_base_type(type_spec))
    value = NULL;
  else if (IDL_PRINTA(&value, get_cpp11_default_value, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  fmt = " %s %s_%s%s;\n";
  if (idl_fprintf(gen->header.handle, fmt, type, name, value ? " = " : "", value ? value : "") < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static bool is_first(const idl_declarator_t *declarator)
{
  if (declarator->node.previous)
    return false;
  if (declarator->node.parent && declarator->node.parent->previous)
    return false;
  return true;
}

static idl_retcode_t
emit_parameter(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  bool simple;
  char *type;
  const char *name, *fmt, *sep;
  const idl_type_spec_t *type_spec;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (idl_is_array(node))
    type_spec = node;
  else
    type_spec = idl_type_spec(node);

  simple = idl_mask(type_spec) & (IDL_BASE_TYPE|IDL_ENUM);
  sep = is_first(node) ? "" : ",\n";
  fmt = simple ? "%s    %s %s"
               : "%s    const %s& %s";
  name = get_cpp11_name(node);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, fmt, sep, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_member_initializer(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const char *name, *fmt, *sep;

  (void)pstate;
  (void)revisit;
  (void)path;

  fmt = "%1$s    %2$s_(%2$s)";
  sep = is_first(node) ? "" : ",\n";
  name = get_cpp11_name(node);
  if (idl_fprintf(gen->header.handle, fmt, sep, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_member_methods(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const idl_type_spec_t *type_spec;
  char *type;
  const char *name, *fmt;

  (void)pstate;
  (void)revisit;
  (void)path;

  name = get_cpp11_name(node);

  if (idl_is_array(node))
    type_spec = node;
  else
    type_spec = idl_type_spec(node);

  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (idl_mask(type_spec) & (IDL_BASE_TYPE | IDL_ENUM))
    fmt = "  %1$s %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(%1$s _val_) { this->%2$s_ = _val_; }\n";
  else
    fmt = "  const %1$s& %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(const %1$s& _val_) { this->%2$s_ = _val_; }\n"
          "  void %2$s(%1$s&& _val_) { this->%2$s_ = _val_; }\n";

  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_member_comparison_operator(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const char *name, *fmt, *sep;

  (void)pstate;
  (void)revisit;
  (void)path;

  name = get_cpp11_name(node);
  sep = is_first(node) ? "" : " &&\n      ";
  fmt = "%2$s%1$s_ == _other.%1$s_";
  if (idl_fprintf(gen->header.handle, fmt, name, sep) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_struct(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  idl_retcode_t ret;
  struct generator *gen = user_data;
  const idl_struct_t *_struct = node;
  const char *name, *fmt;
  idl_visitor_t visitor;

  (void)revisit;
  (void)path;

  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_DECLARATOR;

  /* class */
  name = get_cpp11_name(_struct);
  if (_struct->inherit_spec) {
    const idl_type_spec_t *type_spec = _struct->inherit_spec->base;
    char *base = NULL;
    if (IDL_PRINTA(&base, get_cpp11_fully_scoped_name, type_spec, gen) < 0)
      return IDL_RETCODE_NO_MEMORY;
    if (idl_fprintf(gen->header.handle, "class %s : public %s\n", name, base) < 0)
      return IDL_RETCODE_NO_MEMORY;
  } else {
    if (idl_fprintf(gen->header.handle, "class %s\n", name) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  if (fputs("{\nprivate:\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* members */
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_member;
  if (_struct->members && (ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

  /* constructors */
  fmt = "\n"
        "public:\n"
        "  %1$s() = default;\n\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (_struct->members)
  {
    fmt = "  explicit %1$s(\n";
    if (idl_fprintf(gen->header.handle, fmt, name) < 0)
      return IDL_RETCODE_NO_MEMORY;

    /* constructor parameters */
    visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_parameter;
    if (_struct->members && (ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
      return ret;

    if (fputs(") :\n", gen->header.handle) < 0)
      return IDL_RETCODE_NO_MEMORY;

    /* constructor initializers */
    visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_member_initializer;
    if (_struct->members && (ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
      return ret;

    if (fputs(" { }\n\n", gen->header.handle) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  /* getters and setters */
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_member_methods;
  if (_struct->members && (ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

  /* comparison operators */
  fmt = "\n"
        "  bool operator==(const %s& _other) const\n"
        "  {\n"
        "    return ";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_member_comparison_operator;
  if (_struct->members && (ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

  if (_struct->inherit_spec)
  {
    char* base = NULL;
    if (IDL_PRINTA(&base, get_cpp11_fully_scoped_name, _struct->inherit_spec->base, gen) < 0)
      return IDL_RETCODE_NO_MEMORY;
    fmt = "%2$s static_cast<const %1$s&>(*this) == static_cast<const %1$s&>(_other)";
    if (idl_fprintf(gen->header.handle, fmt, base, _struct->members ? " &&\n      " : "") < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  if (!_struct->members &&
      !_struct->inherit_spec &&
      idl_fprintf(gen->header.handle, "true") < 0)
      return IDL_RETCODE_NO_MEMORY;

  fmt = ";\n"
        "  }\n\n"
        "  bool operator!=(const %s& _other) const\n"
        "  {\n"
        "    return !(*this == _other);\n"
        "  }\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (fputs("\n};\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_enum(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator *gen = user_data;
  const idl_enum_t *_enum = (const idl_enum_t *)node;
  const idl_enumerator_t *enumerator;
  uint32_t skip = 0, value;
  const char *name, *fmt, *sep = "  ";

  (void)pstate;
  (void)revisit;
  (void)path;

  name = get_cpp11_name(node);
  if (idl_fprintf(gen->header.handle, "enum class %s\n{\n", name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  IDL_FOREACH(enumerator, _enum->enumerators) {
    name = get_cpp11_name(enumerator);
    value = enumerator->value;
    fmt = (value == skip) ? "%s%s" : "%s%s = %" PRIu32 "\n";
    if (idl_fprintf(gen->header.handle, fmt, sep, name, value) < 0)
      return IDL_RETCODE_NO_MEMORY;
    skip = value + 1;
    sep = ",\n  ";
  }

  if (fputs("};\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_VISIT_DONT_RECURSE;
}

static idl_retcode_t
expand_typedef(
  struct generator* gen,
  const idl_declarator_t* declarator)
{
  char *type = NULL;
  const char *name = get_cpp11_name(declarator);
  const idl_type_spec_t *type_spec;

  if (idl_is_array(declarator))
    type_spec = declarator;
  else
    type_spec = idl_type_spec(declarator);

  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, "typedef %s %s;\n\n", type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_typedef(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator *gen = user_data;
  const idl_typedef_t *_typedef = (const idl_typedef_t *)node;
  const idl_declarator_t *declarator;

  (void)pstate;
  (void)revisit;
  (void)path;

  idl_retcode_t ret = IDL_RETCODE_OK;
  IDL_FOREACH(declarator, _typedef->declarators) {
    if ((ret = expand_typedef(gen, declarator)) != IDL_RETCODE_OK)
      break;
  }

  return ret;
}

static idl_retcode_t
emit_module(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator *gen = user_data;
  const char *name;

  (void)pstate;
  (void)path;

  if (revisit) {
    if (fputs("}\n\n", gen->header.handle) < 0)
      return IDL_RETCODE_NO_MEMORY;
  } else  {
    name = get_cpp11_name(node);
    if (idl_fprintf(gen->header.handle, "namespace %s\n{\n", name) < 0)
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_const(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator *gen = user_data;
  char *type, *value;
  const char *name;
  const idl_const_t *_const = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  name = get_cpp11_name(_const);
  if (IDL_PRINTA(&type, get_cpp11_type, _const->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&value, get_cpp11_value, _const->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, "const %s %s = %s;\n\n", type, name, value) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_variant(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  char *type = NULL;
  const char *sep;
  const idl_case_t *_case = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  sep = !idl_previous(_case) ? "" : ", ";
  if (IDL_PRINTA(&type, get_cpp11_type, _case->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, "%s%s", sep, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_case(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  char *value;
  const idl_case_t *branch = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  /* short-circuit on default case */
  if (!branch->labels->const_expr)
    return IDL_VISIT_DONT_RECURSE;
  if (!revisit)
    return IDL_VISIT_REVISIT;
  if (IDL_PRINTA(&value, get_cpp11_value, branch->labels->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, "        return %s;\n", value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_case_label(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  char *value;
  const idl_case_label_t *label = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  assert(label->const_expr);
  if (IDL_PRINTA(&value, get_cpp11_value, label->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_mask(label) == IDL_IMPLICIT_DEFAULT_CASE_LABEL || idl_mask(label) == IDL_DEFAULT_CASE_LABEL)
  {
    if (idl_fprintf(gen->header.handle, "      default:\n") < 0)
      return IDL_RETCODE_NO_MEMORY;
  } else if (idl_fprintf(gen->header.handle, "      case %s:\n", value) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_case_comparison(
  const idl_pstate_t* pstate,
  bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator* gen = user_data;
  const idl_case_t* branch = node;

  (void)pstate;
  (void)path;

  if (!revisit)
    return IDL_VISIT_REVISIT;

  const char* branch_name = get_cpp11_name(branch->declarator);
  static const char* fmt = "        return %1$s() == _other.%1$s();\n";
  if (idl_fprintf(gen->header.handle, fmt, branch_name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_discriminator_methods(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const char *fmt;
  char *type;
  const idl_type_spec_t *type_spec;

  (void)pstate;
  (void)revisit;
  (void)path;

  type_spec = ((const idl_switch_type_spec_t *)node)->type_spec;
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  fmt = "  %1$s _d() const\n"
        "  {\n"
        "    return m__d;\n"
        "  }\n\n"
        "  void _d(%1$s d)\n"
        "  {\n"
        "    if (!_is_compatible_discriminator(m__d, d)) {\n"
        "      throw dds::core::InvalidArgumentError(\n"
        "        \"Discriminator value does not match current discriminator\");\n"
        "    }\n"
        "    m__d = d;\n"
        "  }\n\n";
  if (idl_fprintf(gen->header.handle, fmt, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_case_methods(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  bool simple, single;
  char *type, *value, *discr_type;
  const idl_case_t *branch = node;
  const char *name, *fmt;
  const idl_union_t *_union = idl_parent(branch);

  (void)pstate;
  (void)revisit;
  (void)path;

  static const char *getter =
    "    if (!_is_compatible_discriminator(m__d, %1$s)) {\n"
    "      throw dds::core::InvalidArgumentError(\n"
    "        \"Requested branch does not match current discriminator\");\n"
    "    }\n"
    "    return %2$s<%3$s>(m__u);\n"
    "  }\n\n";

  static const char *setter =
    "    if (!_is_compatible_discriminator(%1$s, d)) {\n"
    "      throw dds::core::InvalidArgumentError(\n"
    "        \"Discriminator does not match current discriminator\");\n"
    "    }\n"
    "    m__d = d;\n"
    "    m__u = u;\n"
    "  }\n\n";

  name = get_cpp11_name(branch->declarator);
  if (IDL_PRINTA(&type, get_cpp11_type, branch->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&discr_type, get_cpp11_type, _union->switch_type_spec->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&value, get_cpp11_value, branch->labels->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  simple = (idl_mask(branch->type_spec) & IDL_BASE_TYPE) != 0;
  single = (idl_degree(branch->labels) == 1);

  /* const-getter */
  fmt = simple ? "  %1$s %2$s() const\n  {\n"
               : "  const %1$s &%2$s() const\n  {\n";
  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, getter, value, gen->union_getter_format, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* ref-getter */
  fmt = "  %1$s& %2$s()\n  {\n";
  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, getter, value, gen->union_getter_format, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* setter */
  if (single)
    fmt = simple ? "  void %1$s(%2$s u)\n"
                   "  {\n"
                   "    const %3$s d = %4$s;\n"
                 : "  void %1$s(const %2$s& u)\n"
                   "  {\n"
                   "    const %3$s d = %4$s;\n";
  else
    fmt = simple ? "  void %1$s(%2$s u, %3$s d = %4$s)\n"
                   "  {\n"
                 : "  void %1$s(const %2$s& u, %3$s d = %4$s)\n"
                   "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discr_type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter, value) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (simple)
    return IDL_RETCODE_OK;

  /* setter with move semantics */
  fmt = single ? "  void %1$s(%2$s&& u)\n"
                 "  {\n"
                 "    const %3$s d = %4$s;\n"
               : "  void %1$s(%2$s&& u, %3$s d = %4$s)\n"
                 "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discr_type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_union(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  idl_retcode_t ret;
  struct generator *gen = user_data;
  char *type, *value;
  const char *name, *fmt;
  const idl_union_t *_union;
  const idl_type_spec_t *type_spec;
  idl_visitor_t visitor;

  (void)revisit;
  (void)path;
  memset(&visitor, 0, sizeof(visitor));

  _union = node;
  type_spec = _union->switch_type_spec->type_spec;

  name = get_cpp11_name(_union);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&value, get_cpp11_value, _union->default_case->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  fmt = "class %1$s\n"
        "{\n"
        "private:\n"
        "  %2$s m__d;\n\n"
        "  %3$s<";
  if (idl_fprintf(gen->header.handle, fmt, name, type, gen->union_format) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* variant */
  visitor.visit = IDL_CASE;
  visitor.accept[IDL_ACCEPT_CASE] = emit_variant;
  if ((ret = idl_visit(pstate, _union->cases, &visitor, user_data)))
    return ret;
  if (idl_fprintf(gen->header.handle, "> m__u;\n\n") < 0)
    return ret;

  /**/
  fmt = "  static const %1$s _default_discriminator = %2$s;\n\n"
        "  static %1$s _is_discriminator(const %1$s d)\n"
        "  {\n"
        "    switch (d) {\n";
  if (idl_fprintf(gen->header.handle, fmt, type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.visit = IDL_CASE | IDL_CASE_LABEL;
  visitor.accept[IDL_ACCEPT_CASE] = emit_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = emit_case_label;
  if ((ret = idl_visit(pstate, _union->cases, &visitor, user_data)))
    return ret;

  fmt = "    }\n"
        "    return _default_discriminator;\n"
        "  }\n\n"
        "  static bool _is_compatible_discriminator(const %1$s d1, const %1$s d2)\n"
        "  {\n"
        "    return _is_discriminator(d1) == _is_discriminator(d2);\n"
        "  }\n\n";
  if (idl_fprintf(gen->header.handle, fmt, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* contructor */
  fmt = "public:\n"
        "  %1$s() :\n"
        "      m__d(_default_discriminator),\n      m__u(";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (idl_mask(_union->default_case) == IDL_DEFAULT_CASE_LABEL) {
    /* default case is present */
    const idl_case_t* _case = idl_parent(_union->default_case);
    char *default_type = NULL;
    if (IDL_PRINTA(&default_type, get_cpp11_type, _case->type_spec, gen) < 0)
      return IDL_RETCODE_NO_MEMORY;

    /* add default constructor for type of default branch */
    fmt = "%1$s()";
    if (idl_fprintf(gen->header.handle, fmt, default_type) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }
  if (fputs(")\n { }\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* getters and setters */
  visitor.visit = IDL_SWITCH_TYPE_SPEC | IDL_CASE;
  visitor.accept[IDL_ACCEPT_SWITCH_TYPE_SPEC] = emit_discriminator_methods;
  visitor.accept[IDL_ACCEPT_CASE] = emit_case_methods;
  if ((ret = idl_visit(pstate, _union->switch_type_spec, &visitor, user_data)))
    return ret;
  if ((ret = idl_visit(pstate, _union->cases, &visitor, user_data)))
    return ret;

  /* comparison operators */

  fmt = "  bool operator==(const %s& _other) const\n"
        "  {\n"
        "    if (_d() != _other._d()) return false;\n"
        "    switch (_d()) {\n";

  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.visit = IDL_CASE | IDL_CASE_LABEL;
  visitor.accept[IDL_ACCEPT_CASE] = emit_case_comparison;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = emit_case_label;
  if ((ret = idl_visit(pstate, _union->cases, &visitor, user_data)))
    return ret;

  fmt = "    }\n"
        "    return false;\n"
        "  }\n\n"
        "  bool operator!=(const %s& _other) const\n"
        "  {\n"
        "    return !(*this == _other);\n"
        "  }\n\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;


  /* implicit default setter */
  if (idl_mask(_union->default_case) == IDL_IMPLICIT_DEFAULT_CASE_LABEL) {
    if (_union->unused_labels > 1)
      fmt = "  void _default(%1$s d = %2$ss)\n"
            "  {\n"
            "    if (!_is_compatible_discriminator(d, %2$s))\n"
            "      return;\n";
    else
      fmt = "  void _default()\n"
            "  {\n"
            "    const %1$s d = %2$s;\n";
    if (idl_fprintf(gen->header.handle, fmt, type, value) < 0)
      return IDL_RETCODE_NO_MEMORY;
    fmt = "    m__d = d;\n"
          "  }\n\n";
    if (fputs(fmt, gen->header.handle) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  if (fputs("};\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

idl_retcode_t
generate_types(const idl_pstate_t *pstate, struct generator *generator)
{
  idl_visitor_t visitor;
  const char *sources[] = { NULL, NULL };

  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_CONST | IDL_TYPEDEF | IDL_STRUCT | IDL_MODULE | IDL_ENUM | IDL_UNION;
  visitor.accept[IDL_ACCEPT_CONST] = &emit_const;
  visitor.accept[IDL_ACCEPT_TYPEDEF] = &emit_typedef;
  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_struct;
  visitor.accept[IDL_ACCEPT_UNION] = &emit_union;
  visitor.accept[IDL_ACCEPT_ENUM] = &emit_enum;
  visitor.accept[IDL_ACCEPT_MODULE] = &emit_module;
  assert(pstate->sources);
  sources[0] = pstate->sources->path->name;
  visitor.sources = sources;

  return idl_visit(pstate, pstate->root, &visitor, generator);
}
