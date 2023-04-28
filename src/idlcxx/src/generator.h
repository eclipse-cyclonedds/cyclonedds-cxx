// Copyright(c) 2021 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include "idlc/generator.h"

struct generator {
  const char *path;
  char *array_format;
  const char *array_include;
  char *sequence_format;
  const char *sequence_include;
  char *bounded_sequence_format;
  const char *bounded_sequence_include;
  char *string_format;
  const char *string_include;
  char *bounded_string_format;
  const char *bounded_string_include;
  char *optional_format;
  const char *optional_include;
  char *external_format;
  const char *external_include;
  char *union_format;
  char *union_getter_format;
  const char *union_include;
  bool uses_integers;
  bool uses_array;
  bool uses_sequence;
  bool uses_bounded_sequence;
  bool uses_string;
  bool uses_bounded_string;
  bool uses_union;
  bool uses_optional;
  bool uses_external;
  struct {
    FILE *handle;
    char *path;
  } header;
  struct {
    FILE *handle;
    char *path;
  } impl;
  const idlc_generator_config_t *config;
};

const char *get_cpp11_name(const void *);

int get_cpp11_type(
  char *str, size_t size, const void *node, void *user_data);

int get_cpp11_fully_scoped_name(
  char *str, size_t size, const void *node, void *user_data);

int get_cpp11_name_typedef(
  char *str, size_t size, const void *node, void *user_data);

int get_cpp11_base_type_const_value(
  char *str, size_t size, const void *node, void *user_data);

int get_cpp11_default_value(
  char *str, size_t size, const void *node, void *user_data);

int get_cpp11_value(
  char *str, size_t size, const void *node, void *user_data);

bool is_optional(
  const void *node);

bool is_external(
  const void *node);

bool must_understand(
  const void *node);

bool is_nested(
  const void *node);

bool is_selfcontained(
  const void *node);

idl_extensibility_t get_extensibility(
  const void *node);

idl_type_t unalias_bitmask(
  const idl_node_t *node);

idl_retcode_t
generate_streamers(const idl_pstate_t *pstate, struct generator *generator);

idl_retcode_t
generate_traits(const idl_pstate_t *pstate, struct generator *generator);

idl_retcode_t
generate_types(const idl_pstate_t *pstate, struct generator *generator);

#endif /* GENERATOR_H */
