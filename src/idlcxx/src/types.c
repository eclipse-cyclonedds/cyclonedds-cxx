// Copyright(c) 2021 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 
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
  const char *name, *fmt, *eofmt = NULL;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (((const idl_member_t *)idl_parent(node))->try_construct.annotation != NULL) {
    idl_warning(pstate, IDL_WARN_UNSUPPORTED_ANNOTATIONS, idl_location(node),
      "The @try_construct annotation is not supported yet in the C++ generator, the default try-construct behavior will be used");
  }

  if (idl_is_array(node))
    type_spec = node;
  else
    type_spec = idl_type_spec(node);

  const idl_member_t *mem = idl_parent(node);
  assert(idl_is_member(mem));

  name = get_cpp11_name(node);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  type_spec = idl_strip(type_spec, IDL_STRIP_ALIASES | IDL_STRIP_FORWARD);
  value = NULL;
  if (idl_is_array(type_spec)) {
    value = "{ }";
  } else if (idl_is_enum(type_spec) || idl_is_base_type(type_spec) || idl_is_string(type_spec) || idl_is_bitmask(type_spec)) {
    if (mem->value.annotation) {
      if ((idl_is_base_type(type_spec) || idl_is_string(type_spec) || idl_is_bitmask(type_spec)) && IDL_PRINTA(&value, get_cpp11_value, mem->value.value, gen) < 0)
        return IDL_RETCODE_NO_MEMORY;
      else if (idl_is_enum(type_spec))
        return IDL_RETCODE_UNSUPPORTED;  //implement writing enum value
    } else if (!idl_is_string(type_spec) && IDL_PRINTA(&value, get_cpp11_default_value, type_spec, gen) < 0) {
      return IDL_RETCODE_NO_MEMORY;
    }
  }

  fmt = " %1$s %2$s_%3$s%4$s;\n";
  if (is_optional(node) || is_external(node)) {
    fmt = " %5$s<%1$s> %2$s_%3$s%4$s;\n";
    value = NULL;
    eofmt = is_external(node) ? gen->external_format : gen->optional_format;
  }

  if (idl_fprintf(gen->header.handle, fmt, type, name, value ? " = " : "", value ? value : "", eofmt) < 0)
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
  const char *name, *fmt, *sep, *eofmt = NULL;
  const idl_type_spec_t *type_spec;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (idl_is_array(node))
    type_spec = node;
  else
    type_spec = idl_type_spec(node);

  simple = idl_mask(idl_strip(type_spec, IDL_STRIP_ALIASES | IDL_STRIP_FORWARD)) & (IDL_BASE_TYPE|IDL_ENUM);
  sep = is_first(node) ? "" : ",\n";
  if (is_optional(node) || is_external(node)) {
    fmt = "%1$s    const %4$s<%2$s>& %3$s";
    eofmt = is_external(node) ? gen->external_format : gen->optional_format;
  } else {
    fmt = simple ? "%1$s    %2$s %3$s"
                 : "%1$s    const %2$s& %3$s";
  }
  name = get_cpp11_name(node);
  if (IDL_PRINTA(&type, get_cpp11_type, type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, fmt, sep, type, name, eofmt) < 0)
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
  const char *name, *fmt, *eofmt = NULL;

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

  type_spec = idl_strip(type_spec, IDL_STRIP_ALIASES | IDL_STRIP_FORWARD);
  if (is_optional(node) || is_external(node)) {
    fmt = "  const %3$s<%1$s>& %2$s() const { return this->%2$s_; }\n"
          "  %3$s<%1$s>& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(const %3$s<%1$s>& _val_) { this->%2$s_ = _val_; }\n"
          "  void %2$s(%3$s<%1$s>&& _val_) { this->%2$s_ = _val_; }\n";
    eofmt = is_external(node) ? gen->external_format : gen->optional_format;
  } else if (idl_is_base_type(type_spec) || idl_is_enum(type_spec))
    fmt = "  %1$s %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(%1$s _val_) { this->%2$s_ = _val_; }\n";
  else
    fmt = "  const %1$s& %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(const %1$s& _val_) { this->%2$s_ = _val_; }\n"
          "  void %2$s(%1$s&& _val_) { this->%2$s_ = _val_; }\n";

  if (idl_fprintf(gen->header.handle, fmt, type, name, eofmt) < 0)
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
        "    (void) _other;\n"
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
    value = enumerator->value.value;
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
emit_bitmask(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  (void)pstate;
  (void)path;
  (void)revisit;

  struct generator *gen = user_data;
  const idl_bitmask_t *bm = node;
  const idl_bit_value_t *bv = NULL;
  uint16_t bit_bound = bm->bit_bound.value;

  /*convert largest bit position to sizes of native int types*/
  if (bit_bound > 32)
    bit_bound = 64;
  else if (bit_bound > 16)
    bit_bound = 32;
  else if (bit_bound > 8)
    bit_bound = 16;
  else
    bit_bound = 8;

  static const char *open_bitmask =
    "enum %sBits {\n",
                    *bitmask_entry =
    "  %s = UINT%u_C(0x1) << %u%s\n",
                    *close_bitmask =
    "};\n"
    "typedef uint%u_t %s;\n\n";

  if (idl_fprintf(gen->header.handle, open_bitmask, bm->name->identifier) < 0)
    return IDL_RETCODE_NO_MEMORY;

  IDL_FOREACH(bv, bm->bit_values) {
    if (idl_fprintf(gen->header.handle, bitmask_entry, bv->name->identifier, bit_bound, bv->position.value, bv->node.next ? "," : "") < 0)
      return IDL_RETCODE_NO_MEMORY;
  };

  if (idl_fprintf(gen->header.handle, close_bitmask, bit_bound, bm->name->identifier) < 0)
    return IDL_RETCODE_NO_MEMORY;

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

  if (((const idl_case_t *)node)->try_construct.annotation != NULL) {
    idl_warning(pstate, IDL_WARN_UNSUPPORTED_ANNOTATIONS, idl_location(node),
      "The @try_construct annotation is not supported yet in the C++ generator, the default try-construct behavior will be used");
  }

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
  bool simple;
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
  if (IDL_PRINTA(&type, get_cpp11_type, branch, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&discr_type, get_cpp11_type, _union->switch_type_spec->type_spec, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&value, get_cpp11_value, branch->labels->const_expr, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  simple = (idl_mask(idl_strip(branch->type_spec, IDL_STRIP_ALIASES | IDL_STRIP_FORWARD)) & (IDL_BASE_TYPE|IDL_ENUM)) != 0;

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
  fmt = "  void %1$s(%2$s&& u, %3$s d = %4$s)\n"
        "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discr_type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t is_same_type(
  const idl_pstate_t *pstate,
  const idl_type_spec_t *type_a,
  const idl_declarator_t *decl_a,
  const idl_type_spec_t *type_b,
  const idl_declarator_t *decl_b,
  bool *outval)
{
  assert(pstate);
  assert(type_a);
  assert(type_b);
  assert(outval);

  /* container for array sizes in nested arrays
     255 should be more than enough nested arrays*/
  uint32_t a_sz[255] = {0};
  size_t i = 0;
  const idl_const_expr_t *ce = NULL;
  if (decl_a) {
    IDL_FOREACH(ce, decl_a->const_expr) {
      a_sz[i++] = ((const idl_literal_t*)ce)->value.uint32;
      if (i >= 255) {
        idl_error(pstate, idl_location(decl_a),
          "@maximum array depth exceeded in type comparison");
        return IDL_RETCODE_NO_MEMORY;
      }
    }
  }

  while (idl_is_alias(type_a) || idl_is_forward(type_a)) {
    if (idl_is_forward(type_a)) {
      type_a = ((const idl_forward_t *)type_a)->type_spec;
    } else if (idl_is_alias(type_a)) {
      const idl_typedef_t *td = idl_parent(type_a);
      type_a = td->type_spec;
      decl_a = td->declarators;
      IDL_FOREACH(ce, decl_a->const_expr) {
        a_sz[i++] = ((const idl_literal_t*)ce)->value.uint32;
        if (i >= 255) {
          idl_error(pstate, idl_location(decl_a),
            "@maximum array depth exceeded in type comparison");
          return IDL_RETCODE_NO_MEMORY;
        }
      }
    }
  }

  size_t j = 0;
  *outval = false;
  /* check that the array sizes in the declarators match up */
  if (decl_b) {
    IDL_FOREACH(ce, decl_b->const_expr) {
      if (j >= i || a_sz[j++] != ((const idl_literal_t*)ce)->value.uint32)
        return IDL_RETCODE_OK;
    }
  }

  while (idl_is_alias(type_b) || idl_is_forward(type_b)) {
    if (idl_is_forward(type_b)) {
      type_b = ((const idl_forward_t *)type_b)->type_spec;
    } else if (idl_is_alias(type_b)) {
      const idl_typedef_t *td = idl_parent(type_b);
      type_b = td->type_spec;
      decl_b = td->declarators;
      IDL_FOREACH(ce, decl_b->const_expr) {
        if (j >= i || a_sz[j++] != ((const idl_literal_t*)ce)->value.uint32)
          return IDL_RETCODE_OK;
      }
    }
  }

  /* check that we compared the same number of array sizes */
  if (i != j)
    return IDL_RETCODE_OK;

  /* unalias bitmasks to underlying integer types */
  idl_type_t t_a = idl_is_bitmask(type_a) ? unalias_bitmask(type_a) : idl_type(type_a),
             t_b = idl_is_bitmask(type_b) ? unalias_bitmask(type_b) : idl_type(type_b);

  switch (t_a) {
    case IDL_SEQUENCE:
      if (t_b == IDL_SEQUENCE) {
        const idl_sequence_t *seq_a = type_a,
                             *seq_b = type_b;
        type_a = seq_a->type_spec;
        type_b = seq_b->type_spec;
        decl_a = NULL;
        decl_b = NULL;
        if (idl_is_alias(type_a)) {
          const idl_typedef_t *td_a = idl_parent(type_a);
          decl_a = td_a->declarators;
          type_a = td_a->type_spec;
        } else if (idl_is_forward(type_a)) {
          type_a = ((const idl_forward_t *)type_a)->type_spec;
        }
        if (idl_is_alias(type_b)) {
          const idl_typedef_t *td_b = idl_parent(type_b);
          decl_b = td_b->declarators;
          type_b = td_b->type_spec;
        } else if (idl_is_forward(type_b)) {
          type_b = ((const idl_forward_t *)type_b)->type_spec;
        }
        return is_same_type(pstate, type_a, decl_a, type_b, decl_b, outval);
      }
      break;
    case IDL_STRING:
      if (t_b == IDL_STRING)
        *outval = ((const idl_string_t *)type_a)->maximum == ((const idl_string_t *)type_b)->maximum;
      break;
    case IDL_STRUCT:
    case IDL_UNION:
    case IDL_ENUM:
      *outval = (type_a == type_b);
      break;
    /* in the IDL - C++ binding integer types (long, short, etc.) are aliases of fixed width integers
       and their comparison should treat them interchangeably */
    case IDL_OCTET:
    case IDL_UINT8:
      *outval = (t_b == IDL_OCTET || t_b == IDL_UINT8);
      break;
    case IDL_INT16:
    case IDL_SHORT:
        *outval = (t_b == IDL_SHORT || t_b == IDL_INT16);
        break;
    case IDL_UINT16:
    case IDL_USHORT:
        *outval = (t_b == IDL_USHORT || t_b == IDL_UINT16);
        break;
    case IDL_INT32:
    case IDL_LONG:
        *outval = (t_b == IDL_LONG || t_b == IDL_INT32);
        break;
    case IDL_UINT32:
    case IDL_ULONG:
        *outval = (t_b == IDL_ULONG || t_b == IDL_UINT32);
        break;
    case IDL_INT64:
    case IDL_LLONG:
        *outval = (t_b == IDL_LLONG || t_b == IDL_INT64);
        break;
    case IDL_UINT64:
    case IDL_ULLONG:
        *outval = (t_b == IDL_ULLONG || t_b == IDL_UINT64);
        break;
    case IDL_WSTRING:
    case IDL_FIXED_PT:
      return IDL_RETCODE_UNSUPPORTED;
    case IDL_NULL:
    case IDL_TYPEDEF:
    case IDL_BITMASK:
      /*should be unaliased in previous steps*/
      assert(0);
    default:
      *outval = (t_a == t_b);
  }

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
  idl_retcode_t ret = IDL_RETCODE_OK;
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

  /*open union definition*/
  fmt = "class %1$s\n"
        "{\n"
        "private:\n"
        "  %2$s m__d;\n\n"
        "  %3$s<";
  if (idl_fprintf(gen->header.handle, fmt, name, type, gen->union_format) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /*count number of cases*/
  size_t ncases = 0;
  const idl_case_t *_case = NULL;
  IDL_FOREACH(_case, _union->cases) {
    ncases++;
  }

  const idl_case_t **cases = NULL;
  if (ncases) {
    cases = malloc(sizeof(const idl_case_t *)*ncases);
    if (!cases)
      return IDL_RETCODE_NO_MEMORY;
    memset(((void*)cases),0x0,sizeof(const idl_case_t *)*ncases);
  }

  size_t i = 0;
  /*deduplicate types in cases*/
  IDL_FOREACH(_case, _union->cases) {
    bool type_already_present = false;
    for (size_t j = 0; j < i && !type_already_present; j++) {
      assert(cases[j]);
      if (!cases[j])
        ret = IDL_RETCODE_BAD_PARAMETER;
      else
        ret = is_same_type(pstate, cases[j]->type_spec, cases[j]->declarator, _case->type_spec, _case->declarator, &type_already_present);
      if (ret != IDL_RETCODE_OK)
        goto cleanup;
    }

    if (!type_already_present)
      cases[i++] = _case;
  }

  /*print deduplicated types in variant holder*/
  for (size_t j = 0; j < i && ret == IDL_RETCODE_OK; j++) {
    const char *sep = j ? ", " : "";
    char *variant_type = NULL;
    /*suppress error C6263 (using _alloca in a loop, danger of stack overflow)
      other solutions will be more complex and error sensitive*/
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 6263 )
#endif
    if (IDL_PRINTA(&variant_type, get_cpp11_type, cases[j], gen) < 0)
      ret = IDL_RETCODE_NO_MEMORY;
#ifdef _WIN32
#pragma warning ( pop )
#endif
    if (idl_fprintf(gen->header.handle, "%s%s", sep, variant_type) < 0)
      ret = IDL_RETCODE_NO_MEMORY;
  }
cleanup:
  free((void*)cases);
  if (ret)
    return ret;

  if (idl_fprintf(gen->header.handle, "> m__u;\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  /*add default discriminator and validation*/
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
    const idl_case_t* default_case = idl_parent(_union->default_case);
    char *default_type = NULL;
    if (IDL_PRINTA(&default_type, get_cpp11_type, default_case, gen) < 0)
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
    fmt = "  void _default(%1$s d = %2$s)\n"
          "  {\n"
          "    if (!_is_compatible_discriminator(d, %2$s))\n"
          "      throw dds::core::InvalidArgumentError(\n"
          "        \"Discriminator does not match default branch\");\n"
          "    m__d = d;\n"
          "  }\n\n";
    if (idl_fprintf(gen->header.handle, fmt, type, value) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  if (fputs("};\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_forward(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  (void)pstate;
  (void)revisit;
  (void)path;

  struct generator *gen = user_data;
  assert(idl_is_forward(node));
  const char *name = get_cpp11_name(node);

  const char *fmt = "class %1$s;\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

idl_retcode_t
generate_types(const idl_pstate_t *pstate, struct generator *generator)
{
  idl_visitor_t visitor;
  const char *sources[] = { NULL, NULL };

  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_CONST | IDL_TYPEDEF | IDL_STRUCT | IDL_MODULE | IDL_ENUM | IDL_UNION | IDL_BITMASK | IDL_FORWARD;
  visitor.accept[IDL_ACCEPT_CONST] = &emit_const;
  visitor.accept[IDL_ACCEPT_TYPEDEF] = &emit_typedef;
  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_struct;
  visitor.accept[IDL_ACCEPT_UNION] = &emit_union;
  visitor.accept[IDL_ACCEPT_ENUM] = &emit_enum;
  visitor.accept[IDL_ACCEPT_MODULE] = &emit_module;
  visitor.accept[IDL_ACCEPT_BITMASK] = &emit_bitmask;
  visitor.accept[IDL_ACCEPT_FORWARD] = &emit_forward;
  assert(pstate->sources);
  sources[0] = pstate->sources->path->name;
  visitor.sources = sources;

  return idl_visit(pstate, pstate->root, &visitor, generator);
}
