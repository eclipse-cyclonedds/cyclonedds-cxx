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
#define _strdup(str) strdup(str)
#endif

#define format_ostream_indented(depth,ostr,str,...) \
if (depth > 0) format_ostream(ostr, "%*c", depth, ' '); \
format_ostream(ostr, str, ##__VA_ARGS__);

#define format_write_stream(indent,ctx,str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->write_stream, str, ##__VA_ARGS__);

#define format_write_size_stream(indent,ctx,str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->write_size_stream, str, ##__VA_ARGS__);

#define format_read_stream(indent,ctx,str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->read_stream, str, ##__VA_ARGS__);

#define format_header_stream(indent,ctx,str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->header_stream, str, ##__VA_ARGS__);

static const char* struct_write_func_fmt = "size_t write_struct(const %s &obj, void *data, size_t position)";
static const char* union_switch_fmt = "  switch (obj._d())\n";
static const char* union_case_fmt = "  case %s:\n";
static const char* union_case_ending = "  break;\n";
static const char* union_clear_func = "  obj.clear();\n";
static const char* primitive_calc_alignment_modulo_fmt = "(%d - position%%%d)%%%d;";
static const char* primitive_calc_alignment_shift_fmt = "(%d - position&%#x)&%#x;";
static const char* primitive_incr_fmt = "  position += ";
static const char* primitive_incr_alignment_fmt = "  position += alignmentbytes;";
static const char* primitive_write_func_padding_fmt = "  memset((char*)data+position,0x0,%d);  //setting padding bytes to 0x0\n";
static const char* primitive_write_func_alignment_fmt = "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n";
static const char* primitive_write_func_write_fmt = "  *((%s*)((char*)data+position)) = obj.%s();  //writing bytes for member: %s\n";
static const char* primitive_write_func_seq_fmt = "sequenceentries = obj.%s()%s;  //number of entries in the sequence\n";
static const char* primitive_write_func_seq2_fmt = "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing bytes for member: %s\n";
static const char* incr_comment = "  //moving position indicator\n";
static const char* align_comment = "  //alignment\n";
static const char* padding_comment = "  //padding bytes\n";
static const char* instance_write_func_fmt = "  position = write_struct(obj.%s(), data, position);\n";
static const char* namespace_declaration_fmt = "namespace %s\n";
static const char* namespace_closure_fmt = "} //end namespace %s\n\n";
static const char* struct_write_size_func_fmt = "size_t write_size(const %s &obj, size_t offset)";
static const char* primitive_incr_pos = "  position += %d;";
static const char* instance_size_func_calc_fmt = "  position += write_size(obj.%s(), position);\n";
static const char* struct_read_func_fmt = "size_t read_struct(%s &obj, void *data, size_t position)";
static const char* primitive_read_func_read_fmt = "  obj.%s(*((%s*)((char*)data+position)));  //reading bytes for member: %s\n";
static const char* primitive_read_func_seq_fmt = "sequenceentries = *((%s*)((char*)data+position));  //number of entries in the sequence\n";
static const char* instance_read_func_fmt = "  position = read_struct(obj.%s(), data, position);\n";
static const char* seq_size_fmt = "%s().size";
static const char* seq_read_resize_fmt = "  obj.%s().resize(sequenceentries);\n";
static const char* seq_structured_write_fmt = "  for (const auto &%s:obj.%s()) position = write_struct(%s,data,position);\n";
static const char* seq_structured_write_size_fmt = "  for (const auto &%s:obj.%s()) position += write_size(%s, position);\n";
static const char* seq_structured_read_copy_fmt = "  for (size_t %s = 0; %s < sequenceentries; %s++) position = read_struct(obj.%s()[%s], data, position);\n";
static const char* seq_primitive_write_fmt = "  memcpy((char*)data+position,obj.%s().data(),sequenceentries*%d);  //contents for %s\n";
static const char* seq_primitive_read_fmt = "  obj.%s().assign((%s*)((char*)data+position),(%s*)((char*)data+position)+sequenceentries);  //putting data into container\n";
static const char* seq_incr_fmt = "  position += sequenceentries*%d;";
static const char* seq_entries_fmt = "  position += (obj.%s().size()%s)*%d;  //entries of sequence\n";

static const char* char_cast = "char";
static const char* bool_cast = "bool";
static const char* int8_cast = "int8_t";
static const char* uint8_cast = "uint8_t";
static const char* int16_cast = "int16_t";
static const char* uint16_cast = "uint16_t";
static const char* int32_cast = "int32_t";
static const char* uint32_cast = "uint32_t";
static const char* int64_cast = "int64_t";
static const char* uint64_cast = "uint64_t";
static const char* float_cast = "float";
static const char* double_cast = "double";
static const char* ldouble_cast = "long double";

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
static idl_retcode_t process_instance(context_t* ctx, idl_declarator_t* decl);
static idl_retcode_t process_base(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec);
static idl_retcode_t process_template(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec);
static idl_retcode_t process_known_width(context_t* ctx, const char* name, idl_mask_t typespec, int sequence, const char *seqsizeappend);
static int determine_byte_width(idl_mask_t typespec);
static const char* determine_cast(idl_mask_t mask);
static idl_retcode_t add_alignment(context_t* ctx, int bytewidth);
static idl_retcode_t add_null(context_t* ctx, int nbytes);
static idl_retcode_t process_member(context_t* ctx, idl_member_t* member);
static idl_retcode_t process_module(context_t* ctx, idl_module_t* module);
static idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_case(context_t* ctx, idl_case_t* _case);
static idl_retcode_t process_case_label(context_t* ctx, idl_case_label_t* label);
static context_t* create_context(idl_streamer_output_t* str, const char* name);
static context_t* child_context(context_t* ctx, const char* name);
static void flush_streams(context_t* ctx);
static void close_context(context_t* ctx);

static char* generatealignment(int alignto)
{
  char* returnval = NULL;
  if (alignto < 2)
  {
    size_t len = strlen("0;") + 1;
    returnval = calloc(len,1);
    strcpy_s(returnval, len, "0;");
  }
  else if (alignto == 2)
  {
    size_t len = strlen("position&0x1;") + 1;
    returnval = calloc(len,1);
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
        returnval = calloc(len,1);
        sprintf_s(returnval, len, primitive_calc_alignment_shift_fmt, alignto, mask, mask);
        return returnval;
      }
      mask >>= 1;
    }

    size_t len = strlen(primitive_calc_alignment_modulo_fmt) - 10 + 5 + 1;
    returnval = calloc(len,1);
    sprintf_s(returnval, len, primitive_calc_alignment_modulo_fmt, alignto, alignto, alignto);
  }
  return returnval;
}

