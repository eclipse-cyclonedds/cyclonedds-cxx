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

#include "idlcxx/backend.h"

#define CPP11_SEQUENCE_TEMPLATE     "std::vector"
#define CPP11_SEQUENCE_INCLUDE      "<vector>"

#define CPP11_ARRAY_TEMPLATE        "std::array"
#define CPP11_ARRAY_INCLUDE         "<array>"

#define CPP11_STRING_TEMPLATE       "std::string"
#define CPP11_WSTRING_TEMPLATE      "std::wstring"
#define CPP11_STRING_INCLUDE        "<string>"

#define CPP11_UNION_TEMPLATE        "std::variant"
#define CPP11_UNION_GETTER_TEMPLATE "std::get"
#define CPP11_UNION_INCLUDE         "<variant>"

char*
get_cpp11_name(const char* name);

char *
get_cpp11_type(const idl_node_t *node);

char *
get_cpp11_fully_scoped_name(const idl_node_t *node);

char *
get_default_value(idl_backend_ctx ctx, const idl_node_t *node);

char *
get_cpp11_const_value(const idl_constval_t *literal);

char *
get_cpp11_literal_value(const idl_literal_t *literal);

