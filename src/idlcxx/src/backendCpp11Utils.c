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

#ifdef _WIN32
#pragma warning(disable : 4996)
#endif

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
  static const idl_mask_t mask = (IDL_BASE_TYPE|(IDL_BASE_TYPE-1));

  switch (node->mask & mask)
  {
  case IDL_CHAR:
    return idl_strdup("char");
  case IDL_WCHAR:
    return idl_strdup("wchar");
  case IDL_BOOL:
    return idl_strdup("bool");
  case IDL_INT8:
    return idl_strdup("int8_t");
  case IDL_UINT8:
  case IDL_OCTET:
    return idl_strdup("uint8_t");
  case IDL_INT16:
    return idl_strdup("int16_t");
  case IDL_UINT16:
    return idl_strdup("uint16_t");
  case IDL_INT32:
    return idl_strdup("int32_t");
  case IDL_UINT32:
    return idl_strdup("uint32_t");
  case IDL_INT64:
    return idl_strdup("int64_t");
  case IDL_UINT64:
    return idl_strdup("uint64_t");
  case IDL_FLOAT:
    return idl_strdup("float");
  case IDL_DOUBLE:
    return idl_strdup("double");
  case IDL_LDOUBLE:
    return idl_strdup("long double");
  default:
    assert(0);
    break;
  }
  return NULL;
}

