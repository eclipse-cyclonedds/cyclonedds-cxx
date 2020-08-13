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
#else
#define _strdup(str) strdup(str)
#endif

#include "idlcxx/streamer_generator.h"
#include "idl/processor.h"

#include "CUnit/Theory.h"

static const idl_kind_t idl_type[] = {
  IDL_CHAR,
  IDL_OCTET,
  IDL_BOOL,
  IDL_SHORT,
  IDL_LONG,
  IDL_LLONG,
  IDL_USHORT,
  IDL_ULONG,
  IDL_ULLONG,
  IDL_FLOAT,
  IDL_DOUBLE
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

#define format_ostream_indented(depth,ostr,str,...) \
if (depth > 0) format_ostream(ostr, "%*c", depth, ' '); \
format_ostream(ostr, str, ##__VA_ARGS__);

void create_funcs_base_width_1(idl_ostream_t *ostr, const char *tn, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n",tn);
  format_ostream_indented(ns * 2, ostr, "  position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += 1;  //bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_base_width_2(idl_ostream_t* ostr, const char* tn, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = position&0x1;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 2;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += position&0x1;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 2;  //bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += position&0x1;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 2;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_base_width_4(idl_ostream_t* ostr, const char* tn, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_base_width_8(idl_ostream_t* ostr, const char* tn, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (8 - position&0x7)&0x7;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 8;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (8 - position&0x7)&0x7;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 8;  //bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (8 - position&0x7)&0x7;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj.mem() = *((%s*)((char*)data+position));  //reading bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += 8;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_instance(idl_ostream_t* ostr, const char* in, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = write_struct(obj.%s(), data, position);\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += write_size(obj.%s(), position);\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = read_struct(obj.%s(), data, position);\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }

}

void create_funcs_sequence_width_1(idl_ostream_t* ostr, const char* in, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = obj.%s().size();  //number of entries in the sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,obj.%s().data(),sequenceentries*1);  //contents for %s\n", in, in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += (obj.%s().size())*1;  //entries of sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  obj.%s().assign((char*)data+position,(char*)data+position+sequenceentries*1);  //putting data into container\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_sequence_width_2(idl_ostream_t* ostr, const char* in, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = obj.%s().size();  //number of entries in the sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,obj.%s().data(),sequenceentries*2);  //contents for %s\n", in, in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*2;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += (obj.%s().size())*2;  //entries of sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  obj.%s().assign((char*)data+position,(char*)data+position+sequenceentries*2);  //putting data into container\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*2;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_sequence_width_4(idl_ostream_t* ostr, const char* in, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = obj.%s().size();  //number of entries in the sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,obj.%s().data(),sequenceentries*4);  //contents for %s\n", in, in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += (obj.%s().size())*4;  //entries of sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  obj.%s().assign((char*)data+position,(char*)data+position+sequenceentries*4);  //putting data into container\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

void create_funcs_sequence_width_8(idl_ostream_t* ostr, const char* in, bool ns)
{
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = obj.%s().size();  //number of entries in the sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  alignmentbytes = (8 - position&0x7)&0x7;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,obj.%s().data(),sequenceentries*8);  //contents for %s\n", in, in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*8;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: %s().size\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += (8 - position&0x7)&0x7;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += (obj.%s().size())*8;  //entries of sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  position += (8 - position&0x7)&0x7;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj.%s().assign((char*)data+position,(char*)data+position+sequenceentries*8);  //putting data into container\n", in);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*8;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}\n\n");
  }
}

#define HNA "size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"size_t write_size(const s &obj, size_t offset);\n\n"\
"size_t read_struct(s &obj, void *data, size_t position);\n\n"

#define HNP "namespace N\n{\n\n"\
"  size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"  size_t write_size(const s &obj, size_t offset);\n\n"\
"  size_t read_struct(s &obj, void *data, size_t position);\n\n"\
"}\n\n"

idl_member_t* generate_member_base(idl_kind_t member_type, const char* member_name)
{
  idl_member_t* returnval = idl_create_member();

  returnval->declarators = idl_create_declarator();
  returnval->declarators->node.parent = (idl_node_t*)returnval;
  returnval->declarators->node.previous = NULL;
  returnval->declarators->node.next = NULL;
  returnval->declarators->identifier = _strdup(member_name);

  returnval->type_spec = malloc(sizeof(idl_type_spec_t));
  returnval->type_spec->kind = member_type;

  return returnval;
}

idl_member_t* generate_member_instance(const char* member_type, const char* member_name)
{
  idl_member_t* returnval = idl_create_member();

  returnval->declarators = idl_create_declarator();
  returnval->declarators->node.parent = (idl_node_t*)returnval;
  returnval->declarators->node.previous = NULL;
  returnval->declarators->node.next = NULL;
  returnval->declarators->identifier = _strdup(member_name);

  returnval->type_spec = malloc(sizeof(idl_type_spec_t));
  returnval->type_spec->kind = IDL_SCOPED_NAME;

  return returnval;
}

idl_member_t* generate_sequence_instance(idl_kind_t sequenced_type, const char* member_name)
{
  idl_member_t* returnval = idl_create_member();

  returnval->declarators = idl_create_declarator();
  returnval->declarators->node.parent = (idl_node_t*)returnval;
  returnval->declarators->node.previous = NULL;
  returnval->declarators->node.next = NULL;
  returnval->declarators->identifier = _strdup(member_name);

  returnval->type_spec = malloc(sizeof(idl_type_spec_t));
  returnval->type_spec->kind = IDL_SEQUENCE_TYPE;
  returnval->type_spec->sequence_type.type_spec = malloc(sizeof(idl_simple_type_spec_t));
  returnval->type_spec->sequence_type.type_spec->kind = sequenced_type;

  return returnval;
}

idl_struct_type_t* generate_struct_base(const char* name, const idl_kind_t* member_types, const char** member_names, size_t nmembers)
{
  idl_struct_type_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_member_t *prevmem = NULL;
  for (size_t i = 0; i < nmembers; i++)
  {
    idl_member_t* mem = generate_member_base(member_types[i],member_names[i]);
    mem->node.previous = (idl_node_t*)prevmem;
    mem->node.next = NULL;
    if (NULL != prevmem)
      prevmem->node.next = (idl_node_t*)mem;
    prevmem = mem;

    if (NULL == returnval->members)
      returnval->members = mem;
  }

  return returnval;
}

idl_struct_type_t* generate_struct_instance(const char* name, const char** instance_types, const char** member_names, size_t nmembers)
{
  idl_struct_type_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_member_t* prevmem = NULL;
  for (size_t i = 0; i < nmembers; i++)
  {
    idl_member_t* mem = generate_member_instance(instance_types[i], member_names[i]);
    mem->node.previous = (idl_node_t*)prevmem;
    mem->node.next = NULL;
    if (NULL != prevmem)
      prevmem->node.next = (idl_node_t*)mem;
    prevmem = mem;

    if (NULL == returnval->members)
      returnval->members = mem;
  }

  return returnval;
}

idl_struct_type_t* generate_struct_sequence(const char* name, const idl_kind_t* sequence_types, const char** member_names, size_t nmembers)
{
  idl_struct_type_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_member_t* prevmem = NULL;
  for (size_t i = 0; i < nmembers; i++)
  {
    idl_member_t* mem = generate_sequence_instance(sequence_types[i], member_names[i]);
    mem->node.previous = (idl_node_t*)prevmem;
    mem->node.next = NULL;
    if (NULL != prevmem)
      prevmem->node.next = (idl_node_t*)mem;
    prevmem = mem;

    if (NULL == returnval->members)
      returnval->members = mem;
  }

  return returnval;
}

void test_base(size_t n, bool ns)
{
  idl_tree_t* tree = malloc(sizeof(idl_tree_t));
  tree->files = NULL;

  idl_kind_t kinds[1] = { idl_type[n] };
  const char* names[1] = { "mem" };

  idl_struct_type_t* str = generate_struct_base("s", kinds, names, 1);
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = malloc(sizeof(idl_definition_t));
    mod->definitions = (idl_definition_t*)str;
    mod->identifier = _strdup("N");

    tree->root = (idl_node_t*)mod;
    str->node.parent = (idl_node_t*)mod;
    mod->node.parent = (idl_node_t*)tree;
  }
  else
  {
    tree->root = (idl_node_t*)str;
    str->node.parent = (idl_node_t*)tree;
  }

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  switch (cxx_width[n])
  {
  case 1:
    create_funcs_base_width_1(impl,cxx_type[n],ns);
    break;
  case 2:
    create_funcs_base_width_2(impl, cxx_type[n], ns);
    break;
  case 4:
    create_funcs_base_width_4(impl, cxx_type[n], ns);
    break;
  case 8:
    create_funcs_base_width_8(impl, cxx_type[n], ns);
      break;
  default:
    CU_ASSERT_FATAL(0);
  }

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));

  /*
  printf("=========generated============\n%s\n============================\n", get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

void test_instance(bool ns)
{
  idl_tree_t* tree = malloc(sizeof(idl_tree_t));
  tree->files = NULL;

  const char* types[1] = { "I" };
  const char* names[1] = { "mem" };

  idl_struct_type_t* str = generate_struct_instance("s", types, names, 1);
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = malloc(sizeof(idl_definition_t));
    mod->definitions = (idl_definition_t*)str;
    mod->identifier = _strdup("N");

    tree->root = (idl_node_t*)mod;
    str->node.parent = (idl_node_t*)mod;
    mod->node.parent = (idl_node_t*)tree;
  }
  else
  {
    tree->root = (idl_node_t*)str;
    str->node.parent = (idl_node_t*)tree;
  }

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  create_funcs_instance(impl, names[0], ns);

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
}

void test_sequence(size_t n, bool ns)
{
  idl_tree_t* tree = malloc(sizeof(idl_tree_t));
  tree->files = NULL;

  const idl_kind_t types[1] = { idl_type[n] };
  const char* names[1] = { "mem" };

  idl_struct_type_t* str = generate_struct_sequence("s", types, names, 1);
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = malloc(sizeof(idl_definition_t));
    mod->definitions = (idl_definition_t*)str;
    mod->identifier = _strdup("N");

    tree->root = (idl_node_t*)mod;
    str->node.parent = (idl_node_t*)mod;
    mod->node.parent = (idl_node_t*)tree;
  }
  else
  {
    tree->root = (idl_node_t*)str;
    str->node.parent = (idl_node_t*)tree;
  }

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);

  switch (cxx_width[n])
  {
  case 1:
    create_funcs_sequence_width_1(impl, names[0], ns);
    break;
  case 2:
    create_funcs_sequence_width_2(impl, names[0], ns);
    break;
  case 4:
    create_funcs_sequence_width_4(impl, names[0], ns);
    break;
  case 8:
    create_funcs_sequence_width_8(impl, names[0], ns);
    break;
  default:
    CU_ASSERT_FATAL(0);
  }

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
}

CU_Test(streamer_generator, base_types_namespace_absent)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_kind_t); i++)
    test_base(i, false);
}

CU_Test(streamer_generator, base_types_namespace_present)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_kind_t); i++)
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

CU_Test(streamer_generator, sequence_namespace_absent)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_kind_t); i++)
    test_sequence(i,false);
}

CU_Test(streamer_generator, sequence_namespace_present)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_kind_t); i++)
    test_sequence(i, true);
}
