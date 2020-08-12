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

static const char* idl_type[] = {
  "char",
  "octet",
  "boolean",
  "short",
  "long",
  "long long",
  "unsigned short",
  "unsigned long",
  "unsigned long long",
  "float",
  "double"
};

static const char* cxx_type[] = {
  "char",
  "uint8_t",
  "bool",
  "int16_t",
  "int32_t",
  "int64_t",
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "float",
  "double"
};

static int cxx_width[] = {
  1,
  1,
  1,
  2,
  4,
  8,
  2,
  4,
  8,
  4,
  8
};


static const char* struct_fmt =
"struct s { %s mem;\n};\n";

static const char* cxx_write_func_fmt_1 =
"size_t write_struct(const s &obj, void *data, size_t position)\n{\n"\
"  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n"\
"  position += 1;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";
static const char* cxx_write_size_func_fmt_1 =
"size_t write_size(const s &obj, size_t offset)\n"\
"{\n"\
"  size_t position = offset;\n"\
"  position += 1;  //bytes for member: mem\n"\
"  return position-offset;\n"\
"}\n\n";
static const char* cxx_read_func_fmt_1 =
"size_t read_struct(s &obj, void *data, size_t position)\n"\
"{\n"\
"  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n"\
"  position += 1;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";

static const char* cxx_write_func_fmt_2 =
"size_t write_struct(const s &obj, void *data, size_t position)\n"\
"{\n"\
"  size_t alignmentbytes = position&0x1;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n"\
"  position += 2;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";
static const char* cxx_write_size_func_fmt_2 =
"size_t write_size(const s &obj, size_t offset)\n"\
"{\n"\
"  size_t position = offset;\n"\
"  position += position&0x1;  //alignment\n"\
"  position += 2;  //bytes for member: mem\n"\
"  return position-offset;\n"\
"}\n\n";
static const char* cxx_read_func_fmt_2 =
"size_t read_struct(s &obj, void *data, size_t position)\n"\
"{\n"\
"  position += position&0x1;  //alignment\n"\
"  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n"\
"  position += 2;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";

static const char* cxx_write_func_fmt_4 =
"size_t write_struct(const s &obj, void *data, size_t position)\n"\
"{\n"\
"  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";
static const char* cxx_write_size_func_fmt_4 =
"size_t write_size(const s &obj, size_t offset)\n"\
"{\n"\
"  size_t position = offset;\n"\
"  position += (4 - position&0x3)&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: mem\n"\
"  return position-offset;\n"\
"}\n\n";
static const char* cxx_read_func_fmt_4 =
"size_t read_struct(s &obj, void *data, size_t position)\n"\
"{\n"\
"  position += (4 - position&0x3)&0x3;  //alignment\n"\
"  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";

static const char* cxx_write_func_fmt_8 =
"size_t write_struct(const s &obj, void *data, size_t position)\n"\
"{\n"\
"  size_t alignmentbytes = (8 - position&0x7)&0x7;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n"\
"  position += 8;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";
static const char* cxx_write_size_func_fmt_8 =
"size_t write_size(const s &obj, size_t offset)\n"\
"{\n"\
"  size_t position = offset;\n"\
"  position += (8 - position&0x7)&0x7;  //alignment\n"\
"  position += 8;  //bytes for member: mem\n"\
"  return position-offset;\n"\
"}\n\n";
static const char* cxx_read_func_fmt_8 =
"size_t read_struct(s &obj, void *data, size_t position)\n"\
"{\n"\
"  position += (8 - position&0x7)&0x7;  //alignment\n"\
"  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n"\
"  position += 8;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n";

#define HNS "size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"size_t write_size(const s &obj, size_t offset);\n\n"\
"size_t read_struct(s &obj, void *data, size_t position);\n\n"

void test_bare(int n)
{
  idl_tree_t* tree = 0x0;

  char buffer[1024] = { 0x0 };
  sprintf(buffer, struct_fmt, idl_type[n]);
  idl_parse_string(buffer, 0x0, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  switch (cxx_width[n])
  {
  case 1:
    format_ostream(impl, cxx_write_func_fmt_1, cxx_type[n]);
    format_ostream(impl, cxx_write_size_func_fmt_1);
    format_ostream(impl, cxx_read_func_fmt_1, cxx_type[n]);
    break;
  case 2:
    format_ostream(impl, cxx_write_func_fmt_2, cxx_type[n]);
    format_ostream(impl, cxx_write_size_func_fmt_2);
    format_ostream(impl, cxx_read_func_fmt_2, cxx_type[n]);
    break;
  case 4:
    format_ostream(impl, cxx_write_func_fmt_4, cxx_type[n]);
    format_ostream(impl, cxx_write_size_func_fmt_4);
    format_ostream(impl, cxx_read_func_fmt_4, cxx_type[n]);
    break;
  case 8:
    format_ostream(impl, cxx_write_func_fmt_8, cxx_type[n]);
    format_ostream(impl, cxx_write_size_func_fmt_8, cxx_type[n]);
    format_ostream(impl, cxx_read_func_fmt_8, cxx_type[n]);
    break;
  default:
    CU_ASSERT_FATAL(0);
  }

  CU_ASSERT_STRING_EQUAL(HNS, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));

  /*
  printf("=========generated============\n%s\n============================\n", get_idl_streamer_header_buf(generated));
  printf("=========tested============\n%s\n============================\n", get_ostream_buffer(impl));
  for (int i = 0; i < get_ostream_buffer(impl) && i < strlen(get_ostream_buffer(get_idl_streamer_impl_buf(generated))); i++)
  {
    char c1 = get_ostream_buffer(impl)[i], c2 = get_ostream_buffer(get_idl_streamer_impl_buf(generated))[i];
    if (c1 != c2) printf("%d: %d: buf: '%c' (%.x) - tree: '%c' (%.x) => %s\n", n, i, c1, c1, c2, c2, c1 != c2 ? "X" : "O");
  }
  */

  destruct_idl_ostream(impl);

  destruct_idl_streamer_output(generated);
}

CU_Test(streamer_generator, base_types_no_namespace)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(const char*); i++)
    test_bare(i);
  CU_PASS("base_types_no_namespace");
}
