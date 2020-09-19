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

#ifndef IDL_BACKEND_H
#define IDL_BACKEND_H

#include <stdio.h>
#include <stdarg.h>
#include "idl/tree.h"
#include "idl/retcode.h"
#include "idl/export.h"
#include "idlcxx/idl_ostream.h"

#define IDL_BASE_TYPE_MASK (IDL_INTEGER_TYPE | IDL_FLOATING_PT_TYPE)
#define IDL_INTEGER_MASK (IDL_INTEGER_TYPE | 0x0F)
#define IDL_INTEGER_MASK_IGNORE_SIGN (IDL_INTEGER_TYPE | 0x0E)
#define IDL_FLOAT_MASK (IDL_FLOAT | IDL_DOUBLE | IDL_LDOUBLE)
#define IDL_BASE_OTHERS_MASK (IDL_BASE_TYPE | 0x07)

#define IDL_TEMPL_TYPE_MASK (IDL_SEQUENCE | IDL_STRING | IDL_WSTRING | IDL_FIXED_PT)
#define IDL_CONSTR_TYPE_MASK (IDL_CONSTR_TYPE | IDL_STRUCT | IDL_UNION | IDL_ENUM)
#define IDL_CATEGORY_MASK (IDL_BASE_TYPE | IDL_TEMPL_TYPE | IDL_MODULE | IDL_STRUCT |\
            IDL_UNION | IDL_ENUM | IDL_TYPEDEF | IDL_CONST | IDL_MEMBER | IDL_DECLARATOR | IDL_CASE)

typedef struct idl_file_out_s {
  FILE *file;
} *idl_file_out;

struct idl_backend_ctx_s;
typedef struct idl_backend_ctx_s *idl_backend_ctx;

idl_backend_ctx
IDLCXX_EXPORT idl_backend_context_new(uint32_t indent_size, const char *target_file, void *custom_context);

idl_retcode_t
IDLCXX_EXPORT idl_backend_context_free(idl_backend_ctx ctx);

idl_retcode_t
IDLCXX_EXPORT idl_file_out_new(idl_backend_ctx ctx, const char *file_name);

idl_retcode_t
IDLCXX_EXPORT idl_file_out_new_membuf(idl_backend_ctx ctx, char *mem_buf, size_t buf_size);

void
IDLCXX_EXPORT idl_file_out_close(idl_backend_ctx ctx);

idl_ostream_t
IDLCXX_EXPORT *idl_get_output_stream(idl_backend_ctx ctx);

void
IDLCXX_EXPORT idl_indent_incr(idl_backend_ctx ctx);

void
IDLCXX_EXPORT idl_indent_double_incr(idl_backend_ctx ctx);

void
IDLCXX_EXPORT idl_indent_decr(idl_backend_ctx ctx);

void
IDLCXX_EXPORT idl_indent_double_decr(idl_backend_ctx ctx);

void
IDLCXX_EXPORT *idl_get_custom_context(idl_backend_ctx ctx);

void
IDLCXX_EXPORT idl_reset_custom_context(idl_backend_ctx ctx);

idl_retcode_t
IDLCXX_EXPORT idl_set_custom_context(idl_backend_ctx ctx, void *custom_context);

void
IDLCXX_EXPORT idl_file_out_printf (
    idl_backend_ctx ctx,
    const char *format,
    ...);

void
IDLCXX_EXPORT idl_file_out_printf_no_indent (
    idl_backend_ctx ctx,
    const char *format,
    ...);

bool
IDLCXX_EXPORT idl_is_reference(const idl_node_t *node);

bool
IDLCXX_EXPORT idl_declarator_is_array(const idl_declarator_t *node);

bool
IDLCXX_EXPORT idl_declarator_is_primitive(const idl_declarator_t *declarator);

#define IDL_MASK_ALL 0xffffffff

typedef uint32_t idl_walkResult;

typedef idl_retcode_t
(idl_walkAction)(idl_backend_ctx ctx, const idl_node_t *node);

idl_retcode_t
IDLCXX_EXPORT idl_walk_node_list(idl_backend_ctx ctx, const idl_node_t *starting_node, idl_walkAction, idl_mask_t mask);

idl_retcode_t
IDLCXX_EXPORT idl_walk_tree(idl_backend_ctx ctx, const idl_node_t *starting_node, idl_walkAction, idl_mask_t mask);

typedef struct idl_include idl_include_t;
struct idl_include {
  idl_include_t *next;
  bool indirect;
  idl_file_t *file;
};

IDLCXX_EXPORT idl_include_t *
idl_get_include_list(idl_backend_ctx ctx, const idl_tree_t *tree);

#if 0
idl_retcode_t
IDLCXX_EXPORT idl_backendGenerate(idl_backend_ctx ctx, const idl_tree_t *parse_tree);
#endif

#endif /* IDL_BACKEND_H */

