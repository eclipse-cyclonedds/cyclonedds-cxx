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

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (%d - position&%#x)&%#x;  //alignment\n",width,width-1,width-1);
    }
    format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
    format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  }
  format_ostream_indented(ns * 2, ostr, "  *((%s*)((char*)data+position)) = obj.mem();  //writing bytes for member: mem\n", tn);
  format_ostream_indented(ns * 2, ostr, "  position += %d;  //moving position indicator\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  position += position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  position += (%d - position&%#x)&%#x;  //alignment\n",width,width-1,width-1);
    }
  }
  format_ostream_indented(ns * 2, ostr, "  position += %d;  //bytes for member: mem\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  if (width > 1)
  {
    if (width == 2)
    {
      format_ostream_indented(ns * 2, ostr, "  position += position&0x1;  //alignment\n");
    }
    else
    {
      format_ostream_indented(ns * 2, ostr, "  position += (%d - position&%#x)&%#x;  //alignment\n", width, width - 1, width - 1);
    }
  }
  format_ostream_indented(ns * 2, ostr, "  obj.mem(*((%s*)((char*)data+position)));  //reading bytes for member: mem\n", tn);
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
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const I &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = obj.l();  //writing bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = %swrite_struct(obj.%s(), data, position);\n", ns ? "N::" : "", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const I &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += %swrite_size(obj.%s(), position);\n", ns ? "N::" : "", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(I &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj.l(*((int32_t*)((char*)data+position)));  //reading bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position = %sread_struct(obj.%s(), data, position);\n", ns ? "N::" : "", in);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }

}

void create_funcs_string(idl_ostream_t* ostr, const char* in, bool ns)
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
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = obj.%s().size()+1;  //number of entries in the sequence\n", in);
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
  format_ostream_indented(ns * 2, ostr, "  position += (obj.%s().size()+1)*1;  //entries of sequence\n", in);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  obj.%s().assign((char*)((char*)data+position),(char*)((char*)data+position)+sequenceentries);  //putting data into container\n", in);
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

  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = obj.%s().size();  //number of entries in the sequence\n", seq_name);
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: %s().size\n", seq_name);
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  alignmentbytes = (%d - position&%#x)&%#x;  //alignment\n", width, width-1, width-1);
    format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
    format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  }
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,obj.%s().data(),sequenceentries*%d);  //contents for %s\n", seq_name, width, seq_name);
  format_ostream_indented(ns * 2, ostr, "  position += sequenceentries*%d;  //moving position indicator\n", width);
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: %s().size\n", seq_name);
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  position += (%d - position&%#x)&%#x;  //alignment\n", width, width - 1, width - 1);
  }
  format_ostream_indented(ns * 2, ostr, "  position += (obj.%s().size())*%d;  //entries of sequence\n", seq_name, width);
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  if (width > 4)
  {
    format_ostream_indented(ns * 2, ostr, "  position += (%d - position&%#x)&%#x;  //alignment\n", width, width - 1, width - 1);
  }
  format_ostream_indented(ns * 2, ostr, "  obj.%s().assign((%s*)((char*)data+position),(%s*)((char*)data+position)+sequenceentries);  //putting data into container\n", seq_name, cxx_type_name, cxx_type_name);
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
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = obj._d();  //writing bytes for member: _d\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (obj._d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint8_t*)((char*)data+position)) = obj.o();  //writing bytes for member: o\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((int32_t*)((char*)data+position)) = obj.l();  //writing bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = obj.str().size()+1;  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: str().size\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      memcpy((char*)data+position,obj.str().data(),sequenceentries*1);  //contents for str\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((float*)((char*)data+position)) = obj.f();  //writing bytes for member: f\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: _d\n");
  format_ostream_indented(ns * 2, ostr, "  switch (obj._d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //bytes for member: o\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: str().size\n");
  format_ostream_indented(ns * 2, ostr, "      position += (obj.str().size()+1)*1;  //entries of sequence\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: f\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  obj.clear();\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj._d(*((int32_t*)((char*)data+position)));  //reading bytes for member: _d\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (obj._d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      obj.o(*((uint8_t*)((char*)data+position)));  //reading bytes for member: o\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      obj.l(*((int32_t*)((char*)data+position)));  //reading bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      obj.str().assign((char*)((char*)data+position),(char*)((char*)data+position)+sequenceentries);  //putting data into container\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    default:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      obj.f(*((float*)((char*)data+position)));  //reading bytes for member: f\n");
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
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy((char*)data+position,obj.mem().data(),24);  //writing bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((float*)((char*)data+position)) = obj.mem2();  //writing bytes for member: mem2\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: mem2\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memcpy(obj.mem().data(),(char*)data+position,24);  //reading bytes for member: mem\n");
  format_ostream_indented(ns * 2, ostr, "  position += 24;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  obj.mem2(*((float*)((char*)data+position)));  //reading bytes for member: mem2\n");
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
  if (ns)
  {
    format_ostream_indented(0, ostr, "namespace N\n{\n\n");
  }

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const I &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "  position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  *((int32_t*)((char*)data+position)) = obj.l();  //writing bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_struct(const s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t _i = 0; _i < 6; _i++) position = %swrite_struct(obj.mem()[_i], data, position);\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  position = %swrite_struct(obj.mem2(), data, position);\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const I &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t write_size(const s &obj, size_t offset)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  size_t position = offset;\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t _i = 0; _i < 6; _i++) position += %swrite_size(obj.mem()[_i], position);\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  position += %swrite_size(obj.mem2(), position);\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(I &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj.l(*((int32_t*)((char*)data+position)));  //reading bytes for member: l\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  for (size_t _i = 0; _i < 6; _i++) position = %sread_struct(obj.mem()[_i], data, position);\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  position = %sread_struct(obj.mem2(), data, position);\n", ns ? "N::" : "");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "} //end namespace N\n\n");
  }
}

