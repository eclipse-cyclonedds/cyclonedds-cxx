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
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include "idl/string.h"
#include "idlcxx/backendCpp11Utils.h"

/* Specify a list of all C++11 keywords */
static const char* cpp11_keywords[] =
{
  /* QAC EXPECT 5007; Bypass qactools error */
  "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand",
  "bitor", "bool", "break", "case", "catch", "char", "char16_t",
  "char32_t", "class", "compl", "concept", "const", "constexpr",
  "const_cast", "continue", "decltype", "default", "delete",
  "do", "double", "dynamic_cast", "else", "enum", "explicit",
  "export", "extern", "false", "float", "for", "friend",
  "goto", "if", "inline", "int", "long", "mutable",
  "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
  "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
  "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
  "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
  "true", "try", "typedef", "typeid", "typename", "union", "unsigned",
  "using", "virtual", "void", "volatile", "wchar_t", "while",
  "xor", "xor_eq",
  "int16_t", "int32_t", "int64_t",
  "uint8_t", "uint16_t", "uint32_t", "uint64_t",
};

char*
get_cpp11_name(const char* name)
{
  char* cpp11Name;
  size_t i;

  /* search through the C++ keyword list */
  for (i = 0; i < sizeof(cpp11_keywords) / sizeof(char*); i++) {
    if (strcmp(cpp11_keywords[i], name) == 0) {
      /* If a keyword matches the specified identifier, prepend _cxx_ */
      /* QAC EXPECT 5007; will not use wrapper */
      size_t cpp11NameLen = strlen(name) + 5 + 1;
      cpp11Name = malloc(cpp11NameLen);
      snprintf(cpp11Name, cpp11NameLen, "_cxx_%s", name);
      return cpp11Name;
    }
  }
  /* No match with a keyword is found, thus return the identifier itself */
  cpp11Name = idl_strdup(name);
  return cpp11Name;
}

static char *
get_cpp11_base_type(const idl_node_t *node)
{
  char *cpp11Type = NULL;

  switch (node->mask & IDL_BASE_TYPE_MASK)
  {
  case IDL_INTEGER_TYPE:
    switch(node->mask & IDL_INTEGER_MASK_IGNORE_SIGN)
    {
    case IDL_INT8:
      cpp11Type = idl_strdup("int8_t");
      break;
    case IDL_INT16:
      cpp11Type = idl_strdup("int16_t");
      break;
    case IDL_INT32:
      cpp11Type = idl_strdup("int32_t");
      break;
    case IDL_INT64:
      cpp11Type = idl_strdup("int64_t");
      break;
    default:
      assert(0);
      break;
    }
    if (node->mask & IDL_UNSIGNED)
    {
      char *signedCpp11Type = cpp11Type;
      size_t unsignedCpp11TypeSize;

      assert(node->mask & (IDL_INT8 | IDL_INT16 | IDL_INT32 | IDL_INT64));
      unsignedCpp11TypeSize = strlen(signedCpp11Type) + 1 + 1;
      cpp11Type = malloc(unsignedCpp11TypeSize);
      snprintf(cpp11Type, unsignedCpp11TypeSize, "u%s", signedCpp11Type);
      free(signedCpp11Type);
    }
    break;
  case IDL_FLOATING_PT_TYPE:
    switch(node->mask & IDL_FLOAT_MASK)
    {
    case IDL_FLOAT:
      cpp11Type = idl_strdup("float");
      break;
    case IDL_DOUBLE:
      cpp11Type = idl_strdup("double");
      break;
    case IDL_LDOUBLE:
      cpp11Type = idl_strdup("long double");
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
      cpp11Type = idl_strdup("char");
      break;
    case IDL_WCHAR:
      cpp11Type = idl_strdup("wchar");
      break;
    case IDL_BOOL:
      cpp11Type = idl_strdup("bool");
      break;
    case IDL_OCTET:
      cpp11Type = idl_strdup("uint8_t");
      break;
    default:
      assert(0);
      break;
    }
    break;
  }
  return cpp11Type;
}

