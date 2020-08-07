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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "idlcxx/streamer_generator.h"
#include "idl/processor.h"

#include "CUnit/Theory.h"

static char* input = "module M\n"\
"{\n"\
"	struct E {\n"\
"		long L;\n"\
"   float f;\n"\
"	};\n"\
"	\n"\
"	struct F {\n"\
"   long double d;\n"\
"		char c;\n"\
"		E e;\n"\
"	};\n"\
" module N {\n"\
"  struct G {\n"\
"    char g_c;\n"\
"  };\n"\
" };\n"\
"};\n"\
" module O {\n"\
"  struct H {\n"\
"    unsigned long h_i;\n"\
"  };\n"\
" };\n";

CU_Test(streamer_generator, streamer_1)
{
  idl_tree_t* tree = 0x0;
  idl_parse_string(input, 0x0, &tree);

  CU_PASS("streamer_1");
}
