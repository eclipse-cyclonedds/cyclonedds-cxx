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

#include <stdlib.h>
#include <string.h>

#include "idlcxx/streamer_generator.h"
#include "idlcxx/cpp11backend.h"
#include "idl/tree.h"

#ifndef _WIN32
#define strcpy_s(ptr, len, str) strcpy(ptr, str)
#define sprintf_s(ptr, len, str, ...) sprintf(ptr, str, __VA_ARGS__)
#define strcat_s(ptr, len, str) strcat(ptr, str)
#endif

#define format_ostream_indented(depth,ostr,str,...) \
if (depth > 0) format_ostream(ostr, "%*c", depth, ' '); \
format_ostream(ostr, str, ##__VA_ARGS__);

static const char* struct_write_func_fmt = "size_t write_struct(const %s &obj, void *data, size_t position)";
static const char* primitive_calc_alignment_modulo_fmt = "(%d - position%%%d)%%%d;";
static const char* primitive_calc_alignment_shift_fmt = "(%d - position&%#x)&%#x;";
static const char* primitive_incr_fmt = "  position += ";
static const char* primitive_incr_alignment_fmt = "  position += alignmentbytes;";
static const char* primitive_write_func_padding_fmt = "  memset(data+position,0x0,%d);  //setting padding bytes to 0x0\n";
static const char* primitive_write_func_alignment_fmt = "  memset(data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n";
static const char* primitive_write_func_write_fmt = "  memcpy(data+position,&obj.%s(),%d%s);  //bytes for member: %s\n";
static const char* incr_comment = "  //moving position indicator\n";
static const char* align_comment = "  //alignment\n";
static const char* padding_comment = "  //padding bytes\n";
static const char* instance_write_func_fmt = "  position = write_struct(obj.%s(), data, position);\n";
static const char* namespace_declaration_fmt = "namespace %s\n";
static const char* struct_write_size_func_fmt = "size_t write_size(const %s &obj, size_t offset)";
static const char* primitive_incr_pos = "  position += %d;";
static const char* instance_size_func_calc_fmt = "  position += write_size(obj.%s(), position);\n";
static const char* struct_read_func_fmt = "size_t read_struct(%s &obj, void *data, size_t position)";
static const char* primitive_read_func_read_fmt = "  memcpy(&obj.%s(), data+position, %d);  //bytes for member: %s\n";
static const char* primitive_read_func_seq_fmt = "  memcpy(&sequenceentries, data+position, %d);  //number of entries in the sequence\n";
static const char* instance_read_func_fmt = "  position = read_struct(obj.%s(), data, position);\n";
static const char* seq_size_fmt = "%s().size";
static const char* seq_read_reserve_fmt = "  obj.%s().reserve(sequenceentries);\n";
static const char* seq_structured_write_fmt = "  for (const auto &%s:obj.%s()) position = write_struct(%s,data,position);\n";
static const char* seq_structured_write_size_fmt = "  for (const auto &%s:obj.%s()) position += write_size(%s, position);\n";
static const char* seq_structured_read_copy_fmt = "  for (size_t %s = 0; %s < sequenceentries; %s++) position = read_struct(obj.%s()[%s], data, position);\n";
static const char* seq_primitive_write_fmt = "  memcpy(data+position,obj.%s().data(),sequenceentries*%d);  //contents for %s\n";
static const char* seq_primitive_read_fmt = "  memcpy(obj.%s().data(),data+position,sequenceentries*%d);\n";
static const char* seq_incr_fmt = "  position += sequenceentries*%d;\n";

#if 0
static const char* fixed_pt_write_digits = "    long long int digits = ((long double)obj.%s()/pow(10.0,obj.%s().fixed_scale()));\n";
static const char* fixed_pt_write_byte = "    int byte = (obj.%s().fixed_digits())/2;\n";
static const char* fixed_pt_write_fill[] = {
"    if (digits < 0)\n",
"    {\n",
"      digits *= -1;\n",
"      data[position + byte] = (digits % 10 << 4) | 0x0d;\n",
"    }\n",
"    else\n",
"    {\n",
"      data[position + byte] = (digits % 10 << 4) | 0x0c;\n",
"    }\n",
"    while (byte && digits)\n",
"    {\n",
"      byte--;\n",
"      digits /= 10;\n",
"      data[position + byte] = ((unsigned char)digits) % 10;\n",
"      digits /= 10;\n",
"      data[position + byte] |= ((unsigned char)digits) % 10 * 16;\n",
"    }\n",
"    memset(data + position,0x0,byte);\n"
};
static const char* fixed_pt_write_position = "  position += (obj.%s().fixed_digits()/2) + 1;\n";

static const char* fixed_pt_read_byte = "    int byte = obj.%s().fixed_digits()/2;\n";
static const char* fixed_pt_read_fill[] = {
"    long long int digits = ((unsigned_char)data[byte] & 0xf0) >> 4;\n",
"    if (data[byte] & 0x0d == 0x0d)\n",
"      digits *= -1;\n",
"    while (byte >= 0)\n",
"    {\n",
"      unsigned char temp = *((usigned char*)(data + position + byte));\n",
"      digits *= 10;\n",
"      digits *= 10;\n",
"      byte--;\n",
"    }\n"
};
static const char* fixed_pt_read_assign = "    obj.%s() = (pow((long double)0.1, obj.%s().fixed_scale()) * digits);\n";
static const char* fixed_pt_read_position = "    position += (obj.%s().fixed_digits()/2) + 1;\n";
#endif

struct idl_streamer_output
{
  size_t indent;
  idl_ostream_t* header_stream;
  idl_ostream_t* impl_stream;
};

typedef struct context context_t;

struct context
{
  idl_streamer_output_t* str;
  char* context;
  idl_ostream_t* header_stream;
  idl_ostream_t* write_size_stream;
  idl_ostream_t* write_stream;
  idl_ostream_t* read_stream;
  size_t depth;
  int currentalignment;
  int accumulatedalignment;
  int alignmentpresent;
  int sequenceentriespresent;
  context_t* parent;
};


static idl_retcode_t process_node(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_instance(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_base(context_t* ctx, idl_member_t* member);
static idl_retcode_t process_template(context_t* ctx, idl_member_t* member);
static idl_retcode_t process_known_width(context_t* ctx, const char* name, int bytewidth, int sequence, const char* seqappend);
static int determine_byte_width(const idl_type_spec_t* typespec);
static idl_retcode_t add_alignment(context_t* ctx, int bytewidth);
static idl_retcode_t process_member(context_t* ctx, idl_member_t* member);
static idl_retcode_t process_module(context_t* ctx, idl_module_t* module);
static idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node);
static context_t* create_context(idl_streamer_output_t* str, const char* ctx);
static void flush_streams(context_t* ctx);
static void close_context(context_t* ctx);

static char* generatealignment(int alignto)
{
  char* returnval = NULL;
  if (alignto < 2)
  {
    size_t len = strlen("0;") + 1;
    returnval = malloc(len);
    strcpy_s(returnval, len, "0;");
  }
  else if (alignto == 2)
  {
    size_t len = strlen("position&0x1;") + 1;
    returnval = malloc(len);
    strcpy_s(returnval, len, "position&0x1;");
  }
  else
  {
    int mask = 0xFFFFFF;
    while (mask != 0)
    {
      if (alignto == mask + 1)
      {
        size_t len = strlen(primitive_calc_alignment_shift_fmt) - 2 + 30 + 1;
        returnval = malloc(len);
        sprintf_s(returnval, len, primitive_calc_alignment_shift_fmt, alignto, mask, mask);
        return returnval;
      }
      mask >>= 1;
    }

    size_t len = strlen(primitive_calc_alignment_modulo_fmt) - 10 + 5 + 1;
    returnval = malloc(len);
    sprintf_s(returnval, len, primitive_calc_alignment_modulo_fmt, alignto, alignto, alignto);
  }
  return returnval;
}

int determine_byte_width(const idl_type_spec_t* typespec)
{
  if ((typespec->kind & IDL_ENUM_TYPE) == IDL_ENUM_TYPE)
    return 4;

  switch (typespec->kind)
  {
  case IDL_INT8:
  case IDL_UINT8:
  case IDL_CHAR:
  case IDL_WCHAR:
  case IDL_BOOL:
  case IDL_OCTET:
    return 1;
  case IDL_INT16: // is equal to IDL_SHORT
  case IDL_UINT16: // is equal to IDL_USHORT
    return 2;
  case IDL_INT32: //is equal to IDL_LONG
  case IDL_UINT32: //is equal to IDL_ULONG
  case IDL_FLOAT:
    return 4;
  case IDL_INT64: //is equal to IDL_LLONG
  case IDL_UINT64: //is equal to IDL_ULLONG
  case IDL_DOUBLE:
  case IDL_LDOUBLE:
    return 8;
  }

  return -1;
}

idl_streamer_output_t* create_idl_streamer_output()
{
  idl_streamer_output_t* ptr = malloc(sizeof(idl_streamer_output_t));
  if (NULL != ptr)
  {
    ptr->indent = 0;
    ptr->header_stream = create_idl_ostream(NULL);
    ptr->impl_stream = create_idl_ostream(NULL);
  }
  return ptr;
}

void destruct_idl_streamer_output(idl_streamer_output_t* str)
{
  if (NULL == str)
    return;

  if (str->header_stream != NULL)
    destruct_idl_ostream(str->header_stream);
  if (str->impl_stream != NULL)
    destruct_idl_ostream(str->impl_stream);
  free(str);
}

idl_ostream_t* get_idl_streamer_impl_buf(const idl_streamer_output_t* str)
{
  return str->impl_stream;
}

idl_ostream_t* get_idl_streamer_header_buf(const idl_streamer_output_t* str)
{
  return str->header_stream;
}

context_t* create_context(idl_streamer_output_t* str, const char* ctx)
{
  context_t* ptr = malloc(sizeof(context_t));
  if (NULL != ptr)
  {
    ptr->str = str;
    ptr->depth = 0;
    size_t len = strlen(ctx) + 1;
    ptr->context = malloc(len);
    ptr->context[strlen(ctx)] = 0x0;
    strcpy_s(ptr->context, len, ctx);
    ptr->currentalignment = -1;
    ptr->accumulatedalignment = 0;
    ptr->alignmentpresent = 0;
    ptr->sequenceentriespresent = 0;
    ptr->header_stream = create_idl_ostream(NULL);
    ptr->write_size_stream = create_idl_ostream(NULL);
    ptr->write_stream = create_idl_ostream(NULL);
    ptr->read_stream = create_idl_ostream(NULL);
  }
  //printf("new context generated: %s\n", ctx);
  return ptr;
}

void flush_streams(context_t* ctx)
{
  transfer_ostream_buffer(ctx->header_stream, ctx->str->header_stream);
  transfer_ostream_buffer(ctx->write_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->write_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->read_stream, ctx->str->impl_stream);
}

void close_context(context_t* ctx)
{
  //add closing statements to buffers?
  flush_streams(ctx);

  destruct_idl_ostream(ctx->header_stream);
  destruct_idl_ostream(ctx->write_stream);
  destruct_idl_ostream(ctx->write_size_stream);
  destruct_idl_ostream(ctx->read_stream);

  free(ctx->context);
}

idl_retcode_t process_node(context_t* ctx, idl_node_t* node)
{
  if (idl_is_member(node))
    process_member(ctx, (idl_member_t*)node);
  else if (idl_is_module(node))
    process_module(ctx, (idl_module_t*)node);
  else if (idl_is_constructed_type(node))
    process_constructed(ctx, node);

  if (node->next)
    process_node(ctx, node->next);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_member(context_t* ctx, idl_member_t* member)
{
  if (NULL == ctx || NULL == member)
    return IDL_RETCODE_INVALID_PARSETREE;
  if (member->type_spec->kind & IDL_BASE_TYPE)
    process_base(ctx, member);
  else if (member->type_spec->kind & IDL_SCOPED_NAME)
    process_instance(ctx, &member->node);
  else if (member->type_spec->kind & IDL_TEMPL_TYPE)
    process_template(ctx, member);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_instance(context_t* ctx, idl_node_t* node)
{
  if (NULL == ctx || NULL == node)
    return IDL_RETCODE_INVALID_PARSETREE;
  idl_member_t* member = (idl_member_t*)node;
  // FIXME: this probably needs to loop?
  char* cpp11name = get_cpp_name(member->declarators->identifier);
  format_ostream_indented(ctx->depth * 2, ctx->write_stream, instance_write_func_fmt, cpp11name);
  format_ostream_indented(ctx->depth * 2, ctx->read_stream, instance_read_func_fmt, cpp11name);
  format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, instance_size_func_calc_fmt, cpp11name);

  ctx->accumulatedalignment = 0;
  ctx->currentalignment = -1;

  free(cpp11name);
  return IDL_RETCODE_OK;
}

idl_retcode_t add_alignment(context_t* ctx, int bytewidth)
{
  if (NULL == ctx)
    return IDL_RETCODE_INVALID_PARSETREE;

  //printf("current alignment: %d, byte width: %d, acc: %d\n", ctx->currentalignment, bytewidth, ctx->accumulatedalignment);
  if ((0 > ctx->currentalignment || bytewidth > ctx->currentalignment) && bytewidth != 1)
  {
    if (0 == ctx->alignmentpresent)
    {
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, "  size_t alignmentbytes = ");
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, "  size_t alignmentbytes = ");
      ctx->alignmentpresent = 1;
    }
    else
    {
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, "  alignmentbytes = ");
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, "  alignmentbytes = ");
    }


    char* buffer = generatealignment(bytewidth);
    format_ostream_indented(0, ctx->write_stream, buffer);
    format_ostream_indented(0, ctx->write_stream, align_comment);
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, primitive_write_func_alignment_fmt);
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, primitive_incr_alignment_fmt);
    format_ostream_indented(0, ctx->write_stream, incr_comment);

    format_ostream_indented(0, ctx->read_stream, buffer);
    format_ostream_indented(0, ctx->read_stream, align_comment);
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, primitive_incr_alignment_fmt);
    format_ostream_indented(0, ctx->read_stream, incr_comment);

    format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, primitive_incr_fmt);
    format_ostream_indented(0, ctx->write_size_stream, buffer);
    format_ostream_indented(0, ctx->write_size_stream, align_comment);

    ctx->accumulatedalignment = 0;
    ctx->currentalignment = bytewidth;

    if (buffer)
      free(buffer);
  }
  else
  {
    int missingbytes = (bytewidth - (ctx->accumulatedalignment % bytewidth)) % bytewidth;
    if (0 != missingbytes)
    {
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, primitive_write_func_padding_fmt, missingbytes);
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, primitive_incr_pos, missingbytes);
      format_ostream_indented(0, ctx->write_size_stream, padding_comment);
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, primitive_incr_pos, missingbytes);
      format_ostream_indented(0, ctx->read_stream, padding_comment);
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, primitive_incr_pos, missingbytes);
      format_ostream_indented(0, ctx->write_stream, incr_comment);

      ctx->accumulatedalignment = 0;
    }
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_known_width(context_t* ctx, const char* name, int bytewidth, int sequence, const char* seqappend)
{
  if (NULL == ctx || NULL == name)
    return IDL_RETCODE_INVALID_PARSETREE;

  if (ctx->currentalignment != bytewidth)
    add_alignment(ctx, bytewidth);
  format_ostream_indented(ctx->depth * 2, ctx->write_stream, primitive_write_func_write_fmt, name, bytewidth, seqappend ? seqappend : "", name);

  ctx->accumulatedalignment += bytewidth;

  if (0 == sequence)
  {
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, primitive_read_func_read_fmt, name, bytewidth, name);
  }
  else
  {
    if (0 == ctx->sequenceentriespresent)
    {
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, "  uint32_t sequenceentries;");
      ctx->sequenceentriespresent = 1;
    }
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, primitive_read_func_seq_fmt, bytewidth);
  }

  format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, primitive_incr_pos, bytewidth);
  format_ostream_indented(0, ctx->write_size_stream, "  //bytes for member: ");
  format_ostream_indented(0, ctx->write_size_stream, name);
  format_ostream_indented(0, ctx->write_size_stream, "\n");

  format_ostream_indented(ctx->depth * 2, ctx->write_stream, primitive_incr_pos, bytewidth);
  format_ostream_indented(0, ctx->write_stream, incr_comment);

  format_ostream_indented(ctx->depth * 2, ctx->read_stream, primitive_incr_pos, bytewidth);
  format_ostream_indented(0, ctx->read_stream, incr_comment);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_template(context_t* ctx, idl_member_t* member)
{
  if (NULL == ctx || NULL == member)
    return IDL_RETCODE_INVALID_PARSETREE;

  char* cpp11name = NULL;
  idl_type_spec_t* type_spec = member->type_spec;

  if ((type_spec->kind & IDL_SEQUENCE_TYPE) == IDL_SEQUENCE_TYPE)
  {
    // FIXME: loop!?
    cpp11name = get_cpp_name(member->declarators->identifier);

    size_t cpp11len = strlen(cpp11name);
    size_t bufsize = cpp11len + strlen(seq_size_fmt) - 2 + 1;
    char* buffer = malloc(bufsize);
    sprintf_s(buffer, bufsize, seq_size_fmt, cpp11name);
    process_known_width(ctx, buffer, 4, 1, NULL);

    if (buffer)
      free(buffer);

    //TODO!!! determine whether the templated type is a base type or no
    bool is_base_type = false;
    if (is_base_type)
    {
      int bytewidth = determine_byte_width(type_spec);  //determine byte width of base type?

      if (bytewidth > 4)
        add_alignment(ctx, bytewidth);

      format_ostream_indented(ctx->depth * 2, ctx->write_stream, seq_primitive_write_fmt, cpp11name, bytewidth, cpp11name);
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_read_reserve_fmt, cpp11name);
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_primitive_read_fmt, cpp11name, bytewidth);
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, seq_incr_fmt, bytewidth);
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, seq_incr_fmt, bytewidth);
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_incr_fmt, bytewidth);
    }
    else
    {
      char* iterated_name = NULL;
      size_t iterated_name_size = 0;

      if (strcmp(cpp11name, "_1"))
      {
        iterated_name_size = strlen("_1") + 1;
        iterated_name = malloc(iterated_name_size);
        strcpy_s(iterated_name, iterated_name_size, "_1");
      }
      else
      {
        iterated_name_size = strlen("_2") + 1;
        iterated_name = malloc(iterated_name_size);
        strcpy_s(iterated_name, iterated_name_size, "_2");
      }

      format_ostream_indented(ctx->depth * 2, ctx->write_stream, seq_structured_write_fmt, iterated_name, cpp11name, iterated_name);
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, seq_structured_write_size_fmt, iterated_name, cpp11name, iterated_name);
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_read_reserve_fmt, cpp11name);
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_structured_read_copy_fmt, iterated_name, iterated_name, iterated_name, cpp11name, iterated_name);

      if (NULL != iterated_name)
        free(iterated_name);
    }

    ctx->accumulatedalignment = 0;
    ctx->currentalignment = -1;
  }
  else if ((type_spec->kind & IDL_STRING_TYPE) == IDL_STRING_TYPE ||
    (type_spec->kind & IDL_WSTRING_TYPE) == IDL_WSTRING_TYPE)
  {
    // FIXME: loop!?
    cpp11name = get_cpp_name(member->declarators->identifier);
    size_t cpp11len = strlen(cpp11name);
    size_t bufsize = cpp11len + strlen(seq_size_fmt) - 2 + 1;
    char* buffer = malloc(bufsize);
    sprintf_s(buffer, bufsize, seq_size_fmt, cpp11name);
    process_known_width(ctx, buffer, 4, 1, "+1");

    if (buffer)
      free(buffer);

    format_ostream_indented(ctx->depth * 2, ctx->write_stream, seq_primitive_write_fmt, cpp11name, 1, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_read_reserve_fmt, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_primitive_read_fmt, cpp11name, 1);
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, seq_incr_fmt, 1);
    format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, seq_incr_fmt, 1);
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, seq_incr_fmt, 1);

    ctx->accumulatedalignment = 0;
    ctx->currentalignment = -1;
  }
