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

extern const unsigned char bounded_sequence_impl_cpp_in[];
extern const unsigned char bounded_sequence_of_structs_impl_cpp_in[];
extern const unsigned char bounded_string_impl_cpp_in[];
extern const unsigned char cross_call_impl_cpp_in[];
extern const unsigned char keys_base_impl_cpp_in[];
extern const unsigned char keys_struct_explicit_impl_cpp_in[];
extern const unsigned char keys_struct_implicit_impl_cpp_in[];
extern const unsigned char keys_typedef_impl_cpp_in[];
extern const unsigned char keys_typedef_header_cpp_in[];
extern const unsigned char keys_union_implicit_impl_cpp_in[];
extern const unsigned char sequence_recursive_header_cpp_in[];
extern const unsigned char sequence_recursive_impl_cpp_in[];
extern const unsigned char struct_inheritance_impl_cpp_in[];
extern const unsigned char typedef_resolution_header_cpp_in[];
extern const unsigned char typedef_resolution_impl_cpp_in[];

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

static const char* ses[] = {
  "_se0",
  "_se1",
  "_se2",
  "_se3"
};

static const char* als[] = {
  "_al0",
  "_al1",
  "_al2",
  "_al3"
};

static const char* is[] = {
  "_i0",
  "_i1",
  "_i2",
  "_i3"
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

static size_t cxx_width[] = {
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

static void format_ostream_indented(int depth, idl_ostream_t* ostr, const char* fmt, ...)
{
  if (depth > 0) format_ostream(ostr, "%*c", depth, ' ');
  va_list args;
  va_start(args, fmt);
  format_ostream_va_args(ostr, fmt, args);
  va_end(args);
}

void create_funcs_base(idl_ostream_t * ostr, size_t n, bool ns)
{
  const char* tn = cxx_type[n];
  const char* al = als[ns];
  size_t width = cxx_width[n];

  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  size_t %s = position&0x1;  //alignment\n",al);
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  size_t %s = (%d - (position&%#x))&%#x;  //alignment\n",al,width,width-1,width-1);
    }
    format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n",al);
    format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n",al);
  }
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<%s*>(static_cast<char*>(data)+position) = mem();  //writing bytes for member: mem()\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += %d;  //moving position indicator\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  position += position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  position += (%d - (position&%#x))&%#x;  //alignment\n",width,width-1,width-1);
    }
  }
  format_ostream_indented(ns * 2, ostr, "  position += %d;  //bytes for member: mem()\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += (%d - (position&%#x))&%#x;  //alignment\n", width, width - 1, width - 1);
    }
  }
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += %d;  //bytes for member: mem()\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  position += position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  position += (%d - (position&%#x))&%#x;  //alignment\n", width, width - 1, width - 1);
    }
  }
  format_ostream_indented(ns * 2, ostr, "  mem() = *reinterpret_cast<const %s*>(static_cast<const char*>(data)+position);  //reading bytes for member: mem()\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += %d;  //moving position indicator\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

void create_funcs_instance(idl_ostream_t* ostr, const char* in, bool ns)
{
  const char* al = als[ns];

  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t I::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n",al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n",al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n",al);
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<int32_t*>(static_cast<char*>(data)+position) = l();  //writing bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = %s().write_struct(data, position);\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = %s().write_size(position);\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += 4;  //bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position = mem().max_size(position);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool I::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  l() = *reinterpret_cast<const int32_t*>(static_cast<const char*>(data)+position);  //reading bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = %s().read_struct(data, position);\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }

}

void create_funcs_string(idl_ostream_t* ostr, const char* in, bool ns)
{
  const char* al = als[ns];

  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  const char* se = ses[ns];

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n",al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n",al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n",al);
  format_ostream_indented(ns * 2, ostr, "  uint32_t %s = static_cast<uint32_t>(%s().size()+1);  //number of entries in the sequence\n", se, in);
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<uint32_t*>(static_cast<char*>(data) + position) = %s;  //writing entries for member: %s()\n", se, in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy(static_cast<char*>(data)+position,%s().data(),%s*1); //writing bytes for member: %s()\n", in, se, in);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //entries of sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t %s = static_cast<uint32_t>(%s().size()+1);  //number of entries in the sequence\n", se, in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for sequence entries\n");
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //entries of sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)position;\n");
  format_ostream_indented(ns * 2, ostr, "  return UINT_MAX;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t %s = *reinterpret_cast<const uint32_t*>(static_cast<const char*>(data)+position);  //number of entries in the sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  %s().assign(static_cast<const char*>(data)+position,static_cast<const char*>(data)+position+%s-1); //reading bytes for member: %s()\n", in, se, in);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //entries of sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

void create_funcs_sequence(idl_ostream_t* ostr, const char* seq_name, size_t n, bool ns)
{
  size_t width = cxx_width[n];
  const char* cast = cxx_type[n];
  const char* se = ses[ns];
  const char* al = als[ns];

  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n",al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n", al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n", al);
  format_ostream_indented(ns * 2, ostr, "  uint32_t %s = static_cast<uint32_t>(%s().size());  //number of entries in the sequence\n", se, seq_name);
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<uint32_t*>(static_cast<char*>(data) + position) = %s;  //writing entries for member: %s()\n", se, seq_name);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  %s = (%d - (position&%#x))&%#x;  //alignment\n", al, width, width - 1, width - 1);
    format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n", al);
    format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n", al);
  }
  if (n == 2)
  {
    format_ostream_indented(ns * 2, ostr, "  for (size_t _b = 0; _b < %s; _b++) *(static_cast<char*>(data)+position++) = (%s()[_b] ? 0x1 : 0x0); //writing bytes for member: %s()\n", se, seq_name, seq_name);
  }
  else
  {
    format_ostream_indented(ns * 2, ostr, "  if (0 < %s().size()) memcpy(static_cast<char*>(data)+position,%s().data(),%s*%d); //writing bytes for member: %s()\n", seq_name, seq_name, se, width, seq_name);
    format_ostream_indented(ns * 2, ostr, "  position += %s*%d;  //entries of sequence\n", se, width);
  }
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t %s = static_cast<uint32_t>(%s().size());  //number of entries in the sequence\n", se, seq_name);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for sequence entries\n", seq_name);
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  position += (%d - (position&%#x))&%#x;  //alignment\n", width, width - 1, width - 1);
  }
  format_ostream_indented(ns * 2, ostr, "  position += %s*%d;  //entries of sequence\n", se, width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)position;\n");
  format_ostream_indented(ns * 2, ostr, "  return UINT_MAX;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t %s = *reinterpret_cast<const uint32_t*>(static_cast<const char*>(data)+position);  //number of entries in the sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  position += (%d - (position&%#x))&%#x;  //alignment\n", width, width - 1, width - 1);
  }
  if (n == 2)
  {
    format_ostream_indented(ns * 2, ostr, "  %s().resize(%s);\n", seq_name, se);
    format_ostream_indented(ns * 2, ostr, "  for (size_t _b = 0; _b < %s; _b++) %s()[_b] = (*(static_cast<const char*>(data)+position++) ? 0x1 : 0x0); //reading bytes for member: %s()\n", se, seq_name, seq_name);
  }
  else
  {
    format_ostream_indented(ns * 2, ostr, "  %s().assign(reinterpret_cast<const %s*>(static_cast<const char*>(data)+position),reinterpret_cast<const %s*>(static_cast<const char*>(data)+position)+%s); //reading bytes for member: %s()\n", seq_name, cast, cast, se, seq_name);
    format_ostream_indented(ns * 2, ostr, "  position += %s*%d;  //entries of sequence\n", se, width);
  }
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

void generate_union_funcs(idl_ostream_t* ostr, bool ns)
{
  const char* se = ses[ns+2];
  const char* al = als[ns];

  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n", al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n", al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n", al);
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<int32_t*>(static_cast<char*>(data)+position) = _d();  //writing bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (_d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *reinterpret_cast<uint8_t*>(static_cast<char*>(data)+position) = o();  //writing bytes for member: o()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *reinterpret_cast<int32_t*>(static_cast<char*>(data)+position) = l();  //writing bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t %s = static_cast<uint32_t>(str().size()+1);  //number of entries in the sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "      *reinterpret_cast<uint32_t*>(static_cast<char*>(data) + position) = %s;  //writing entries for member: str()\n", se);
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      memcpy(static_cast<char*>(data)+position,str().data(),%s*1); //writing bytes for member: str()\n", se);
  format_ostream_indented(ns * 2, ostr, "      position += %s;  //entries of sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *reinterpret_cast<float*>(static_cast<char*>(data)+position) = f();  //writing bytes for member: f()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  switch (_d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //bytes for member: o()\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t %s = static_cast<uint32_t>(str().size()+1);  //number of entries in the sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for sequence entries\n");
  format_ostream_indented(ns * 2, ostr, "      position += %s;  //entries of sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: f()\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)position;\n");
  format_ostream_indented(ns * 2, ostr, "  return UINT_MAX;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  clear();\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  clear();\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  _d() = *reinterpret_cast<const int32_t*>(static_cast<const char*>(data)+position);  //reading bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (_d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      o() = *reinterpret_cast<const uint8_t*>(static_cast<const char*>(data)+position);  //reading bytes for member: o()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      l() = *reinterpret_cast<const int32_t*>(static_cast<const char*>(data)+position);  //reading bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t %s = *reinterpret_cast<const uint32_t*>(static_cast<const char*>(data)+position);  //number of entries in the sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      str().assign(static_cast<const char*>(data)+position,static_cast<const char*>(data)+position+%s-1); //reading bytes for member: str()\n", se);
  format_ostream_indented(ns * 2, ostr, "      position += %s;  //entries of sequence\n", se);
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      f() = *reinterpret_cast<const float*>(static_cast<const char*>(data)+position);  //reading bytes for member: f()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

void generate_enum_funcs(idl_ostream_t* ostr, bool ns)
{
  const char* al = als[ns];
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n", al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n", al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n", al);
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<%sE*>(static_cast<char*>(data)+position) = mem();  //writing bytes for member: mem()\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += 4;  //bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  mem() = *reinterpret_cast<const %sE*>(static_cast<const char*>(data)+position);  //reading bytes for member: mem()\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }

}

void generate_array_base_funcs(idl_ostream_t* ostr, bool ns)
{
  const char* al = als[ns];
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n", al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n", al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n", al);
  format_ostream_indented(ns * 2, ostr, "  memcpy(static_cast<char*>(data)+position,mem().data(),24);  //writing bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<float*>(static_cast<char*>(data)+position) = mem2();  //writing bytes for member: mem2()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: mem2()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += 24;  //bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += 4;  //bytes for member: mem2()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy(mem().data(),static_cast<const char*>(data)+position,24);  //reading bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  mem2() = *reinterpret_cast<const float*>(static_cast<const char*>(data)+position);  //reading bytes for member: mem2()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}


void generate_array_instance_funcs(idl_ostream_t* ostr, bool ns)
{
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  const char* it = is[ns + 1];
  const char* al = als[ns];

  format_ostream_indented(ns * 2, ostr, "size_t I::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t %s = (4 - (position&0x3))&0x3;  //alignment\n", al);
  format_ostream_indented(ns * 2, ostr, "  memset(static_cast<char*>(data)+position,0x0,%s);  //setting alignment bytes to 0x0\n", al);
  format_ostream_indented(ns * 2, ostr, "  position += %s;  //moving position indicator\n", al);
  format_ostream_indented(ns * 2, ostr, "  *reinterpret_cast<int32_t*>(static_cast<char*>(data)+position) = l();  //writing bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t %s = 0; %s < 6; %s++)  {\n", it, it, it);
  format_ostream_indented(ns * 2, ostr, "    position = mem()[%s].write_struct(data, position);\n", it);
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  position = mem2().write_struct(data, position);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t %s = 0; %s < 6; %s++)  {\n", it, it, it);
  format_ostream_indented(ns * 2, ostr, "    position = mem()[%s].write_size(position);\n", it);
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  position = mem2().write_size(position);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += 4;  //bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t %s = 0; %s < 6; %s++)  {\n", it, it, it);
  format_ostream_indented(ns * 2, ostr, "    if (position != UINT_MAX)   position = mem()[%s].max_size(position);\n", it);
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position = mem2().max_size(position);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool I::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_write(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_write(buffer.data(),0);\n");
  format_ostream_indented(ns * 2, ostr, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
  format_ostream_indented(ns * 2, ostr, "  if (fptr == NULL)\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    if (key_max_size(0) <= 16)\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to unmodified function which just copies buffer into the keyhash\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    else\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      //bind to MD5 hash function\n");
  format_ostream_indented(ns * 2, ostr, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return (*fptr)(buffer,hash);\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_read(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  (void)data;\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t I::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  l() = *reinterpret_cast<const int32_t*>(static_cast<const char*>(data)+position);  //reading bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t %s = 0; %s < 6; %s++)  {\n", it, it, it);
  format_ostream_indented(ns * 2, ostr, "    position = mem()[%s].read_struct(data, position);\n", it);
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  position = mem2().read_struct(data, position);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

void test_base(size_t n, bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "struct s {\n"\
      "%s mem;\n"\
      "};\n"\
      "};\n",
      idl_type[n]);
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "struct s {\n"\
      "%s mem;\n"\
      "};\n",
      idl_type[n]);
  }
  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  create_funcs_base(impl, n, ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_instance(bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "struct I {\n"\
      "long l;\n"
      "};\n"
      "struct s {\n"\
      "I mem;\n"\
      "};\n"\
      "};\n");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "struct I {\n"\
      "long l;\n"
      "};\n"
      "struct s {\n"\
      "I mem;\n"\
      "};\n");
  }
  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  create_funcs_instance(impl, "mem", ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_string(bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "struct s {\n"\
      "string str;\n"\
      "};\n"\
      "};\n");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "struct s {\n"\
      "string str;\n"\
      "};\n");
  }
  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  create_funcs_string(impl, "str", ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_sequence(size_t n, bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "struct s {\n"\
      "sequence<%s> mem;\n"\
      "};\n"\
      "};\n",
      idl_type[n]);
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "struct s {\n"\
      "sequence<%s> mem;\n"\
      "};\n",
      idl_type[n]);
  }
  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  create_funcs_sequence(impl, "mem", n, ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_union(bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "union s switch (long) {\n"\
      "case 0:\n"\
      "case 1: octet o;\n"\
      "case 2:\n"\
      "case 3: long l;\n"\
      "case 4:\n"\
      "case 5: string str;\n"\
      "default: float f;\n"\
      "};\n"\
      "};\n");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "union s switch (long) {\n"\
      "case 0:\n"\
      "case 1: octet o;\n"\
      "case 2:\n"\
      "case 3: long l;\n"\
      "case 4:\n"\
      "case 5: string str;\n"\
      "default: float f;\n"\
      "};\n");
  }
  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);
  generate_union_funcs(impl, ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_enum(bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "enum E {e_0, e_1, e_2, e_3};\n"\
      "struct s {\n"\
      "E mem;\n"\
      "};"\
      "};\n");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "enum E {e_0, e_1, e_2, e_3};\n"\
      "struct s {\n"\
      "E mem;\n"\
      "};\n");
  }
  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);
  generate_enum_funcs(impl, ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_array_base(bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "struct s {\n"\
      "float mem[3][2], mem2;\n"\
      "};\n"\
      "};\n");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "struct s {\n"\
      "float mem[3][2], mem2;\n"\
      "};\n");
  }

  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);
  generate_array_base_funcs(impl, ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_array_instance(bool ns)
{
  char buffer[1024];
  if (ns)
  {
    snprintf(buffer, sizeof(buffer),
      "module N {\n"\
      "struct I {\n"\
      "long l;\n"\
      "};\n"\
      "struct s {\n"\
      "I mem[3][2], mem2;\n"\
      "};\n"\
      "};\n");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
      "struct I {\n"\
      "long l;\n"\
      "};\n"\
      "struct s {\n"\
      "I mem[3][2], mem2;\n"\
      "};\n");
  }

  idl_tree_t* tree = NULL;
  idl_parse_string(buffer, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);
  generate_array_instance_funcs(impl, ns);

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_ostream(impl);
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_namespace_cross_call()
{
  char* str = "module A_1 { module A_2 { struct s_1 { long m_1; }; }; };\n"\
              "module B_1 { module B_2 { struct s_2 { A_1::A_2::s_1 m_2; }; }; };\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(cross_call_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_struct_inheritance()
{
  const char* str = "struct I {\n"\
  "  long inherited_member; \n"\
  "};\n"\
  "struct s : I {\n"\
  "  long new_member; \n"\
  "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_EXTENDED_DATA_TYPES, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(struct_inheritance_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_bounded_sequence()
{
  const char* str =
    "struct s {\n"\
    "sequence<long,20> mem;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(bounded_sequence_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_bounded_sequence_of_structs()
{
  const char* str =
    "struct s_sub {\n"\
    "long l;\n"\
    "};\n"\
    "struct s {\n"\
    "sequence<s_sub,20> mem;\n"\
    "};\n"\
    "#pragma keylist s mem";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(bounded_sequence_of_structs_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_bounded_string()
{
  const char* str =
    "struct s {\n"\
    "string<20> mem;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(bounded_string_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_typedef_resolution()
{
  const char* str =
    "module M {\n"\
    "typedef long td_0;\n"\
    "typedef td_0 td_1;\n"\
    "typedef td_1 td_2;\n"\
    "typedef td_2 td_3;\n"\
    "typedef td_3 td_4;\n"\
    "typedef td_4 td_5;\n"\
    "typedef sequence<td_5> td_6;\n"\
    "};\n"\
    "module N {\n"\
    "struct s {\n"\
    "M::td_5 mem_simple;\n"\
    "sequence<M::td_6> mem;\n"\
    "};\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(typedef_resolution_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL(typedef_resolution_header_cpp_in, get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_keys_base()
{
  const char* str =
    "struct s {\n"\
    "octet _o;\n"\
    "@key long _l;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_ANNOTATIONS, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(keys_base_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_keys_union_implicit()
{
  const char* str =
  "union u switch (long) {\n"\
    "case 0:\n"\
    "case 1: octet o;\n"\
    "case 2:\n"\
    "case 3: long l;\n"\
    "case 4:\n"\
    "case 5: string str;\n"\
    "default: float f;\n"\
    "};\n"\
    "struct ss {\n"\
    "octet _o;\n"\
    "@key u _u;\n"\
    "@key long _l;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_ANNOTATIONS, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(keys_union_implicit_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_keys_struct_explicit()
{
  const char* str =
    "struct s {\n"\
    "octet _o;\n"\
    "@key long _l;\n"\
    "};\n"\
    "struct ss {\n"\
    "octet _o;\n"\
    "@key s _s_;\n"\
    "@key long _l;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_ANNOTATIONS, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(keys_struct_explicit_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_keys_struct_implicit()
{
  const char* str =
    "struct s {\n"\
    "octet _o;\n"\
    "long _l;\n"\
    "};\n"\
    "struct ss {\n"\
    "octet _o;\n"\
    "@key s _s_;\n"\
    "@key long _l;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_ANNOTATIONS, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(keys_struct_implicit_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_keys_typedef()
{
  const char* str =
    "typedef sequence<long> td_1;\n"\
    "struct s {\n"\
    "octet _o;\n"\
    "@key td_1 _t;\n"\
    "};\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_ANNOTATIONS, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(keys_typedef_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL(keys_typedef_header_cpp_in, get_ostream_buffer(get_idl_streamer_head_buf(generated)));
  
  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_sequence_recursive()
{
  const char* str =
    "typedef sequence<sequence<sequence<string> > > recseq;";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, IDL_FLAG_ANNOTATIONS, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(sequence_recursive_impl_cpp_in, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL(sequence_recursive_header_cpp_in, get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

CU_Test(streamer_generator, base_types_namespace_absent)
{
  for (size_t i = 0; i < sizeof(cxx_width) / sizeof(size_t); i++)
    test_base(i, false);
}

CU_Test(streamer_generator, base_types_namespace_present)
{
  for (size_t i = 0; i < sizeof(cxx_width) / sizeof(size_t); i++)
    test_base(i, true);
}

CU_Test(streamer_generator, instance_namespace_absent)
{
  test_instance(false);
}

CU_Test(streamer_generator, instance_namespace_present)
{
  test_instance(true);
}

CU_Test(streamer_generator, string_namespace_absent)
{
  test_string(false);
}

CU_Test(streamer_generator, string_namespace_present)
{
  test_string(true);
}

CU_Test(streamer_generator, sequence_namespace_absent)
{
  for (size_t i = 0; i < sizeof(cxx_width) / sizeof(size_t); i++)
    test_sequence(i, false);
}

CU_Test(streamer_generator, sequence_namespace_present)
{
  for (size_t i = 0; i < sizeof(cxx_width) / sizeof(size_t); i++)
    test_sequence(i, true);
}

CU_Test(streamer_generator, union_case_int_namespace_absent)
{
  test_union(false);
}

CU_Test(streamer_generator, union_case_int_namespace_present)
{
  test_union(true);
}

CU_Test(streamer_generator, enumerator_namespace_absent)
{
  test_enum(false);
}

CU_Test(streamer_generator, enumerator_namespace_present)
{
  test_enum(true);
}

CU_Test(streamer_generator, array_base_type_namespace_absent)
{
  test_array_base(false);
}

CU_Test(streamer_generator, array_base_type_namespace_present)
{
  test_array_base(true);
}

CU_Test(streamer_generator, array_instance_namespace_absent)
{
  test_array_instance(false);
}

CU_Test(streamer_generator, array_instance_namespace_present)
{
  test_array_instance(true);
}

CU_Test(streamer_generator, namespace_cross_call)
{
  test_namespace_cross_call();
}

CU_Test(streamer_generator, struct_inheritance)
{
  test_struct_inheritance();
}

CU_Test(streamer_generator, bounded_sequence)
{
  test_bounded_sequence();
}

CU_Test(streamer_generator, bounded_sequence_of_structs)
{
  test_bounded_sequence_of_structs();
}

CU_Test(streamer_generator, bounded_string)
{
  test_bounded_string();
}

CU_Test(streamer_generator, typedef_resolution)
{
  test_typedef_resolution();
}

CU_Test(streamer_generator, key_base)
{
  test_keys_base();
}

CU_Test(streamer_generator, key_struct_explicit)
{
  test_keys_struct_explicit();
}

CU_Test(streamer_generator, key_struct_implicit)
{
  test_keys_struct_implicit();
}

CU_Test(streamer_generator, key_union_implicit)
{
  test_keys_union_implicit();
}

CU_Test(streamer_generator, key_typedef)
{
  test_keys_typedef();
}

CU_Test(streamer_generator, sequence_recursive)
{
  test_sequence_recursive();
}