#define CCFI "namespace A_1\n"\
  "{\n\n"\
"  namespace A_2\n"\
"  {\n\n"\
"    size_t write_struct(const s_1 &obj, void *data, size_t position)\n"\
"    {\n"\
"      size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n"\
"      memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
"      position += alignmentbytes;  //moving position indicator\n"\
"      *((int32_t*)((char*)data+position)) = obj.m_1();  //writing bytes for member: m_1\n"\
"      position += 4;  //moving position indicator\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t write_size(const s_1 &obj, size_t offset)\n"\
"    {\n"\
"      size_t position = offset;\n"\
"      position += (4 - position&0x3)&0x3;  //alignment\n"\
"      position += 4;  //bytes for member: m_1\n"\
"      return position-offset;\n"\
"    }\n\n"\
"    size_t read_struct(s_1 &obj, void *data, size_t position)\n"\
"    {\n"\
"      position += (4 - position&0x3)&0x3;  //alignment\n"\
"      obj.m_1(*((int32_t*)((char*)data+position)));  //reading bytes for member: m_1\n"\
"      position += 4;  //moving position indicator\n"\
"      return position;\n"\
"    }\n\n"\
"  } //end namespace A_2\n\n"\
"} //end namespace A_1\n\n"\
"namespace B_1\n"\
"{\n\n"\
"  namespace B_2\n"\
"  {\n\n"\
"    size_t write_struct(const s_2 &obj, void *data, size_t position)\n"\
"    {\n"\
"      position = A_1::A_2::write_struct(obj.m_2(), data, position);\n"\
"      return position;\n"\
"    }\n\n"\
"    size_t write_size(const s_2 &obj, size_t offset)\n"\
"    {\n"\
"      size_t position = offset;\n"\
"      position += A_1::A_2::write_size(obj.m_2(), position);\n"\
"      return position-offset;\n"\
"    }\n\n"\
"    size_t read_struct(s_2 &obj, void *data, size_t position)\n"\
"    {\n"\
"      position = A_1::A_2::read_struct(obj.m_2(), data, position);\n"\
"      return position;\n"\
"    }\n\n"\
"  } //end namespace B_2\n\n"\
"} //end namespace B_1\n\n"

#define CCFH "namespace A_1\n"\
"{\n\n"\
"  namespace A_2\n"\
"  {\n\n"\
"    size_t write_struct(const s_1 &obj, void *data, size_t position);\n\n"\
"    size_t write_size(const s_1 &obj, size_t offset);\n\n"\
"    size_t read_struct(s_1 &obj, void *data, size_t position);\n\n"\
"  } //end namespace A_2\n\n"\
"} //end namespace A_1\n\n"\
"namespace B_1\n"\
"{\n\n"\
"  namespace B_2\n"\
"  {\n\n"\
"    size_t write_struct(const s_2 &obj, void *data, size_t position);\n\n"\
"    size_t write_size(const s_2 &obj, size_t offset);\n\n"\
"    size_t read_struct(s_2 &obj, void *data, size_t position);\n\n"\
"  } //end namespace B_2\n\n"\
"} //end namespace B_1\n\n"