static char *
get_cpp11_templ_type(const idl_node_t *node)
{
  char *cpp11Type = NULL;
  const char *vector_tmplt;
  size_t vector_size;
  char *vector_element;

  switch (node->mask & IDL_TEMPL_TYPE_MASK)
  {
  case IDL_SEQUENCE:
    vector_tmplt = "std::vector<%s>";
    vector_element = get_cpp11_type(((const idl_sequence_t *) node)->type_spec);
    vector_size = strlen(vector_tmplt) + strlen(vector_element) - 2 /* Compensate for '%s' */ + 1 /* '\0' */;
    cpp11Type = malloc(vector_size);
    snprintf(cpp11Type, vector_size, vector_tmplt, vector_element);
    free(vector_element);
    break;
  case IDL_STRING:
    cpp11Type = idl_strdup("std::string");
    break;
  case IDL_WSTRING:
    cpp11Type = idl_strdup("std::wstring");
    break;
  case IDL_FIXED_PT:
    assert(0);
    break;
  default:
    assert(0);
    break;
  }

  return cpp11Type;
}

char *
get_cpp11_type(const idl_node_t *node)
{
  char *cpp11Type = NULL;

  switch (node->mask & IDL_CATEGORY_MASK) {
  case IDL_BASE_TYPE:
    cpp11Type = get_cpp11_base_type(node);
    break;
  case IDL_TEMPL_TYPE:
    cpp11Type = get_cpp11_templ_type(node);
    break;
  case IDL_CONSTR_TYPE:
  case IDL_TYPEDEF:
    cpp11Type = get_cpp11_fully_scoped_name(node);
    break;
  default:
    assert(0);
    break;
  }
  return cpp11Type;
}

char *
get_cpp11_fully_scoped_name(const idl_node_t *node)
{
  uint32_t nr_scopes = 0;
  size_t scoped_enumerator_len = 0;
  char *scoped_enumerator;
  char **scope_names;
  const idl_node_t *current_node = node;
  idl_mask_t scope_type;

  while (current_node) {
    ++nr_scopes;
    current_node = current_node->parent;
  }
  scope_names = malloc(sizeof(char *) * nr_scopes);
  current_node = node;
  for (uint32_t i = 0; i < nr_scopes; ++i)
  {
    scope_type = current_node->mask & (IDL_ENUMERATOR | IDL_ENUM | IDL_STRUCT | IDL_UNION | IDL_TYPEDEF);
    assert(scope_type);
    switch (scope_type)
    {
    case IDL_ENUMERATOR:
      scope_names[i] = get_cpp11_name(((const idl_enumerator_t *) current_node)->identifier);
      break;
    case IDL_ENUM:
      scope_names[i] = get_cpp11_name(((const idl_enum_t *) current_node)->identifier);
      break;
    case IDL_MODULE:
      scope_names[i] = get_cpp11_name(((const idl_module_t *) node)->identifier);
      break;
    case IDL_STRUCT:
      scope_names[i] = get_cpp11_name(((const idl_struct_t *) node)->identifier);
      break;
    case IDL_UNION:
      scope_names[i] = get_cpp11_name(((const idl_union_t *) node)->identifier);
      break;
    case IDL_TYPEDEF:
      scope_names[i] = get_cpp11_name(((const idl_typedef_t *) node)->declarators->identifier);
      break;
    }
    scoped_enumerator_len += (strlen(scope_names[i]) + 2); /* scope + "::" */
    current_node = current_node->parent;
  }
  scoped_enumerator = malloc(++scoped_enumerator_len); /* Add one for '\0' */
  scoped_enumerator[0] = '\0';
  while(nr_scopes)
  {
    strncat(scoped_enumerator, "::", scoped_enumerator_len);
    strncat(scoped_enumerator, scope_names[--nr_scopes], scoped_enumerator_len);
    free(scope_names[nr_scopes]);
  }
  free(scope_names);
  return scoped_enumerator;
}

