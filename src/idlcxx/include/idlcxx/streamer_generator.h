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
#ifndef IDL_STREAMER_GENERATOR_H
#define IDL_STREAMER_GENERATOR_H

#include <stdarg.h>
#include <stdio.h>

#include "idl/tree.h"
#include "idl/retcode.h"
#include "idl/idl_ostream.h"

typedef struct idl_streamer_output idl_streamer_output_t;

IDL_EXPORT idl_streamer_output_t* create_idl_streamer_output(void);

IDL_EXPORT idl_ostream_t* get_idl_streamer_impl_buf(const idl_streamer_output_t* str);

IDL_EXPORT idl_ostream_t* get_idl_streamer_header_buf(const idl_streamer_output_t* str);

IDL_EXPORT void destruct_idl_streamer_output(idl_streamer_output_t* str);

IDL_EXPORT void idl_streamers_generate(idl_tree_t* tree, idl_streamer_output_t* str);

#endif
