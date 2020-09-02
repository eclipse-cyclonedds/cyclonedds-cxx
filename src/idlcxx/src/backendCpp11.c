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

#include "strdup.h"

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
get_cpp_name(const char* name)
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