int determine_byte_width(idl_mask_t mask)
{
  switch (mask % (IDL_BASE_TYPE*2))
  {
  case IDL_INT8:
  case IDL_UINT8:
  case IDL_CHAR:
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
    return 8;
  case IDL_LDOUBLE:
    return sizeof(long double);
  }

  return -1;
}

idl_streamer_output_t* create_idl_streamer_output()
{
  idl_streamer_output_t* ptr = calloc(sizeof(idl_streamer_output_t),1);
  if (NULL != ptr)
  {
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

context_t* create_context(idl_streamer_output_t* str, const char* name)
{
  context_t* ptr = calloc(sizeof(context_t),1);
  if (NULL != ptr)
  {
    ptr->str = str;
    ptr->context = _strdup(name);
    ptr->currentalignment = -1;
    ptr->header_stream = create_idl_ostream(NULL);
    ptr->write_size_stream = create_idl_ostream(NULL);
    ptr->write_stream = create_idl_ostream(NULL);
    ptr->read_stream = create_idl_ostream(NULL);
  }
  return ptr;
}

context_t* child_context(context_t* ctx, const char* name)
{
  context_t *ptr = create_context(ctx->str, name);

  if (NULL != ptr)
  {
    ptr->parent = ctx;
    ptr->depth = ctx->depth + 1;
  }

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
  flush_streams(ctx);

  destruct_idl_ostream(ctx->header_stream);
  destruct_idl_ostream(ctx->write_stream);
  destruct_idl_ostream(ctx->write_size_stream);
  destruct_idl_ostream(ctx->read_stream);

  free(ctx->context);
}

idl_retcode_t process_node(context_t* ctx, idl_node_t* node)
{
  if (idl_is_module(node))
    process_module(ctx, (idl_module_t*)node);
  else if (idl_is_struct(node) || idl_is_union(node) || idl_is_enum(node))
    process_constructed(ctx, node);

  if (node->next)
    process_node(ctx, node->next);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_member(context_t* ctx, idl_member_t* member)
{
  if (NULL == ctx || NULL == member)
    return IDL_RETCODE_INVALID_PARSETREE;
  if ((member->type_spec->mask & IDL_BASE_TYPE) == IDL_BASE_TYPE)
    // FIXME: this probably needs to loop to find the correct declarator?
    process_base(ctx, member->declarators, member->type_spec);
  else if ((member->type_spec->mask & IDL_STRUCT) == IDL_STRUCT)
    // FIXME: this probably needs to loop to find the correct declarator?
    process_instance(ctx, member->declarators);
  else if ((member->type_spec->mask & IDL_TEMPL_TYPE) == IDL_TEMPL_TYPE)
    // FIXME: this probably needs to loop to find the correct declarator?
    process_template(ctx, member->declarators, member->type_spec);

  if (member->node.next)
    process_member(ctx, (idl_member_t*)member->node.next);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_instance(context_t* ctx, idl_declarator_t* decl)
{
  if (NULL == ctx || NULL == decl)
    return IDL_RETCODE_INVALID_PARSETREE;
  char* cpp11name = get_cpp_name(decl->identifier);
  format_write_stream(1, ctx, instance_write_func_fmt, cpp11name);
  format_read_stream(1, ctx, instance_read_func_fmt, cpp11name);
  format_write_size_stream(1, ctx, instance_size_func_calc_fmt, cpp11name);

  ctx->accumulatedalignment = 0;
  ctx->currentalignment = -1;

  free(cpp11name);
  return IDL_RETCODE_OK;
}

idl_retcode_t add_alignment(context_t* ctx, int bytewidth)
{
  if (NULL == ctx)
    return IDL_RETCODE_INVALID_PARSETREE;

  if ((0 > ctx->currentalignment || bytewidth > ctx->currentalignment) && bytewidth != 1)
  {
    if (0 == ctx->alignmentpresent)
    {
      format_write_stream(1, ctx, "  size_t alignmentbytes = ");
      ctx->alignmentpresent = 1;
    }
    else
    {
      format_write_stream(1, ctx, "  alignmentbytes = ");
    }


    char* buffer = generatealignment(bytewidth);
    format_write_stream(0, ctx, buffer);
    format_write_stream(0, ctx, align_comment);
    format_write_stream(1, ctx, primitive_write_func_alignment_fmt);
    format_write_stream(1, ctx, primitive_incr_alignment_fmt);
    format_write_stream(0, ctx, incr_comment);

    format_read_stream(1, ctx, primitive_incr_fmt);
    format_read_stream(0, ctx, buffer);
    format_read_stream(0, ctx, align_comment);

    format_write_size_stream(1, ctx, primitive_incr_fmt);
    format_write_size_stream(0, ctx, buffer);
    format_write_size_stream(0, ctx, align_comment);

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
      add_null(ctx, missingbytes);
      ctx->accumulatedalignment = 0;
    }
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t add_null(context_t* ctx, int nbytes)
{
  format_write_stream(1, ctx, primitive_write_func_padding_fmt, nbytes);
  format_write_stream(1, ctx, primitive_incr_pos, nbytes);
  format_write_stream(0, ctx, incr_comment);
  format_write_size_stream(1, ctx, primitive_incr_pos, nbytes);
  format_write_size_stream(0, ctx, padding_comment);
  format_read_stream(1, ctx, primitive_incr_pos, nbytes);
  format_read_stream(0, ctx, padding_comment);

  return IDL_RETCODE_OK;
}

const char* determine_cast(idl_mask_t mask)
{
  mask %= IDL_BASE_TYPE * 2;
  switch (mask)
  {
  case IDL_CHAR:
    return char_cast;
    break;
  case IDL_BOOL:
    return bool_cast;
    break;
  case IDL_INT8:
    return int8_cast;
    break;
  case IDL_UINT8:
  case IDL_OCTET:
    return uint8_cast;
    break;
  case IDL_INT16:
    //case IDL_SHORT:
    return int16_cast;
    break;
  case IDL_UINT16:
    //case IDL_USHORT:
    return uint16_cast;
    break;
  case IDL_INT32:
    //case IDL_LONG:
    return int32_cast;
    break;
  case IDL_UINT32:
    //case IDL_ULONG:
    return uint32_cast;
    break;
  case IDL_INT64:
    //case IDL_LLONG:
    return int64_cast;
    break;
  case IDL_UINT64:
    //case IDL_ULLONG:
    return uint64_cast;
    break;
  case IDL_FLOAT:
    return float_cast;
    break;
  case IDL_DOUBLE:
    return double_cast;
    break;
  case IDL_LDOUBLE:
    return ldouble_cast;
  }
  return NULL;
}

idl_retcode_t process_known_width(context_t* ctx, const char* name, idl_mask_t mask, int sequence, const char *seqsizeappend)
{
  if (NULL == ctx || NULL == name)
    return IDL_RETCODE_INVALID_PARSETREE;

  if ((mask & IDL_ENUM) == IDL_ENUM)
    mask = IDL_UINT32;

  const char* cast_fmt = determine_cast(mask);

  if (NULL == cast_fmt)
    return IDL_RETCODE_INVALID_PARSETREE;

  int bytewidth = determine_byte_width(mask);
  if (-1 == bytewidth)
    return IDL_RETCODE_INVALID_PARSETREE;

  if (ctx->currentalignment != bytewidth)
    add_alignment(ctx, bytewidth);

  ctx->accumulatedalignment += bytewidth;

  if (0 == sequence)
  {
    format_read_stream(1, ctx, primitive_read_func_read_fmt, name, cast_fmt, name);
    format_write_stream(1, ctx, primitive_write_func_write_fmt, cast_fmt, name, name);
  }
  else
  {
    format_read_stream(1, ctx, "  ");
    format_write_stream(1, ctx, "  ");
    if (0 == ctx->sequenceentriespresent)
    {
      format_read_stream(0, ctx, "uint32_t ");
      format_write_stream(0, ctx, "uint32_t ");
      ctx->sequenceentriespresent = 1;
    }
    format_read_stream(0, ctx, primitive_read_func_seq_fmt, cast_fmt);
    format_write_stream(0, ctx, primitive_write_func_seq_fmt, name, seqsizeappend);
    format_write_stream(1, ctx, primitive_write_func_seq2_fmt, name);
  }

  format_write_size_stream(1, ctx, primitive_incr_pos, bytewidth);
  format_write_size_stream(0, ctx, "  //bytes for member: ");
  format_write_size_stream(0, ctx, name);
  format_write_size_stream(0, ctx, "\n");

  format_write_stream(1, ctx, primitive_incr_pos, bytewidth);
  format_write_stream(0, ctx, incr_comment);

  format_read_stream(1, ctx, primitive_incr_pos, bytewidth);
  format_read_stream(0, ctx, incr_comment);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_template(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec)
{
  if (NULL == ctx || NULL == decl || NULL == tspec)
    return IDL_RETCODE_INVALID_PARSETREE;

  char* cpp11name = NULL;
  idl_mask_t member_mask = tspec->mask;

  if ((member_mask & IDL_SEQUENCE) == IDL_SEQUENCE ||
      (member_mask & IDL_STRING) == IDL_STRING)
  {
    // FIXME: loop!?
    cpp11name = get_cpp_name(decl->identifier);

    if ((member_mask & IDL_SEQUENCE) == IDL_SEQUENCE)
      //change member_mask to the type of the sequence template
      member_mask = ((idl_sequence_t*)tspec)->type_spec->mask;

    size_t bufsize = strlen(cpp11name) + strlen(seq_size_fmt) - 2 + 1;
    char* buffer = calloc(bufsize,1);
    sprintf_s(buffer, bufsize, seq_size_fmt, cpp11name);
    process_known_width(ctx, buffer, IDL_UINT32, 1, (member_mask & IDL_STRING) == IDL_STRING ? "+1":"");

    if (buffer)
      free(buffer);

    if (member_mask == IDL_WCHAR)
    {
      return IDL_RETCODE_INVALID_PARSETREE;
    }
    else if ((member_mask & IDL_BASE_TYPE) == IDL_BASE_TYPE ||
             (member_mask & IDL_STRING) == IDL_STRING)
    {
      int bytewidth = 1;

      const char* cast_fmt = char_cast;
      if ((member_mask & IDL_BASE_TYPE) == IDL_BASE_TYPE)
      {
        cast_fmt = determine_cast(member_mask);
        bytewidth = determine_byte_width(member_mask);  //determine byte width of base type
      }
      if (bytewidth > 4)
        add_alignment(ctx, bytewidth);

      format_write_stream(1, ctx, seq_primitive_write_fmt, cpp11name, bytewidth, cpp11name);
      format_read_stream(1, ctx, seq_primitive_read_fmt, cpp11name, cast_fmt, cast_fmt);
      format_write_stream(1, ctx, seq_incr_fmt, bytewidth);
      format_write_stream(0, ctx, incr_comment);
      format_write_size_stream(1, ctx, seq_entries_fmt, cpp11name, (member_mask & IDL_STRING) == IDL_STRING ? "+1" : "", bytewidth);
      format_read_stream(1, ctx, seq_incr_fmt, bytewidth);
      format_read_stream(0, ctx, incr_comment);
    }
    else
    {
      char* iterated_name = _strdup("_1");

      if (0 == strcmp(cpp11name, iterated_name))
        iterated_name = _strdup("_2");

      format_write_stream(1, ctx, seq_structured_write_fmt, iterated_name, cpp11name, iterated_name);
      format_write_size_stream(1, ctx, seq_structured_write_size_fmt, iterated_name, cpp11name, iterated_name);
      format_read_stream(1, ctx, seq_read_resize_fmt, cpp11name);
      format_read_stream(1, ctx, seq_structured_read_copy_fmt, iterated_name, iterated_name, iterated_name, cpp11name, iterated_name);

      free(iterated_name);
    }

    ctx->accumulatedalignment = 0;
    ctx->currentalignment = -1;
  }
  else if (member_mask == IDL_WSTRING)
  {
    return IDL_RETCODE_INVALID_PARSETREE;
  }
#if 0
  else if (member_mask == IDL_FIXED_PT_TYPE)
  {
    //fputs("fixed point type template classes not supported at this time", stderr);

    format_write_stream(1, ctx, "  {\n");
    format_write_stream(1, ctx, fixed_pt_write_digits, cpp11name, cpp11name);
    format_write_stream(1, ctx, fixed_pt_write_byte, cpp11name);

    for (size_t i = 0; i < sizeof(fixed_pt_write_fill) / sizeof(const char*); i++)
    {
      format_write_stream(1, ctx, fixed_pt_write_fill[i]);
    }
    format_write_stream(1, ctx, fixed_pt_write_position, cpp11name);
    format_write_size_stream(1, ctx, "  ");
    format_write_size_stream(0, ctx, fixed_pt_write_position, cpp11name);
    format_write_stream(1, ctx, "  }\n");
    format_read_stream(1, ctx, "  {\n");
    format_read_stream(1, ctx, fixed_pt_read_byte, cpp11name);

    for (size_t i = 0; i < sizeof(fixed_pt_read_fill) / sizeof(const char*); i++)
    {
      format_read_stream(1, ctx, fixed_pt_read_fill[i]);
    }

    format_read_stream(1, ctx, fixed_pt_read_assign, cpp11name, cpp11name);
    format_read_stream(1, ctx, fixed_pt_read_position, cpp11name);
    format_read_stream(1, ctx, "  }\n");

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

    context_t* newctx = child_context(ctx, cpp11name);

    format_header_stream(1, ctx, namespace_declaration_fmt, cpp11name);
    format_header_stream(1, ctx, "{\n\n");
    format_write_stream(1, ctx, namespace_declaration_fmt, cpp11name);
    format_write_stream(1, ctx, "{\n\n");

    flush_streams(ctx);

    process_node(newctx, (idl_node_t*)module->definitions);

    close_context(newctx);
    free(newctx);

    format_header_stream(1, ctx, namespace_closure_fmt, cpp11name);
    format_read_stream(1, ctx, namespace_closure_fmt, cpp11name);


    free(cpp11name);
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node)
{
  if (NULL == ctx || NULL == node)
    return IDL_RETCODE_INVALID_PARSETREE;

  char* cpp11name = NULL;

  if (idl_is_struct(node) ||
      idl_is_union(node))
  {
    if (idl_is_struct(node))
      cpp11name = get_cpp_name(((idl_struct_t*)node)->identifier);
    else if (idl_is_union(node))
      cpp11name = get_cpp_name(((idl_union_t*)node)->identifier);

    format_header_stream(1, ctx, struct_write_func_fmt, cpp11name);
    format_header_stream(0, ctx, ";\n\n");
    format_write_stream(1, ctx, struct_write_func_fmt, cpp11name);
    format_write_stream(0, ctx, "\n");
    format_write_stream(1, ctx, "{\n");

    format_header_stream(1, ctx, struct_write_size_func_fmt, cpp11name);
    format_header_stream(0, ctx, ";\n\n");
    format_write_size_stream(1, ctx, struct_write_size_func_fmt, cpp11name);
    format_write_size_stream(0, ctx, "\n");
    format_write_size_stream(1, ctx, "{\n");
    format_write_size_stream(1, ctx, "  size_t position = offset;\n");

    format_header_stream(1, ctx, struct_read_func_fmt, cpp11name);
    format_header_stream(0, ctx, ";\n\n");
    format_read_stream(1, ctx, struct_read_func_fmt, cpp11name);
    format_read_stream(0, ctx, "\n");
    format_read_stream(1, ctx, "{\n");

    ctx->currentalignment = -1;
    ctx->alignmentpresent = 0;
    ctx->sequenceentriespresent = 0;
    ctx->accumulatedalignment = 0;

    if (idl_is_struct(node))
    {
      idl_struct_t* _struct = (idl_struct_t*)node;
      if (_struct->members)
        process_member(ctx, _struct->members);
    }
    else if (idl_is_union(node))
    {
      idl_union_t* _union = (idl_union_t*)node;
      idl_switch_type_spec_t* st = _union->switch_type_spec;

      idl_mask_t disc_mask = st->mask;
      if ((disc_mask & IDL_FLOATING_PT_TYPE) == IDL_FLOATING_PT_TYPE)
        return IDL_RETCODE_INVALID_PARSETREE;
      else if ((disc_mask & IDL_ENUMERATOR) == IDL_ENUMERATOR)
        disc_mask = IDL_ULONG;
      else if ((disc_mask & IDL_BASE_TYPE) != IDL_BASE_TYPE)
        return IDL_RETCODE_INVALID_PARSETREE;

      format_read_stream(1, ctx, union_clear_func);
      process_known_width(ctx, "_d", disc_mask, 0, "");
      format_write_size_stream(1, ctx, union_switch_fmt);
      format_write_size_stream(1, ctx, "  {\n");
      format_write_stream(1, ctx, union_switch_fmt);
      format_write_stream(1, ctx, "  {\n");
      format_read_stream(1, ctx, union_switch_fmt);
      format_read_stream(1, ctx, "  {\n");

      if (_union->cases)
      {
        ctx->depth++;
        process_case(ctx, _union->cases);
        ctx->depth--;
      }

      ctx->currentalignment = -1;
      ctx->accumulatedalignment = 0;
      ctx->alignmentpresent = 0;

      format_write_stream(1, ctx, "  }\n");
      format_write_size_stream(1, ctx, "  }\n");
      format_read_stream(1, ctx, "  }\n");
    }

    format_write_size_stream(1, ctx, "  return position-offset;\n");
    format_write_size_stream(1, ctx, "}\n\n");
    format_write_stream(1, ctx, "  return position;\n");
    format_write_stream(1, ctx, "}\n\n");
    format_read_stream(1, ctx, "  return position;\n");
    format_read_stream(1, ctx, "}\n\n");
  }
  else if (idl_is_enum(node))
  {
    fputs("enum constructed types not supported at this time", stderr);
  }

  if (cpp11name)
    free(cpp11name);
  return IDL_RETCODE_OK;
}

idl_retcode_t process_case(context_t* ctx, idl_case_t* _case)
{
  if (_case->case_labels)
    process_case_label(ctx, _case->case_labels);

  format_write_stream(1, ctx, "  {\n");
  format_write_size_stream(1, ctx, "  {\n");
  format_read_stream(1, ctx, "  {\n");
  ctx->depth++;

  if ((_case->type_spec->mask & IDL_BASE_TYPE) == IDL_BASE_TYPE)
    process_base(ctx, _case->declarator, _case->type_spec);
  else if ((_case->type_spec->mask & IDL_STRUCT) == IDL_STRUCT)
    process_instance(ctx, _case->declarator);
  else if ((_case->type_spec->mask & IDL_TEMPL_TYPE) == IDL_TEMPL_TYPE)
    process_template(ctx, _case->declarator, _case->type_spec);
  else
    return IDL_RETCODE_INVALID_PARSETREE;

  ctx->depth--;
  format_write_stream(1, ctx, "  }\n");
  format_write_stream(1, ctx, union_case_ending);
  format_write_size_stream(1, ctx, "  }\n");
  format_write_size_stream(1, ctx, union_case_ending);
  format_read_stream(1, ctx, "  }\n");
  format_read_stream(1, ctx, union_case_ending);

  //reset alignment to 4
  ctx->currentalignment = 4;
  ctx->accumulatedalignment = 0;
  ctx->alignmentpresent = 1;

  //go to next case
  if (_case->node.next)
    process_case(ctx, (idl_case_t*)_case->node.next);
  return IDL_RETCODE_OK;
}

idl_retcode_t process_case_label(context_t* ctx, idl_case_label_t* label)
{
  idl_const_expr_t* ce = label->const_expr;
  char* buffer = NULL;
  if ((ce->mask & IDL_LITERAL) == IDL_LITERAL)
  {
    idl_literal_t* lit = (idl_literal_t*)ce;
    void* ptr = &(lit->value);
    if ((ce->mask & IDL_INTEGER_LITERAL) == IDL_INTEGER_LITERAL)
    {
      int n = snprintf(buffer, 0, "%lu", *((uint64_t*)ptr));
      if (n < 0)
        return IDL_RETCODE_SYNTAX_ERROR;
      buffer = calloc((size_t)n + 1,1);
      snprintf(buffer, (size_t)n + 1, "%lu", *((uint64_t*)ptr));
    }
    else if ((ce->mask & IDL_BOOLEAN_LITERAL) == IDL_BOOLEAN_LITERAL)
    {
      buffer = _strdup(*((bool*)ptr) ? "true" : "false");
    }
    else if ((ce->mask & IDL_CHAR_LITERAL) == IDL_CHAR_LITERAL)
    {
      size_t len = strlen((char*)ptr)+3;
      buffer = calloc(len,1);
      snprintf(buffer, len, "\'%s\'", (char*)ptr);
    }
  }
  else 
    return IDL_RETCODE_INVALID_PARSETREE;

  if (buffer)
  {
    format_write_stream(1, ctx, union_case_fmt, buffer);
    format_write_size_stream(1, ctx, union_case_fmt, buffer);
    format_read_stream(1, ctx, union_case_fmt, buffer);
  }

  if (label->node.next)
    process_case_label(ctx, (idl_case_label_t*)label->node.next);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_base(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec)
{
  if (NULL == ctx || NULL == decl || NULL == tspec)
    return IDL_RETCODE_INVALID_PARSETREE;

  char* cpp11name = get_cpp_name(decl->identifier);
  process_known_width(ctx, cpp11name, tspec->mask, 0, "");

  free(cpp11name);
  return IDL_RETCODE_OK;
}

void idl_streamers_generate(idl_tree_t* tree, idl_streamer_output_t* str)
{
  context_t* ctx = create_context(str, "");
  process_node(ctx, tree->root);
  close_context(ctx);
}