#define IFI "size_t write_struct(const I &obj, void *data, size_t position)\n"\
  "{\n"\
  "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n"\
  "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
  "  position += alignmentbytes;  //moving position indicator\n"\
  "  *((int32_t*)((char*)data+position)) = obj.inherited_member();  //writing bytes for member: inherited_member\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t write_struct(const s &obj, void *data, size_t position)\n"\
  "{\n"\
  "  position = write_struct(dynamic_cast<I&>(obj), data, position);\n"\
  "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n"\
  "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
  "  position += alignmentbytes;  //moving position indicator\n"\
  "  *((int32_t*)((char*)data+position)) = obj.new_member();  //writing bytes for member: new_member\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t write_size(const I &obj, size_t offset)\n"\
  "{\n"\
  "  size_t position = offset;\n"\
  "  position += (4 - position&0x3)&0x3;  //alignment\n"\
  "  position += 4;  //bytes for member: inherited_member\n"\
  "  return position-offset;\n"\
  "}\n\n"\
  "size_t write_size(const s &obj, size_t offset)\n"\
  "{\n"\
  "  size_t position = offset;\n"\
  "  position += write_size(dynamic_cast<I&>(obj), position);\n"\
  "  position += (4 - position&0x3)&0x3;  //alignment\n"\
  "  position += 4;  //bytes for member: new_member\n"\
  "  return position-offset;\n"\
  "}\n\n"\
  "size_t read_struct(I &obj, void *data, size_t position)\n"\
  "{\n"\
  "  position += (4 - position&0x3)&0x3;  //alignment\n"\
  "  obj.inherited_member(*((int32_t*)((char*)data+position)));  //reading bytes for member: inherited_member\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t read_struct(s &obj, void *data, size_t position)\n"\
  "{\n"\
  "  position = read_struct(dynamic_cast<I&>(obj), data, position);\n"\
  "  position += (4 - position&0x3)&0x3;  //alignment\n"\
  "  obj.new_member(*((int32_t*)((char*)data+position)));  //reading bytes for member: new_member\n"\
  "  position += 4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"

#define BSI "size_t write_struct(const s &obj, void *data, size_t position)\n"\
  "{\n"\
  "  size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n"\
  "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"\
  "  position += alignmentbytes;  //moving position indicator\n"\
  "  uint32_t sequenceentries = obj.mem().size();  //number of entries in the sequence\n"\
  "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: mem().size\n"\
  "  position += 4;  //moving position indicator\n"\
  "  if (sequenceentries > 20) throw dds::core::InvalidArgumentError(\"attempt to assign entries to bounded member mem in excess of maximum length 20\");\n"\
  "  memcpy((char*)data+position,obj.mem().data(),sequenceentries*4);  //contents for mem\n"\
  "  position += sequenceentries*4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"\
  "size_t write_size(const s &obj, size_t offset)\n"\
  "{\n"\
  "  size_t position = offset;\n"\
  "  position += (4 - position&0x3)&0x3;  //alignment\n"\
  "  position += 4;  //bytes for member: mem().size\n"\
  "  position += (obj.mem().size())*4;  //entries of sequence\n"\
  "  return position-offset;\n"\
  "}\n\n"\
  "size_t read_struct(s &obj, void *data, size_t position)\n"\
  "{\n"\
  "  position += (4 - position&0x3)&0x3;  //alignment\n"\
  "  uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n"\
  "  position += 4;  //moving position indicator\n"\
  "  obj.mem().assign((int32_t*)((char*)data+position),(int32_t*)((char*)data+position)+sequenceentries);  //putting data into container\n"\
  "  position += sequenceentries*4;  //moving position indicator\n"\
  "  return position;\n"\
  "}\n\n"

#define HNA "size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"size_t write_size(const s &obj, size_t offset);\n\n"\
"size_t read_struct(s &obj, void *data, size_t position);\n\n"

#define HNP "namespace N\n{\n\n"\
"  size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"  size_t write_size(const s &obj, size_t offset);\n\n"\
"  size_t read_struct(s &obj, void *data, size_t position);\n\n"\
"} //end namespace N\n\n"

#define HNAI "size_t write_struct(const I &obj, void *data, size_t position);\n\n"\
"size_t write_size(const I &obj, size_t offset);\n\n"\
"size_t read_struct(I &obj, void *data, size_t position);\n\n"\
"size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"size_t write_size(const s &obj, size_t offset);\n\n"\
"size_t read_struct(s &obj, void *data, size_t position);\n\n"

#define HNPI "namespace N\n{\n\n"\
"  size_t write_struct(const I &obj, void *data, size_t position);\n\n"\
"  size_t write_size(const I &obj, size_t offset);\n\n"\
"  size_t read_struct(I &obj, void *data, size_t position);\n\n"\
"  size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"  size_t write_size(const s &obj, size_t offset);\n\n"\
"  size_t read_struct(s &obj, void *data, size_t position);\n\n"\
"} //end namespace N\n\n"

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

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));

  destruct_idl_ostream(impl);

  destruct_idl_streamer_output(generated);
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

  CU_ASSERT_STRING_EQUAL(ns ? HNPI : HNAI, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(ns ? HNPI : HNAI, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
}

void test_namespace_cross_call()
{
  char* str = "module A_1 { module A_2 { struct s_1 { long m_1; }; }; };\n"\
              "module B_1 { module B_2 { struct s_2 { A_1::A_2::s_1 m_2; }; }; };\n";

  idl_tree_t* tree = NULL;
  idl_parse_string(str, 0u, &tree);

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  CU_ASSERT_STRING_EQUAL(CCFH, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(CCFI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(HNAI, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(IFI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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

  CU_ASSERT_STRING_EQUAL(HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(BSI, get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
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
