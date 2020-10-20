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

//default (C++ STL) container definitions
#define CPP11_SEQUENCE_TEMPLATE     "std::vector"
#define CPP11_SEQUENCE_INCLUDE      "<vector>"

#define CPP11_ARRAY_TEMPLATE        "std::array"
#define CPP11_ARRAY_INCLUDE         "<array>"

#define CPP11_STRING_TEMPLATE       "std::string"
#define CPP11_STRING_INCLUDE        "<string>"

#define CPP11_UNION_TEMPLATE        "std::variant"
#define CPP11_UNION_GETTER_TEMPLATE "std::get"
#define CPP11_UNION_INCLUDE         "<variant>"

//replace the definitions below (CPP11_...) with your own custom classes and the includes necessary
//only the custom class names need be defined, the template parameters will be done by idlc
//N.B.: replacing one definition usually requires all definitions in that group to be replaced

//sequence templates
//unbounded sequences require a template class taking a single typename
//E.G. std::vector<std::string>
#define IDLCXX_SEQUENCE_UNBOUNDED_TEMPLATE  CPP11_SEQUENCE_TEMPLATE
#define IDLCXX_SEQUENCE_UNBOUNDED_INCLUDE   CPP11_SEQUENCE_INCLUDE
//bounded sequences require a template class taking a single typename and a single size
//E.G. custom_bounded_vector<custom_class,255>
#define IDLCXX_SEQUENCE_BOUNDED_TEMPLATE    CPP11_SEQUENCE_TEMPLATE
#define IDLCXX_SEQUENCE_BOUNDED_INCLUDE     CPP11_SEQUENCE_INCLUDE

//array templates
//arrays require a template class taking a with a single typename and a single size
//E.G. custom_array<custom_class,16>
#define IDLCXX_ARRAY_TEMPLATE        CPP11_ARRAY_TEMPLATE
#define IDLCXX_ARRAY_INCLUDE         CPP11_ARRAY_INCLUDE

//string templates
//unbounded strings require just a class name
//E.G. std::string
#define IDLCXX_STRING_UNBOUNDED_TEMPLATE  CPP11_STRING_TEMPLATE
#define IDLCXX_STRING_UNBOUNDED_INCLUDE   CPP11_STRING_INCLUDE
//bounded strings require a template class with a single size
//E.G. custom_bounded_string<127>
#define IDLCXX_STRING_BOUNDED_TEMPLATE    CPP11_STRING_TEMPLATE
#define IDLCXX_STRING_BOUNDED_INCLUDE     CPP11_STRING_INCLUDE

//union templates
#define IDLCXX_UNION_TEMPLATE        CPP11_UNION_TEMPLATE
#define IDLCXX_UNION_GETTER_TEMPLATE CPP11_UNION_GETTER_TEMPLATE
#define IDLCXX_UNION_INCLUDE         CPP11_UNION_INCLUDE

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