#if 0
  else if ((type_spec->kind & IDL_FIXED_PT_TYPE) == IDL_FIXED_PT_TYPE)
  {
    //fputs("fixed point type template classes not supported at this time", stderr);

    format_ostream_indented(ctx->depth * 2, ctx->write_stream, "  {\n");
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, fixed_pt_write_digits, cpp11name, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, fixed_pt_write_byte, cpp11name);

    for (size_t i = 0; i < sizeof(fixed_pt_write_fill) / sizeof(const char*); i++)
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, fixed_pt_write_fill[i]);
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, fixed_pt_write_position, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, "  ");
    format_ostream_indented(0, ctx->write_size_stream, fixed_pt_write_position, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->write_stream, "  }\n");
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, "  {\n");
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, fixed_pt_read_byte, cpp11name);

    for (size_t i = 0; i < sizeof(fixed_pt_read_fill) / sizeof(const char*); i++)
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, fixed_pt_read_fill[i]);

    format_ostream_indented(ctx->depth * 2, ctx->read_stream, fixed_pt_read_assign, cpp11name, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, fixed_pt_read_position, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->read_stream, "  }\n");

    ctx->accumulatedalignment = 0;
    ctx->currentalignment = -1;
  }
#endif

  if (cpp11name)
    free(cpp11name);
  return IDL_RETCODE_OK;
}

