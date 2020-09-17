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
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "idlcxx/streamer_generator.h"
#include "idlcxx/backendCpp11Utils.h"
#include "idl/tree.h"
#include "idl/string.h"

#define format_ostream_indented(depth,ostr,...) \
if (depth > 0) format_ostream(ostr, "%*c", depth, ' '); \
format_ostream(ostr, __VA_ARGS__);

#define format_header_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->str->head_stream, _str, ##__VA_ARGS__);

#define format_impl_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->str->impl_stream, _str, ##__VA_ARGS__);

#define format_key_size_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->key_size_stream, _str, ##__VA_ARGS__);

#define format_key_max_size_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->key_max_size_stream, _str, ##__VA_ARGS__);

#define format_key_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->key_stream, _str, ##__VA_ARGS__);

#define format_write_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->write_stream, _str, ##__VA_ARGS__); \
if (key) {format_key_stream(indent,ctx,_str, ##__VA_ARGS__);}

#define format_write_size_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->write_size_stream, _str, ##__VA_ARGS__); \
if (key) {format_key_size_stream(indent,ctx,_str, ##__VA_ARGS__);}

#define format_read_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->read_stream, _str, ##__VA_ARGS__);

#define namespace_declaration "namespace %s\n"
#define namespace_closure "} //end " namespace_declaration "\n"
#define incr_comment "  //moving position indicator\n"
#define align_comment "  //alignment\n"
#define padding_comment "  //padding bytes\n"
#define bytes_for_member_comment "  //bytes for member: %s\n"
#define bytes_for_seq_entries_comment "  //bytes for sequence entries\n"
#define union_switch "  switch (_d())\n"
#define union_case "  case %s:\n"
#define default_case "  default:\n"
#define union_case_ending "  break;\n"
#define union_clear_func "  clear();\n"
#define open_block "{\n"
#define close_block "}\n"
#define close_function close_block "\n"
#define primitive_calc_alignment_modulo "(%d - position%%%d)%%%d;"
#define primitive_calc_alignment_shift "(%d - position&%#x)&%#x;"
#define position_incr "  position += "
#define position_set "  position = "
#define position_return "  return position;\n"
#define position_incr_alignment position_incr "alignmentbytes;"
#define primitive_incr_pos position_incr "%d;"
#define key_max_size_check "  if (position != UINT_MAX) "
#define key_max_size_incr_checked key_max_size_check primitive_incr_pos
#define key_max_size_boundary position_set "UINT_MAX;\n"
#define key_max_size_instance_checked key_max_size_check position_set "%s.key_max_size(position);"
#define primitive_write_func_padding "  memset((char*)data+position,0x0,%d);  //setting padding bytes to 0x0\n"
#define primitive_write_func_alignment "  memset((char*)data+position,0x0,alignmentbytes);  //setting alignment bytes to 0x0\n"
#define primitive_write_func_write "  *((%s*)((char*)data+position)) = %s;  //writing bytes for member: %s\n"
#define primitive_write_func_array "  memcpy((char*)data+position,%s.data(),%d);  //writing bytes for member: %s\n"
#define primitive_write_func_seq "sequenceentries = %s.size()%s;  //number of entries in the sequence\n"
#define primitive_write_func_seq2 "  *((uint32_t*)((char*)data + position)) = sequenceentries;  //writing entries for member: %s\n"
#define primitive_read_func_read "  %s = *((%s*)((char*)data+position));  //reading bytes for member: %s\n"
#define primitive_read_func_array "  memcpy(%s.data(),(char*)data+position,%d);  //reading bytes for member: %s\n"
#define primitive_read_func_seq "sequenceentries = *((uint32_t*)((char*)data+position));  //number of entries in the sequence\n"
#define sequence_iterate "  for (size_t _i = 0; _i < sequenceentries; _i++) "
#define sequence_iterate_checked "  for (size_t _i = 0; _i < sequenceentries && position != UINT_MAX; _i++) "
#define seq_primitive_write "  memcpy((char*)data+position,%s.data(),sequenceentries*%d);  //contents for %s\n"
#define seq_primitive_read "  %s.assign((%s*)((char*)data+position),(%s*)((char*)data+position)+sequenceentries);  //putting data into container\n"
#define seq_primitive_read_nocast "  %s.assign((char*)data+position,(char*)data+position+sequenceentries);  //putting data into container\n"
#define seq_read_resize "  %s.resize(sequenceentries);\n"
#define seq_length_exception "  if (sequenceentries > %zu) throw dds::core::InvalidArgumentError(\"attempt to assign entries to bounded member %s in excess of maximum length %zu\");\n"
#define seq_typedef_write sequence_iterate position_set "%stypedef_write_%s(%s[_i],data,position);\n"
#define seq_typedef_write_size sequence_iterate position_set "%stypedef_write_size_%s(%s[_i], position);\n"
#define seq_typedef_read_copy sequence_iterate position_set "%stypedef_read_%s(%s[_i], data, position);\n"
#define seq_typedef_key_max_size sequence_iterate_checked position_set "%stypedef_key_max_size_%s(%s[_i], position);\n"
#define seq_typedef_key sequence_iterate position_set "%stypedef_key_stream_%s(%s[_i], data, position);\n"
#define seq_typedef_key_size sequence_iterate position_set "%stypedef_key_size_%s(%s[_i], data, position);\n"
#define seq_struct_write sequence_iterate position_set "%s[_i].write_struct(data,position);\n"
#define seq_struct_write_size sequence_iterate position_set "%s[_i].write_size(position);\n"
#define seq_struct_read_copy sequence_iterate position_set "%s[_i].read_struct(data, position);\n"
#define seq_struct_key sequence_iterate position_set "%s[_i].key_stream(data,position);\n"
#define seq_struct_key_size sequence_iterate position_set "%s[_i].key_size(position);\n"
#define seq_struct_key_max_size sequence_iterate_checked position_set "%s[_i].key_max_size(position);"
#define seq_incr position_incr "sequenceentries*%d;"
#define seq_entries position_incr "(%s.size()%s)*%d;  //entries of sequence\n"
#define array_iterate "  for (size_t _i = 0; _i < %d; _i++) "
#define arr_struct_write array_iterate position_set "%s[_i].write_struct(data, position);\n"
#define arr_struct_write_size array_iterate position_set "%s[_i].write_size(position);\n"
#define arr_struct_read array_iterate position_set "%s[_i].read_struct(data, position);\n"
#define arr_struct_key array_iterate position_set "%s[_i].key_stream(data, position);\n"
#define arr_struct_key_size array_iterate position_set "%s[_i].key_size(position);\n"
#define arr_struct_key_max_size array_iterate key_max_size_check position_set "%s[_i].key_max_size(position);\n"
#define instance_write_func position_set "%s.write_struct(data, position);\n"
#define instance_key_write_func position_set "%s.key_stream(data, position);\n"
#define instance_size_func_calc position_set "%s.write_size(position);\n"
#define instance_key_size_func_calc position_set "%s.key_size(position);\n"
#define instance_key_max_size_func_calc key_max_size_check position_set "%s.key_max_size(position);\n"
#define instance_key_max_size_union_func_calc "case_max = %s.key_max_size(case_max);\n"
#define instance_read_func position_set "%s.read_struct(data, position);\n"
#define ref_cast "dynamic_cast<%s%s&>(*this)"
#define member_access "%s()"
#define write_func_define "size_t %s::write_struct(void *data, size_t position) const"
#define write_size_func_define "size_t %s::write_size(size_t position) const"
#define read_func_define "size_t %s::read_struct(const void *data, size_t position)"
#define key_size_define "size_t %s::key_size(size_t position) const"
#define key_max_size_define "size_t %s::key_max_size(size_t position) const"
#define key_stream_define "size_t %s::key_stream(void *data, size_t position) const"
#define key_calc_define "bool %s::key(ddsi_keyhash_t &hash) const"
#define typedef_write_define "size_t typedef_write_%s(const %s &obj, void* data, size_t position)"
#define typedef_write_size_define "size_t typedef_write_size_%s(const %s &obj, size_t position)"
#define typedef_read_define "size_t typedef_read_%s(%s &obj, void* data, size_t position)"
#define typedef_key_size_define "size_t typedef_key_size_%s(const %s &obj, size_t position)"
#define typedef_key_max_size_define "size_t typedef_key_max_size_%s(const %s &obj, size_t position)"
#define typedef_key_stream_define "size_t typedef_key_stream_%s(const %s &obj, void *data, size_t position)"
#define typedef_write_call position_set "%stypedef_write_%s(%s, data, position);\n"
#define typedef_write_size_call position_set "%stypedef_write_size_%s(%s, position);\n"
#define typedef_read_call position_set "%stypedef_read_%s(%s, data, position);\n"
#define typedef_key_size_call position_set "%stypedef_key_size_%s(%s, position);\n"
#define typedef_key_max_size_call position_set "%stypedef_key_max_size_%s(%s, position);\n"
#define typedef_key_stream_call position_set "%stypedef_key_stream_%s(%s, *data, position);\n"
#define union_case_max_check "if (case_max != UINT_MAX) "
#define union_case_max_incr union_case_max_check "case_max += "
#define union_case_max_set "case_max = "
#define union_case_max_set_limit union_case_max_set "UINT_MAX;\n"
#define union_case_seq_max_case_typedef sequence_iterate union_case_max_check union_case_max_set "%stypedef_key_max_size_%s(%s[_i], case_max);\n"
#define union_case_seq_max_case_struct sequence_iterate union_case_max_check union_case_max_set "%s[_i].key_max_size(case_max);"
#define arr_struct_key_max_size_union array_iterate union_case_max_check union_case_max_set "%s[_i].key_max_size(case_max);\n"

#define char_cast "char"
#define bool_cast "bool"
#define int8_cast "int8_t"
#define uint8_cast "uint8_t"
#define int16_cast "int16_t"
#define uint16_cast "uint16_t"
#define int32_cast "int32_t"
#define uint32_cast "uint32_t"
#define int64_cast "int64_t"
#define uint64_cast "uint64_t"
#define float_cast "float"
#define double_cast "double"

struct idl_streamer_output
{
  size_t indent;
  idl_ostream_t* impl_stream;
  idl_ostream_t* head_stream;
};

typedef struct context context_t;

struct functioncontents {
  int currentalignment;
  int accumulatedalignment;
  bool alignmentpresent;
  bool sequenceentriespresent;
};

typedef struct functioncontents functioncontents_t;

struct context
{
  idl_streamer_output_t* str;
  char* context;
  idl_ostream_t* write_size_stream;
  idl_ostream_t* write_stream;
  idl_ostream_t* read_stream;
  idl_ostream_t* key_size_stream;
  idl_ostream_t* key_max_size_stream;
  idl_ostream_t* key_stream;
  size_t depth;
  functioncontents_t streamer_funcs;
  functioncontents_t key_funcs;
  context_t* parent;
  bool in_union;
};

static uint64_t array_entries(idl_declarator_t* decl);
static idl_retcode_t add_default_case(context_t* ctx);
static idl_retcode_t process_node(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_instance(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_struct(context_t* ctx, idl_declarator_t* decl, idl_struct_t* spec, bool is_key);
static idl_retcode_t process_base(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_template(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_type_spec_t* resolve_typedef(idl_type_spec_t* def);
static idl_retcode_t process_typedef_definition(context_t* ctx, idl_typedef_t* node);
static idl_retcode_t process_typedef_instance(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_known_width(context_t* ctx, const char* accessor, idl_mask_t typespec, bool is_key);
static idl_retcode_t process_known_width_sequence(context_t* ctx, const char* accessor, idl_type_spec_t *typespec, bool is_key);
static idl_retcode_t process_sequence_entries(context_t* ctx, const char* accessor, bool plusone, bool is_key);
static idl_retcode_t process_known_width_array(context_t* ctx, const char* accessor, uint64_t entries, idl_mask_t mask, bool is_key);
static int determine_byte_width(idl_mask_t typespec);
static const char* determine_cast(idl_mask_t mask);
static idl_retcode_t check_alignment(context_t* ctx, int bytewidth, bool is_key);
static idl_retcode_t add_null(context_t* ctx, int nbytes, bool stream, bool is_key);
static idl_retcode_t process_member(context_t* ctx, idl_member_t* mem);
static idl_retcode_t process_module(context_t* ctx, idl_module_t* module);
static idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_case(context_t* ctx, idl_case_t* _case);
static idl_retcode_t process_case_label(context_t* ctx, idl_case_label_t* label);
static idl_retcode_t write_instance_funcs(context_t* ctx, const char* accessor, uint64_t entries, bool is_key);
static context_t* create_context(idl_streamer_output_t* str, const char* name);
static context_t* child_context(context_t* ctx, const char* name);
static void flush_streams(context_t* ctx);
static void close_context(context_t* ctx);
static void resolve_namespace(idl_node_t* node, char** up);

static char* generatealignment(int alignto)
{
  char* returnval = NULL;
  if (alignto < 2)
  {
    returnval = idl_strdup("0;");
  }
  else if (alignto == 2)
  {
    returnval = idl_strdup("position&0x1;");
  }
  else
  {
    int mask = 0xFFFFFF;
    while (mask != 0)
    {
      if (alignto == mask + 1)
      {
        (void)idl_asprintf(&returnval, primitive_calc_alignment_shift, alignto, mask, mask);
        return returnval;
      }
      mask >>= 1;
    }

    (void)idl_asprintf(&returnval, primitive_calc_alignment_modulo, alignto, alignto, alignto);
  }
  return returnval;
}

int determine_byte_width(idl_mask_t mask)
{
  if ((mask & IDL_ENUM) == IDL_ENUM)
    mask = IDL_UINT32;

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
  }

  return -1;
}

idl_streamer_output_t* create_idl_streamer_output()
{
  idl_streamer_output_t* ptr = calloc(sizeof(idl_streamer_output_t),1);
  if (NULL != ptr)
  {
    ptr->impl_stream = create_idl_ostream(NULL);
    ptr->head_stream = create_idl_ostream(NULL);
  }
  return ptr;
}

void destruct_idl_streamer_output(idl_streamer_output_t* str)
{
  if (NULL == str)
    return;

  if (str->impl_stream != NULL)
  {
    destruct_idl_ostream(str->impl_stream);
    destruct_idl_ostream(str->head_stream);
  }
  free(str);
}

idl_ostream_t* get_idl_streamer_impl_buf(const idl_streamer_output_t* str)
{
  return str->impl_stream;
}

idl_ostream_t* get_idl_streamer_head_buf(const idl_streamer_output_t* str)
{
  return str->head_stream;
}

context_t* create_context(idl_streamer_output_t* str, const char* name)
{
  context_t* ptr = calloc(sizeof(context_t),1);
  if (NULL != ptr)
  {
    ptr->str = str;
    ptr->context = idl_strdup(name);
    ptr->write_size_stream = create_idl_ostream(NULL);
    ptr->write_stream = create_idl_ostream(NULL);
    ptr->read_stream = create_idl_ostream(NULL);
    ptr->key_size_stream = create_idl_ostream(NULL);
    ptr->key_max_size_stream = create_idl_ostream(NULL);
    ptr->key_stream = create_idl_ostream(NULL);
    ptr->streamer_funcs.currentalignment = -1;
    ptr->key_funcs.currentalignment = -1;
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
  transfer_ostream_buffer(ctx->write_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->write_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_max_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->read_stream, ctx->str->impl_stream);
}

void close_context(context_t* ctx)
{
  flush_streams(ctx);

  destruct_idl_ostream(ctx->write_stream);
  destruct_idl_ostream(ctx->write_size_stream);
  destruct_idl_ostream(ctx->key_size_stream);
  destruct_idl_ostream(ctx->key_max_size_stream);
  destruct_idl_ostream(ctx->key_stream);
  destruct_idl_ostream(ctx->read_stream);

  free(ctx->context);
  free(ctx);
}

void resolve_namespace(idl_node_t* node, char** up)
{
  if (!node)
    return;

  if (idl_is_module(node))
  {
    idl_module_t* mod = (idl_module_t*)node;
    if (*up)
    {
      char *temp = NULL;
      idl_asprintf(&temp, "%s::%s", idl_identifier(mod), *up);
      free(*up);
      *up = temp;
    }
    else
    {
      idl_asprintf(up, "%s::", idl_identifier(mod));
    }
  }

  resolve_namespace(node->parent, up);
}

idl_retcode_t process_node(context_t* ctx, idl_node_t* node)
{
  if (idl_is_module(node))
    process_module(ctx, (idl_module_t*)node);
  else if (idl_is_struct(node) || idl_is_union(node))
    process_constructed(ctx, node);
  else if (idl_is_typedef(node))
    process_typedef_definition(ctx, (idl_typedef_t*)node);

  if (node->next)
    process_node(ctx, node->next);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_member(context_t* ctx, idl_member_t* mem)
{
  assert(ctx);
  assert(mem);

  bool is_key = idl_is_masked(mem, IDL_KEY);
  process_instance(ctx, mem->declarators, mem->type_spec, is_key);

  if (mem->node.next)
    process_member(ctx, (idl_member_t*)(mem->node.next));

  return IDL_RETCODE_OK;
}

idl_retcode_t process_instance(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key)
{
  if (idl_is_base_type(spec) || idl_is_enum(spec)) {
    return process_base(ctx, decl, spec, is_key);
  } else if (idl_is_struct(spec)) {
    return process_struct(ctx, decl, (idl_struct_t*)spec, is_key);
  } else if (idl_is_templ_type(spec)) {
    // FIXME: this probably needs to loop to find the correct declarator?
    return process_template(ctx, decl, spec, is_key);
  } else {
    assert(idl_is_typedef(spec));
    return process_typedef_instance(ctx, decl, spec, is_key);
  }
}

uint64_t array_entries(idl_declarator_t* decl)
{
  if (NULL == decl)
    return 0;

  idl_const_expr_t* ce = decl->const_expr;
  uint64_t entries = 0;
  while (ce)
  {
    if ((ce->mask & IDL_CONST) == IDL_CONST)
    {
      idl_constval_t* var = (idl_constval_t*)ce;
      idl_mask_t mask = var->node.mask;
      if ((mask & IDL_UINT8) == IDL_UINT8)
      {
        if (entries)
          entries *= var->value.oct;
        else
          entries = var->value.oct;
      }
      else if ((mask & IDL_UINT32) == IDL_UINT32)
      {
        if (entries)
          entries *= var->value.ulng;
        else
          entries = var->value.ulng;
      }
      else if ((mask & IDL_UINT64) == IDL_UINT64)
      {
        if (entries)
          entries *= var->value.ullng;
        else
          entries = var->value.ullng;
      }
    }

    ce = ce->next;
  }
  return entries;
}

idl_retcode_t process_struct(context_t* ctx, idl_declarator_t* decl, idl_struct_t* spec, bool is_key)
{
  assert(ctx);
  assert(decl);

  uint64_t entries = array_entries(decl);

  char* accessor = NULL;
  if (decl)
  {
    char* cpp11name = get_cpp11_name(idl_identifier(decl));

    idl_asprintf(&accessor, member_access, cpp11name);
    free(cpp11name);
  }
  else
  {
    accessor = idl_strdup("obj");
  }

  write_instance_funcs(ctx, accessor, entries, is_key);
  free(accessor);

  if (NULL != decl &&
      ((idl_node_t*)decl)->next)
    process_struct(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, spec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t write_instance_funcs(context_t* ctx, const char* accessor, uint64_t entries, bool is_key)
{
  if (entries)
  {
    format_write_stream(1, ctx, false, arr_struct_write, entries, accessor);
    format_write_size_stream(1, ctx, false, arr_struct_write_size, entries, accessor);
    format_read_stream(1, ctx, arr_struct_read, entries, accessor);

    if (is_key)
    {
      format_key_stream(1, ctx, arr_struct_key, entries, accessor);
      format_key_size_stream(1, ctx, arr_struct_key_size, entries, accessor);
      if (ctx->in_union)
      {
        format_key_max_size_stream(1, ctx, arr_struct_key_max_size_union, entries, accessor);
      }
      else
      {
        format_key_max_size_stream(1, ctx, arr_struct_key_max_size, entries, accessor);
      }
    }
  }
  else
  {
    format_write_stream(1, ctx, false , instance_write_func, accessor);
    format_write_size_stream(1, ctx, false, instance_size_func_calc, accessor);
    format_read_stream(1, ctx, instance_read_func, accessor);

    if (is_key)
    {
      format_key_stream(1, ctx, instance_key_write_func, accessor);
      format_key_size_stream(1, ctx, instance_key_size_func_calc, accessor);
      if (ctx->in_union)
      {
        format_key_max_size_stream(1, ctx, instance_key_max_size_union_func_calc, accessor);
      }
      else
      {
        format_key_max_size_stream(1, ctx, instance_key_max_size_func_calc, accessor);
      }
    }
  }

  ctx->streamer_funcs.accumulatedalignment = 0;
  ctx->streamer_funcs.currentalignment = -1;

  if (is_key)
  {
    ctx->key_funcs.accumulatedalignment = 0;
    ctx->key_funcs.currentalignment = -1;
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t check_alignment(context_t* ctx, int bytewidth, bool is_key)
{
  assert(ctx);

  if (ctx->streamer_funcs.currentalignment == bytewidth && ctx->key_funcs.currentalignment == bytewidth)
    return IDL_RETCODE_OK;

  char* buffer = generatealignment(bytewidth);

  if ((0 > ctx->streamer_funcs.currentalignment || bytewidth > ctx->streamer_funcs.currentalignment) && bytewidth != 1)
  {
    if (!ctx->streamer_funcs.alignmentpresent)
    {
      format_write_stream(1, ctx, false, "  size_t alignmentbytes = ");
      ctx->streamer_funcs.alignmentpresent = true;
    }
    else
    {
      format_write_stream(1, ctx, false, "  alignmentbytes = ");
    }

    format_write_stream(0, ctx, false, buffer);
    format_write_stream(0, ctx, false, align_comment);
    format_write_stream(1, ctx, false, primitive_write_func_alignment);
    format_write_stream(1, ctx, false, position_incr_alignment incr_comment);

    format_write_size_stream(1, ctx, false, position_incr);
    format_write_size_stream(0, ctx, false, buffer);
    format_write_size_stream(0, ctx, false, align_comment);

    format_read_stream(1, ctx, position_incr);
    format_read_stream(0, ctx, buffer);
    format_read_stream(0, ctx, align_comment);

    ctx->streamer_funcs.accumulatedalignment = 0;
    ctx->streamer_funcs.currentalignment = bytewidth;
  }
  else
  {
    int missingbytes = (bytewidth - (ctx->streamer_funcs.accumulatedalignment % bytewidth)) % bytewidth;
    if (0 != missingbytes)
    {
      add_null(ctx, missingbytes, true, false);
      ctx->streamer_funcs.accumulatedalignment = 0;
    }
  }

  if (is_key)
  {
    if ((0 > ctx->key_funcs.currentalignment || bytewidth > ctx->key_funcs.currentalignment) && bytewidth != 1)
    {
      if (!ctx->key_funcs.alignmentpresent)
      {
        format_key_stream(1, ctx, "  size_t alignmentbytes = ");
        ctx->key_funcs.alignmentpresent = true;
      }
      else
      {
        format_key_stream(1, ctx, "  alignmentbytes = ");
      }

      format_key_stream(0, ctx, buffer);
      format_key_stream(0, ctx, align_comment);
      format_key_stream(1, ctx, primitive_write_func_alignment);
      format_key_stream(1, ctx, position_incr_alignment incr_comment);

      format_key_size_stream(1, ctx, position_incr);
      format_key_size_stream(0, ctx, buffer);
      format_key_size_stream(0, ctx, align_comment);
      if (ctx->in_union)
      {
        format_key_max_size_stream(1, ctx, key_max_size_check);
        format_key_max_size_stream(0, ctx, union_case_max_incr);
        format_key_max_size_stream(0, ctx, buffer);
        format_key_max_size_stream(0, ctx, align_comment);
      }
      else
      {
        format_key_max_size_stream(1, ctx, key_max_size_check);
        format_key_max_size_stream(0, ctx, position_incr);
        format_key_max_size_stream(0, ctx, buffer);
        format_key_max_size_stream(0, ctx, align_comment);
      }

      ctx->key_funcs.accumulatedalignment = 0;
      ctx->key_funcs.currentalignment = bytewidth;
    }
    else
    {
      int missingbytes = (bytewidth - (ctx->key_funcs.accumulatedalignment % bytewidth)) % bytewidth;
      if (0 != missingbytes)
      {
        add_null(ctx, missingbytes,false,true);
        ctx->key_funcs.accumulatedalignment = 0;
      }
    }
  }

  if (buffer)
    free(buffer);

  return IDL_RETCODE_OK;
}

idl_retcode_t add_null(context_t* ctx, int nbytes, bool stream, bool is_key)
{
  if (stream)
  {
    format_write_stream(1, ctx, false, primitive_write_func_padding, nbytes);
    format_write_stream(1, ctx, false, primitive_incr_pos incr_comment, nbytes);
    format_write_size_stream(1, ctx, false, primitive_incr_pos padding_comment, nbytes);
    format_read_stream(1, ctx, primitive_incr_pos padding_comment, nbytes);
  }

  if (is_key)
  {
    format_key_stream(1, ctx, primitive_write_func_padding, nbytes);
    format_key_stream(1, ctx, primitive_incr_pos incr_comment, nbytes);
    format_key_size_stream(1, ctx, primitive_incr_pos padding_comment, nbytes);
    if (ctx->in_union)
    {
      format_key_max_size_stream(1, ctx, union_case_max_incr " %d;", nbytes);
      format_key_max_size_stream(0, ctx, padding_comment);
    }
    else
    {
      format_key_max_size_stream(1, ctx, primitive_incr_pos padding_comment, nbytes);
    }
  }

  return IDL_RETCODE_OK;
}

const char* determine_cast(idl_mask_t mask)
{
  if ((mask & IDL_ENUM) == IDL_ENUM)
    return uint32_cast;

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
  }
  return NULL;
}

idl_retcode_t process_known_width(context_t* ctx, const char* accessor, idl_mask_t typespec, bool is_key)
{
  assert(ctx);
  assert(accessor);

  if ((typespec & IDL_ENUM) == IDL_ENUM)
    typespec = IDL_UINT32;

  int bytewidth = determine_byte_width(typespec);
  assert(bytewidth != -1);

  const char* cast = determine_cast(typespec);
  assert(cast);

  check_alignment(ctx, bytewidth, is_key);

  format_write_stream(1, ctx, is_key, primitive_write_func_write, cast, accessor, accessor);
  format_write_stream(1, ctx, is_key, primitive_incr_pos incr_comment, bytewidth);

  format_write_size_stream(1, ctx, is_key, primitive_incr_pos bytes_for_member_comment, bytewidth, accessor);

  if (is_key)
  {
    if (ctx->in_union)
    {
      format_key_max_size_stream(1, ctx, union_case_max_incr " %d;\n", bytewidth);
    }
    else
    {
      format_key_max_size_stream(1, ctx, key_max_size_incr_checked bytes_for_member_comment, bytewidth, accessor);
    }
  }

  format_read_stream(1, ctx, primitive_read_func_read, accessor, cast, accessor);
  format_read_stream(1, ctx, primitive_incr_pos incr_comment, bytewidth);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_known_width_sequence(context_t* ctx, const char* accessor, idl_type_spec_t *tspec, bool is_key)
{
  assert(ctx);
  assert(accessor);

  idl_mask_t mask = idl_is_string(tspec) ? IDL_CHAR : tspec->mask;
  int bytewidth = determine_byte_width(mask); 
  const char* cast = determine_cast(mask);

  format_write_stream(1, ctx, is_key, seq_primitive_write, accessor, bytewidth, accessor);
  format_write_stream(1, ctx, is_key, seq_incr incr_comment, bytewidth);
  format_write_size_stream(1, ctx, is_key, seq_entries, accessor, idl_is_string(tspec) ? "+1" : "", bytewidth);

  if (bytewidth > 1)
  {
    format_read_stream(1, ctx, seq_primitive_read, accessor, cast, cast);
  }
  else
  {
    format_read_stream(1, ctx, seq_primitive_read_nocast, accessor);
  }
  format_read_stream(1, ctx, seq_incr incr_comment, bytewidth);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_sequence_entries(context_t* ctx, const char* accessor, bool plusone, bool is_key)
{
  assert(ctx);
  assert(accessor);

  check_alignment(ctx, 4, is_key);

  format_read_stream(1, ctx, "  ");
  format_write_stream(1, ctx, is_key, "  ");
  if (!ctx->streamer_funcs.sequenceentriespresent)
  {
    format_read_stream(0, ctx, "uint32_t ");
    format_write_stream(0, ctx, false, "uint32_t ");
    ctx->streamer_funcs.sequenceentriespresent = true;
  }

  if (is_key && !ctx->key_funcs.sequenceentriespresent)
  {
    format_key_stream(0, ctx, "uint32_t ");
    ctx->key_funcs.sequenceentriespresent = true;
  }
  format_write_stream(0, ctx, is_key, primitive_write_func_seq, accessor, plusone ? "+1" : "");
  format_write_stream(1, ctx, is_key, primitive_write_func_seq2, accessor);
  format_write_stream(1, ctx, is_key, primitive_incr_pos incr_comment, 4);

  format_write_size_stream(1, ctx, is_key, primitive_incr_pos bytes_for_seq_entries_comment, 4);

  if (is_key)
  {
    if (ctx->in_union)
    {
      format_key_max_size_stream(1, ctx, union_case_max_incr "4;\n");
    }
    else
    {
      format_key_max_size_stream(1, ctx, key_max_size_incr_checked bytes_for_seq_entries_comment, 4);
    }
  }

  format_read_stream(0, ctx, primitive_read_func_seq);
  format_read_stream(1, ctx, primitive_incr_pos incr_comment, 4);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_known_width_array(context_t* ctx, const char *accessor, uint64_t entries, idl_mask_t mask, bool is_key)
{
  assert(ctx);

  if ((mask & IDL_ENUM) == IDL_ENUM)
    mask = IDL_UINT32;

  int bytewidth = determine_byte_width(mask);
  assert(bytewidth != -1);

  unsigned int bw = (unsigned int)bytewidth;

  size_t bytesinarray = bw * entries;

  check_alignment(ctx, bytewidth, is_key);
  ctx->streamer_funcs.accumulatedalignment += (int)bytesinarray;

  format_write_stream(1, ctx, is_key, primitive_write_func_array, accessor, bytesinarray, accessor);
  format_write_stream(1, ctx, is_key, primitive_incr_pos incr_comment, bytesinarray);

  format_write_size_stream(1, ctx, is_key, primitive_incr_pos bytes_for_member_comment, bytesinarray, accessor);

  format_read_stream(1, ctx, primitive_read_func_array, accessor, bytesinarray, accessor);
  format_read_stream(1, ctx, primitive_incr_pos incr_comment, bytesinarray);

  if (is_key)
  {
    if (ctx->in_union)
    {
      format_key_max_size_stream(1, ctx, union_case_max_incr "%d;\n", bytesinarray);
    }
    else
    {
      format_key_max_size_stream(1, ctx, key_max_size_incr_checked bytes_for_member_comment, bytesinarray, accessor);
    }
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_template(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec, bool is_key)
{
  assert(ctx);
  assert(tspec);

  uint64_t entries = array_entries(decl);

  bool oldal = ctx->streamer_funcs.alignmentpresent,
       oldep = ctx->streamer_funcs.sequenceentriespresent,
       oldkeyal = ctx->key_funcs.alignmentpresent,
       oldkeyep = ctx->key_funcs.sequenceentriespresent;

  if (entries)
  {
    format_write_size_stream(1,ctx, is_key, array_iterate " {\n",entries);
    format_write_stream(1, ctx, is_key, array_iterate " {\n", entries);
    format_read_stream(1, ctx, array_iterate " {\n", entries);

    ctx->depth++;
  }

  char* accessor = NULL;
  if (decl)
  {
    char* cpp11name = get_cpp11_name(idl_identifier(decl));
    idl_asprintf(&accessor, member_access, cpp11name);
    free(cpp11name);
  }
  else
  {
    accessor = idl_strdup("obj");
  }

  if (idl_is_sequence(tspec) ||
      idl_is_string(tspec))
  {
    size_t bound = 0;
    idl_type_spec_t* ispec = tspec;
    if (idl_is_sequence(tspec))
    {
      //change member_mask to the type of the sequence template
      bound = ((idl_sequence_t*)tspec)->maximum;
      ispec = ((idl_sequence_t*)tspec)->type_spec;
    }
    else if (idl_is_string(tspec))
    {
      bound = ((idl_string_t*)tspec)->maximum;
    }

    if (idl_is_typedef(ispec))
    {
      idl_type_spec_t* temp = resolve_typedef(ispec);
      if (idl_is_base_type(temp))
        ispec = temp;
    }

    process_sequence_entries(ctx, accessor, idl_is_string(ispec), is_key);

    if (bound)
    {
      //add boundary checking function
      format_write_stream(1, ctx, is_key, seq_length_exception, idl_is_string(tspec) ? bound+1 : bound, accessor, bound);
    }

    if (ispec->mask == IDL_WCHAR)
    {
      fprintf(stderr, "wchar sequences are currently not supported\n");
    }
    else if (idl_is_base_type(ispec) ||
             idl_is_enum(ispec) ||
             idl_is_string(tspec))
    {
      int bytewidth = 1;
      if (idl_is_base_type(ispec) ||
          idl_is_enum(ispec))
        bytewidth = determine_byte_width(ispec->mask);  //determine byte width of base type
      assert(bytewidth != -1);
      assert(bytewidth > 0);

      if (bytewidth > 4)
        check_alignment(ctx, bytewidth, is_key);  //only need to check for alignment if the sequence entries are larger than 4 bytes, due to the preceding sequence_entries entry

      process_known_width_sequence(ctx, accessor, ispec, is_key);

      if (is_key)
      {
        if (bound)
        {
          //bounded sequences have a fixed max key size
          if (ctx->in_union)
          {
            format_key_max_size_stream(1, ctx, union_case_max_incr "%d;\n", (bound + (idl_is_string(tspec))) * (size_t)bytewidth);
          }
          else
          {
            format_key_max_size_stream(1, ctx, key_max_size_incr_checked bytes_for_member_comment, (bound + (idl_is_string(tspec))) * (size_t)bytewidth, accessor);
          }
        }
        else
        {
          //unbounded sequences do not have a fixed max key size
          if (ctx->in_union)
          {
            format_key_max_size_stream(1, ctx, union_case_max_set_limit);
          }
          else
          {
            format_key_max_size_stream(1, ctx, key_max_size_boundary);
          }
        }
      }
    }
    else
    {
      if (idl_is_typedef(ispec))
      {
        char* ns = idl_strdup("");
        idl_typedef_t* td = ((idl_typedef_t*)ispec);
        resolve_namespace(td->type_spec, &ns);
        format_write_stream(1, ctx, false, seq_typedef_write, ns, idl_identifier(td->declarators), accessor);
        format_write_size_stream(1, ctx, false, seq_typedef_write_size, ns, idl_identifier(td->declarators), accessor);
        format_read_stream(1, ctx, seq_read_resize, accessor);
        format_read_stream(1, ctx, seq_typedef_read_copy, ns, idl_identifier(td->declarators), accessor);
        
        if (is_key)
        {
          format_key_stream(1, ctx, seq_typedef_key, ns, idl_identifier(td->declarators), accessor);
          format_key_size_stream(1, ctx, seq_typedef_key_size, ns, idl_identifier(td->declarators), accessor);
          if (bound)
          {
            //bounded sequences have a fixed max key size
            if (ctx->in_union)
            {
              format_key_max_size_stream(1, ctx, union_case_seq_max_case_typedef, ns, idl_identifier(td->declarators), accessor);
            }
            else
            {
              format_key_max_size_stream(1, ctx, seq_typedef_key_max_size, ns, idl_identifier(td->declarators), accessor);
            }
          }
          else
          {
            //unbounded sequences do not have a fixed max key size
            if (ctx->in_union)
            {
              format_key_max_size_stream(1, ctx, union_case_max_set_limit);
            }
            else
            {
              format_key_max_size_stream(1, ctx, key_max_size_boundary);
            }
          }
        }

        free(ns);
      }
      else
      {
        format_write_stream(1, ctx, false, seq_struct_write, accessor);
        format_write_size_stream(1, ctx, false, seq_struct_write_size, accessor);
        format_read_stream(1, ctx, seq_read_resize, accessor);
        format_read_stream(1, ctx, seq_struct_read_copy, accessor);

        if (is_key)
        {
          format_key_stream(1, ctx, seq_struct_key, accessor);
          format_key_size_stream(1, ctx, seq_struct_key_size, accessor);
          if (bound)
          {
            //bounded sequences have a fixed max key size
            if (ctx->in_union)
            {
              format_key_max_size_stream(1, ctx, union_case_seq_max_case_struct, accessor);
            }
            else
            {
              format_key_max_size_stream(1, ctx, seq_struct_key_max_size, accessor);
            }
          }
          else
          {
            //unbounded sequences do not have a fixed max key size
            if (ctx->in_union)
            {
              format_key_max_size_stream(1, ctx, union_case_max_set_limit);
            }
            else
            {
              format_key_max_size_stream(1, ctx, key_max_size_boundary);
            }
          }
        }
      }
    }

    ctx->streamer_funcs.accumulatedalignment = 0;
    ctx->streamer_funcs.currentalignment = -1;
    if (is_key)
    {
      ctx->key_funcs.accumulatedalignment = 0;
      ctx->key_funcs.currentalignment = -1;
    }
  }
  else if (tspec->mask == IDL_WSTRING)
  {
    fprintf(stderr, "wstring types are currently not supported\n");
    assert(0);
  }
  else if (tspec->mask == IDL_FIXED_PT)
  {
    fprintf(stderr, "fixed point types are currently not supported\n");
    assert(0);
  }

  if (entries)
  {
    ctx->depth--;

    ctx->streamer_funcs.alignmentpresent = oldal;
    ctx->streamer_funcs.sequenceentriespresent = oldep;
    ctx->key_funcs.alignmentpresent = oldkeyal;
    ctx->key_funcs.sequenceentriespresent = oldkeyep;

    format_write_size_stream(1, ctx, is_key, "  }\n");
    format_write_stream(1, ctx, is_key, "  }\n");
    format_read_stream(1, ctx, "  }\n");
  }

  if (accessor)
    free(accessor);

  if (NULL != decl &&
    ((idl_node_t*)decl)->next)
    process_template(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, tspec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_module(context_t* ctx, idl_module_t* module)
{
  assert(ctx);
  assert(module);

  if (module->definitions)
  {
    char* cpp11name = get_cpp11_name(idl_identifier(module));
    assert(cpp11name);

    context_t* newctx = child_context(ctx, cpp11name);

    format_write_stream(1, ctx, false, namespace_declaration, cpp11name);
    format_write_stream(1, ctx, false, open_block "\n");

    format_header_stream(1, ctx, namespace_declaration, cpp11name);
    format_header_stream(1, ctx, open_block "\n");

    flush_streams(ctx);

    process_node(newctx, (idl_node_t*)module->definitions);

    close_context(newctx);
    format_read_stream(1, ctx, namespace_closure, cpp11name);
    format_header_stream(1, ctx, namespace_closure, cpp11name);

    flush_streams(ctx);

    free(cpp11name);
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node)
{
  assert(ctx);
  assert(node);

  char* cpp11name = NULL;

  if (idl_is_struct(node) ||
      idl_is_union(node))
  {
    if (idl_is_struct(node))
      cpp11name = get_cpp11_name(idl_identifier((idl_struct_t*)node));
    else if (idl_is_union(node))
      cpp11name = get_cpp11_name(idl_identifier((idl_union_t*)node));
    assert(cpp11name);

    format_write_stream(1, ctx, false, write_func_define "\n", cpp11name);
    format_write_stream(1, ctx, false, open_block);

    format_write_size_stream(1, ctx, false, write_size_func_define "\n", cpp11name);
    format_write_size_stream(1, ctx, false, open_block);

    format_key_stream(1, ctx, key_stream_define "\n", cpp11name);
    format_key_stream(1, ctx, open_block);
    format_key_size_stream(1, ctx, key_size_define "\n", cpp11name);
    format_key_size_stream(1, ctx, open_block);
    format_key_max_size_stream(1, ctx, key_max_size_define "\n", cpp11name);
    format_key_max_size_stream(1, ctx, open_block);

    format_read_stream(1, ctx, read_func_define "\n", cpp11name);
    format_read_stream(1, ctx, open_block);

    ctx->streamer_funcs.currentalignment = -1;
    ctx->streamer_funcs.alignmentpresent = false;
    ctx->streamer_funcs.sequenceentriespresent = false;
    ctx->streamer_funcs.accumulatedalignment = 0;

    if (idl_is_struct(node))
    {
      idl_struct_t* _struct = (idl_struct_t*)node;
      if (_struct->base_type)
      {
        char* base_cpp11name = get_cpp11_name(idl_identifier(_struct->base_type));
        char* ns = idl_strdup("");
        assert(base_cpp11name);
        resolve_namespace((idl_node_t*)_struct->base_type, &ns);
        char* accessor = NULL;
        if (idl_asprintf(&accessor, ref_cast, ns, base_cpp11name) == -1)
          return IDL_RETCODE_NO_MEMORY;
        write_instance_funcs(ctx, accessor, 0,false);
        free(base_cpp11name);
        free(accessor);
        free(ns);
      }

      if (_struct->members)
        process_member(ctx, _struct->members);
    }
    else if (idl_is_union(node))
    {
      idl_union_t* _union = (idl_union_t*)node;
      idl_switch_type_spec_t* st = _union->switch_type_spec;

      idl_mask_t disc_mask = st->mask;
      if (idl_is_enumerator(st)) {
        disc_mask = IDL_ULONG;
      } else {
        assert(idl_is_masked(st, IDL_BASE_TYPE));
      }

      format_read_stream(1, ctx, union_clear_func);
      process_known_width(ctx, "_d()", disc_mask, true);
      format_write_size_stream(1, ctx, true, union_switch);
      format_write_size_stream(1, ctx, true, "  {\n");
      format_write_stream(1, ctx, true, union_switch);
      format_write_stream(1, ctx, true, "  {\n");
      format_read_stream(1, ctx, union_switch);
      format_read_stream(1, ctx, "  {\n");

      ctx->in_union = true;
      format_key_max_size_stream(1, ctx, "  size_t union_max = position;\n");
      if (_union->cases)
      {
        ctx->depth++;
        process_case(ctx, _union->cases);
        ctx->depth--;
      }

      ctx->streamer_funcs.currentalignment = -1;
      ctx->streamer_funcs.accumulatedalignment = 0;
      ctx->key_funcs.currentalignment = -1;
      ctx->key_funcs.accumulatedalignment = 0;
      format_key_max_size_stream(1, ctx, "  position = max(position,union_max);\n");
      ctx->in_union = false;

      format_write_stream(1, ctx, true, "  }\n");
      format_write_size_stream(1, ctx, true, "  }\n");
      format_read_stream(1, ctx, "  }\n");
    }

    format_write_size_stream(1, ctx, true, position_return);
    format_write_size_stream(1, ctx, true, close_function);
    format_write_stream(1, ctx, true, position_return);
    format_write_stream(1, ctx, true, close_function);
    format_read_stream(1, ctx, position_return);
    format_read_stream(1, ctx, close_function);

    format_key_max_size_stream(1, ctx, position_return);
    format_key_max_size_stream(1, ctx, close_function);
    format_key_stream(1, ctx, key_calc_define "\n", cpp11name);
    format_key_stream(1, ctx, "{\n");
    format_key_stream(1, ctx, "  size_t sz = key_size(0);\n");
    format_key_stream(1, ctx, "  size_t padding = 16 - sz%%16;\n");
    format_key_stream(1, ctx, "  if (sz != 0 && padding == 16) padding = 0;\n");
    format_key_stream(1, ctx, "  std::vector<unsigned char> buffer(sz+padding);\n");
    format_key_stream(1, ctx, "  memset(buffer.data()+sz,0x0,padding);\n");
    format_key_stream(1, ctx, "  key_stream(buffer.data(),0);\n");
    format_key_stream(1, ctx, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
    format_key_stream(1, ctx, "  if (fptr == NULL)\n");
    format_key_stream(1, ctx, "  {\n");
    format_key_stream(1, ctx, "    if (key_max_size(0) <= 16)\n");
    format_key_stream(1, ctx, "    {\n");
    format_key_stream(1, ctx, "      //bind to unmodified function which just copies buffer into the keyhash\n");
    format_key_stream(1, ctx, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n")
    format_key_stream(1, ctx, "    }\n");
    format_key_stream(1, ctx, "    else\n");
    format_key_stream(1, ctx, "    {\n");
    format_key_stream(1, ctx, "      //bind to MD5 hash function\n");
    format_key_stream(1, ctx, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n")
    format_key_stream(1, ctx, "    }\n");
    format_key_stream(1, ctx, "  }\n");
    format_key_stream(1, ctx, "  return (*fptr)(buffer,hash);\n");
    format_key_stream(1, ctx, close_function);
  }

  if (cpp11name)
    free(cpp11name);
  return IDL_RETCODE_OK;
}

idl_retcode_t process_case(context_t* ctx, idl_case_t* _case)
{
  functioncontents_t sfuncs = ctx->streamer_funcs;
  functioncontents_t kfuncs = ctx->key_funcs;
  if (_case->case_labels)
    process_case_label(ctx, _case->case_labels);

  format_write_stream(1, ctx, true, "  {\n");
  format_write_size_stream(1, ctx, true, "  {\n");
  format_read_stream(1, ctx, "  {\n");
  format_key_max_size_stream(1, ctx, "{  //cases\n");
  format_key_max_size_stream(1, ctx, "  size_t case_max = position;\n");
  ctx->depth++;

  process_instance(ctx, _case->declarator, _case->type_spec, true);

  ctx->depth--;
  format_key_max_size_stream(1, ctx, "  union_max = max(case_max,union_max);\n");
  format_key_max_size_stream(1, ctx, "}\n");
  format_write_stream(1, ctx, true, "  }\n");
  format_write_stream(1, ctx, true, union_case_ending);
  format_write_size_stream(1, ctx, true, "  }\n");
  format_write_size_stream(1, ctx, true, union_case_ending);
  format_read_stream(1, ctx, "  }\n");
  format_read_stream(1, ctx, union_case_ending);

  ctx->streamer_funcs = sfuncs;
  ctx->key_funcs = kfuncs;

  //go to next case
  if (_case->node.next)
    process_case(ctx, (idl_case_t*)_case->node.next);
  return IDL_RETCODE_OK;
}

idl_type_spec_t* resolve_typedef(idl_type_spec_t* spec)
{
  while (NULL != spec &&
         idl_is_typedef(spec))
    spec = ((idl_typedef_t*)spec)->type_spec;

  return spec;
}

idl_retcode_t process_typedef_definition(context_t* ctx, idl_typedef_t* node)
{
  idl_type_spec_t* spec = resolve_typedef(node->type_spec);
  if (idl_is_base_type(spec) ||
      idl_is_enum(spec))
    return IDL_RETCODE_OK;
  else
  {
    const char* tsname = idl_identifier(node->declarators);

    format_write_stream(1, ctx, false, typedef_write_define "\n", tsname, tsname);
    format_write_stream(1, ctx, false, open_block);
    format_write_size_stream(1, ctx, false, typedef_write_size_define "\n", tsname, tsname);
    format_write_size_stream(1, ctx, false, open_block);
    format_read_stream(1, ctx, typedef_read_define "\n", tsname, tsname);
    format_read_stream(1, ctx, open_block);
    format_key_stream(1, ctx, typedef_key_stream_define "\n", tsname, tsname);
    format_key_stream(1, ctx, open_block);
    format_key_size_stream(1, ctx, typedef_key_size_define "\n", tsname, tsname);
    format_key_size_stream(1, ctx, open_block);
    format_key_max_size_stream(1, ctx, typedef_key_max_size_define "\n", tsname, tsname);
    format_key_max_size_stream(1, ctx, open_block);

    format_header_stream(1, ctx, typedef_write_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_write_size_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_read_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_stream_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_size_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_max_size_define ";\n\n", tsname, tsname);

    process_instance(ctx, NULL, spec, true);

    format_write_stream(1, ctx, false, position_return);
    format_write_stream(1, ctx, false,close_function);
    format_write_size_stream(1, ctx, false, position_return);
    format_write_size_stream(1, ctx, false, close_function);
    format_read_stream(1, ctx, position_return);
    format_read_stream(1, ctx, close_function);
    format_key_stream(1, ctx, position_return);
    format_key_stream(1, ctx, close_function);
    format_key_size_stream(1, ctx, position_return);
    format_key_size_stream(1, ctx, close_function);
    format_key_max_size_stream(1, ctx, position_return);
    format_key_max_size_stream(1, ctx, close_function);
  }
  return IDL_RETCODE_OK;
}

idl_retcode_t process_typedef_instance(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key)
{
  idl_type_spec_t* ispec = resolve_typedef(spec);
  if (idl_is_base_type(ispec))
  {
    process_base(ctx, decl, ispec, is_key);
  }
  else
  {
    char* ns = idl_strdup("");  //namespace in which the typedef is declared
    resolve_namespace(spec, &ns);
    const char* tdname = idl_identifier(((idl_typedef_t*)spec)->declarators);  //name of the typedef
    char* accessor = NULL;  //call accessing the typedef instance
    if (decl)
    {
      char* cpp11name = get_cpp11_name(idl_identifier(decl));
      idl_asprintf(&accessor, member_access, cpp11name);
      free(cpp11name);
    }
    else
    {
      accessor = idl_strdup("obj");
    }
    assert(accessor);

    format_write_stream(1, ctx, false, typedef_write_call, ns, tdname, accessor);
    format_write_size_stream(1, ctx, false, typedef_write_size_call, ns, tdname, accessor);
    format_read_stream(1, ctx, typedef_read_call, ns, tdname, accessor);
    if (is_key)
    {
      format_key_stream(1, ctx, typedef_key_stream_call, ns, tdname, accessor);
      format_key_size_stream(1, ctx, typedef_key_size_call, ns, tdname, accessor);
      if (ctx->in_union)
      {
        format_key_max_size_stream(1, ctx, union_case_max_set "%stypedef_key_max_size_%(%s, position);\n", ns, tdname, accessor);
      }
      else
      {
        format_key_max_size_stream(1, ctx, typedef_key_max_size_call, ns, tdname, accessor);
      }
    }
    free(ns);
    free(accessor);
  }

  if (NULL != decl &&
    ((idl_node_t*)decl)->next)
    process_typedef_instance(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, spec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_case_label(context_t* ctx, idl_case_label_t* label)
{
  idl_const_expr_t* ce = label->const_expr;
  if (ce)
  {
    char* buffer = NULL;
    idl_constval_t* cv = (idl_constval_t*)ce;

    if (idl_is_masked(ce, IDL_INTEGER_TYPE))
    {
      if (idl_asprintf(&buffer, "%lu", cv->value.ullng) == -1)
        return IDL_RETCODE_NO_MEMORY;
    }
    else if (idl_is_masked(ce, IDL_BOOL))
    {
      if (!(buffer = idl_strdup(cv->value.bln ? "true" : "false")))
        return IDL_RETCODE_NO_MEMORY;
    }
    else if (idl_is_masked(ce, IDL_CHAR))
    {
      if (idl_asprintf(&buffer, "\'%s\'", cv->value.str) == -1)
        return IDL_RETCODE_NO_MEMORY;
    }

    if (buffer)
    {
      format_write_stream(1, ctx, true, union_case, buffer);
      format_write_size_stream(1, ctx, true, union_case, buffer);
      format_read_stream(1, ctx, union_case, buffer);
      free(buffer);
    }
  }
  else
  {
    add_default_case(ctx);
  }

  if (label->node.next)
    process_case_label(ctx, (idl_case_label_t*)label->node.next);

  return IDL_RETCODE_OK;
}

idl_retcode_t add_default_case(context_t* ctx)
{
  format_write_stream(1, ctx, true, default_case);
  format_write_size_stream(1, ctx, true, default_case);
  format_read_stream(1, ctx, default_case);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_base(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec, bool is_key)
{
  assert(ctx);
  assert(tspec);

  uint64_t entries = array_entries(decl);
  char* accessor = NULL;
  if (decl)
  {
    char* cpp11name = get_cpp11_name(idl_identifier(decl));
    idl_asprintf(&accessor, member_access, cpp11name);
    free(cpp11name);
  }
  else
  {
    accessor = idl_strdup("obj");
  }
  assert(accessor);

  if (entries)
    process_known_width_array(ctx, accessor, entries, tspec->mask, is_key);
  else
    process_known_width(ctx, accessor, tspec->mask, is_key);

  if (accessor)
    free(accessor);

  if (NULL != decl &&
      ((idl_node_t*)decl)->next)
    process_base(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, tspec, is_key);

  return IDL_RETCODE_OK;
}

void idl_streamers_generate(const idl_tree_t* tree, idl_streamer_output_t* str)
{
  context_t* ctx = create_context(str, "");

  format_impl_stream(0, ctx, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");

  process_node(ctx, tree->root);
  close_context(ctx);
}
