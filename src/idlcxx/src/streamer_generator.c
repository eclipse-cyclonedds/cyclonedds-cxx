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
#include <inttypes.h>

#include "idlcxx/streamer_generator.h"
#include "idlcxx/backendCpp11Utils.h"
#include "idl/tree.h"
#include "idl/scope.h"
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

#define format_key_max_size_intermediate_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->key_max_size_intermediate_stream, _str, ##__VA_ARGS__);

#define format_key_write_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->key_write_stream, _str, ##__VA_ARGS__);

#define format_key_read_stream(indent,ctx,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->key_read_stream, _str, ##__VA_ARGS__);

#define format_max_size_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->max_size_stream, _str, ##__VA_ARGS__); \
if (key) { format_key_max_size_stream(indent, ctx, _str, ##__VA_ARGS__); }

#define format_max_size_intermediate_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->max_size_intermediate_stream, _str, ##__VA_ARGS__); \
if (key) { format_key_max_size_intermediate_stream(indent, ctx, _str, ##__VA_ARGS__); }

#define format_write_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->write_stream, _str, ##__VA_ARGS__); \
if (key) {format_key_write_stream(indent,ctx,_str, ##__VA_ARGS__);}

#define format_write_size_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->write_size_stream, _str, ##__VA_ARGS__); \
if (key) {format_key_size_stream(indent,ctx,_str, ##__VA_ARGS__);}

#define format_read_stream(indent,ctx,key,_str,...) \
format_ostream_indented(indent ? ctx->depth*2 : 0, ctx->read_stream, _str, ##__VA_ARGS__); \
if (key) {format_key_read_stream(indent,ctx,_str, ##__VA_ARGS__);}

#define namespace_declaration "namespace %s\n"
#define namespace_closure "} //end " namespace_declaration "\n"
#define incr_comment "  //moving position indicator\n"
#define align_comment "  //alignment\n"
#define padding_comment "  //padding bytes\n"
#define bytes_for_member_comment "  //bytes for member: %s\n"
#define bytes_for_seq_entries_comment "  //bytes for sequence entries\n"
#define entries_of_sequence_comment "  //entries of sequence\n"
#define union_switch "  switch (_d())\n"
#define union_case "  case %s:\n"
#define default_case "  default:\n"
#define union_case_ending "  break;\n"
#define union_clear_func "  clear();\n"
#define open_block "{\n"
#define close_block "}\n"
#define close_function close_block "\n"
#define primitive_calc_alignment_modulo "(%d - (position%%%d))%%%d;"
#define primitive_calc_alignment_shift "(%d - (position&%#x))&%#x;"
#define seqentries "_se%d"
#define alignmentbytes "_al%d"
#define position_incr "  position += "
#define position_set "  position = "
#define position_return "  return position;\n"
#define position_incr_alignment position_incr alignmentbytes ";"
#define primitive_incr_pos position_incr "%d;"
#define max_size_check "  if (position != UINT_MAX) "
#define max_size_incr_checked max_size_check primitive_incr_pos
#define primitive_write_func_padding "  memset(static_cast<char*>(data)+position,0x0,%d);  //setting padding bytes to 0x0\n"
#define primitive_write_func_alignment "  memset(static_cast<char*>(data)+position,0x0,"alignmentbytes");  //setting alignment bytes to 0x0\n"
#define primitive_write_func_write "  *reinterpret_cast<%s*>(static_cast<char*>(data)+position) = %s;  //writing bytes for member: %s\n"
#define primitive_write_func_array "  memcpy(static_cast<char*>(data)+position,%s.data(),%d);  //writing bytes for member: %s\n"
#define primitive_write_func_seq seqentries" = static_cast<uint32_t>(%s.size()%s);  //number of entries in the sequence\n"
#define primitive_write_func_seq2 "  *reinterpret_cast<uint32_t*>(static_cast<char*>(data) + position) = "seqentries";  //writing entries for member: %s\n"
#define primitive_read_func_read "  %s = *reinterpret_cast<const %s*>(static_cast<const char*>(data)+position);  //reading bytes for member: %s\n"
#define primitive_read_func_array "  memcpy(%s.data(),static_cast<const char*>(data)+position,%d);  //reading bytes for member: %s\n"
#define primitive_read_func_seq seqentries" = *reinterpret_cast<const uint32_t*>(static_cast<const char*>(data)+position);  //number of entries in the sequence\n"
#define primitive_write_seq "memcpy(static_cast<char*>(data)+position,%s.data(),"seqentries"*%d); //writing bytes for member: %s\n"
#define primitive_write_seq_checked "  if (0 < %s.size()) " primitive_write_seq
#define primitive_read_seq "  %s.assign(reinterpret_cast<const %s*>(static_cast<const char*>(data)+position),reinterpret_cast<const %s*>(static_cast<const char*>(data)+position)+"seqentries"); //reading bytes for member: %s\n"
#define string_write "  " primitive_write_seq
#define bool_write_seq "  for (size_t _b = 0; _b < "seqentries"; _b++) *(static_cast<char*>(data)+position++) = (%s[_b] ? 0x1 : 0x0); //writing bytes for member: %s\n"
#define bool_read_seq "  for (size_t _b = 0; _b < "seqentries"; _b++) %s[_b] = (*(static_cast<const char*>(data)+position++) ? 0x1 : 0x0); //reading bytes for member: %s\n"
#define sequence_iterate "  for (size_t _i%d = 0; _i%d < "seqentries"; _i%d++) {\n"
#define seq_read_resize "  %s.resize("seqentries");\n"
#define seq_length_exception "  if ("seqentries" > %"PRIu64") throw dds::core::InvalidArgumentError(\"attempt to assign entries to bounded member %s in excess of maximum length %"PRIu64"\");\n"
#define seq_incr position_incr seqentries"*%d;"
#define seq_inc_1 position_incr seqentries ";"
#define max_size_incr_checked_multiple max_size_check seq_incr entries_of_sequence_comment
#define array_iterate "  for (size_t _i%d = 0; _i%d < %"PRIu64"; _i%d++)  {\n"
#define array_accessor "%s[_i%d]"
#define instance_write_func position_set "%s.write_struct(data, position);\n"
#define instance_key_write_func position_set "%s.key_write(data, position);\n"
#define instance_key_read_func position_set "%s.key_read(data, position);\n"
#define instance_size_func_calc position_set "%s.write_size(position);\n"
#define instance_key_size_func_calc position_set "%s.key_size(position);\n"
#define instance_key_max_size_func_calc max_size_check position_set "%s.key_max_size(position);\n"
#define instance_max_size_func max_size_check position_set "%s.max_size(position);\n"
#define instance_key_max_size_union_func_calc "case_max = %s.key_max_size(case_max);\n"
#define instance_max_size_func_union "case_max = %s.max_size(case_max);\n"
#define instance_read_func position_set "%s.read_struct(data, position);\n"
#define const_ref_cast "dynamic_cast<const %s%s&>(*this)"
#define ref_cast "dynamic_cast<%s%s&>(*this)"
#define member_access "%s()"
#define write_func_define "size_t %s::write_struct(void *data, size_t position) const"
#define write_size_func_define "size_t %s::write_size(size_t position) const"
#define max_size_define "size_t %s::max_size(size_t position) const"
#define read_func_define "size_t %s::read_struct(const void *data, size_t position)"
#define key_size_define "size_t %s::key_size(size_t position) const"
#define key_max_size_define "size_t %s::key_max_size(size_t position) const"
#define key_write_define "size_t %s::key_write(void *data, size_t position) const"
#define key_read_define "size_t %s::key_read(const void *data, size_t position)"
#define key_calc_define "bool %s::key(ddsi_keyhash_t &hash) const"
#define typedef_write_define "size_t typedef_write_%s(const %s &obj, void* data, size_t position)"
#define typedef_write_size_define "size_t typedef_write_size_%s(const %s &obj, size_t position)"
#define typedef_read_define "size_t typedef_read_%s(%s &obj, const void* data, size_t position)"
#define typedef_max_size_define "size_t typedef_max_size_%s(const %s &obj, size_t position)"
#define typedef_key_size_define "size_t typedef_key_size_%s(const %s &obj, size_t position)"
#define typedef_key_max_size_define "size_t typedef_key_max_size_%s(const %s &obj, size_t position)"
#define typedef_key_write_define "size_t typedef_key_write_%s(const %s &obj, void *data, size_t position)"
#define typedef_key_read_define "size_t typedef_key_read_%s(%s &obj, const void *data, size_t position)"
#define typedef_write_call position_set "%stypedef_write_%s(%s, data, position);\n"
#define typedef_write_size_call position_set "%stypedef_write_size_%s(%s, position);\n"
#define typedef_read_call position_set "%stypedef_read_%s(%s, data, position);\n"
#define typedef_max_size_call position_set "%stypedef_max_size_%s(%s, position);\n"
#define typedef_key_size_call position_set "%stypedef_key_size_%s(%s, position);\n"
#define typedef_key_max_size_call position_set "%stypedef_key_max_size_%s(%s, position);\n"
#define typedef_key_write_call position_set "%stypedef_key_write_%s(%s, data, position);\n"
#define typedef_key_read_call position_set "%stypedef_key_read_%s(%s, data, position);\n"
#define union_case_max_check "if (case_max != UINT_MAX) "
#define union_case_max_incr union_case_max_check "case_max += "
#define union_case_max_set "case_max = "
#define union_case_max_set_limit union_case_max_set "UINT_MAX;\n"

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

static void reset_function_contents(functioncontents_t* funcs)
{
  assert(funcs);
  funcs->currentalignment = -1;
  funcs->accumulatedalignment = 0;
  funcs->alignmentpresent = false;
  funcs->sequenceentriespresent = false;
}

struct context
{
  idl_streamer_output_t* str;
  char* context;
  idl_ostream_t* write_size_stream;
  idl_ostream_t* write_stream;
  idl_ostream_t* read_stream;
  idl_ostream_t* key_size_stream;
  idl_ostream_t* max_size_stream;
  idl_ostream_t* max_size_intermediate_stream;
  idl_ostream_t* key_max_size_stream;
  idl_ostream_t* key_max_size_intermediate_stream;
  idl_ostream_t* key_write_stream;
  idl_ostream_t* key_read_stream;
  size_t depth;
  functioncontents_t streamer_funcs;
  functioncontents_t key_funcs;
  context_t* parent;
  bool in_union;
  bool key_max_size_unlimited;
  bool max_size_unlimited;
  const char* parsed_file;
};

static uint64_t array_entries(idl_declarator_t* decl);
static idl_retcode_t add_default_case(context_t* ctx);
static idl_retcode_t process_node(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_instance(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_constructed_type_decl(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_constructed_type_impl(context_t* ctx, const char* accessor, bool is_key, bool key_is_all_members);
static idl_retcode_t process_base_decl(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_base_impl(context_t* ctx, const char *accessor, uint64_t entries, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_string_decl(context_t* ctx, idl_declarator_t* decl, idl_string_t* spec, bool is_key);
static idl_retcode_t process_string_impl(context_t* ctx, const char *accessor, idl_string_t* spec, bool is_key);
static idl_retcode_t process_sequence_decl(context_t* ctx, idl_declarator_t* decl, idl_sequence_t* spec, bool is_key);
static idl_retcode_t process_sequence_impl(context_t* ctx, const char* accessor, idl_sequence_t* spec, bool is_key);
static idl_type_spec_t* resolve_typedef(idl_type_spec_t* def);
static idl_retcode_t process_typedef_definition(context_t* ctx, idl_typedef_t* node);
static idl_retcode_t process_typedef_instance_decl(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_typedef_instance_impl(context_t* ctx, const char* accessor, idl_type_spec_t* spec, bool is_key);
static idl_retcode_t process_known_width(context_t* ctx, const char* accessor, idl_node_t* typespec, bool is_key);
static idl_retcode_t process_sequence_entries(context_t* ctx, const char* accessor, bool plusone, bool is_key);
static idl_retcode_t process_known_width_array(context_t* ctx, const char* accessor, uint64_t entries, idl_node_t* typespec, bool is_key);
static int determine_byte_width(idl_node_t* typespec);
static char* determine_cast(idl_node_t* typespec);
static idl_retcode_t check_alignment(context_t* ctx, int bytewidth, bool is_key);
static idl_retcode_t add_null(context_t* ctx, int nbytes, bool stream, bool is_key);
static idl_retcode_t process_member(context_t* ctx, idl_member_t* mem);
static idl_retcode_t process_module(context_t* ctx, idl_module_t* module);
static idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node);
static idl_retcode_t process_case(context_t* ctx, idl_case_t* _case);
static idl_retcode_t process_case_label(context_t* ctx, idl_case_label_t* label);
static idl_retcode_t write_instance_funcs(context_t* ctx, const char* write_accessor, const char* read_accessor, bool is_key, bool key_is_all_members);
static context_t* create_context(const char* name);
static context_t* child_context(context_t* ctx, const char* name);
static void close_context(context_t* ctx, idl_streamer_output_t* str);
static void resolve_namespace(idl_node_t* node, char** up);
static char* generate_accessor(idl_declarator_t* decl);
static void start_array(context_t* ctx, uint64_t entries, bool is_key, bool* locals);
static void stop_array(context_t* ctx, bool is_key, bool* locals);
static void store_locals(context_t* ctx, bool* storage);
static void load_locals(context_t* ctx, bool* storage);
static bool has_keys(idl_type_spec_t* spec);

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
  assert(returnval);
  return returnval;
}

int determine_byte_width(idl_node_t* typespec)
{
  if (idl_is_enum(typespec))
    return 4;
  else if (idl_is_string(typespec))
    return 1;

  switch (typespec->mask % (IDL_BASE_TYPE * 2))
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

context_t* create_context(const char* name)
{
  context_t* ptr = calloc(sizeof(context_t),1);
  if (NULL != ptr)
  {
    ptr->str = create_idl_streamer_output();
    assert(ptr->str);
    ptr->context = idl_strdup(name);
    ptr->write_size_stream = create_idl_ostream(NULL);
    ptr->write_stream = create_idl_ostream(NULL);
    ptr->read_stream = create_idl_ostream(NULL);
    ptr->key_size_stream = create_idl_ostream(NULL);
    ptr->max_size_stream = create_idl_ostream(NULL);
    ptr->max_size_intermediate_stream = create_idl_ostream(NULL);
    ptr->key_max_size_stream = create_idl_ostream(NULL);
    ptr->key_max_size_intermediate_stream = create_idl_ostream(NULL);
    ptr->key_write_stream = create_idl_ostream(NULL);
    ptr->key_read_stream = create_idl_ostream(NULL);
    reset_function_contents(&ptr->streamer_funcs);
    reset_function_contents(&ptr->key_funcs);
  }
  return ptr;
}

context_t* child_context(context_t* ctx, const char* name)
{
  context_t* ptr = create_context(name);
  assert(ptr);

  ptr->parent = ctx;
  ptr->depth = ctx->depth + 1;
  ptr->parsed_file = ctx->parsed_file;

  return ptr;
}

void close_context(context_t* ctx, idl_streamer_output_t* str)
{
  assert(ctx);

  if (get_ostream_buffer_position(ctx->str->head_stream) != 0)
  {
    if (ctx->parent)
    {
      //there is a parent context (so all statements are made inside a namespace)
      //move the contents to the parent
      format_header_stream(1, ctx->parent, namespace_declaration, ctx->context);
      format_header_stream(1, ctx->parent, open_block "\n");
      format_header_stream(0, ctx->parent, "%s", get_ostream_buffer(ctx->str->head_stream));
      format_header_stream(1, ctx->parent, namespace_closure, ctx->context);
    }
    else if (str)
    {
      //no parent context (so this is the root namespace)
      //move the contents to the "exit" stream
      format_ostream(str->head_stream, "%s", get_ostream_buffer(ctx->str->head_stream));
    }
  }

  //bundle impl contents
  transfer_ostream_buffer(ctx->write_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->write_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->max_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_max_size_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_write_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->key_read_stream, ctx->str->impl_stream);
  transfer_ostream_buffer(ctx->read_stream, ctx->str->impl_stream);

  if (get_ostream_buffer_position(ctx->str->impl_stream) != 0)
  {
    if (ctx->parent)
    {
      //there is a parent context (so all statements are made inside a namespace)
      //move the contents to the parent
      format_impl_stream(1, ctx->parent, namespace_declaration, ctx->context);
      format_impl_stream(1, ctx->parent, open_block "\n");
      format_impl_stream(0, ctx->parent, "%s", get_ostream_buffer(ctx->str->impl_stream));
      format_impl_stream(1, ctx->parent, namespace_closure, ctx->context);
    }
    else if (str)
    {
      //no parent context (so this is the root namespace)
      //move the contents to the "exit" stream
      format_ostream(str->impl_stream, "%s", get_ostream_buffer(ctx->str->impl_stream));
    }
  }

  destruct_idl_ostream(ctx->write_stream);
  destruct_idl_ostream(ctx->write_size_stream);
  destruct_idl_ostream(ctx->key_size_stream);
  destruct_idl_ostream(ctx->max_size_stream);
  destruct_idl_ostream(ctx->max_size_intermediate_stream);
  destruct_idl_ostream(ctx->key_max_size_stream);
  destruct_idl_ostream(ctx->key_max_size_intermediate_stream);
  destruct_idl_ostream(ctx->key_write_stream);
  destruct_idl_ostream(ctx->key_read_stream);
  destruct_idl_ostream(ctx->read_stream);

  destruct_idl_streamer_output(ctx->str);
  free(ctx->context);
  free(ctx);
}

void resolve_namespace(idl_node_t* node, char** up)
{
  if (!node)
    return;

  if (!*up)
    *up = idl_strdup("");

  if (idl_is_module(node))
  {
    idl_module_t* mod = (idl_module_t*)node;
    char* cppname = get_cpp11_name(idl_identifier(mod));
    assert(cppname);
    char *temp = NULL;
    idl_asprintf(&temp, "%s::%s", cppname, *up);
    free(*up);
    *up = temp;
    free(cppname);
  }

  resolve_namespace(node->parent, up);
}

char* generate_accessor(idl_declarator_t* decl)
{
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
  return accessor;
}

void start_array(context_t* ctx, uint64_t entries, bool is_key, bool* locals)
{
  store_locals(ctx, locals);

  format_write_size_stream(1, ctx, is_key, array_iterate, ctx->depth + 1, ctx->depth + 1, entries, ctx->depth + 1);
  format_write_stream(1, ctx, is_key, array_iterate, ctx->depth + 1, ctx->depth + 1, entries, ctx->depth + 1);
  format_read_stream(1, ctx, is_key, array_iterate, ctx->depth + 1, ctx->depth + 1, entries, ctx->depth + 1);
  format_max_size_intermediate_stream(1, ctx, is_key, array_iterate, ctx->depth + 1, ctx->depth + 1, entries, ctx->depth + 1);

  ctx->depth++;
}

void stop_array(context_t* ctx, bool is_key, bool* locals)
{
  load_locals(ctx, locals);

  format_write_size_stream(1, ctx, is_key, close_block);
  format_write_stream(1, ctx, is_key, close_block);
  format_read_stream(1, ctx, is_key, close_block);
  format_max_size_intermediate_stream(1, ctx, is_key, close_block);

  ctx->depth--;
}

void store_locals(context_t* ctx, bool* storage)
{
  storage[0] = ctx->streamer_funcs.alignmentpresent;
  storage[1] = ctx->streamer_funcs.sequenceentriespresent;
  storage[2] = ctx->key_funcs.alignmentpresent;
  storage[3] = ctx->key_funcs.sequenceentriespresent;

  ctx->streamer_funcs.alignmentpresent = false;
  ctx->streamer_funcs.sequenceentriespresent = false;
  ctx->key_funcs.alignmentpresent = false;
  ctx->key_funcs.sequenceentriespresent = false;
}

void load_locals(context_t* ctx, bool* storage)
{
  ctx->streamer_funcs.alignmentpresent = storage[0];
  ctx->streamer_funcs.sequenceentriespresent = storage[1];
  ctx->key_funcs.alignmentpresent = storage[2];
  ctx->key_funcs.sequenceentriespresent = storage[3];
}

bool has_keys(idl_type_spec_t* spec)
{
  //is struct
  if (idl_is_struct(spec))
  {
    idl_member_t* mem = ((idl_struct_t*)spec)->members;
    while (mem)
    {
      if (idl_is_masked(mem, IDL_KEY)) return true;
      mem = (idl_member_t*)(mem->node.next);
    }
  }
  //is union
  else if (idl_is_union(spec))
  {
    if (idl_is_masked(((idl_union_t*)spec)->switch_type_spec, IDL_KEY)) return true;
  }
  //is typedef
  else if (idl_is_typedef(spec))
  {
    idl_type_spec_t* newspec = resolve_typedef(spec);
    return has_keys(newspec);
  }
  return false;
}

idl_retcode_t process_node(context_t* ctx, idl_node_t* node)
{

  if (idl_is_module(node))
  {
    process_module(ctx, (idl_module_t*)node);  //module entries are not filtered on which file they are defined in, since their first occurance may be in another (previous) file
  }
  else if (ctx->parsed_file == NULL ||
           0 == strcmp(node->location.first.file, ctx->parsed_file))
  {
    if (idl_is_struct(node) || idl_is_union(node))
      process_constructed(ctx, node);
    else if (idl_is_typedef(node))
      process_typedef_definition(ctx, (idl_typedef_t*)node);
  }

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
  if (idl_is_base_type(spec) || idl_is_enum(spec))
  {
    return process_base_decl(ctx, decl, spec, is_key);
  }
  else if (idl_is_struct(spec) || idl_is_union(spec))
  {
    return process_constructed_type_decl(ctx, decl, spec, is_key);
  }
  else if (idl_is_string(spec))
  {
    return process_string_decl(ctx, decl, (idl_string_t*)spec, is_key);
  }
  else if (idl_is_sequence(spec))
  {
    return process_sequence_decl(ctx, decl, (idl_sequence_t*)spec, is_key);
  } else {
    assert(idl_is_typedef(spec));
    return process_typedef_instance_decl(ctx, decl, spec, is_key);
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

idl_retcode_t process_constructed_type_decl(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key)
{
  assert(ctx);

  uint64_t entries = array_entries(decl);
  char* accessor = generate_accessor(decl);

  bool locals[4];
  if (entries)
  {
    start_array(ctx, entries, is_key, locals);

    //modify accessor
    char* temp = NULL;
    idl_asprintf(&temp, array_accessor, accessor, ctx->depth);
    assert(temp);
    free(accessor);
    accessor = temp;
  }

  process_constructed_type_impl(ctx, accessor, is_key, !has_keys(spec));
  free(accessor);

  if (entries)
    stop_array(ctx, is_key, locals);

  if (NULL != decl &&
      ((idl_node_t*)decl)->next)
    process_constructed_type_decl(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, spec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_constructed_type_impl(context_t* ctx, const char* accessor, bool is_key, bool key_is_all_members)
{
  return write_instance_funcs(ctx, accessor, accessor, is_key, key_is_all_members);
}

idl_retcode_t write_instance_funcs(context_t* ctx, const char* write_accessor, const char* read_accessor, bool is_key, bool key_is_all_members)
{

  format_write_stream(1, ctx, false , instance_write_func, write_accessor);
  format_write_size_stream(1, ctx, false, instance_size_func_calc, write_accessor);
  format_read_stream(1, ctx, false, instance_read_func, read_accessor);

  if (ctx->in_union)
  {
    format_max_size_intermediate_stream(1, ctx, false, instance_max_size_func_union, write_accessor);
  }
  else
  {
    format_max_size_intermediate_stream(1, ctx, false, instance_max_size_func, write_accessor);
  }

  if (is_key)
  {
    if (!key_is_all_members)
    {
      format_key_write_stream(1, ctx, instance_key_write_func, write_accessor);
      format_key_read_stream(1, ctx, instance_key_read_func, read_accessor);
      format_key_size_stream(1, ctx, instance_key_size_func_calc, write_accessor);
      if (ctx->in_union)
      {
        format_key_max_size_intermediate_stream(1, ctx, instance_key_max_size_union_func_calc, write_accessor);
      }
      else
      {
        format_key_max_size_intermediate_stream(1, ctx, instance_key_max_size_func_calc, write_accessor);
      }
    }
    else
    {
      format_key_write_stream(1, ctx, instance_write_func, write_accessor);
      format_key_read_stream(1, ctx, instance_read_func, read_accessor);
      format_key_size_stream(1, ctx, instance_size_func_calc, write_accessor);
      if (ctx->in_union)
      {
        format_key_max_size_intermediate_stream(1, ctx, instance_max_size_func_union, write_accessor);
      }
      else
      {
        format_key_max_size_intermediate_stream(1, ctx, instance_max_size_func, write_accessor);
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
    format_write_stream(1, ctx, false, "  ");
    if (!ctx->streamer_funcs.alignmentpresent)
    {
      format_write_stream(0, ctx, false, "size_t ");
      ctx->streamer_funcs.alignmentpresent = true;
    }
    format_write_stream(0, ctx, false, alignmentbytes " = ", ctx->depth);
    format_write_stream(0, ctx, false, buffer);
    format_write_stream(0, ctx, false, align_comment);
    format_write_stream(1, ctx, false, primitive_write_func_alignment, ctx->depth);
    format_write_stream(1, ctx, false, position_incr_alignment incr_comment, ctx->depth);

    format_write_size_stream(1, ctx, false, position_incr);
    format_write_size_stream(0, ctx, false, buffer);
    format_write_size_stream(0, ctx, false, align_comment);

    if (ctx->in_union)
    {
      format_max_size_intermediate_stream(1, ctx, false, max_size_check union_case_max_incr);
      format_max_size_intermediate_stream(0, ctx, false, buffer);
      format_max_size_intermediate_stream(0, ctx, false, align_comment);
    }
    else
    {
      format_max_size_intermediate_stream(1, ctx, false, max_size_check position_incr);
      format_max_size_intermediate_stream(0, ctx, false, buffer);
      format_max_size_intermediate_stream(0, ctx, false, align_comment);
    }

    format_read_stream(1, ctx, false, position_incr);
    format_read_stream(0, ctx, false, buffer);
    format_read_stream(0, ctx, false, align_comment);

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
      format_key_write_stream(1, ctx, "  ");
      if (!ctx->key_funcs.alignmentpresent)
      {
        format_key_write_stream(0, ctx, "size_t ");
        ctx->key_funcs.alignmentpresent = true;
      }
      format_key_write_stream(0, ctx, alignmentbytes " = ", ctx->depth);

      format_key_write_stream(0, ctx, buffer);
      format_key_write_stream(0, ctx, align_comment);
      format_key_write_stream(1, ctx, primitive_write_func_alignment, ctx->depth);
      format_key_write_stream(1, ctx, position_incr_alignment incr_comment, ctx->depth);

      format_key_size_stream(1, ctx, position_incr);
      format_key_size_stream(0, ctx, buffer);
      format_key_size_stream(0, ctx, align_comment);

      format_key_read_stream(1, ctx, position_incr);
      format_key_read_stream(0, ctx, buffer);
      format_key_read_stream(0, ctx, align_comment);

      if (ctx->in_union)
      {
        format_key_max_size_intermediate_stream(1, ctx, max_size_check union_case_max_incr);
        format_key_max_size_intermediate_stream(0, ctx, buffer);
        format_key_max_size_intermediate_stream(0, ctx, align_comment);
      }
      else
      {
        format_key_max_size_intermediate_stream(1, ctx, max_size_check position_incr);
        format_key_max_size_intermediate_stream(0, ctx, buffer);
        format_key_max_size_intermediate_stream(0, ctx, align_comment);
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
    format_read_stream(1, ctx, false, primitive_incr_pos padding_comment, nbytes);
    if (ctx->in_union)
    {
      format_max_size_intermediate_stream(1, ctx, false, union_case_max_incr " %d;" padding_comment, nbytes);
    }
    else
    {
      format_max_size_intermediate_stream(1, ctx, false, primitive_incr_pos padding_comment, nbytes);
    }
  }

  if (is_key)
  {
    format_key_write_stream(1, ctx, primitive_write_func_padding, nbytes);
    format_key_write_stream(1, ctx, primitive_incr_pos incr_comment, nbytes);
    format_key_size_stream(1, ctx, primitive_incr_pos padding_comment, nbytes);
    format_key_read_stream(1, ctx, primitive_incr_pos padding_comment, nbytes);
    if (ctx->in_union)
    {
      format_key_max_size_intermediate_stream(1, ctx, union_case_max_incr " %d;" padding_comment, nbytes);
    }
    else
    {
      format_key_max_size_intermediate_stream(1, ctx, primitive_incr_pos padding_comment, nbytes);
    }
  }

  return IDL_RETCODE_OK;
}

char* determine_cast(idl_node_t* typespec)
{
  if (idl_is_enum(typespec))
  {
    char* ns = NULL, *returnval = NULL;
    resolve_namespace(typespec, &ns);
    assert(ns);

    idl_asprintf(&returnval, "%s%s", ns, ((idl_enum_t*)typespec)->name->identifier);
    free(ns);
    return returnval;
  }
  else if (idl_is_string(typespec))
  {
    return idl_strdup(char_cast);
  }

  idl_mask_t mask = typespec->mask;
  mask %= IDL_BASE_TYPE * 2;
  switch (mask)
  {
  case IDL_CHAR:
    return idl_strdup(char_cast);
    break;
  case IDL_BOOL:
    return idl_strdup(bool_cast);
    break;
  case IDL_INT8:
    return idl_strdup(int8_cast);
    break;
  case IDL_UINT8:
  case IDL_OCTET:
    return idl_strdup(uint8_cast);
    break;
  case IDL_INT16:
    //case IDL_SHORT:
    return idl_strdup(int16_cast);
    break;
  case IDL_UINT16:
    //case IDL_USHORT:
    return idl_strdup(uint16_cast);
    break;
  case IDL_INT32:
    //case IDL_LONG:
    return idl_strdup(int32_cast);
    break;
  case IDL_UINT32:
    //case IDL_ULONG:
    return idl_strdup(uint32_cast);
    break;
  case IDL_INT64:
    //case IDL_LLONG:
    return idl_strdup(int64_cast);
    break;
  case IDL_UINT64:
    //case IDL_ULLONG:
    return idl_strdup(uint64_cast);
    break;
  case IDL_FLOAT:
    return idl_strdup(float_cast);
    break;
  case IDL_DOUBLE:
    return idl_strdup(double_cast);
    break;
  }
  return NULL;
}

idl_retcode_t process_known_width(context_t* ctx, const char* accessor, idl_node_t* typespec, bool is_key)
{
  assert(ctx);
  assert(accessor);

  int bytewidth = determine_byte_width(typespec);
  assert(bytewidth != -1);

  char* cast = determine_cast(typespec);
  assert(cast);

  check_alignment(ctx, bytewidth, is_key);

  format_write_stream(1, ctx, is_key, primitive_write_func_write, cast, accessor, accessor);
  format_write_stream(1, ctx, is_key, primitive_incr_pos incr_comment, bytewidth);

  format_write_size_stream(1, ctx, is_key, primitive_incr_pos bytes_for_member_comment, bytewidth, accessor);

  if (ctx->in_union)
  {
    format_max_size_intermediate_stream(1, ctx, is_key, union_case_max_incr " %d;\n", bytewidth);
  }
  else
  {
    format_max_size_intermediate_stream(1, ctx, is_key, max_size_incr_checked bytes_for_member_comment, bytewidth, accessor);
  }

  format_read_stream(1, ctx, is_key, primitive_read_func_read, accessor, cast, accessor);
  format_read_stream(1, ctx, is_key, primitive_incr_pos incr_comment, bytewidth);
  free(cast);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_sequence_entries(context_t* ctx, const char* accessor, bool plusone, bool is_key)
{
  assert(ctx);
  assert(accessor);

  check_alignment(ctx, 4, is_key);

  format_read_stream(1, ctx, is_key, "  ");
  format_write_stream(1, ctx, is_key, "  ");
  format_write_size_stream(1, ctx, is_key, "  ");
  format_max_size_intermediate_stream(1, ctx, is_key, "  ");
  if (!ctx->streamer_funcs.sequenceentriespresent)
  {
    format_read_stream(0, ctx, false, "uint32_t ");
    format_write_stream(0, ctx, false, "uint32_t ");
    format_write_size_stream(0, ctx, false, "uint32_t ");
    format_max_size_intermediate_stream(0, ctx, false, "uint32_t ");
    ctx->streamer_funcs.sequenceentriespresent = true;
  }

  if (is_key && !ctx->key_funcs.sequenceentriespresent)
  {
    format_key_read_stream(0, ctx, "uint32_t ");
    format_key_write_stream(0, ctx, "uint32_t ");
    format_key_size_stream(0, ctx, "uint32_t ");
    format_key_max_size_intermediate_stream(0, ctx, "uint32_t ");
    ctx->key_funcs.sequenceentriespresent = true;
  }

  format_read_stream(0, ctx, is_key, primitive_read_func_seq, ctx->depth);
  format_read_stream(1, ctx, is_key, primitive_incr_pos incr_comment, (int)4);

  format_write_stream(0, ctx, is_key, primitive_write_func_seq, ctx->depth, accessor, plusone ? "+1" : "");
  format_write_stream(1, ctx, is_key, primitive_write_func_seq2, ctx->depth, accessor);
  format_write_stream(1, ctx, is_key, primitive_incr_pos incr_comment, (int)4);

  format_write_size_stream(0, ctx, is_key, primitive_write_func_seq, ctx->depth, accessor, plusone ? "+1" : "");
  format_write_size_stream(1, ctx, is_key, primitive_incr_pos bytes_for_seq_entries_comment, (int)4);

  format_max_size_intermediate_stream(0, ctx, is_key, primitive_write_func_seq, ctx->depth, accessor, plusone ? "+1" : "");
  if (ctx->in_union)
  {
    format_max_size_intermediate_stream(1, ctx, is_key, union_case_max_incr "4;\n");
  }
  else
  {
    format_max_size_intermediate_stream(1, ctx, is_key, max_size_incr_checked bytes_for_seq_entries_comment, (int)4);
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_known_width_array(context_t* ctx, const char *accessor, uint64_t entries, idl_node_t* typespec, bool is_key)
{
  assert(ctx);

  int bytewidth = determine_byte_width(typespec);
  assert(bytewidth > 0);

  //check whether the result fits in int?
  int bytesinarray = bytewidth * (int)entries;

  check_alignment(ctx, bytewidth, is_key);
  ctx->streamer_funcs.accumulatedalignment += (int)bytesinarray;

  format_write_stream(1, ctx, is_key, primitive_write_func_array, accessor, bytesinarray, accessor);
  format_write_stream(1, ctx, is_key, primitive_incr_pos incr_comment, bytesinarray);

  format_write_size_stream(1, ctx, is_key, primitive_incr_pos bytes_for_member_comment, bytesinarray, accessor);

  format_read_stream(1, ctx, is_key, primitive_read_func_array, accessor, bytesinarray, accessor);
  format_read_stream(1, ctx, is_key, primitive_incr_pos incr_comment, bytesinarray);

  if (ctx->in_union)
  {
    format_max_size_intermediate_stream(1, ctx, is_key, union_case_max_incr "%d;\n", bytesinarray);
  }
  else
  {
    format_max_size_intermediate_stream(1, ctx, is_key, max_size_incr_checked bytes_for_member_comment, bytesinarray, accessor);
  }

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

    process_node(newctx, (idl_node_t*)module->definitions);

    close_context(newctx, NULL);

    free(cpp11name);
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_constructed(context_t* ctx, idl_node_t* node)
{
  assert(ctx);
  assert(node);

  char* cpp11name = NULL;
  idl_retcode_t returnval = IDL_RETCODE_OK;

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

    format_max_size_stream(1, ctx, false ,max_size_define "\n", cpp11name);
    format_max_size_stream(1, ctx, false, open_block);

    format_key_write_stream(1, ctx, key_write_define "\n", cpp11name);
    format_key_write_stream(1, ctx, open_block);
    format_key_write_stream(1, ctx, "  (void)data;\n");

    format_key_size_stream(1, ctx, key_size_define "\n", cpp11name);
    format_key_size_stream(1, ctx, open_block);

    format_key_max_size_stream(1, ctx, key_max_size_define "\n", cpp11name);
    format_key_max_size_stream(1, ctx, open_block);

    format_read_stream(1, ctx, false, read_func_define "\n", cpp11name);
    format_read_stream(1, ctx, false, open_block);

    format_key_read_stream(1, ctx, key_read_define "\n", cpp11name);
    format_key_read_stream(1, ctx, open_block);
    format_key_read_stream(1, ctx, "  (void)data;\n");

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
        char* ns = NULL;
        assert(base_cpp11name);
        resolve_namespace((idl_node_t*)_struct->base_type, &ns);
        assert(ns);
        char* write_accessor = NULL, *read_accessor = NULL;
        if (idl_asprintf(&write_accessor, const_ref_cast, ns, base_cpp11name) != -1 &&
            idl_asprintf(&read_accessor, ref_cast, ns, base_cpp11name) != -1)
          returnval = write_instance_funcs(ctx, write_accessor, read_accessor, true, false);
        else
          returnval = IDL_RETCODE_NO_MEMORY;

        if (write_accessor)
          free(write_accessor);
        if (read_accessor)
          free(read_accessor);

        free(base_cpp11name);
        free(ns);
      }

      if (_struct->members &&
          returnval == IDL_RETCODE_OK)
        returnval = process_member(ctx, _struct->members);
    }
    else if (idl_is_union(node))
    {
      idl_union_t* _union = (idl_union_t*)node;
      idl_switch_type_spec_t* st = _union->switch_type_spec;
      bool disc_is_key = idl_is_masked(st, IDL_KEY);

      format_read_stream(1, ctx, true, union_clear_func);
      process_known_width(ctx, "_d()", st, disc_is_key);
      format_write_size_stream(1, ctx, false, union_switch);
      format_write_size_stream(1, ctx, false, "  {\n");
      format_write_stream(1, ctx, false, union_switch);
      format_write_stream(1, ctx, false, "  {\n");
      format_read_stream(1, ctx, false, union_switch);
      format_read_stream(1, ctx, false, "  {\n");

      ctx->in_union = true;
      format_max_size_intermediate_stream(1, ctx, false, "  size_t union_max = position;\n");
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
      format_max_size_intermediate_stream(1, ctx, false, "  position = max(position,union_max);\n");
      ctx->in_union = false;

      format_write_stream(1, ctx, false, "  }\n");
      format_write_size_stream(1, ctx, false, "  }\n");
      format_read_stream(1, ctx, false, "  }\n");
    }

    format_write_size_stream(1, ctx, true, position_return);
    format_write_size_stream(1, ctx, true, close_function);
    format_write_stream(1, ctx, true, position_return);
    format_write_stream(1, ctx, true, close_function);
    format_read_stream(1, ctx, true, position_return);
    format_read_stream(1, ctx, true, close_function);

    if (ctx->key_max_size_unlimited)
    {
      flush_ostream(ctx->key_max_size_intermediate_stream);
      format_key_max_size_stream(1, ctx, "  (void)position;\n");
      format_key_max_size_stream(1, ctx, "  return UINT_MAX;\n");
      ctx->key_max_size_unlimited = false;
    }
    else
    {
      transfer_ostream_buffer(ctx->key_max_size_intermediate_stream, ctx->key_max_size_stream);
      format_key_max_size_stream(1, ctx, position_return);
    }

    if (ctx->max_size_unlimited)
    {
      flush_ostream(ctx->max_size_intermediate_stream);
      format_max_size_stream(1, ctx, false, "  (void)position;\n");
      format_max_size_stream(1, ctx, false, "  return UINT_MAX;\n");
      ctx->max_size_unlimited = false;
    }
    else
    {
      transfer_ostream_buffer(ctx->max_size_intermediate_stream, ctx->max_size_stream);
      format_max_size_stream(1, ctx, false, position_return);
    }
    format_max_size_stream(1, ctx, true, close_function);

    format_key_write_stream(1, ctx, key_calc_define "\n", cpp11name);
    format_key_write_stream(1, ctx, "{\n");
    format_key_write_stream(1, ctx, "  size_t sz = key_size(0);\n");
    format_key_write_stream(1, ctx, "  size_t padding = 16 - sz%%16;\n");
    format_key_write_stream(1, ctx, "  if (sz != 0 && padding == 16) padding = 0;\n");
    format_key_write_stream(1, ctx, "  std::vector<unsigned char> buffer(sz+padding);\n");
    format_key_write_stream(1, ctx, "  memset(buffer.data()+sz,0x0,padding);\n");
    format_key_write_stream(1, ctx, "  key_write(buffer.data(),0);\n");
    format_key_write_stream(1, ctx, "  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t &) = NULL;\n");
    format_key_write_stream(1, ctx, "  if (fptr == NULL)\n");
    format_key_write_stream(1, ctx, "  {\n");
    format_key_write_stream(1, ctx, "    if (key_max_size(0) <= 16)\n");
    format_key_write_stream(1, ctx, "    {\n");
    format_key_write_stream(1, ctx, "      //bind to unmodified function which just copies buffer into the keyhash\n");
    format_key_write_stream(1, ctx, "      fptr = &org::eclipse::cyclonedds::topic::simple_key;\n")
    format_key_write_stream(1, ctx, "    }\n");
    format_key_write_stream(1, ctx, "    else\n");
    format_key_write_stream(1, ctx, "    {\n");
    format_key_write_stream(1, ctx, "      //bind to MD5 hash function\n");
    format_key_write_stream(1, ctx, "      fptr = &org::eclipse::cyclonedds::topic::complex_key;\n")
    format_key_write_stream(1, ctx, "    }\n");
    format_key_write_stream(1, ctx, "  }\n");
    format_key_write_stream(1, ctx, "  return (*fptr)(buffer,hash);\n");
    format_key_write_stream(1, ctx, close_function);
  }

  reset_function_contents(&ctx->streamer_funcs);
  reset_function_contents(&ctx->key_funcs);

  if (cpp11name)
    free(cpp11name);
  return returnval;
}

idl_retcode_t process_case(context_t* ctx, idl_case_t* _case)
{
  functioncontents_t sfuncs = ctx->streamer_funcs;
  functioncontents_t kfuncs = ctx->key_funcs;
  if (_case->case_labels)
    process_case_label(ctx, _case->case_labels);

  format_write_stream(1, ctx, false, "  {\n");
  format_write_size_stream(1, ctx, false, "  {\n");
  format_read_stream(1, ctx, false, "  {\n");
  format_max_size_intermediate_stream(1, ctx, false, "{  //cases\n");
  format_max_size_intermediate_stream(1, ctx, false, "  size_t case_max = position;\n");
  ctx->depth++;

  process_instance(ctx, _case->declarator, _case->type_spec, false);

  ctx->depth--;
  format_max_size_intermediate_stream(1, ctx, false, "  union_max = max(case_max,union_max);\n");
  format_max_size_intermediate_stream(1, ctx, false, "}\n");
  format_write_stream(1, ctx, false, "  }\n");
  format_write_stream(1, ctx, false, union_case_ending);
  format_write_size_stream(1, ctx, false, "  }\n");
  format_write_size_stream(1, ctx, false, union_case_ending);
  format_read_stream(1, ctx, false, "  }\n");
  format_read_stream(1, ctx, false, union_case_ending);

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
    format_max_size_stream(1, ctx, false, typedef_max_size_define "\n", tsname, tsname);
    format_max_size_stream(1, ctx, false, open_block);
    format_max_size_stream(1, ctx, false, "  (void)obj;\n");
    format_read_stream(1, ctx, false, typedef_read_define "\n", tsname, tsname);
    format_read_stream(1, ctx, false, open_block);
    format_key_write_stream(1, ctx, typedef_key_write_define "\n", tsname, tsname);
    format_key_write_stream(1, ctx, open_block);
    format_key_read_stream(1, ctx, typedef_key_read_define "\n", tsname, tsname);
    format_key_read_stream(1, ctx, open_block);
    format_key_size_stream(1, ctx, typedef_key_size_define "\n", tsname, tsname);
    format_key_size_stream(1, ctx, open_block);
    format_key_max_size_stream(1, ctx, typedef_key_max_size_define "\n", tsname, tsname);
    format_key_max_size_stream(1, ctx, open_block);
    format_key_max_size_stream(1, ctx, "  (void)obj;\n");

    format_header_stream(1, ctx, typedef_write_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_write_size_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_max_size_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_read_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_write_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_read_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_size_define ";\n\n", tsname, tsname);
    format_header_stream(1, ctx, typedef_key_max_size_define ";\n\n", tsname, tsname);

    process_instance(ctx, NULL, spec, true);

    if (ctx->max_size_unlimited)
    {
      flush_ostream(ctx->max_size_intermediate_stream);
      format_max_size_stream(1, ctx, false, "  (void)position;\n");
      format_max_size_stream(1, ctx, false, "  return UINT_MAX;\n");
      ctx->max_size_unlimited = false;
    }
    else
    {
      transfer_ostream_buffer(ctx->max_size_intermediate_stream, ctx->max_size_stream);
      format_max_size_stream(1, ctx, false, position_return);
    }

    if (ctx->key_max_size_unlimited)
    {
      flush_ostream(ctx->key_max_size_intermediate_stream);
      format_key_max_size_stream(1, ctx, "  (void)position;\n");
      format_key_max_size_stream(1, ctx, "  return UINT_MAX;\n");
      ctx->key_max_size_unlimited = false;
    }
    else
    {
      transfer_ostream_buffer(ctx->key_max_size_intermediate_stream, ctx->key_max_size_stream);
      format_key_max_size_stream(1, ctx, position_return);
    }

    format_write_stream(1, ctx, true, position_return);
    format_write_stream(1, ctx, true,close_function);
    format_write_size_stream(1, ctx, true, position_return);
    format_write_size_stream(1, ctx, true, close_function);
    format_read_stream(1, ctx, true, position_return);
    format_read_stream(1, ctx, true, close_function);
    format_max_size_stream(1, ctx, true, close_function);

    reset_function_contents(&ctx->streamer_funcs);
    reset_function_contents(&ctx->key_funcs);
  }
  return IDL_RETCODE_OK;
}

idl_retcode_t process_typedef_instance_decl(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* spec, bool is_key)
{
  idl_type_spec_t* ispec = resolve_typedef(spec);
  if (idl_is_base_type(ispec))
  {
    return process_base_decl(ctx, decl, ispec, is_key);
  }

  uint64_t entries = array_entries(decl);
  char* accessor = generate_accessor(decl);

  bool locals[4];
  if (entries)
  {
    start_array(ctx, entries, is_key, locals);

    //modify accessor
    char* temp = NULL;
    idl_asprintf(&temp, array_accessor, accessor, ctx->depth);
    assert(temp);
    free(accessor);
    accessor = temp;
  }

  process_typedef_instance_impl(ctx, accessor, spec, is_key);
  free(accessor);

  if (entries)
    stop_array(ctx, is_key, locals);

  if (NULL != decl &&
    ((idl_node_t*)decl)->next)
    process_typedef_instance_decl(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, spec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_typedef_instance_impl(context_t* ctx, const char* accessor, idl_type_spec_t* spec, bool is_key)
{
  char* ns = NULL;  //namespace in which the typedef is declared
  resolve_namespace(spec, &ns);
  assert(ns);
  const char* tdname = idl_identifier(((idl_typedef_t*)spec)->declarators);  //name of the typedef

  format_write_stream(1, ctx, false, typedef_write_call, ns, tdname, accessor);
  format_write_size_stream(1, ctx, false, typedef_write_size_call, ns, tdname, accessor);
  format_read_stream(1, ctx, false, typedef_read_call, ns, tdname, accessor);
  if (ctx->in_union)
  {
    format_max_size_intermediate_stream(1, ctx, false, union_case_max_set "%stypedef_max_size_%s(%s, case_max);\n", ns, tdname, accessor);
  }
  else
  {
    format_max_size_intermediate_stream(1, ctx, false, typedef_max_size_call, ns, tdname, accessor);
  }

  if (is_key)
  {
    format_key_write_stream(1, ctx, typedef_key_write_call, ns, tdname, accessor);
    format_key_read_stream(1, ctx, typedef_key_read_call, ns, tdname, accessor);
    format_key_size_stream(1, ctx, typedef_key_size_call, ns, tdname, accessor);
    if (ctx->in_union)
    {
      format_key_max_size_intermediate_stream(1, ctx, union_case_max_set "%stypedef_key_max_size_%s(%s, case_max);\n", ns, tdname, accessor);
    }
    else
    {
      format_key_max_size_intermediate_stream(1, ctx, typedef_key_max_size_call, ns, tdname, accessor);
    }
  }
  free(ns);

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
      format_write_stream(1, ctx, false, union_case, buffer);
      format_write_size_stream(1, ctx, false, union_case, buffer);
      format_read_stream(1, ctx, false, union_case, buffer);
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
  format_write_stream(1, ctx, false, default_case);
  format_write_size_stream(1, ctx, false, default_case);
  format_read_stream(1, ctx, false, default_case);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_base_decl(context_t* ctx, idl_declarator_t* decl, idl_type_spec_t* tspec, bool is_key)
{
  assert(ctx);
  assert(tspec);

  char* accessor = generate_accessor(decl);
  process_base_impl(ctx, accessor, array_entries(decl), tspec, is_key);
  free(accessor);

  if (NULL != decl &&
      ((idl_node_t*)decl)->next)
    process_base_decl(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, tspec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_base_impl(context_t* ctx, const char* accessor, uint64_t entries, idl_type_spec_t* tspec, bool is_key)
{
  if (entries)
    process_known_width_array(ctx, accessor, entries, tspec, is_key);
  else
    process_known_width(ctx, accessor, tspec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_string_decl(context_t* ctx, idl_declarator_t* decl, idl_string_t* spec, bool is_key)
{
  assert(ctx);
  assert(spec);

  uint64_t entries = array_entries(decl);
  char* accessor = generate_accessor(decl);

  bool locals[4];
  if (entries)
  {
    start_array(ctx, entries, is_key, locals);

    //modify accessor
    char* temp = NULL;
    idl_asprintf(&temp, array_accessor, accessor, ctx->depth);
    assert(temp);
    free(accessor);
    accessor = temp;
  }

  process_string_impl(ctx, accessor, spec, is_key);
  free(accessor);

  if (entries)
    stop_array(ctx, is_key, locals);

  if (NULL != decl &&
    ((idl_node_t*)decl)->next)
    process_string_decl(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, spec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_string_impl(context_t* ctx, const char* accessor, idl_string_t* spec, bool is_key)
{
  //sequence entries
  process_sequence_entries(ctx, accessor, true, is_key);
  uint64_t bound = spec->maximum;
  if (bound)
  {
    //add boundary checking function
    format_write_stream(1, ctx, is_key, seq_length_exception, ctx->depth, bound+1, accessor, bound);
    //adding one to the length check here, since the sequence_entries CDR field for a string includes the terminating NULL
  }
  else
  {
    ctx->max_size_unlimited = true;
    if (is_key)
      ctx->key_max_size_unlimited = true;
  }

  //data
  format_write_stream(1, ctx, is_key, string_write, accessor, ctx->depth, 1, accessor);
  format_write_stream(1, ctx, is_key, seq_inc_1 entries_of_sequence_comment, ctx->depth);
  format_write_size_stream(1, ctx, is_key, seq_inc_1 entries_of_sequence_comment, ctx->depth);

  format_read_stream(1, ctx, is_key, primitive_read_seq, accessor, "char", "char", ctx->depth, accessor);
  format_read_stream(1, ctx, is_key, seq_inc_1 entries_of_sequence_comment, ctx->depth);

  if (bound)
  {
    if (ctx->in_union)
    {
      format_max_size_intermediate_stream(1, ctx, is_key, union_case_max_incr "%d;\n", bound + 1);
    }
    else
    {
      format_max_size_intermediate_stream(1, ctx, is_key, max_size_incr_checked entries_of_sequence_comment, bound + 1);
    }
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t process_sequence_decl(context_t* ctx, idl_declarator_t* decl, idl_sequence_t* spec, bool is_key)
{
  assert(ctx);
  assert(spec);

  uint64_t entries = array_entries(decl);
  char* accessor = generate_accessor(decl);

  bool locals[4];
  if (entries)
  {
    start_array(ctx, entries, is_key, locals);

    //modify accessor
    char* temp = NULL;
    idl_asprintf(&temp, array_accessor, accessor, ctx->depth);
    assert(temp);
    free(accessor);
    accessor = temp;
  }

  process_sequence_impl(ctx, accessor, spec, is_key);
  free(accessor);

  if (entries)
    stop_array(ctx, is_key, locals);

  if (NULL != decl &&
    ((idl_node_t*)decl)->next)
    process_sequence_decl(ctx, (idl_declarator_t*)((idl_node_t*)decl)->next, spec, is_key);

  return IDL_RETCODE_OK;
}

idl_retcode_t process_sequence_impl(context_t* ctx, const char* accessor, idl_sequence_t* spec, bool is_key)
{
  idl_type_spec_t* ispec = spec->type_spec;
  if (idl_is_typedef(ispec))
  {
    idl_type_spec_t* temp = resolve_typedef(ispec);
    if (idl_is_base_type(temp))
      ispec = temp;
  }

  //sequence entries
  process_sequence_entries(ctx, accessor, false, is_key);
  uint64_t bound = spec->maximum;
  if (bound)
  {
    //add boundary checking function
    format_write_stream(1, ctx, is_key, seq_length_exception, ctx->depth, bound, accessor, bound);
  }
  else
  {
    ctx->max_size_unlimited = true;
    if (is_key)
      ctx->key_max_size_unlimited = true;
  }

  if (idl_is_base_type(ispec))
  {
    //base types are treated differently from more complex template types
    int bytewidth = determine_byte_width(ispec);
    assert(bytewidth > 0);
    if (bytewidth > 4)
      check_alignment(ctx, bytewidth, is_key);
    char* cast = determine_cast(ispec);
    assert(cast);

    if (0 == strcmp(cast,bool_cast))  //necessary because IDL_BOOL has overlap with IDL_ULONG
    {
      format_write_stream(1, ctx, is_key, bool_write_seq, ctx->depth, accessor, accessor);
      format_read_stream(1, ctx, is_key, seq_read_resize, accessor, ctx->depth);
      format_read_stream(1, ctx, is_key, bool_read_seq, ctx->depth, accessor, accessor);
    }
    else
    {
      format_write_stream(1, ctx, is_key, primitive_write_seq_checked, accessor, accessor, ctx->depth, bytewidth, accessor);
      format_write_stream(1, ctx, is_key, seq_incr entries_of_sequence_comment, ctx->depth, bytewidth);
      format_read_stream(1, ctx, is_key, primitive_read_seq, accessor, cast, cast, ctx->depth, accessor);
      format_read_stream(1, ctx, is_key, seq_incr entries_of_sequence_comment, ctx->depth, bytewidth);
    }
    format_write_size_stream(1, ctx, is_key, seq_incr entries_of_sequence_comment, ctx->depth, bytewidth);
    if (bound)
    {
      format_max_size_intermediate_stream(1, ctx, is_key, max_size_incr_checked_multiple, ctx->depth, bytewidth);
    }

    free(cast);
  }
  else
  {
    format_read_stream(1, ctx, is_key, seq_read_resize, accessor, ctx->depth);

    //loop over
    format_write_stream(1, ctx, is_key, sequence_iterate, ctx->depth + 1, ctx->depth + 1, ctx->depth, ctx->depth + 1);
    format_write_size_stream(1, ctx, is_key, sequence_iterate, ctx->depth + 1, ctx->depth + 1, ctx->depth, ctx->depth + 1);
    format_read_stream(1, ctx, is_key, sequence_iterate, ctx->depth + 1, ctx->depth + 1, ctx->depth, ctx->depth + 1);
    format_max_size_intermediate_stream(1, ctx, is_key, sequence_iterate, ctx->depth + 1, ctx->depth + 1, ctx->depth, ctx->depth + 1);
    ctx->depth++;

    bool locals[4];
    store_locals(ctx, locals);

    char* entryaccess = NULL;
    idl_asprintf(&entryaccess, array_accessor, accessor, ctx->depth);
    assert(entryaccess);

    if (idl_is_string(ispec))
    {
      process_string_impl(ctx, entryaccess, (idl_string_t*)ispec, is_key);
    }
    else if (idl_is_sequence(ispec))
    {
      process_sequence_impl(ctx, entryaccess, (idl_sequence_t*)ispec, is_key);
    }
    else if (idl_is_struct(ispec) || idl_is_union(ispec))
    {
      process_constructed_type_impl(ctx, entryaccess, is_key, !has_keys((idl_type_spec_t*)spec));
    }
    else if (idl_is_typedef(ispec))
    {
      process_typedef_instance_impl(ctx, entryaccess, ispec, is_key);
    }
    free(entryaccess);

    //close loop
    format_write_size_stream(1, ctx, is_key, close_block);
    format_write_stream(1, ctx, is_key, close_block);
    format_read_stream(1, ctx, is_key, close_block);
    format_max_size_intermediate_stream(1, ctx, is_key, close_block);

    ctx->depth--;
    load_locals(ctx, locals);
  }

  return IDL_RETCODE_OK;
}

void idl_streamers_generate(const idl_tree_t* tree, idl_streamer_output_t* str)
{
  context_t* ctx = create_context("");

  format_impl_stream(0, ctx, "#include \"org/eclipse/cyclonedds/topic/hash.hpp\"\n\n");
  if (tree->files)
    ctx->parsed_file = tree->files->name;

  process_node(ctx, tree->root);

  close_context(ctx, str);
}
