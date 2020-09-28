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

#define format_ostream_indented(depth,ostr,str,...) \
if (depth > 0) format_ostream(ostr, "%*c", depth, ' '); \
format_ostream(ostr, str, ##__VA_ARGS__);

void create_funcs_base(idl_ostream_t * ostr, size_t n, bool ns)
{
  const char* tn = cxx_type[n];
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
      format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (%d - (position&%#x))&%#x;  //alignment\n",width,width-1,width-1);
    }
    format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
    format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  }
  format_ostream_indented(ns * 2, ostr, "  *((%s*)((char*)data+position)) = mem();  //writing bytes for member: mem()\n", tn);
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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
  format_ostream_indented(ns * 2, ostr, "  mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem()\n", tn);
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
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t I::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t I::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool I::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t I::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  l() = *((int32_t*)((char*)data+position));  //reading bytes for member: l()\n");
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
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }


  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = (uint32_t) %s().size()+1;  //number of entries in the sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: %s()\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,%s().data(),sequenceentries*1);  //contents for %s()\n", in, in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for sequence entries\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += (%s().size()+1)*1;  //entries of sequence\n", in);
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  %s().assign((char*)data+position,(char*)data+position+sequenceentries);  //putting data into container\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*1;  //moving position indicator\n");
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
  const char* cxx_type_name = cxx_type[n];

  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = (uint32_t) %s().size();  //number of entries in the sequence\n", seq_name);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: %s()\n", seq_name);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  alignmentbytes = (%d - (position&%#x))&%#x;  //alignment\n", width, width-1, width-1);
    format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
    format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  }
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,%s().data(),sequenceentries*%d);  //contents for %s()\n", seq_name, width, seq_name);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*%d;  //moving position indicator\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for sequence entries\n", seq_name);
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  position += (%d - (position&%#x))&%#x;  //alignment\n", width, width - 1, width - 1);
  }
  format_ostream_indented(ns * 2, ostr, "  position += (%s().size())*%d;  //entries of sequence\n", seq_name, width);
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  position += (%d - (position&%#x))&%#x;  //alignment\n", width, width - 1, width - 1);
  }
  if (width > 1)
  {
    format_ostream_indented(ns * 2, ostr, "  %s().assign((%s*)((char*)data+position),(%s*)((char*)data+position)+sequenceentries);  //putting data into container\n", seq_name, cxx_type_name, cxx_type_name);
  }
  else
  {
    format_ostream_indented(ns * 2, ostr, "  %s().assign((char*)data+position,(char*)data+position+sequenceentries);  //putting data into container\n", seq_name);
  }
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*%d;  //moving position indicator\n",width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

void generate_union_funcs(idl_ostream_t* ostr, bool ns)
{
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = _d();  //writing bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (_d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint8_t*)((char*)data+position)) = o();  //writing bytes for member: o()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = (uint32_t) str().size()+1;  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: str()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      memcpy((char*)data+position,str().data(),sequenceentries*1);  //contents for str()\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((float*)((char*)data+position)) = f();  //writing bytes for member: f()\n");
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
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for sequence entries\n");
  format_ostream_indented(ns * 2, ostr, "      position += (str().size()+1)*1;  //entries of sequence\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
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
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for sequence entries\n");
  format_ostream_indented(ns * 2, ostr, "      position += (str().size()+1)*1;  //entries of sequence\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  if (position != UINT_MAX)   position += 4;  //bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  size_t union_max = position;\n");
  format_ostream_indented(ns * 2, ostr, "  {  //cases\n");
  format_ostream_indented(ns * 2, ostr, "    size_t case_max = position;\n");
  format_ostream_indented(ns * 2, ostr, "    if (case_max != UINT_MAX) case_max +=  1;\n");
  format_ostream_indented(ns * 2, ostr, "    union_max = max(case_max,union_max);\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  {  //cases\n");
  format_ostream_indented(ns * 2, ostr, "    size_t case_max = position;\n");
  format_ostream_indented(ns * 2, ostr, "    if (case_max != UINT_MAX) case_max +=  4;\n");
  format_ostream_indented(ns * 2, ostr, "    union_max = max(case_max,union_max);\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  {  //cases\n");
  format_ostream_indented(ns * 2, ostr, "    size_t case_max = position;\n");
  format_ostream_indented(ns * 2, ostr, "    if (case_max != UINT_MAX) case_max += 4;\n");
  format_ostream_indented(ns * 2, ostr, "    case_max = UINT_MAX;\n");
  format_ostream_indented(ns * 2, ostr, "    union_max = max(case_max,union_max);\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  {  //cases\n");
  format_ostream_indented(ns * 2, ostr, "    size_t case_max = position;\n");
  format_ostream_indented(ns * 2, ostr, "    if (case_max != UINT_MAX) case_max +=  4;\n");
  format_ostream_indented(ns * 2, ostr, "    union_max = max(case_max,union_max);\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  position = max(position,union_max);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = _d();  //writing bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (_d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint8_t*)((char*)data+position)) = o();  //writing bytes for member: o()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = (uint32_t) str().size()+1;  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: str()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      memcpy((char*)data+position,str().data(),sequenceentries*1);  //contents for str()\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((float*)((char*)data+position)) = f();  //writing bytes for member: f()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  clear();\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  _d() = *((int32_t*)((char*)data+position));  //reading bytes for member: _d()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (_d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      o() = *((uint8_t*)((char*)data+position));  //reading bytes for member: o()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      l() = *((int32_t*)((char*)data+position));  //reading bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      str().assign((char*)data+position,(char*)data+position+sequenceentries);  //putting data into container\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      f() = *((float*)((char*)data+position));  //reading bytes for member: f()\n");
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
  create_funcs_base(ostr, 7, ns);
}

void generate_array_base_funcs(idl_ostream_t* ostr, bool ns)
{
  format_ostream_indented(0, ostr, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,mem().data(),24);  //writing bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((float*)((char*)data+position)) = mem2();  //writing bytes for member: mem2()\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_max_size(size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy(mem().data(),(char*)data+position,24);  //reading bytes for member: mem()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  mem2() = *((float*)((char*)data+position));  //reading bytes for member: mem2()\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t I::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::write_struct(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t _i = 0; _i < 6; _i++)   position = mem()[_i].write_struct(data, position);\n");
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
  format_ostream_indented(ns * 2, ostr, "  for (size_t _i = 0; _i < 6; _i++)   position = mem()[_i].write_size(position);\n");
  format_ostream_indented(ns * 2, ostr, "  position = mem2().write_size(position);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t I::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool I::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t s::key_stream(void *data, size_t position) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "bool s::key(ddsi_keyhash_t &hash) const\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t sz = key_size(0);\n");
  format_ostream_indented(ns * 2, ostr, "  size_t padding = 16 - sz%%16;\n");
  format_ostream_indented(ns * 2, ostr, "  if (sz != 0 && padding == 16) padding = 0;\n");
  format_ostream_indented(ns * 2, ostr, "  std::vector<unsigned char> buffer(sz+padding);\n");
  format_ostream_indented(ns * 2, ostr, "  memset(buffer.data()+sz,0x0,padding);\n");
  format_ostream_indented(ns * 2, ostr, "  key_stream(buffer.data(),0);\n");
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

  format_ostream_indented(ns * 2, ostr, "size_t I::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - (position&0x3))&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  l() = *((int32_t*)((char*)data+position));  //reading bytes for member: l()\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t s::read_struct(const void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t _i = 0; _i < 6; _i++)   position = mem()[_i].read_struct(data, position);\n");
  format_ostream_indented(ns * 2, ostr, "  position = mem2().read_struct(data, position);\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

#define CCFI  "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
"namespace A_1\n"\
"{\n\n"\
"  namespace A_2\n"\
"  {\n\n"\
"    size_t s_1::write_struct(void *data, size_t position) const\n"\
"    {\n"\
"      size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"      memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"      position += alignmentbytes;  //moving position indicator\n"\
"      *((int32_t*)((char*)data+position)) = m_1();  //writing bytes for member: m_1()\n"\
"      position += 4;  //moving position indicator\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_1::write_size(size_t position) const\n"\
"    {\n"\
"      position += (4 - (position&0x3))&0x3;  //alignment\n"\
"      position += 4;  //bytes for member: m_1()\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_1::key_size(size_t position) const\n"\
"    {\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_1::key_max_size(size_t position) const\n"\
"    {\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_1::key_stream(void *data, size_t position) const\n"\
"    {\n"\
"      return position;\n"\
"    }\n\n"\
"    bool s_1::key(ddsi_keyhash_t &hash) const\n"\
"    {\n"\
"      size_t sz = key_size(0);\n"\
"      size_t padding = 16 - sz%16;\n"\
"      if (sz != 0 && padding == 16) padding = 0;\n"\
"      std::vector<unsigned char> buffer(sz+padding);\n"\
"      memset(buffer.data()+sz,0x0,padding);\n"\
"      key_stream(buffer.data(),0);\n"\
"      static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"      if (fptr == NULL)\n"\
"      {\n"\
"        if (key_max_size(0) <= 16)\n"\
"        {\n"\
"          //bind to unmodified function which just copies buffer into the keyhash\n"\
"          fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"        }\n"\
"        else\n"\
"        {\n"\
"          //bind to MD5 hash function\n"\
"          fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"        }\n"\
"      }\n"\
"      return (*fptr)(buffer,hash);\n"\
"    }\n\n"\
"    size_t s_1::read_struct(const void *data, size_t position)\n"\
"    {\n"\
"      position += (4 - (position&0x3))&0x3;  //alignment\n"\
"      m_1() = *((int32_t*)((char*)data+position));  //reading bytes for member: m_1()\n"\
"      position += 4;  //moving position indicator\n"\
"      return position;\n"\
"    }\n\n"\
"  } //end namespace A_2\n\n"\
"} //end namespace A_1\n\n"\
"namespace B_1\n"\
"{\n\n"\
"  namespace B_2\n"\
"  {\n\n"\
"    size_t s_2::write_struct(void *data, size_t position) const\n"\
"    {\n"\
"      position = m_2().write_struct(data, position);\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_2::write_size(size_t position) const\n"\
"    {\n"\
"      position = m_2().write_size(position);\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_2::key_size(size_t position) const\n"\
"    {\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_2::key_max_size(size_t position) const\n"\
"    {\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t s_2::key_stream(void *data, size_t position) const\n"\
"    {\n"\
"      return position;\n"\
"    }\n\n"\
"    bool s_2::key(ddsi_keyhash_t &hash) const\n"\
"    {\n"\
"      size_t sz = key_size(0);\n"\
"      size_t padding = 16 - sz%16;\n"\
"      if (sz != 0 && padding == 16) padding = 0;\n"\
"      std::vector<unsigned char> buffer(sz+padding);\n"\
"      memset(buffer.data()+sz,0x0,padding);\n"\
"      key_stream(buffer.data(),0);\n"\
"      static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"      if (fptr == NULL)\n"\
"      {\n"\
"        if (key_max_size(0) <= 16)\n"\
"        {\n"\
"          //bind to unmodified function which just copies buffer into the keyhash\n"\
"          fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"        }\n"\
"        else\n"\
"        {\n"\
"          //bind to MD5 hash function\n"\
"          fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"        }\n"\
"      }\n"\
"      return (*fptr)(buffer,hash);\n"\
"    }\n\n"\
"    size_t s_2::read_struct(const void *data, size_t position)\n"\
"    {\n"\
"      position = m_2().read_struct(data, position);\n"\
"      return position;\n"\
"    }\n\n"\
"  } //end namespace B_2\n\n"\
"} //end namespace B_1\n\n"

#define IFI "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
  "size_t I::write_struct(void *data, size_t position) const\n"\
  "{\n"\
  "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
  "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
  "  position += alignmentbytes;  //moving position indicator\n"\
  "  *((int32_t*)((char*)data+position)) = inherited_member();  //writing bytes for member: inherited_member()\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::write_struct(void *data, size_t position) const\n"\
  "{\n"\
  "  position = dynamic_cast<const I&>(*this).write_struct(data, position);\n"\
  "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
  "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
  "  position += alignmentbytes;  //moving position indicator\n"\
  "  *((int32_t*)((char*)data+position)) = new_member();  //writing bytes for member: new_member()\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t I::write_size(size_t position) const\n"\
  "{\n"\
  "  position += (4 - (position&0x3))&0x3;  //alignment\n"\
  "  position += 4;  //bytes for member: inherited_member()\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::write_size(size_t position) const\n"\
  "{\n"\
  "  position = dynamic_cast<const I&>(*this).write_size(position);\n"\
  "  position += (4 - (position&0x3))&0x3;  //alignment\n"\
  "  position += 4;  //bytes for member: new_member()\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t I::key_size(size_t position) const\n"\
  "{\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::key_size(size_t position) const\n"\
  "{\n"\
  "  position = dynamic_cast<const I&>(*this).key_size(position);\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t I::key_max_size(size_t position) const\n"\
  "{\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::key_max_size(size_t position) const\n"\
  "{\n"\
  "  if (position != UINT_MAX)   position = dynamic_cast<const I&>(*this).key_max_size(position);\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t I::key_stream(void *data, size_t position) const\n"\
  "{\n"\
  "  return position;\n"\
  "}\n\n"\
  "bool I::key(ddsi_keyhash_t &hash) const\n"\
  "{\n"\
  "  size_t sz = key_size(0);\n"\
  "  size_t padding = 16 - sz%16;\n"\
  "  if (sz != 0 && padding == 16) padding = 0;\n"\
  "  std::vector<unsigned char> buffer(sz+padding);\n"\
  "  memset(buffer.data()+sz,0x0,padding);\n"\
  "  key_stream(buffer.data(),0);\n"\
  "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
  "  if (fptr == NULL)\n"\
  "  {\n"\
  "    if (key_max_size(0) <= 16)\n"\
  "    {\n"\
  "      //bind to unmodified function which just copies buffer into the keyhash\n"\
  "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
  "    }\n"\
  "    else\n"\
  "    {\n"\
  "      //bind to MD5 hash function\n"\
  "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
  "    }\n"\
  "  }\n"\
  "  return (*fptr)(buffer,hash);\n"\
  "}\n\n"\
  "size_t s::key_stream(void *data, size_t position) const\n"\
  "{\n"\
  "  position = dynamic_cast<const I&>(*this).key_stream(data, position);\n"\
  "  return position;\n"\
  "}\n\n"\
  "bool s::key(ddsi_keyhash_t &hash) const\n"\
  "{\n"\
  "  size_t sz = key_size(0);\n"\
  "  size_t padding = 16 - sz%16;\n"\
  "  if (sz != 0 && padding == 16) padding = 0;\n"\
  "  std::vector<unsigned char> buffer(sz+padding);\n"\
  "  memset(buffer.data()+sz,0x0,padding);\n"\
  "  key_stream(buffer.data(),0);\n"\
  "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
  "  if (fptr == NULL)\n"\
  "  {\n"\
  "    if (key_max_size(0) <= 16)\n"\
  "    {\n"\
  "      //bind to unmodified function which just copies buffer into the keyhash\n"\
  "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
  "    }\n"\
  "    else\n"\
  "    {\n"\
  "      //bind to MD5 hash function\n"\
  "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
  "    }\n"\
  "  }\n"\
  "  return (*fptr)(buffer,hash);\n"\
  "}\n\n"\
  "size_t I::read_struct(const void *data, size_t position)\n"\
  "{\n"\
  "  position += (4 - (position&0x3))&0x3;  //alignment\n"\
  "  inherited_member() = *((int32_t*)((char*)data+position));  //reading bytes for member: inherited_member()\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::read_struct(const void *data, size_t position)\n"\
  "{\n"\
  "  position = dynamic_cast<I&>(*this).read_struct(data, position);\n"\
  "  position += (4 - (position&0x3))&0x3;  //alignment\n"\
  "  new_member() = *((int32_t*)((char*)data+position));  //reading bytes for member: new_member()\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"

#define BSI "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
  "size_t s::write_struct(void *data, size_t position) const\n"\
  "{\n"\
  "  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
  "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
  "  position += alignmentbytes;  //moving position indicator\n"\
  "  uint32_t sequenceentries = (uint32_t) mem().size();  //number of entries in the sequence\n"\
  "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: mem()\n"\
  "  position += 4;  //moving position indicator\n"\
  "  if (sequenceentries > 20) throw dds::core::InvalidArgumentError(\"attempt to assign entries to bounded member mem() in excess of maximum length 20\");\n"\
  "  memcpy((char*)data+position,mem().data(),sequenceentries*4);  //contents for mem()\n"\
  "  position += sequenceentries*4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::write_size(size_t position) const\n"\
  "{\n"\
  "  position += (4 - (position&0x3))&0x3;  //alignment\n"\
  "  position += 4;  //bytes for sequence entries\n"\
  "  position += (mem().size())*4;  //entries of sequence\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::key_size(size_t position) const\n"\
  "{\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::key_max_size(size_t position) const\n"\
  "{\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t s::key_stream(void *data, size_t position) const\n"\
  "{\n"\
  "  return position;\n"\
  "}\n\n"\
  "bool s::key(ddsi_keyhash_t &hash) const\n"\
  "{\n"\
  "  size_t sz = key_size(0);\n"\
  "  size_t padding = 16 - sz%16;\n"\
  "  if (sz != 0 && padding == 16) padding = 0;\n"\
  "  std::vector<unsigned char> buffer(sz+padding);\n"\
  "  memset(buffer.data()+sz,0x0,padding);\n"\
  "  key_stream(buffer.data(),0);\n"\
  "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
  "  if (fptr == NULL)\n"\
  "  {\n"\
  "    if (key_max_size(0) <= 16)\n"\
  "    {\n"\
  "      //bind to unmodified function which just copies buffer into the keyhash\n"\
  "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
  "    }\n"\
  "    else\n"\
  "    {\n"\
  "      //bind to MD5 hash function\n"\
  "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
  "    }\n"\
  "  }\n"\
  "  return (*fptr)(buffer,hash);\n"\
  "}\n\n"\
  "size_t s::read_struct(const void *data, size_t position)\n"\
  "{\n"\
  "  position += (4 - (position&0x3))&0x3;  //alignment\n"\
  "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n"\
  "  position += 4;  //moving position indicator\n"\
  "  mem().assign((int32_t*)((char*)data+position),(int32_t*)((char*)data+position)+sequenceentries);  //putting data into container\n"\
  "  position += sequenceentries*4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"

#define TDI "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
"namespace M\n"\
"{\n\n"\
"  size_t typedef_write_td_6(const td_6 &obj, void* data, size_t position)\n"\
"  {\n"\
"    size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"    memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"    position += alignmentbytes;  //moving position indicator\n"\
"    uint32_t sequenceentries = (uint32_t) obj.size();  //number of entries in the sequence\n"\
"    *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: obj\n"\
"    position += 4;  //moving position indicator\n"\
"    memcpy((char*)data+position,obj.data(),sequenceentries*4);  //contents for obj\n"\
"    position += sequenceentries*4;  //moving position indicator\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t typedef_write_size_td_6(const td_6 &obj, size_t position)\n"\
"  {\n"\
"    position += (4 - (position&0x3))&0x3;  //alignment\n"\
"    position += 4;  //bytes for sequence entries\n"\
"    position += (obj.size())*4;  //entries of sequence\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t typedef_key_size_td_6(const td_6 &obj, size_t position)\n"\
"  {\n"\
"    position += (4 - (position&0x3))&0x3;  //alignment\n"\
"    position += 4;  //bytes for sequence entries\n"\
"    position += (obj.size())*4;  //entries of sequence\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t typedef_key_max_size_td_6(const td_6 &obj, size_t position)\n"\
"  {\n"\
"    (void)position;\n"\
"    return UINT_MAX;\n"\
"  }\n\n"\
"  size_t typedef_key_stream_td_6(const td_6 &obj, void *data, size_t position)\n"\
"  {\n"\
"    size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"    memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"    position += alignmentbytes;  //moving position indicator\n"\
"    uint32_t sequenceentries = (uint32_t) obj.size();  //number of entries in the sequence\n"\
"    *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: obj\n"\
"    position += 4;  //moving position indicator\n"\
"    memcpy((char*)data+position,obj.data(),sequenceentries*4);  //contents for obj\n"\
"    position += sequenceentries*4;  //moving position indicator\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t typedef_read_td_6(td_6 &obj, void* data, size_t position)\n"\
"  {\n"\
"    position += (4 - (position&0x3))&0x3;  //alignment\n"\
"    uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n"\
"    position += 4;  //moving position indicator\n"\
"    obj.assign((int32_t*)((char*)data+position),(int32_t*)((char*)data+position)+sequenceentries);  //putting data into container\n"\
"    position += sequenceentries*4;  //moving position indicator\n"\
"    return position;\n"\
"  }\n\n"\
"} //end namespace M\n\n"\
"namespace N\n"\
"{\n\n"\
"  size_t s::write_struct(void *data, size_t position) const\n"\
"  {\n"\
"    size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"    memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"    position += alignmentbytes;  //moving position indicator\n"\
"    *((int32_t*)((char*)data+position)) = mem_simple();  //writing bytes for member: mem_simple()\n"\
"    position += 4;  //moving position indicator\n"\
"    uint32_t sequenceentries = (uint32_t) mem().size();  //number of entries in the sequence\n"\
"    *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: mem()\n"\
"    position += 4;  //moving position indicator\n"\
"    for (size_t _i = 0; _i < sequenceentries; _i++)   position = M::typedef_write_td_6(mem()[_i],data,position);\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t s::write_size(size_t position) const\n"\
"  {\n"\
"    position += (4 - (position&0x3))&0x3;  //alignment\n"\
"    position += 4;  //bytes for member: mem_simple()\n"\
"    position += 4;  //bytes for sequence entries\n"\
"    for (size_t _i = 0; _i < sequenceentries; _i++)   position = M::typedef_write_size_td_6(mem()[_i], position);\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t s::key_size(size_t position) const\n"\
"  {\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t s::key_max_size(size_t position) const\n"\
"  {\n"\
"    return position;\n"\
"  }\n\n"\
"  size_t s::key_stream(void *data, size_t position) const\n"\
"  {\n"\
"    return position;\n"\
"  }\n\n"\
"  bool s::key(ddsi_keyhash_t &hash) const\n"\
"  {\n"\
"    size_t sz = key_size(0);\n"\
"    size_t padding = 16 - sz%16;\n"\
"    if (sz != 0 && padding == 16) padding = 0;\n"\
"    std::vector<unsigned char> buffer(sz+padding);\n"\
"    memset(buffer.data()+sz,0x0,padding);\n"\
"    key_stream(buffer.data(),0);\n"\
"    static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"    if (fptr == NULL)\n"\
"    {\n"\
"      if (key_max_size(0) <= 16)\n"\
"      {\n"\
"        //bind to unmodified function which just copies buffer into the keyhash\n"\
"        fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"      }\n"\
"      else\n"\
"      {\n"\
"        //bind to MD5 hash function\n"\
"        fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"      }\n"\
"    }\n"\
"    return (*fptr)(buffer,hash);\n"\
"  }\n\n"\
"  size_t s::read_struct(const void *data, size_t position)\n"\
"  {\n"\
"    position += (4 - (position&0x3))&0x3;  //alignment\n"\
"    mem_simple() = *((int32_t*)((char*)data+position));  //reading bytes for member: mem_simple()\n"\
"    position += 4;  //moving position indicator\n"\
"    uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n"\
"    position += 4;  //moving position indicator\n"\
"    mem().resize(sequenceentries);\n"\
"    for (size_t _i = 0; _i < sequenceentries; _i++)   position = M::typedef_read_td_6(mem()[_i], data, position);\n"\
"    return position;\n"\
"  }\n\n"\
"} //end namespace N\n\n"

#define TDH "namespace M\n"\
"{\n\n"\
"  size_t typedef_write_td_6(const td_6 &obj, void* data, size_t position);\n\n"\
"  size_t typedef_write_size_td_6(const td_6 &obj, size_t position);\n\n"\
"  size_t typedef_read_td_6(td_6 &obj, void* data, size_t position);\n\n"\
"  size_t typedef_key_stream_td_6(const td_6 &obj, void *data, size_t position);\n\n"\
"  size_t typedef_key_size_td_6(const td_6 &obj, size_t position);\n\n"\
"  size_t typedef_key_max_size_td_6(const td_6 &obj, size_t position);\n\n"\
"} //end namespace M\n\n"

#define KBI "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
"size_t s::write_struct(void *data, size_t position) const\n"\
"{\n"\
"  *((uint8_t*)((char*)data+position)) = o();  //writing bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::write_size(size_t position) const\n"\
"{\n"\
"  position += 1;  //bytes for member: o()\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_size(size_t position) const\n"\
"{\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_max_size(size_t position) const\n"\
"{\n"\
"  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  if (position != UINT_MAX)   position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_stream(void *data, size_t position) const\n"\
"{\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"bool s::key(ddsi_keyhash_t &hash) const\n"\
"{\n"\
"  size_t sz = key_size(0);\n"\
"  size_t padding = 16 - sz%16;\n"\
"  if (sz != 0 && padding == 16) padding = 0;\n"\
"  std::vector<unsigned char> buffer(sz+padding);\n"\
"  memset(buffer.data()+sz,0x0,padding);\n"\
"  key_stream(buffer.data(),0);\n"\
"  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"  if (fptr == NULL)\n"\
"  {\n"\
"    if (key_max_size(0) <= 16)\n"\
"    {\n"\
"      //bind to unmodified function which just copies buffer into the keyhash\n"\
"      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"    }\n"\
"    else\n"\
"    {\n"\
"      //bind to MD5 hash function\n"\
"      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"    }\n"\
"  }\n"\
"  return (*fptr)(buffer,hash);\n"\
"}\n\n"\
"size_t s::read_struct(const void *data, size_t position)\n"\
"{\n"\
"  o() = *((uint8_t*)((char*)data+position));  //reading bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  l() = *((int32_t*)((char*)data+position));  //reading bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"

#define KSI "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
"size_t s::write_struct(void *data, size_t position) const\n"\
"{\n"\
"  *((uint8_t*)((char*)data+position)) = o();  //writing bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t ss::write_struct(void *data, size_t position) const\n"\
"{\n"\
"  *((uint8_t*)((char*)data+position)) = o();  //writing bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  position = s_().write_struct(data, position);\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::write_size(size_t position) const\n"\
"{\n"\
"  position += 1;  //bytes for member: o()\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t ss::write_size(size_t position) const\n"\
"{\n"\
"  position += 1;  //bytes for member: o()\n"\
"  position = s_().write_size(position);\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_size(size_t position) const\n"\
"{\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t ss::key_size(size_t position) const\n"\
"{\n"\
"  position = s_().key_size(position);\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_max_size(size_t position) const\n"\
"{\n"\
"  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  if (position != UINT_MAX)   position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t ss::key_max_size(size_t position) const\n"\
"{\n"\
"  if (position != UINT_MAX)   position = s_().key_max_size(position);\n"\
"  if (position != UINT_MAX)   position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  if (position != UINT_MAX)   position += 4;  //bytes for member: l()\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_stream(void *data, size_t position) const\n"\
"{\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"bool s::key(ddsi_keyhash_t &hash) const\n"\
"{\n"\
"  size_t sz = key_size(0);\n"\
"  size_t padding = 16 - sz%16;\n"\
"  if (sz != 0 && padding == 16) padding = 0;\n"\
"  std::vector<unsigned char> buffer(sz+padding);\n"\
"  memset(buffer.data()+sz,0x0,padding);\n"\
"  key_stream(buffer.data(),0);\n"\
"  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"  if (fptr == NULL)\n"\
"  {\n"\
"    if (key_max_size(0) <= 16)\n"\
"    {\n"\
"      //bind to unmodified function which just copies buffer into the keyhash\n"\
"      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"    }\n"\
"    else\n"\
"    {\n"\
"      //bind to MD5 hash function\n"\
"      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"    }\n"\
"  }\n"\
"  return (*fptr)(buffer,hash);\n"\
"}\n\n"\
"size_t ss::key_stream(void *data, size_t position) const\n"\
"{\n"\
"  position = s_().key_stream(data, position);\n"\
"  alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  *((int32_t*)((char*)data+position)) = l();  //writing bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"bool ss::key(ddsi_keyhash_t &hash) const\n"\
"{\n"\
"  size_t sz = key_size(0);\n"\
"  size_t padding = 16 - sz%16;\n"\
"  if (sz != 0 && padding == 16) padding = 0;\n"\
"  std::vector<unsigned char> buffer(sz+padding);\n"\
"  memset(buffer.data()+sz,0x0,padding);\n"\
"  key_stream(buffer.data(),0);\n"\
"  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"  if (fptr == NULL)\n"\
"  {\n"\
"    if (key_max_size(0) <= 16)\n"\
"    {\n"\
"      //bind to unmodified function which just copies buffer into the keyhash\n"\
"      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"    }\n"\
"    else\n"\
"    {\n"\
"      //bind to MD5 hash function\n"\
"      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"    }\n"\
"  }\n"\
"  return (*fptr)(buffer,hash);\n"\
"}\n\n"\
"size_t s::read_struct(const void *data, size_t position)\n"\
"{\n"\
"  o() = *((uint8_t*)((char*)data+position));  //reading bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  l() = *((int32_t*)((char*)data+position));  //reading bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t ss::read_struct(const void *data, size_t position)\n"\
"{\n"\
"  o() = *((uint8_t*)((char*)data+position));  //reading bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  position = s_().read_struct(data, position);\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  l() = *((int32_t*)((char*)data+position));  //reading bytes for member: l()\n"\
"  position += 4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"

#define TDKI "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n"\
"size_t typedef_write_td_1(const td_1 &obj, void* data, size_t position)\n"\
"{\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  uint32_t sequenceentries = (uint32_t) obj.size();  //number of entries in the sequence\n"\
"  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: obj\n"\
"  position += 4;  //moving position indicator\n"\
"  memcpy((char*)data+position,obj.data(),sequenceentries*4);  //contents for obj\n"\
"  position += sequenceentries*4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::write_struct(void *data, size_t position) const\n"\
"{\n"\
"  *((uint8_t*)((char*)data+position)) = o();  //writing bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  position = typedef_write_td_1(t(), data, position);\n"\
"  return position;\n"\
"}\n\n"\
"size_t typedef_write_size_td_1(const td_1 &obj, size_t position)\n"\
"{\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for sequence entries\n"\
"  position += (obj.size())*4;  //entries of sequence\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::write_size(size_t position) const\n"\
"{\n"\
"  position += 1;  //bytes for member: o()\n"\
"  position = typedef_write_size_td_1(t(), position);\n"\
"  return position;\n"\
"}\n\n"\
"size_t typedef_key_size_td_1(const td_1 &obj, size_t position)\n"\
"{\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  position += 4;  //bytes for sequence entries\n"\
"  position += (obj.size())*4;  //entries of sequence\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_size(size_t position) const\n"\
"{\n"\
"  position = typedef_key_size_td_1(t(), position);\n"\
"  return position;\n"\
"}\n\n"\
"size_t typedef_key_max_size_td_1(const td_1 &obj, size_t position)\n"\
"{\n"\
"  (void)position;\n"\
"  return UINT_MAX;\n"\
"}\n\n"\
"size_t s::key_max_size(size_t position) const\n"\
"{\n"\
"  position = typedef_key_max_size_td_1(t(), position);\n"\
"  return position;\n"\
"}\n\n"\
"size_t typedef_key_stream_td_1(const td_1 &obj, void *data, size_t position)\n"\
"{\n"\
"  size_t alignmentbytes = (4 - (position&0x3))&0x3;  //alignment\n"\
"  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"  position += alignmentbytes;  //moving position indicator\n"\
"  uint32_t sequenceentries = (uint32_t) obj.size();  //number of entries in the sequence\n"\
"  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: obj\n"\
"  position += 4;  //moving position indicator\n"\
"  memcpy((char*)data+position,obj.data(),sequenceentries*4);  //contents for obj\n"\
"  position += sequenceentries*4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::key_stream(void *data, size_t position) const\n"\
"{\n"\
"  position = typedef_key_stream_td_1(t(), *data, position);\n"\
"  return position;\n"\
"}\n\n"\
"bool s::key(ddsi_keyhash_t &hash) const\n"\
"{\n"\
"  size_t sz = key_size(0);\n"\
"  size_t padding = 16 - sz%16;\n"\
"  if (sz != 0 && padding == 16) padding = 0;\n"\
"  std::vector<unsigned char> buffer(sz+padding);\n"\
"  memset(buffer.data()+sz,0x0,padding);\n"\
"  key_stream(buffer.data(),0);\n"\
"  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n"\
"  if (fptr == NULL)\n"\
"  {\n"\
"    if (key_max_size(0) <= 16)\n"\
"    {\n"\
"      //bind to unmodified function which just copies buffer into the keyhash\n"\
"      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n"\
"    }\n"\
"    else\n"\
"    {\n"\
"      //bind to MD5 hash function\n"\
"      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n"\
"    }\n"\
"  }\n"\
"  return (*fptr)(buffer,hash);\n"\
"}\n\n"\
"size_t typedef_read_td_1(td_1 &obj, void* data, size_t position)\n"\
"{\n"\
"  position += (4 - (position&0x3))&0x3;  //alignment\n"\
"  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n"\
"  position += 4;  //moving position indicator\n"\
"  obj.assign((int32_t*)((char*)data+position),(int32_t*)((char*)data+position)+sequenceentries);  //putting data into container\n"\
"  position += sequenceentries*4;  //moving position indicator\n"\
"  return position;\n"\
"}\n\n"\
"size_t s::read_struct(const void *data, size_t position)\n"\
"{\n"\
"  o() = *((uint8_t*)((char*)data+position));  //reading bytes for member: o()\n"\
"  position += 1;  //moving position indicator\n"\
"  position = typedef_read_td_1(t(), data, position);\n"\
"  return position;\n"\
"}\n\n"

#define TDKH "size_t typedef_write_td_1(const td_1 &obj, void* data, size_t position);\n\n"\
"size_t typedef_write_size_td_1(const td_1 &obj, size_t position);\n\n"\
"size_t typedef_read_td_1(td_1 &obj, void* data, size_t position);\n\n"\
"size_t typedef_key_stream_td_1(const td_1 &obj, void *data, size_t position);\n\n"\
"size_t typedef_key_size_td_1(const td_1 &obj, size_t position);\n\n"\
"size_t typedef_key_max_size_td_1(const td_1 &obj, size_t position);\n\n"

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

  CU_ASSERT_STRING_EQUAL(CCFI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(IFI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(BSI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(TDI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL(TDH, get_ostream_buffer(get_idl_streamer_head_buf(generated)));

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

  CU_ASSERT_STRING_EQUAL(KBI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL("", get_ostream_buffer(get_idl_streamer_head_buf(generated)));

  destruct_idl_streamer_output(generated);
  idl_delete_tree(tree);
}

void test_keys_struct()
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

  CU_ASSERT_STRING_EQUAL(KSI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(TDKI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  CU_ASSERT_STRING_EQUAL(TDKH, get_ostream_buffer(get_idl_streamer_head_buf(generated)));
  
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

CU_Test(streamer_generator, typedef_resolution)
{
  test_typedef_resolution();
}

CU_Test(streamer_generator, key_base)
{
  test_keys_base();
}

CU_Test(streamer_generator, key_struct)
{
  test_keys_struct();
}

CU_Test(streamer_generator, key_typedef)
{
  test_keys_typedef();
}