idl_retcode_t process_module(context_t* ctx, idl_module_t* module)
{
  if (NULL == ctx || NULL == module)
    return IDL_RETCODE_INVALID_PARSETREE;

  if (module->definitions)
  {
    char* cpp11name = get_cpp_name(module->identifier);
    format_ostream_indented(ctx->depth * 2, ctx->str->header_stream, namespace_declaration_fmt, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->str->header_stream, "{\n\n");
    format_ostream_indented(ctx->depth * 2, ctx->str->impl_stream, namespace_declaration_fmt, cpp11name);
    format_ostream_indented(ctx->depth * 2, ctx->str->impl_stream, "{\n\n");

    context_t* newctx = create_context(ctx->str, cpp11name);
    newctx->depth = ctx->depth + 1;

    process_node(newctx, (idl_node_t*)module->definitions);

    close_context(newctx);

    free(newctx);

    format_ostream_indented(ctx->depth * 2, ctx->str->header_stream, "}\n\n");
    format_ostream_indented(ctx->depth * 2, ctx->str->impl_stream, "}\n\n");

    free(cpp11name);
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node)
{
  if (NULL == ctx || NULL == node)
    return IDL_RETCODE_INVALID_PARSETREE;

  char* cpp11name = NULL;

  if (idl_is_struct(node))
  {
    idl_struct_type_t* _struct = (idl_struct_type_t*)node;
    if (_struct->members)
    {
      cpp11name = get_cpp_name(_struct->identifier);
      format_ostream_indented(ctx->depth * 2, ctx->header_stream, struct_write_func_fmt, cpp11name);
      format_ostream_indented(0, ctx->header_stream, ";\n\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, struct_write_func_fmt, cpp11name);
      format_ostream_indented(0, ctx->write_stream, "\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, "{\n");

      format_ostream_indented(ctx->depth * 2, ctx->header_stream, struct_write_size_func_fmt, cpp11name);
      format_ostream_indented(0, ctx->header_stream, ";\n\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, struct_write_size_func_fmt, cpp11name);
      format_ostream_indented(0, ctx->write_size_stream, "\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, "{\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, "  size_t position = offset;\n");

      format_ostream_indented(ctx->depth * 2, ctx->header_stream, struct_read_func_fmt, cpp11name);
      format_ostream_indented(0, ctx->header_stream, ";\n\n");
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, struct_read_func_fmt, cpp11name);
      format_ostream_indented(0, ctx->read_stream, "\n");
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, "{\n");

      ctx->currentalignment = -1;
      ctx->alignmentpresent = 0;
      ctx->sequenceentriespresent = 0;
      ctx->accumulatedalignment = 0;

      process_node(ctx, (idl_node_t*)_struct->members);

      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, "  return position-offset;\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_size_stream, "}\n\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, "  return position;\n");
      format_ostream_indented(ctx->depth * 2, ctx->write_stream, "}\n\n");
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, "  return position;\n");
      format_ostream_indented(ctx->depth * 2, ctx->read_stream, "}\n\n");

      //flush_streams(ctx);
    }
  }
  else if (idl_is_union(node))
  {
    fputs("union constructed types not supported at this time", stderr);
  }
  else if (idl_is_enum(node))
  {
    fputs("enum constructed types not supported at this time", stderr);
  }

  if (cpp11name)
    free(cpp11name);
  return IDL_RETCODE_OK;
}

idl_retcode_t process_base(context_t* ctx, idl_member_t* member)
{
  if (NULL == ctx || NULL == member)
    return IDL_RETCODE_INVALID_PARSETREE;

  char* cpp11name = get_cpp_name(member->declarators->identifier);
  int bytewidth = determine_byte_width(member->type_spec);
  if (1 > bytewidth)
    return IDL_RETCODE_PARSE_ERROR;

  process_known_width(ctx, cpp11name, bytewidth, 0, NULL);

  free(cpp11name);
  return IDL_RETCODE_OK;
}

void idl_streamers_generate(idl_tree_t* tree, idl_streamer_output_t* str)
{
  context_t* ctx = create_context(str, "");
  process_node(ctx, tree->root);
  close_context(ctx);
}
