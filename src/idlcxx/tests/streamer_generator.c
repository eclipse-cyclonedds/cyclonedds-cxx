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

static const idl_mask_t idl_type[] = {
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

static size_t ncases = 3;
static size_t labelspercase = 2;

static uint64_t union_switch_int_case_labels[] = {
  0,1,
  2,3,
  4,5
};

static idl_mask_t union_switch_int_case_types[] = {
  IDL_OCTET,
  IDL_LONG,
  IDL_STRING
};

static const char* union_switch_int_case_names[] = {
  "_oct",
  "_long",
  "_str"
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

#define HNA "size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"size_t write_size(const s &obj, size_t offset);\n\n"\
"size_t read_struct(s &obj, void *data, size_t position);\n\n"

#define HNP "namespace N\n{\n\n"\
"  size_t write_struct(const s &obj, void *data, size_t position);\n\n"\
"  size_t write_size(const s &obj, size_t offset);\n\n"\
"  size_t read_struct(s &obj, void *data, size_t position);\n\n"\
"} //end namespace N\n\n"

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
  format_ostream_indented(ns * 2, ostr, "  *((uint32_t*)((char*)data+position)) = obj._d();  //writing bytes for member: _d\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (obj._d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint8_t*)((char*)data+position)) = obj._oct();  //writing bytes for member: _oct\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      size_t alignmentbytes = (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "      memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n");
  format_ostream_indented(ns * 2, ostr, "      position += alignmentbytes;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      *((int32_t*)((char*)data+position)) = obj._long();  //writing bytes for member: _long\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = obj._str().size()+1;  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: _str().size\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      memcpy((char*)data+position,obj._str().data(),sequenceentries*1);  //contents for _str\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n");
  format_ostream_indented(0, ostr, "\n");
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
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //bytes for member: _oct\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: _long\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //bytes for member: _str().size\n");
  format_ostream_indented(ns * 2, ostr, "      position += (obj._str().size()+1)*1;  //entries of sequence\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position-offset;\n");
  format_ostream_indented(ns * 2, ostr, "}\n");
  format_ostream_indented(0, ostr, "\n");
  format_ostream_indented(ns * 2, ostr, "size_t read_struct(s &obj, void *data, size_t position)\n");
  format_ostream_indented(ns * 2, ostr, "{\n");
  format_ostream_indented(ns * 2, ostr, "  position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "  obj._d(*((uint32_t*)((char*)data+position)));  //reading bytes for member: _d\n");
  format_ostream_indented(ns * 2, ostr, "  position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "  switch (obj._d())\n");
  format_ostream_indented(ns * 2, ostr, "  {\n");
  format_ostream_indented(ns * 2, ostr, "    case 0:\n");
  format_ostream_indented(ns * 2, ostr, "    case 1:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      obj._oct(*((uint8_t*)((char*)data+position)));  //reading bytes for member: _oct\n");
  format_ostream_indented(ns * 2, ostr, "      position += 1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 2:\n");
  format_ostream_indented(ns * 2, ostr, "    case 3:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      position += (4 - position&0x3)&0x3;  //alignment\n");
  format_ostream_indented(ns * 2, ostr, "      obj._long(*((int32_t*)((char*)data+position)));  //reading bytes for member: _long\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "    case 4:\n");
  format_ostream_indented(ns * 2, ostr, "    case 5:\n");
  format_ostream_indented(ns * 2, ostr, "    {\n");
  format_ostream_indented(ns * 2, ostr, "      uint32_t sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n");
  format_ostream_indented(ns * 2, ostr, "      position += 4;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "      obj._str().assign((char*)data+position,(char*)data+position+sequenceentries*1);  //putting data into container\n");
  format_ostream_indented(ns * 2, ostr, "      position += sequenceentries*1;  //moving position indicator\n");
  format_ostream_indented(ns * 2, ostr, "    }\n");
  format_ostream_indented(ns * 2, ostr, "    break;\n");
  format_ostream_indented(ns * 2, ostr, "  }\n");
  format_ostream_indented(ns * 2, ostr, "  return position;\n");
  format_ostream_indented(ns * 2, ostr, "}\n\n");

  if (ns)
  {
    format_ostream_indented(0, ostr, "}  //end namespace N\n\n");
  }
}

idl_member_t* generate_member_base(idl_mask_t member_type, const char* member_name)
{
  idl_member_t* returnval = idl_create_member();

  returnval->declarators = idl_create_declarator();
  returnval->declarators->node.parent = (idl_node_t*)returnval;
  returnval->declarators->node.previous = NULL;
  returnval->declarators->node.next = NULL;
  returnval->declarators->identifier = _strdup(member_name);

  returnval->type_spec = calloc(sizeof(idl_type_spec_t),1);
  returnval->type_spec->mask = member_type;

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

  returnval->type_spec = calloc(sizeof(idl_type_spec_t),1);
  returnval->type_spec->mask = IDL_STRUCT;

  return returnval;
}

idl_member_t* generate_sequence_instance(idl_mask_t sequenced_type, const char* member_name)
{
  idl_member_t* returnval = idl_create_member();

  returnval->declarators = idl_create_declarator();
  returnval->declarators->node.parent = (idl_node_t*)returnval;
  returnval->declarators->node.previous = NULL;
  returnval->declarators->node.next = NULL;
  returnval->declarators->identifier = _strdup(member_name);

  idl_sequence_t *seq = calloc(sizeof(idl_sequence_t), 1);
  returnval->type_spec = (idl_type_spec_t*)seq;
  returnval->type_spec->mask = IDL_SEQUENCE;
  seq->type_spec = calloc(sizeof(idl_type_spec_t), 1);
  seq->type_spec->mask = sequenced_type;

  return returnval;
}

idl_struct_t* generate_struct_base(const char* name, const idl_mask_t* member_types, const char** member_names, size_t nmembers)
{
  idl_struct_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_node_t* prevmem = NULL;
  for (size_t i = 0; i < nmembers; i++)
  {
    idl_member_t* mem = generate_member_base(member_types[i],member_names[i]);

    idl_node_t* currentnode = (idl_node_t*)mem;
    currentnode->parent = (idl_node_t*)returnval;
    currentnode->previous = prevmem;
    if (NULL != prevmem)
      prevmem->next = currentnode;
    prevmem = currentnode;

    if (NULL == returnval->members)
      returnval->members = mem;
  }

  return returnval;
}

idl_struct_t* generate_struct_instance(const char* name, const char** instance_types, const char** member_names, size_t nmembers)
{
  idl_struct_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_node_t* prevmem = NULL;
  for (size_t i = 0; i < nmembers; i++)
  {
    idl_member_t* mem = generate_member_instance(instance_types[i], member_names[i]);

    idl_node_t* currentnode = (idl_node_t*)mem;
    currentnode->parent = (idl_node_t*)returnval;
    currentnode->previous = prevmem;
    if (NULL != prevmem)
      prevmem->next = currentnode;
    prevmem = currentnode;

    if (NULL == returnval->members)
      returnval->members = mem;
  }

  return returnval;
}

idl_struct_t* generate_struct_string(const char* name)
{
  idl_struct_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_member_t* mem = generate_member_base(IDL_STRING, "str");

  idl_node_t* currentnode = (idl_node_t*)mem;
  currentnode->parent = (idl_node_t*)returnval;

  returnval->members = mem;

  return returnval;
}

idl_struct_t* generate_struct_sequence(const char* name, const idl_mask_t* sequence_types, const char** member_names, size_t nmembers)
{
  idl_struct_t* returnval = idl_create_struct();
  returnval->identifier = _strdup(name);

  idl_node_t* prevmem = NULL;
  for (size_t i = 0; i < nmembers; i++)
  {
    idl_member_t* mem = generate_sequence_instance(sequence_types[i], member_names[i]);

    idl_node_t* currentnode = (idl_node_t*)mem;
    currentnode->parent = (idl_node_t*)returnval;
    currentnode->previous = prevmem;
    if (NULL != prevmem)
      prevmem->next = currentnode;
    prevmem = currentnode;

    if (NULL == returnval->members)
      returnval->members = mem;
  }

  return returnval;
}

idl_case_label_t* generate_union_case_int_label(uint64_t _case)
{
  idl_case_label_t* returnval = idl_create_case_label();

  returnval->const_expr = (idl_const_expr_t*)idl_create_integer_literal(_case);
  ((idl_node_t*)returnval->const_expr)->parent = (idl_node_t*)returnval;

  return returnval;
}

idl_case_t* generate_union_int_switch_case(const uint64_t* labels, const idl_mask_t case_type, const char* case_name)
{
  idl_case_t* returnval = idl_create_case();

  idl_node_t* prevlabel = NULL;
  for (size_t label = 0; label < labelspercase; label++)
  {
    idl_case_label_t* currentlabel = generate_union_case_int_label(labels[label]);

    idl_node_t* currentnode = (idl_node_t*)currentlabel;
    currentnode->parent = (idl_node_t*)returnval;
    currentnode->previous = prevlabel;
    if (NULL != prevlabel)
      prevlabel->next = currentnode;
    prevlabel = currentnode;

    if (NULL == returnval->case_labels)
      returnval->case_labels = currentlabel;
  }

  returnval->type_spec = calloc(sizeof(idl_type_spec_t),1);
  returnval->type_spec->mask = case_type;
  returnval->declarator = idl_create_declarator();
  returnval->declarator->node.parent = (idl_node_t*)returnval;
  returnval->declarator->identifier = _strdup(case_name);

  return returnval;
}

idl_union_t* generate_union_int_switch(const char* name, const uint64_t* cases, const idl_mask_t* case_types, const char** case_names)
{
  idl_union_t* returnval = idl_create_union();

  returnval->identifier = _strdup(name);

  returnval->switch_type_spec = calloc(sizeof(idl_switch_type_spec_t), 1);
  returnval->switch_type_spec->mask = IDL_ULONG;

  idl_node_t* prevcase = NULL;
  for (size_t _case = 0; _case < ncases; _case++)
  {
    idl_case_t* currentcase = generate_union_int_switch_case(cases + _case*labelspercase, case_types[_case], case_names[_case]);

    idl_node_t* currentnode = (idl_node_t*)currentcase;
    currentnode->parent = (idl_node_t*)returnval;
    currentnode->previous = prevcase;
    if (NULL != prevcase)
      prevcase->next = currentnode;
    prevcase = currentnode;

    if (NULL == returnval->cases)
      returnval->cases = currentcase;
  }

  return returnval;
}

void test_base(size_t n, bool ns)
{
  idl_tree_t* tree = calloc(sizeof(idl_tree_t),1);

  idl_mask_t masks[1] = { idl_type[n] };
  const char* names[1] = { "mem" };

  idl_struct_t* str = generate_struct_base("s", masks, names, 1);
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = calloc(sizeof(idl_definition_t),1);
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

  create_funcs_base(impl, n, ns);

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));

  destruct_idl_ostream(impl);

  destruct_idl_streamer_output(generated);
}

void test_instance(bool ns)
{
  idl_tree_t* tree = calloc(sizeof(idl_tree_t),1);

  const char* types[1] = { "I" };
  const char* names[1] = { "mem" };

  idl_struct_t* str = generate_struct_instance("s", types, names, 1);
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = calloc(sizeof(idl_definition_t),1);
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
  idl_tree_t* tree = calloc(sizeof(idl_tree_t),1);

  const idl_mask_t types[1] = { idl_type[n] };
  const char* names[1] = { "mem" };

  idl_struct_t* str = generate_struct_sequence("s", types, names, 1);
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = calloc(sizeof(idl_definition_t),1);
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

  create_funcs_sequence(impl, names[0], n, ns);

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
}

void test_string(bool ns)
{
  idl_tree_t* tree = calloc(sizeof(idl_tree_t), 1);

  idl_struct_t* str = generate_struct_string("s");
  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = calloc(sizeof(idl_definition_t), 1);
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

  create_funcs_string(impl, "str", ns);

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));

  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
}

void test_union(bool ns)
{
  idl_union_t* _union = generate_union_int_switch("s", union_switch_int_case_labels, union_switch_int_case_types, union_switch_int_case_names);
  idl_tree_t* tree = calloc(sizeof(idl_tree_t),1);

  if (ns)
  {
    idl_module_t* mod = idl_create_module();
    mod->definitions = calloc(sizeof(idl_definition_t), 1);
    mod->definitions = (idl_definition_t*)_union;
    mod->identifier = _strdup("N");

    tree->root = (idl_node_t*)mod;
    _union->node.parent = (idl_node_t*)mod;
    mod->node.parent = (idl_node_t*)tree;
  }
  else
  {
    tree->root = (idl_node_t*)_union;
    _union->node.parent = (idl_node_t*)tree;
  }

  idl_streamer_output_t* generated = create_idl_streamer_output();
  idl_streamers_generate(tree, generated);

  idl_ostream_t* impl = create_idl_ostream(NULL);
  generate_union_funcs(impl, ns);

  /*
  printf("=========generated============\n%s\n============================\n", get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
  printf("=========tested============\n%s\n============================\n", get_ostream_buffer(impl));
  */

  CU_ASSERT_STRING_EQUAL(ns ? HNP : HNA, get_ostream_buffer(get_idl_streamer_header_buf(generated)));
  CU_ASSERT_STRING_EQUAL(get_ostream_buffer(impl), get_ostream_buffer(get_idl_streamer_impl_buf(generated)));
}

CU_Test(streamer_generator, base_types_namespace_absent)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_mask_t); i++)
    test_base(i, false);
}

CU_Test(streamer_generator, base_types_namespace_present)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_mask_t); i++)
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
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_mask_t); i++)
    test_sequence(i, false);
}

CU_Test(streamer_generator, sequence_namespace_present)
{
  for (size_t i = 0; i < sizeof(idl_type) / sizeof(idl_mask_t); i++)
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
