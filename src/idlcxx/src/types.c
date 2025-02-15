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

#define VARIANT_ACCESS_BY_TYPE IDLCXX_USE_BOOST

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

  // ostream - cpp
  fmt = "  os << \"%1$s%2$s: \" << rhs.%2$s();\n";
  sep = is_first(node) ? "" : ", ";
  if (idl_fprintf(gen->impl.handle, fmt, sep, name) < 0)
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
          "  void %2$s(%3$s<%1$s>&& _val_) { this->%2$s_ = std::move(_val_); }\n";
    eofmt = is_external(node) ? gen->external_format : gen->optional_format;
  } else if (idl_is_base_type(type_spec) || idl_is_enum(type_spec))
    fmt = "  %1$s %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(%1$s _val_) { this->%2$s_ = _val_; }\n";
  else
    fmt = "  const %1$s& %2$s() const { return this->%2$s_; }\n"
          "  %1$s& %2$s() { return this->%2$s_; }\n"
          "  void %2$s(const %1$s& _val_) { this->%2$s_ = _val_; }\n"
          "  void %2$s(%1$s&& _val_) { this->%2$s_ = std::move(_val_); }\n";

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

  // ostream - cpp
  fmt = "std::ostream& operator<<(std::ostream& os, %s const& rhs)\n{\n"
        "  (void) rhs;\n"
        "  os << \"[\";\n";
  if (idl_fprintf(gen->impl.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

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
    
    // ostream cpp
    fmt = "  os << \"%2$s%1$s: \" << static_cast<const %1$s&>(rhs);\n";
    if (idl_fprintf(gen->impl.handle, fmt, base, _struct->members ? ", " : "") < 0)
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

  // ostream - hpp
  fmt = "std::ostream& operator<<(std::ostream& os, %s const& rhs);\n\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  // ostream - cpp
  if (fputs("  os << \"]\";\n  return os;\n}\n\n", gen->impl.handle) < 0)
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

  // ostream cpp
  fmt = "std::ostream& operator<<(std::ostream& os, %s const& rhs)\n{\n"
        "  (void) rhs;\n"
        "  switch (rhs)\n  {\n";
  if (idl_fprintf(gen->impl.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  const char* struct_name = get_cpp11_name(node);

  IDL_FOREACH(enumerator, _enum->enumerators) {
    name = get_cpp11_name(enumerator);
    value = enumerator->value.value;
    fmt = (value == skip) ? "%s%s" : "%s%s = %" PRIu32 "\n";
    if (idl_fprintf(gen->header.handle, fmt, sep, name, value) < 0)
      return IDL_RETCODE_NO_MEMORY;

    // ostream cpp
    fmt = "    case %2$s::%1$s:\n"
          "      os << \"%2$s::%1$s\"; break;\n";
    if (idl_fprintf(gen->impl.handle, fmt, name, struct_name) < 0)
      return IDL_RETCODE_NO_MEMORY;

    skip = value + 1;
    sep = ",\n  ";
  }

  if (fputs("};\n\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  // ostream cpp
  if (fputs("    default: break;\n  }\n  return os;\n}\n\n", gen->impl.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  // ostream hpp
  if (idl_fprintf(gen->header.handle, "std::ostream& operator<<(std::ostream& os, %s const& rhs);\n\n", struct_name) < 0)
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

  name = get_cpp11_name(node);
  if (revisit) {
    if (idl_fprintf(gen->header.handle, "} //namespace %s\n\n", name) < 0)
      return IDL_RETCODE_NO_MEMORY;
    
    // ostream cpp
    if (idl_fprintf(gen->impl.handle, "} //namespace %s\n\n", name) < 0)
      return IDL_RETCODE_NO_MEMORY;
  } else  {
    if (idl_fprintf(gen->header.handle, "namespace %s\n{\n", name) < 0)
      return IDL_RETCODE_NO_MEMORY;

    // ostream cpp
    if (idl_fprintf(gen->impl.handle, "namespace %s\n{\n", name) < 0)
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

static bool gen_ostream_case = false;

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

    // ostream cpp
    if (gen_ostream_case) {
      if (idl_fprintf(gen->impl.handle, "    default:\n") < 0)
        return IDL_RETCODE_NO_MEMORY;
    }

  } else {
    if (idl_fprintf(gen->header.handle, "      case %s:\n", value) < 0)
      return IDL_RETCODE_NO_MEMORY;

    // ostream cpp
    if (gen_ostream_case) {
      if (idl_fprintf(gen->impl.handle, "    case %s:\n", value) < 0)
        return IDL_RETCODE_NO_MEMORY;
    }
  }
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

  // ostream cpp
  static const char* fmt2 = "      os << \"%1$s: \" << rhs.%1$s(); break;\n";
  if (idl_fprintf(gen->impl.handle, fmt2, branch_name) < 0)
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
#if VARIANT_ACCESS_BY_TYPE
  char *accessor;
#else
  int accessor = -1;
#endif
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
#if VARIANT_ACCESS_BY_TYPE
    "    return %2$s<%3$s>(m__u);\n"
#else
    "    return %2$s<%3$d>(m__u);\n"
#endif
    "  }\n\n";

  static const char *setter =
    "    if (!_is_compatible_discriminator(%1$s, d)) {\n"
    "      throw dds::core::InvalidArgumentError(\n"
    "        \"Discriminator does not match current discriminator\");\n"
    "    }\n"
    "    m__d = d;\n"
#if VARIANT_ACCESS_BY_TYPE
    "    m__u = u;\n"
#else
    "    m__u.emplace<%2$d>(u);\n"
#endif
    "  }\n\n";

  name = get_cpp11_name(branch->declarator);
  if (IDL_PRINTA(&type, get_cpp11_type, branch, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
#if VARIANT_ACCESS_BY_TYPE
  accessor = type;
#else
  int i = 0;
  const idl_case_t *_case = NULL;
  IDL_FOREACH(_case, _union->cases) {
    if (_case == node) {
      accessor = i;
      break;
    }
    i++;
  }
#endif
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
  if (idl_fprintf(gen->header.handle, getter, value, gen->union_getter_format, accessor) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* ref-getter */
  fmt = "  %1$s& %2$s()\n  {\n";
  if (idl_fprintf(gen->header.handle, fmt, type, name) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, getter, value, gen->union_getter_format, accessor) < 0)
    return IDL_RETCODE_NO_MEMORY;

  /* setter */
  fmt = simple ? "  void %1$s(%2$s u, %3$s d = %4$s)\n"
                 "  {\n"
               : "  void %1$s(const %2$s& u, %3$s d = %4$s)\n"
                 "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discr_type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter, value, accessor) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (simple)
    return IDL_RETCODE_OK;

  /* setter with move semantics */
  fmt = "  void %1$s(%2$s&& u, %3$s d = %4$s)\n"
        "  {\n";
  if (idl_fprintf(gen->header.handle, fmt, name, type, discr_type, value) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (idl_fprintf(gen->header.handle, setter, value, accessor) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

#if VARIANT_ACCESS_BY_TYPE
/* to make sure no overlap exists with the IDL parser's type fields */
#define ARRAY_SZ_FIELD (0x1ull << 63)
#define BOUND_FIELD (ARRAY_SZ_FIELD >> 1)
/* this should be plenty for all practical applications */
#define SZ_MAX 1024
#define PUSH(field) if (tc->idx >= SZ_MAX) { return IDL_RETCODE_NO_MEMORY; } else { tc->info[tc->idx++] = field; }

typedef struct {
  uint64_t info[SZ_MAX];
  size_t idx;
  bool bstr_unique, bseq_unique;
} same_type_checker_t;

static idl_retcode_t push_array(const idl_declarator_t *decl, same_type_checker_t *tc)
{
  const idl_literal_t *literal = decl ? decl->const_expr : NULL;

  for (; literal; literal = idl_next(literal)) {
    if (literal->value.uint64 > 0) {
      PUSH(literal->value.uint64 | ARRAY_SZ_FIELD);
    }
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t push_type(const idl_type_spec_t *type, const idl_declarator_t *decl, same_type_checker_t *tc);

static idl_retcode_t push_sequence(const idl_sequence_t *seq, same_type_checker_t *tc)
{
  PUSH(IDL_SEQUENCE);

  /* if bounded sequence type is not unique, this must be skipped, since it is the same as unbounded*/
  if (tc->bseq_unique) {
    PUSH(idl_bound(seq) | BOUND_FIELD);
  }

  return push_type(seq->type_spec, NULL, tc);
}

static idl_retcode_t push_string(const idl_type_spec_t *type, same_type_checker_t *tc)
{
  PUSH(IDL_STRING);

  /* if bounded string type is not unique, this must be skipped, since it is the same as unbounded*/
  if (tc->bstr_unique) {
    PUSH(idl_bound(type) | BOUND_FIELD);
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t push_simple_type(const idl_type_spec_t *spec, same_type_checker_t *tc)
{
  idl_mask_t mask = idl_mask(spec) & ((IDL_BASE_TYPE << 1) - 1);
  switch (mask) { /* because C++ always translates integers to fixed width types */
    case IDL_OCTET:
      mask = IDL_UINT8;
      break;
    case IDL_SHORT:
    case IDL_USHORT:
      mask = IDL_INT16 | (mask & IDL_UNSIGNED);
      break;
    case IDL_LONG:
    case IDL_ULONG:
      mask = IDL_INT32 | (mask & IDL_UNSIGNED);
      break;
    case IDL_LLONG:
    case IDL_ULLONG:
      mask = IDL_INT64 | (mask & IDL_UNSIGNED);
      break;
  }

  PUSH(mask);
  return IDL_RETCODE_OK;
}

static idl_retcode_t push_bitmask(const idl_type_spec_t *type, same_type_checker_t *tc)
{
  const uint32_t bit_bound = idl_bound(type);

  /* bitmasks larger than 64 bits are caught by the parser */
  if (bit_bound >= 32) {
    PUSH(IDL_UINT64);
  } else if (bit_bound >= 16) {
    PUSH(IDL_UINT32);
  } else if (bit_bound >= 8) {
    PUSH(IDL_UINT16);
  } else {
    PUSH(IDL_UINT8);
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t push_constr_type(const idl_type_spec_t *spec, same_type_checker_t *tc)
{
  PUSH(idl_mask(spec) & IDL_TYPE_MASK);
  PUSH((uint64_t)spec);
  return IDL_RETCODE_OK;
}

static idl_retcode_t push_typedef(const idl_typedef_t *td, same_type_checker_t *tc)
{
  return push_type(td->type_spec, td->declarators, tc);
}

/* to unclutter the namespace */
#undef SZ_MAX
#undef ARRAY_SZ_FIELD
#undef BOUND_FIELD
#undef PUSH

idl_retcode_t push_type(const idl_type_spec_t *type, const idl_declarator_t *decl, same_type_checker_t *tc)
{
  idl_retcode_t ret = push_array(decl, tc);
  if (ret != IDL_RETCODE_OK) {
    return ret;
  } else if (idl_is_alias(type)) {
    return push_typedef((const idl_typedef_t *)idl_parent(type), tc);
  } else if (idl_is_sequence(type)) {
    return push_sequence((const idl_sequence_t *)type, tc);
  } else if (idl_is_string(type)) {
    return push_string(type, tc);
  } else if (idl_is_bitmask(type)) {
    return push_bitmask(type, tc);
  } else if (idl_is_constr_type(type)) {
    return push_constr_type(type, tc);
  } else if (idl_is_base_type(type)) {
    return push_simple_type(type, tc);
  } else {
    return IDL_RETCODE_BAD_PARAMETER;
  }
}

static idl_retcode_t is_same_type(
  const idl_pstate_t *pstate,
  const struct generator *gen,
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

  const bool bstr_unique = (strcmp(gen->bounded_string_format, gen->string_format) != 0),
             bseq_unique = (strcmp(gen->bounded_sequence_format, gen->sequence_format) != 0);
  same_type_checker_t tc_a = {.bstr_unique = bstr_unique, .bseq_unique = bseq_unique},
                      tc_b = {.bstr_unique = bstr_unique, .bseq_unique = bseq_unique};
  idl_retcode_t ret = push_type(type_a, decl_a, &tc_a);
  if (ret != IDL_RETCODE_OK)
    return ret;
  else
    ret = push_type(type_b, decl_b, &tc_b);

  if (ret != IDL_RETCODE_OK)
    return ret;
  else
    *outval = (tc_a.idx == tc_b.idx && memcmp(tc_a.info, tc_b.info, tc_a.idx*sizeof(uint64_t)) == 0);

  return IDL_RETCODE_OK;
}
#endif

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
  size_t default_case_index = 0;

  (void)revisit;
  (void)path;
  memset(&visitor, 0, sizeof(visitor));

  _union = node;
  /* check whether default label is associated with a union case */
  const bool gen_default = (idl_parent(_union->default_case) == _union);
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
  /*deduplicate types in cases
    variant access is by type for Boost, so there can only be one of each
    type in the variant*/
  IDL_FOREACH(_case, _union->cases) {
#if VARIANT_ACCESS_BY_TYPE
    bool type_already_present = false;
    for (size_t j = 0; j < i && !type_already_present; j++) {
      assert(cases[j]);
      if (!cases[j])
        ret = IDL_RETCODE_BAD_PARAMETER;
      else
        ret = is_same_type(pstate, gen, cases[j]->type_spec, cases[j]->declarator, _case->type_spec, _case->declarator, &type_already_present);
      if (ret != IDL_RETCODE_OK)
        goto cleanup;
    }

    if (!type_already_present)
      cases[i++] = _case;
#else
    if (_case == idl_parent(_union->default_case))
      default_case_index = i;
    cases[i++] = _case;
#endif
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
#if VARIANT_ACCESS_BY_TYPE
cleanup:
#endif
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

  // ostream cpp
  fmt = "std::ostream& operator<<(std::ostream& os, %1$s const& rhs)\n"
        "{\n"
        "  (void) rhs;\n"
        "  switch (rhs._d()) {\n";
  if (idl_fprintf(gen->impl.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.visit = IDL_CASE | IDL_CASE_LABEL;
  visitor.accept[IDL_ACCEPT_CASE] = emit_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = emit_case_label;
  if ((ret = idl_visit(pstate, _union->cases, &visitor, user_data)))
    return ret;

  if (gen_default) {
    fmt = "      default:\n"
          "        break;\n";
    if (idl_fprintf(gen->header.handle, fmt, type) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

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
#if VARIANT_ACCESS_BY_TYPE
    fmt = "%1$s()";
#else
    fmt = "std::in_place_index<%2$ld>, %1$s()";
#endif
    if (idl_fprintf(gen->header.handle, fmt, default_type, default_case_index) < 0)
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

  gen_ostream_case = true;
  visitor.visit = IDL_CASE | IDL_CASE_LABEL;
  visitor.accept[IDL_ACCEPT_CASE] = emit_case_comparison;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = emit_case_label;
  if ((ret = idl_visit(pstate, _union->cases, &visitor, user_data)))
    return ret;

  gen_ostream_case = false;

  if (gen_default) {
    fmt =  "      default:\n"
           "        return true;\n";
    if (idl_fprintf(gen->header.handle, fmt, name) < 0)
      return IDL_RETCODE_NO_MEMORY;

    fmt =  "    default:\n    {\n"
           "      // Prevent compiler warnings\n    }\n";
    if (idl_fprintf(gen->impl.handle, fmt, name) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  fmt = "    }\n"
        "    return true;\n"
        "  }\n\n"
        "  bool operator!=(const %s& _other) const\n"
        "  {\n"
        "    return !(*this == _other);\n"
        "  }\n\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  // ostream cpp
  if (idl_fprintf(gen->impl.handle, "  }\n  return os;\n}\n\n") < 0)
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

  // ostream hpp
  fmt = "std::ostream& operator<<(std::ostream& os, %s const& rhs);\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
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
