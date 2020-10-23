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
#include <inttypes.h>
#include "idlcxx/backend.h"

// replace the definitions below (CPP11_...) with your own custom classes and includes.

#define CPP11_SEQUENCE_TEMPLATE(type)                 "std::vector<%s>", type
#define CPP11_SEQUENCE_INCLUDE                        "<vector>"

// default (C++ STL) container definitions
// bounded sequences require a template class taking a single typename and a single size
// E.G. custom_bounded_vector<custom_class,255>
//#define CPP11_BOUNDED_SEQUENCE_TEMPLATE(type, bound)  "non_std::vector<%s, %" PRIu64 ">", type, bound
#define CPP11_BOUNDED_SEQUENCE_TEMPLATE(type, bound)  "std::vector<%s>", type
#define CPP11_BOUNDED_SEQUENCE_INCLUDE                "<vector>"

// array templates
// arrays require a template class taking a with a single typename and a single size
// E.G. custom_array<custom_class,16>
#define CPP11_ARRAY_TEMPLATE(element, const_expr)     "std::array<%s, %s>", element, const_expr
#define CPP11_ARRAY_INCLUDE                           "<array>"

// string templates
// unbounded strings require just a class name
// E.G. std::string
#define CPP11_STRING_TEMPLATE()                       "std::string"
#define CPP11_STRING_INCLUDE                          "<string>"

// bounded strings require a template class with a single size
// E.G. custom_bounded_string<127>
//#define CPP11_BOUNDED_STRING_TEMPLATE(bound)          "non_std::string<%" PRIu64 ">", bound
#define CPP11_BOUNDED_STRING_TEMPLATE(bound)          "std::string"
#define CPP11_BOUNDED_STRING_INCLUDE                  "<string>"

#define CPP11_UNION_TEMPLATE                          "std::variant"
#define CPP11_UNION_GETTER_TEMPLATE                   "std::get"
#define CPP11_UNION_INCLUDE                           "<variant>"

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