static char *
get_cpp11_templ_type(const idl_node_t *node)
{
  char *cpp11Type = NULL;

  switch (node->mask & IDL_TEMPL_TYPE_MASK)
  {
  case IDL_SEQUENCE:
    {
      uint64_t bound = ((const idl_sequence_t*)node)->maximum;
      char* vector_element = get_cpp11_type(((const idl_sequence_t*)node)->type_spec);
      if (bound)
        idl_asprintf(&cpp11Type, CPP11_BOUNDED_SEQUENCE_TEMPLATE(vector_element, bound));
      else
        idl_asprintf(&cpp11Type, CPP11_SEQUENCE_TEMPLATE(vector_element));
      free(vector_element);
    }
    break;
  case IDL_STRING:
    {
      uint64_t bound = ((const idl_string_t*)node)->maximum;
      if (bound)
        idl_asprintf(&cpp11Type, CPP11_BOUNDED_STRING_TEMPLATE(bound));
      else
        idl_asprintf(&cpp11Type, CPP11_STRING_TEMPLATE());
    }
    break;
  case IDL_WSTRING:
    //currently not supported
    assert(0);
    break;
  case IDL_FIXED_PT:
    //currently not supported
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
  assert(node->mask & (IDL_BASE_TYPE|IDL_TEMPL_TYPE|IDL_CONSTR_TYPE|IDL_TYPEDEF));
  if (idl_is_base_type(node))
    return get_cpp11_base_type(node);
  else if (idl_is_templ_type(node))
    return get_cpp11_templ_type(node);
  else
    return get_cpp11_fully_scoped_name(node);
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
    scope_type = current_node->mask & (IDL_MODULE | IDL_ENUMERATOR | IDL_ENUM | IDL_STRUCT | IDL_UNION | IDL_TYPEDEF);
    assert(scope_type);
    switch (scope_type)
    {
    case IDL_ENUMERATOR:
      scope_names[i] = get_cpp11_name(idl_identifier(current_node));
      break;
    case IDL_ENUM:
      scope_names[i] = get_cpp11_name(idl_identifier(current_node));
      break;
    case IDL_MODULE:
      scope_names[i] = get_cpp11_name(idl_identifier(current_node));
      break;
    case IDL_STRUCT:
      scope_names[i] = get_cpp11_name(idl_identifier(current_node));
      break;
    case IDL_UNION:
      scope_names[i] = get_cpp11_name(idl_identifier(current_node));
      break;
    case IDL_TYPEDEF:
      scope_names[i] = get_cpp11_name(idl_identifier(((const idl_typedef_t *)current_node)->declarators));
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
  static const idl_mask_t mask = (IDL_BASE_TYPE|(IDL_BASE_TYPE-1));
  (void)ctx;

  if (idl_is_enum(node))
    return get_cpp11_fully_scoped_name((idl_node_t*)((idl_enum_t*)node)->enumerators);

  switch (node->mask & mask)
  {
  case IDL_BOOL:
    return idl_strdup("false");
  case IDL_CHAR:
  case IDL_WCHAR:
  case IDL_OCTET:
    return idl_strdup("0");
  case IDL_FLOAT:
    return idl_strdup("0.0f");
  case IDL_DOUBLE:
  case IDL_LDOUBLE:
    return idl_strdup("0.0");
  case IDL_INT8:
  case IDL_UINT8:
  case IDL_INT16:
  case IDL_UINT16:
  case IDL_INT32:
  case IDL_UINT32:
  case IDL_INT64:
  case IDL_UINT64:
    return idl_strdup("0");
  default:
    return NULL;
  }
}

static char *
get_cpp11_base_type_const_value(const idl_constval_t *constval)
{
  int cnt = -1;
  char *str = NULL;
  static const idl_mask_t mask = (IDL_BASE_TYPE_MASK|(IDL_BASE_TYPE_MASK-1));

  switch (constval->node.mask & mask)
  {
  case IDL_BOOL:
    return idl_strdup(constval->value.bln ? "true" : "false");
  case IDL_OCTET:
    cnt = idl_asprintf(&str, "%" PRIu8, constval->value.oct);
    break;
  case IDL_INT8:
    cnt = idl_asprintf(&str, "%" PRId8, constval->value.int8);
    break;
  case IDL_UINT8:
    cnt = idl_asprintf(&str, "%" PRIu8, constval->value.uint8);
    break;
  case IDL_INT16:
    cnt = idl_asprintf(&str, "%" PRId16, constval->value.int16);
    break;
  case IDL_UINT16:
    cnt = idl_asprintf(&str, "%" PRIu16, constval->value.uint16);
    break;
  case IDL_INT32:
    cnt = idl_asprintf(&str, "%" PRId32, constval->value.int32);
    break;
  case IDL_UINT32:
    cnt = idl_asprintf(&str, "%" PRIu32, constval->value.uint32);
    break;
  case IDL_INT64:
    cnt = idl_asprintf(&str, "%" PRId64, constval->value.int64);
    break;
  case IDL_UINT64:
    cnt = idl_asprintf(&str, "%" PRIu64, constval->value.uint64);
    break;
  case IDL_FLOAT:
    cnt = idl_asprintf(&str, "%.6f", constval->value.flt);
    break;
  case IDL_DOUBLE:
    cnt = idl_asprintf(&str, "%f", constval->value.dbl);
    break;
  case IDL_LDOUBLE:
    cnt = idl_asprintf(&str, "%Lf", constval->value.ldbl);
    break;
  default:
    assert(0);
    break;
  }

  return cnt >= 0 ? str : NULL;
}

static char *
get_cpp11_templ_type_const_value(const idl_constval_t *constval)
{
  char *str;
  size_t len;

  if (!idl_is_masked(constval, IDL_STRING))
    return NULL;
  assert(constval->value.str);
  len = strlen(constval->value.str);
  if (!(str = malloc(len + 2 /* quotes */ + 1)))
    return NULL;
  str[0] = '"';
  memcpy(str + 1, constval->value.str, len);
  str[1 + len] = '"';
  str[1 + len + 1] = '\0';
  return str;
}

char *
get_cpp11_const_value(const idl_constval_t *constval)
{
  static const idl_mask_t mask = IDL_BASE_TYPE | IDL_TEMPL_TYPE | IDL_ENUMERATOR;

  switch (constval->node.mask & mask) {
  case IDL_BASE_TYPE:
    return get_cpp11_base_type_const_value(constval);
  case IDL_TEMPL_TYPE:
    return get_cpp11_templ_type_const_value(constval);
  case IDL_ENUMERATOR:
    return get_cpp11_fully_scoped_name((const idl_node_t *)constval);
  default:
    assert(0);
    break;
  }
  return NULL;
}
