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
    type_spec = idl_unalias(idl_type_spec(node), 0u);

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
  type_spec = idl_type_spec(node);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (idl_mask(type_spec) & IDL_BASE_TYPE)
    fmt = "  %1$s %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(%1$s _val_) { this->%2$s_ = _val_; }\n";
  else
    fmt = "  const %1$s %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(const %1$s _val_) { this->%2$s_ = _val_; }\n"
          "  void %2$s(%1$s&& _val_) { this->%2$s_ = _val_; }\n";

  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
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
  if ((ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

  /* constructors */
  fmt = "\n"
        "public:\n"
        "  %1$s() = default;\n\n"
        "  explicit %1$s(\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* constructor parameters */
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_parameter;
  if ((ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

  if (fputs(") :\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* constructor initializers */
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_member_initializer;
  if ((ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

  if (fputs(" { }\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* getters and setters */
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &emit_member_methods;
  if ((ret = idl_visit(pstate, _struct->members, &visitor, user_data)))
    return ret;

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
emit_typedef(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator *gen = user_data;
  char *type;
  const char *name;
  const idl_typedef_t *_typedef = (const idl_typedef_t *)node;
  const idl_declarator_t *declarator;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (IDL_PRINTA(&type, get_cpp11_type, _typedef->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  IDL_FOREACH(declarator, _typedef->declarators) {
    name = get_cpp11_name(declarator);
    if (idl_fprintf(gen->header.handle, "typedef %s %s;", type, name) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
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

typedef enum
{
  idl_no_dep                = 0x0,
  idl_string_bounded_dep    = 0x01 << 0,
  idl_string_unbounded_dep  = 0x01 << 1,
  idl_array_dep             = 0x01 << 2,
  idl_vector_bounded_dep    = 0x01 << 3,
  idl_vector_unbounded_dep  = 0x01 << 4,
  idl_variant_dep           = 0x01 << 5,
  idl_optional_dep          = 0x01 << 6,
  idl_all_dep               = (idl_optional_dep*2-1)
} idl_include_dep;

static idl_retcode_t
emit_variant(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const char *name, *sep;
  const idl_case_t *branch = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  sep = !idl_previous(branch) ? "" : ", ";
  name = get_cpp11_name(branch->declarator);
  if (idl_fprintf(gen->header.handle, "%s%s", sep, name) < 0)
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
  if (idl_fprintf(gen->header.handle, "        return %s;\n") < 0)
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
  if (idl_fprintf(gen->header.handle, "      case %s:\n", value) < 0)
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
  const char *type, *fmt;
  const idl_type_spec_t *type_spec;

  (void)pstate;
  (void)revisit;
  (void)path;

  type_spec = ((const idl_switch_type_spec_t *)node)->type_spec;
  type = get_cpp11_name(type_spec);

  fmt = "  %1$s _d() const\n"
        "  {\n"
        "    return m__d;\n"
        "  }\n\n"
        "  void _d(%1$s d)\n"
        "  {\n"
        "    if (!_is_compatible_discriminator(m__d, d)) {\n"
        "      throw dds::core::InvalidArgumentError("
        "        \"Discriminator value does not match current discriminator\");\n"
        "    }\n"
        "    m__d = d;\n"
        "  }\n\n";
  if (idl_fprintf(gen->header.handle, fmt, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_branch_methods(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  bool simple, single;
  char *type, *value, *discriminator = "foobar";
  const idl_case_t *branch = node;
  const char *name, *fmt;

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
    "    if (!_is_compatible_discriminator(m__d, d)) {\n"
    "      throw dds::core::InvalidArgumentError(\n"
    "        \"Discriminator does not match current discriminator\");\n"
    "    }\n"
    "    m__d = d;\n"
    "    m__u = u;\n"
    "  }\n\n";

  name = get_cpp11_name(branch->declarator);
  if (IDL_PRINTA(&type, get_cpp11_type, branch->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&value, get_cpp11_value, branch->labels->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  simple = (idl_mask(branch->type_spec) & IDL_BASE_TYPE) != 0;
  single = (idl_degree(branch->labels) == 1);

  /* const-getter */
  fmt = simple ? "  %1$s %2$s() const\n"
               : "  const %1$s &%2$s() const\n";
  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, getter, value, "std::variant", type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* ref-getter */
  fmt = "  %1$s& %2$s()\n";
  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, getter, value, "std::variant", type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* setter */
  if (single)
    fmt = simple ? "  void %1$s(%2$s _u)\n"
                   "  {\n"
                   "    const %3$s d = %4$s;\n"
                 : "  void %1$s(const %2$s& _u)\n"
                   "  {\n"
                   "    const %3$s d = %4$s;\n";
  else
    fmt = simple ? "  void %1$s(%2$s u, %3$s d = %4$s)\n"
                   "  {\n"
                 : "  void %1$s(const %2$s& _u, %3$s d = %4$s)\n"
                   "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discriminator, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (simple)
    return IDL_RETCODE_OK;

  /* setter with move semantics */
  fmt = single ? "  void %1$s(%2$s&& _u)\n"
                 "  {\n"
                 "    const %3$s d = %4$s\n"
               : "  void %1$s(%2$s&& _u, %3$s d = %4$s)\n"
                 "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discriminator, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter) < 0)
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

  _union = node;
  type_spec = _union->switch_type_spec->type_spec;

  name = get_cpp11_name(_union);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&value, get_cpp11_value, _union->default_discriminator.const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  fmt = "class %1$d\n"
        "{\n"
        "  private:\n"
        "    %2$s m__d;\n\n"
        "    std::variant<";
  if (idl_fprintf(gen->header.handle, fmt, name, type) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* variant */
  visitor.visit = IDL_CASE;
  visitor.accept[IDL_ACCEPT_CASE] = emit_variant;
  if ((ret = idl_visit(pstate, _union->branches, &visitor, user_data)))
    return ret;
  fmt = "> m__u;\n\n";
  if (idl_fprintf(gen->header.handle, fmt) < 0)
    return ret;

  /**/
  fmt = "  %1$s _default_discriminator = %2$s;\n\n"
        "  %1$s _is_discriminator(%1$s d)\n"
        "  {\n"
        "    switch (d) {\n";
  if (idl_fprintf(gen->header.handle, fmt, type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.visit = IDL_CASE | IDL_CASE_LABEL;
  visitor.accept[IDL_ACCEPT_CASE] = emit_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = emit_case_label;
  if ((ret = idl_visit(pstate, _union->branches, &visitor, user_data)))
    return ret;
  /* generate default case */
  if (_union->default_discriminator.condition != IDL_FIRST_DISCRIMINANT) {
    fmt = "      default:\n"
          "        break;\n";
    if (fputs(fmt, gen->header.handle) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  fmt = "    }\n"
        "    return _default_discriminator;\n"
        "  }\n\n"
        "  bool _is_compatible_discriminator(%1$s d1, %2$s d2)\n"
        "  {\n"
        "    return _is_discriminator(d1) == _is_discriminator(d2);\n"
        "  }\n\n";
  if (idl_fprintf(gen->header.handle, fmt, type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* contructor */
  fmt = "public:\n"
        "  %1$s() :\n"
        "      m__d(%2$s)";
  if (idl_fprintf(gen->header.handle, fmt, name, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (_union->default_discriminator.condition != IDL_IMPLICIT_DEFAULT) {
    const idl_case_t *branch = _union->default_discriminator.branch;

    /* set value explicitly if no default case exists */
    if (_union->default_discriminator.condition == IDL_DEFAULT_CASE)
      name = "m__u()";
    else
      name = get_cpp11_name(branch->declarator);
    fmt = ",\n"
          "      %1$s()";
    if (idl_fprintf(gen->header.handle, fmt, name) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }
  if (fputs(" { }\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* getters and setters */
  visitor.visit = IDL_SWITCH_TYPE_SPEC | IDL_CASE;
  visitor.accept[IDL_ACCEPT_SWITCH_TYPE_SPEC] = emit_discriminator_methods;
  visitor.accept[IDL_ACCEPT_CASE] = emit_branch_methods;
  if ((ret = idl_visit(pstate, _union->branches, &visitor, user_data)))
    return ret;

  /* implicit default setter */
  if (idl_mask(_union->default_discriminator.branch) == IDL_IMPLICIT_DEFAULT) {
    // FIXME: implement
    //if (_union->unused_labels)
    //  fmt = "  void _default(%s d = %s)\n"
    //        "  {\n";
    //else
      fmt = "  void _default()\n"
            "  {\n"
            "    const %s d = %s;\n";
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
  if (pstate->sources)
    sources[0] = pstate->sources->path->name;
  visitor.sources = sources;

  return idl_visit(pstate, pstate->root, &visitor, generator);
}