char *
get_default_value(idl_backend_ctx ctx, const idl_node_t *node)
{
  char *def_value = NULL;
  (void)ctx;
  switch (node->mask & (IDL_BASE_TYPE | IDL_CONSTR_TYPE))
  {
  case IDL_BASE_TYPE:
    switch (node->mask & IDL_BASE_TYPE_MASK)
    {
    case IDL_INTEGER_TYPE:
      switch(node->mask & IDL_INTEGER_MASK_IGNORE_SIGN)
      {
      case IDL_INT8:
      case IDL_INT16:
      case IDL_INT32:
      case IDL_INT64:
        def_value = idl_strdup("0");
        break;
      default:
        assert(0);
        break;
      }
      break;
    case IDL_FLOATING_PT_TYPE:
      switch(node->mask & IDL_FLOAT_MASK)
      {
      case IDL_FLOAT:
        def_value = idl_strdup("0.0f");
        break;
      case IDL_DOUBLE:
      case IDL_LDOUBLE:
        def_value = idl_strdup("0.0");
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
      case IDL_WCHAR:
      case IDL_OCTET:
        def_value = idl_strdup("0");
        break;
      case IDL_BOOL:
        def_value = idl_strdup("false");
        break;
      default:
        assert(0);
        break;
      }
      break;
    }
    break;
  case IDL_CONSTR_TYPE:
    switch (node->mask & IDL_CONSTR_TYPE_MASK)
    {
    case IDL_ENUM:
    {
      /* Pick the first of the available enumerators. */
      const idl_enum_t *enumeration = (const idl_enum_t *) node;
      def_value = get_cpp11_fully_scoped_name((const idl_node_t *) enumeration->enumerators);
      break;
    }
    default:
      /* Other constructed types determine their default value in their constructor. */
      break;
    }
    break;
  default:
    /* Other types determine their default value in their constructor. */
    break;
  }
  return def_value;
}

static char *
get_cpp11_base_type_const_value(const idl_constval_t *variant)
{
  size_t const_value_len;
  char *const_value_str = NULL;

  switch (variant->node.mask & IDL_BASE_TYPE_MASK)
  {
  case IDL_INTEGER_TYPE:
    const_value_len = 24; /* Big enough for largest uint64_t */
    const_value_str = malloc(const_value_len);

    switch(variant->node.mask & IDL_INTEGER_MASK)
    {
    case IDL_UINT8:
      snprintf(const_value_str, const_value_len, "%hd", variant->value.oct);
      break;
    case IDL_INT32:
      snprintf(const_value_str, const_value_len, "%dL", variant->value.lng);
      break;
    case IDL_UINT32:
      snprintf(const_value_str, const_value_len, "%ulL", variant->value.ulng);
      break;
    case IDL_INT64:
      snprintf(const_value_str, const_value_len, "%" PRId64, variant->value.llng);
      break;
    case IDL_UINT64:
      snprintf(const_value_str, const_value_len, "%" PRIu64, variant->value.ullng);
      break;
    default:
      assert(0);
      break;
    }
    break;
  case IDL_FLOATING_PT_TYPE:
    switch(variant->node.mask & IDL_FLOAT_MASK)
    {
    case IDL_DOUBLE:
      const_value_len = 64; /* Big enough for largest double */
      const_value_str = malloc(const_value_len);
      snprintf(const_value_str, const_value_len, "%f", variant->value.dbl);
      break;
    case IDL_LDOUBLE:
      const_value_len = 128; /* Big enough for largest double */
      const_value_str = malloc(const_value_len);
      snprintf(const_value_str, const_value_len, "%Lf", variant->value.ldbl);
      break;
    default:
      assert(0);
      break;
    }
    break;
  default:
    assert(0);
    break;
  }

  return const_value_str;
}

static char *
get_cpp11_templ_type_const_value(const idl_constval_t *variant)
{
  char *const_value_str = NULL;

  switch (variant->node.mask & IDL_TEMPL_TYPE_MASK)
  {
  case IDL_STRING:
    const_value_str = idl_strdup(variant->value.str);
    break;
  default:
    assert(0);
    break;
  }

  return const_value_str;
}

char *
get_cpp11_const_value(const idl_constval_t *variant)
{
  char *const_value_str = NULL;

  assert(variant->node.mask & IDL_CONST);

  switch (variant->node.mask & IDL_CATEGORY_MASK) {
  case IDL_BASE_TYPE:
    const_value_str = get_cpp11_base_type_const_value(variant);
    break;
  case IDL_TEMPL_TYPE:
    const_value_str = get_cpp11_templ_type_const_value(variant);
    break;
  default:
    assert(0);
    break;
  }
  return const_value_str;
}
