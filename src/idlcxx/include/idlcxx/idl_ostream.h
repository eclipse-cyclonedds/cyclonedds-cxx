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
#ifndef IDL_OSTREAM_H
#define IDL_OSTREAM_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "idlcxx/export.h"

typedef struct idl_ostream idl_ostream_t;

#define IDL_OSTREAM_BUFFER_INCR 4096

IDLCXX_EXPORT idl_ostream_t* create_idl_ostream(FILE* file);

IDLCXX_EXPORT char* get_ostream_buffer(const idl_ostream_t* str);

IDLCXX_EXPORT size_t get_ostream_buffer_size(const idl_ostream_t* str);

IDLCXX_EXPORT size_t get_ostream_buffer_position(const idl_ostream_t* str);

IDLCXX_EXPORT FILE* get_ostream_file(idl_ostream_t* str);

IDLCXX_EXPORT void destruct_idl_ostream(idl_ostream_t* ostr);

IDLCXX_EXPORT void format_ostream(idl_ostream_t* ostr, const char* fmt, ...);

IDLCXX_EXPORT size_t transfer_ostream_buffer(idl_ostream_t* from, idl_ostream_t* to);

IDLCXX_EXPORT size_t flush_ostream(idl_ostream_t* str);

#endif /* IDL_OSTREAM_H */
