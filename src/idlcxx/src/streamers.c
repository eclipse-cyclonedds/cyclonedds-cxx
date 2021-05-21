/*
 * Copyright(c) 2021 ADLINK Technology Limited and others
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include "idl/string.h"
#include "idl/processor.h"
#include "idl/print.h"
#include "idl/stream.h"

#include "generator.h"

#define CHUNK (4096)

static idl_retcode_t vputf(idl_buffer_t *buf, const char *fmt, va_list ap)
{
  va_list aq;
  int cnt;
  char str[1], *data = str;
  size_t size = 0;

  assert(buf);
  assert(fmt);

  va_copy(aq, ap);
  if (buf->data && (size = (buf->size - buf->used)) > 0)
    data = buf->data + buf->used;
  cnt = idl_vsnprintf(data, size+1, fmt, aq);
  va_end(aq);

  if (cnt >= 0 && size <= (size_t)cnt) {
    size = buf->size + ((((size_t)cnt - size) / CHUNK) + 1) * CHUNK;
    if (!(data = realloc(buf->data, size+1)))
      return IDL_RETCODE_NO_MEMORY;
    buf->data = data;
    buf->size = size;
    cnt = idl_vsnprintf(buf->data + buf->used, size, fmt, ap);
  }

  if (cnt < 0)
    return IDL_RETCODE_NO_MEMORY;
  buf->used += (size_t)cnt;
  return IDL_RETCODE_OK;
}

static idl_retcode_t putf(idl_buffer_t *buf, const char *fmt, ...)
{
  va_list ap;
  idl_retcode_t ret;

  va_start(ap, fmt);
  ret = vputf(buf, fmt, ap);
  va_end(ap);
  return ret;
}

int get_array_accessor(char* str, size_t size, const void* node, void* user_data)
{
  (void)node;
  uint32_t depth = *((uint32_t*)user_data);
  return idl_snprintf(str, size, "a_%u", depth);
}

struct sequence_holder {
  const char* sequence_accessor;
  size_t depth;
};
typedef struct sequence_holder sequence_holder_t;

int get_sequence_member_accessor(char* str, size_t size, const void* node, void* user_data)
{
  (void)node;
  sequence_holder_t* sh = (sequence_holder_t*)user_data;
  return idl_snprintf(str, size, "%s[i_%u]", sh->sequence_accessor, sh->depth);
}

enum instance_location {
  STRUCT_MEMBER,
  TYPEDEF,
  UNION_BRANCH
};

int get_instance_accessor(char* str, size_t size, const void* node, void* user_data)
{
  enum instance_location loc = *(enum instance_location*)user_data;
  if (loc == TYPEDEF)
  {
    return idl_snprintf(str, size, "instance");
  }
  else
  {
    const idl_declarator_t* decl = (const idl_declarator_t*)node;
    const char* name = get_cpp11_name(decl);
    return idl_snprintf(str, size, "instance.%s()", name);
  }
}

struct streams {
  struct generator *generator;
  idl_buffer_t write;
  idl_buffer_t read;
  idl_buffer_t move;
  idl_buffer_t max;
  idl_buffer_t key_write;
  idl_buffer_t key_read;
  idl_buffer_t key_move;
  idl_buffer_t key_max;
  idl_buffer_t key;
  size_t keys;
};

void setup_streams(struct streams* str, struct generator* gen)
{
  assert(str);
  memset(str, 0, sizeof(struct streams));
  str->generator = gen;
}

void cleanup_streams(struct streams* str)
{
  if (str->write.data)
    free(str->write.data);
  if (str->read.data)
    free(str->read.data);
  if (str->move.data)
    free(str->move.data);
  if (str->max.data)
    free(str->max.data);
  if (str->key_write.data)
    free(str->key_write.data);
  if (str->key_read.data)
    free(str->key_read.data);
  if (str->key_move.data)
    free(str->key_move.data);
  if (str->key_max.data)
    free(str->key_max.data);
  if (str->key.data)
    free(str->key.data);
}

static idl_retcode_t flush_stream(idl_buffer_t* str, FILE* f)
{
  if (str->data && fputs(str->data, f) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (str->size &&
      str->data)
      str->data[0] = '\0';
  str->used = 0;

  return IDL_RETCODE_OK;
}

static idl_retcode_t flush(struct generator* gen, struct streams* streams)
{
  if (IDL_RETCODE_OK != flush_stream(&streams->write, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->read, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->move, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->max, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->key_write, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->key_read, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->key_move, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_RETCODE_OK != flush_stream(&streams->key_max, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
check_endpoint(
  const idl_pstate_t* pstate,
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const char* accessor,
  const char* read_accessor,
  bool is_key,
  bool is_sequence)
{
  uint32_t maximum = 0;
  if (idl_is_string(type_spec))
    maximum = ((const idl_string_t*)type_spec)->maximum;

  const char* fmt = "  %4$s::%2$s(streamer, %1$s);\n";
  const char* name = NULL;
  char * ns = "::org::eclipse::cyclonedds::core::cdr";
  if (idl_is_declarator(type_spec) /*typedefs*/) {
    if (IDL_PRINTA(&ns, get_cpp11_namespace, type_spec, streams->generator) < 0)
        return IDL_RETCODE_NO_MEMORY;
    name = get_cpp11_name(type_spec);
    fmt = "  %4$s::%2$s_%5$s(streamer, %1$s);\n";
  } else if (idl_is_string(type_spec)) {
    fmt = "  %4$s::%2$s_string(streamer, %1$s, %3$u);\n";
  } else if (idl_is_struct(type_spec) || idl_is_union(type_spec)) {
    if (IDL_PRINTA(&ns, get_cpp11_namespace, type_spec, streams->generator) < 0)
      return IDL_RETCODE_NO_MEMORY;
    fmt = "  %4$s::%2$s(streamer, %1$s);\n";
  }

  /*
  * due to the differing implementation of std::vector<bool>::operator[]
  * there needs to be a dummy variable which is read into before assigning
  * to the entry in the container
  */
  const char* read_fmt = fmt;
  if ((idl_mask(type_spec) & IDL_BOOL) == IDL_BOOL &&
      is_sequence) {
    read_fmt = "  {\n"
               "    bool b;\n"
               "    ::org::eclipse::cyclonedds::core::cdr::read(streamer, b);\n"
               "    %1$s = b;"
               "  }\n";
  }

  if (putf(&streams->write, fmt, accessor, "write", maximum, ns, name)
   || putf(&streams->read, read_fmt, read_accessor, "read", maximum, ns, name)
   || putf(&streams->move, fmt, accessor, "move", maximum, ns, name)
   || putf(&streams->max, fmt, accessor, "max", maximum, ns, name))
    return IDL_RETCODE_NO_MEMORY;

  /* short-circuit if instance is not a key */
  if (!is_key)
    return IDL_RETCODE_OK;
  streams->keys++;

  if (idl_is_constr_type(type_spec) &&
      !idl_is_keyless(type_spec, pstate->flags & IDL_FLAG_KEYLIST))
  {
    fmt = "  %4$s::key_%2$s(streamer, %1$s);\n";
    read_fmt = fmt;
  }

  if (putf(&streams->key_write, fmt, accessor, "write", maximum, ns, name)
   || putf(&streams->key_read, read_fmt, read_accessor, "read", maximum, ns, name)
   || putf(&streams->key_move, fmt, accessor, "move", maximum, ns, name)
   || putf(&streams->key_max, fmt, accessor, "max", maximum, ns, name))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
unroll_sequence(const idl_pstate_t* pstate,
  struct streams* streams,
  const idl_sequence_t* seq,
  size_t depth,
  const char* accessor,
  const char* read_accessor,
  bool is_key)
{
  uint32_t maximum = seq->maximum;

  const char* wfmt = maximum ? "  {\n"\
                               "  uint32_t se_%1$u = uint32_t(%2$s.size());\n"\
                               "  if (se_%1$u > %3$u &&\n"
                               "      streamer.status(::org::eclipse::cyclonedds::core::cdr::serialization_status::%4$s_bound_exceeded))\n"
                               "        return;\n"\
                               "  ::org::eclipse::cyclonedds::core::cdr::%4$s(streamer, se_%1$u);\n"\
                               "  for (uint32_t i_%1$u = 0; i_%1$u < se_%1$u; i_%1$u++) {\n"
                             : "  {\n"\
                               "  uint32_t se_%1$u = uint32_t(%2$s.size());\n"\
                               "  ::org::eclipse::cyclonedds::core::cdr::%4$s(streamer, se_%1$u);\n"\
                               "  for (uint32_t i_%1$u = 0; i_%1$u < se_%1$u; i_%1$u++) {\n";
  const char* rfmt = maximum ? "  {\n"\
                               "  uint32_t se_%1$u;\n"\
                               "  ::org::eclipse::cyclonedds::core::cdr::read(streamer, se_%1$u);\n"\
                               "  if (se_%1$u > %4$u &&\n"
                               "      streamer.status(::org::eclipse::cyclonedds::core::cdr::serialization_status::read_bound_exceeded))\n"
                               "        return;\n"\
                               "  %3$s.resize(se_%1$u);\n"\
                               "  for (uint32_t i_%1$u = 0; i_%1$u < se_%1$u; i_%1$u++) {\n"\
                             : "  {\n"\
                               "  uint32_t se_%1$u;\n"\
                               "  ::org::eclipse::cyclonedds::core::cdr::read(streamer, se_%1$u);\n"\
                               "  %3$s.resize(se_%1$u);\n"\
                               "  for (uint32_t i_%1$u = 0; i_%1$u < se_%1$u; i_%1$u++) {\n";
  const char* mfmt = "  {\n"\
                     "  uint32_t se_%1$u = uint32_t(%2$u);\n"\
                     "  ::org::eclipse::cyclonedds::core::cdr::max(streamer, se_%1$u);\n"\
                     "  for (uint32_t i_%1$u = 0; i_%1$u < se_%1$u; i_%1$u++) {\n";

  char* type = NULL;
  if (IDL_PRINTA(&type, get_cpp11_type, seq->type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (putf(&streams->read, rfmt, depth, type, read_accessor, maximum)
   || putf(&streams->write, wfmt, depth, accessor, maximum, "write")
   || putf(&streams->move, wfmt, depth, accessor, maximum, "move")
   || putf(&streams->max, mfmt, depth, maximum))
    return IDL_RETCODE_NO_MEMORY;

  if (is_key &&
      (putf(&streams->key_read, rfmt, depth, type, read_accessor, maximum)
    || putf(&streams->key_write, wfmt, depth, accessor, maximum, "write")
    || putf(&streams->key_move, wfmt, depth, accessor, maximum, "move")
    || putf(&streams->key_max, mfmt, depth, maximum)))
    return IDL_RETCODE_NO_MEMORY;

  sequence_holder_t sh = (sequence_holder_t){ .sequence_accessor = accessor, .depth = depth};
  char* new_accessor = NULL;
  if (IDL_PRINTA(&new_accessor, get_sequence_member_accessor, &sh, &sh) < 0)
    return IDL_RETCODE_NO_MEMORY;

  sh.sequence_accessor = read_accessor;
  char* new_read_accessor = NULL;
  if (IDL_PRINTA(&new_read_accessor, get_sequence_member_accessor, &sh, &sh) < 0)
    return IDL_RETCODE_NO_MEMORY;

  idl_retcode_t ret = IDL_RETCODE_OK;
  if (idl_is_sequence(seq->type_spec))
    ret = unroll_sequence (pstate, streams, (idl_sequence_t*)seq->type_spec, depth + 1, new_accessor, new_read_accessor, is_key);
  else
    ret = check_endpoint (pstate, streams, seq->type_spec, new_accessor, new_read_accessor, is_key, true);

  if (ret != IDL_RETCODE_OK)
    return ret;

  //close sequence
  wfmt = "  } //i_%1$u\n  }\n";
  mfmt = maximum ? wfmt
                 : "  } //i_%1$u\n  streamer.position(SIZE_MAX);\n  return;\n  }\n";

  if (putf(&streams->read, wfmt, depth)
   || putf(&streams->write, wfmt, depth)
   || putf(&streams->move, wfmt, depth)
   || putf(&streams->max, mfmt, depth))
  return IDL_RETCODE_NO_MEMORY;

  if (is_key &&
      (putf(&streams->key_read, wfmt, depth)
    || putf(&streams->key_write, wfmt, depth)
    || putf(&streams->key_move, wfmt, depth)
    || putf(&streams->key_max, mfmt, depth)))
      return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
unroll_array(
  struct streams* streams,
  char *accessor,
  uint32_t array_depth,
  bool is_key)
{
  const char* afmt = array_depth ? "  for (%1$sauto & a_%2$u:a_%3$u)\n"
                                 : "  for (%1$sauto & a_%2$u:%4$s)\n";

  if (putf(&streams->write, afmt, "const ", array_depth+1, array_depth, accessor)
   || putf(&streams->read, afmt, "", array_depth+1, array_depth, accessor)
   || putf(&streams->move, afmt, "const ", array_depth+1, array_depth, accessor)
   || putf(&streams->max, afmt, "const ", array_depth+1, array_depth, accessor))
    return IDL_RETCODE_NO_MEMORY;
  if (is_key &&
      (putf(&streams->key_write, afmt, "const ", array_depth+1, array_depth, accessor)
    || putf(&streams->key_read, afmt, "", array_depth+1, array_depth, accessor)
    || putf(&streams->key_move, afmt, "const ", array_depth+1, array_depth, accessor)
    || putf(&streams->key_max, afmt, "const ", array_depth+1, array_depth, accessor)))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_instance(
  const idl_pstate_t *pstate,
  struct streams *streams,
  const idl_declarator_t* declarator,
  const idl_type_spec_t* type_spec,
  bool is_key,
  enum instance_location loc)
{
  assert(declarator);

  char* accessor = NULL;
  if (IDL_PRINTA(&accessor, get_instance_accessor, declarator, &loc) < 0)
    return IDL_RETCODE_NO_MEMORY;

  //unroll arrays
  idl_retcode_t ret = IDL_RETCODE_OK;
  if (idl_is_array(declarator)) {
    uint32_t n_arr = 0;
    const idl_literal_t* lit = (const idl_literal_t*)declarator->const_expr;
    while (lit) {
      if ((ret = unroll_array(streams, accessor, n_arr++, is_key)) != IDL_RETCODE_OK)
        return ret;

      lit = (const idl_literal_t*)((const idl_node_t*)lit)->next;
    }
    //update accessor to become "a_$n_arr$"
    if (IDL_PRINTA(&accessor, get_array_accessor, declarator, &n_arr) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  char* read_accessor;
  if (loc == UNION_BRANCH)
    read_accessor = "obj";
  else
    read_accessor = accessor;

  //unroll sequences (if any)
  if (idl_is_sequence(type_spec))
    ret = unroll_sequence(pstate, streams, (idl_sequence_t*)type_spec, 1, accessor, read_accessor, is_key);
  else
    ret = check_endpoint(pstate, streams, type_spec, accessor, read_accessor, is_key, false);

  return ret;
}

static idl_retcode_t
process_member(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  const idl_declarator_t *declarator;
  const idl_type_spec_t *type_spec;
  bool is_key = false;

  (void)revisit;
  (void)path;

  type_spec = ((const idl_member_t *)node)->type_spec;
  if (!(pstate->flags & IDL_FLAG_KEYLIST))
    is_key = ((const idl_member_t *)node)->key == IDL_TRUE;

  IDL_FOREACH(declarator, ((const idl_member_t *)node)->declarators) {
    if (process_instance(pstate, user_data, declarator, type_spec, is_key, STRUCT_MEMBER))
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_case(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct streams *streams = user_data;
  const idl_case_t* _case = (const idl_case_t*)node;
  bool single = (idl_degree(_case->labels) == 1),
       simple = idl_is_base_type(_case->type_spec);

  static const char max_start[] =
    "  {\n"
    "    size_t pos = streamer.position();\n"
    "    size_t alignment = streamer.alignment();\n";
  static const char max_end[] =
    "    if (union_max < streamer.position()) {\n"
    "      union_max = streamer.position();\n"
    "      alignment_max = streamer.alignment();\n"
    "    }\n"
    "    streamer.position(pos);\n"
    "    streamer.alignment(alignment);\n"
    "  }\n";

  (void)pstate;
  (void)path;

  const char* read_start = simple ? "  {\n"
                                    "    %1$s obj = %2$s;\n"
                                  : "  {\n"
                                    "    %1$s obj;\n";

  const char* read_end = single   ? "    instance.%1$s(obj);\n"
                                    "  }\n"
                                    "  break;\n"
                                  : "    instance.%1$s(obj, d);\n"
                                    "  }\n"
                                    "  break;\n";

  if (revisit) {
    const char *fmt = "      break;\n", *name = get_cpp11_name(_case->declarator);

    char *accessor, *type, *value = NULL;
    enum instance_location loc = UNION_BRANCH;
    if (IDL_PRINTA(&accessor, get_instance_accessor, _case->declarator, &loc) < 0 ||
        IDL_PRINTA(&type, get_cpp11_type, _case->type_spec, streams->generator) < 0 ||
        (simple && IDL_PRINTA(&value, get_cpp11_default_value, _case->type_spec, streams->generator) < 0))
      return IDL_RETCODE_NO_MEMORY;

    if (putf(&streams->read, read_start, type, value)
     || putf(&streams->max, max_start))
      return IDL_RETCODE_NO_MEMORY;

    idl_retcode_t ret = IDL_RETCODE_OK;
    if ((ret = process_instance(pstate, streams, _case->declarator, _case->type_spec, false, loc)) != IDL_RETCODE_OK)
      return ret;

    if (putf(&streams->write, fmt)
     || putf(&streams->move, fmt)
     || putf(&streams->read, read_end, name)
     || putf(&streams->max, max_end))
      return IDL_RETCODE_NO_MEMORY;

    if (idl_next(_case))
      return IDL_RETCODE_OK;

    fmt = "  }\n";
    if (putf(&streams->write, fmt)
     || putf(&streams->read, fmt)
     || putf(&streams->move, fmt))
      return IDL_RETCODE_NO_MEMORY;
  } else {
    const char *fmt = "  switch(d)\n  {\n";
    if (idl_previous(_case))
      return IDL_VISIT_REVISIT;
    if (putf(&streams->write, fmt)
     || putf(&streams->read, fmt)
     || putf(&streams->move, fmt))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_inherit_spec(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct streams *streams = user_data;
  const idl_type_spec_t *type_spec = ((const idl_inherit_spec_t *)node)->base;
  char *type = NULL, *ns = NULL;
  const char *fmt = "  %3$s::%2$s(streamer,dynamic_cast<%1$s&>(instance));\n";
  const char *constfmt = "  %3$s::%2$s(streamer,dynamic_cast<const %1$s&>(instance));\n";

  (void)pstate;
  (void)revisit;
  (void)path;

  if (IDL_PRINTA(&type, get_cpp11_fully_scoped_name, type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (IDL_PRINTA(&ns, get_cpp11_namespace, type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (putf(&streams->write, constfmt, type, "write", ns)
   || putf(&streams->read, fmt, type, "read", ns)
   || putf(&streams->move, constfmt, type, "move", ns)
   || putf(&streams->max, constfmt, type, "max", ns))
    return IDL_RETCODE_NO_MEMORY;
  if (putf(&streams->key_write, constfmt, type, "key_write", ns)
   || putf(&streams->key_read, fmt, type, "key_read", ns)
   || putf(&streams->key_move, constfmt, type, "key_move", ns)
   || putf(&streams->key_max, constfmt, type, "key_max", ns))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_keylist(struct streams *streams, const idl_keylist_t* keylist)
{
  const idl_key_t *key;

  IDL_FOREACH(key, keylist->keys) {
    streams->keys++;

    char* ns = "::org::eclipse::cyclonedds::core::cdr";
    const char *fmt = "  %3$s::%2$s(streamer, instance.%1$s()";
    for (size_t i=0; i < key->field_name->length; i++) {
      const char *identifier = key->field_name->names[i]->identifier;
      if (putf(&streams->key_write, fmt, identifier, "write", ns)
       || putf(&streams->key_read, fmt, identifier, "read", ns)
       || putf(&streams->key_move, fmt, identifier, "move", ns)
       || putf(&streams->key_max, fmt, identifier, "max", ns))
        return IDL_RETCODE_NO_MEMORY;
      fmt = ".%1$s()";
    }
    if (putf(&streams->key_write, ");\n") < 0
     || putf(&streams->key_read, ");\n") < 0
     || putf(&streams->key_move, ");\n") < 0
     || putf(&streams->key_max, ");\n") < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_constructed_type_open(struct streams *streams, const idl_node_t *node)
{
  const char* name = get_cpp11_name(node);
  const char *fmt =
    "template<typename T>\n"
    "void %2$s(T& streamer, %1$s& instance)\n{\n";
  const char *constfmt =
    "template<typename T>\n"
    "void %2$s(T& streamer, const %1$s& instance)\n{\n";

  if (putf(&streams->write, constfmt, name, "write")
   || putf(&streams->read, fmt, name, "read")
   || putf(&streams->move, constfmt, name, "move")
   || putf(&streams->max, constfmt, name, "max")
   || putf(&streams->key_write, constfmt, name, "key_write")
   || putf(&streams->key_read, fmt, name, "key_read")
   || putf(&streams->key_move, constfmt, name, "key_move")
   || putf(&streams->key_max, constfmt, name, "key_max"))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_constructed_type_close(struct streams *streams, const idl_node_t *node)
{
  const char *close_key;

  (void)node;

  if (streams->keys)
    close_key =
      "}\n\n";
  else
    close_key =
      "  (void)streamer;\n"
      "  (void)instance;\n"
      "}\n\n";

  if (putf(&streams->write, "}\n\n")
   || putf(&streams->read, "}\n\n")
   || putf(&streams->move, "}\n\n")
   || putf(&streams->max, "}\n\n"))
    return IDL_RETCODE_NO_MEMORY;
  if (putf(&streams->key_write, close_key)
   || putf(&streams->key_read, close_key)
   || putf(&streams->key_move, close_key)
   || putf(&streams->key_max, close_key))
    return IDL_RETCODE_NO_MEMORY;

  streams->keys = 0;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_struct(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  (void)path;

  if (revisit) {
    return print_constructed_type_close(user_data, node);
  } else {
    bool keylist;
    const idl_struct_t *_struct = ((const idl_struct_t *)node);

    keylist = (pstate->flags & IDL_FLAG_KEYLIST) && _struct->keylist;

    if (print_constructed_type_open(user_data, node))
      return IDL_RETCODE_NO_MEMORY;
    if (keylist && process_keylist(user_data, _struct->keylist))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }
}

static idl_retcode_t
process_module(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct streams* streams = user_data;

  (void)pstate;
  (void)path;

  if (revisit) {
    if (flush(streams->generator, streams) != IDL_RETCODE_OK ||
        idl_fprintf(streams->generator->header.handle, "\n}\n\n") < 0)
      return IDL_RETCODE_NO_MEMORY;
  } else {
    const char* name = get_cpp11_name(node);
    if (flush(streams->generator, streams) != IDL_RETCODE_OK ||
        idl_fprintf(streams->generator->header.handle, "\nnamespace %s\n{\n\n", name) < 0)
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_switch_type_spec(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  static const char writefmt[] =
    "  auto d = instance._d();\n"
    "  ::org::eclipse::cyclonedds::core::cdr::write(streamer, d);\n";
  static const char readfmt[] =
    "  auto d = instance._d();\n"
    "  ::org::eclipse::cyclonedds::core::cdr::read(streamer, d);\n";
  static const char movefmt[] =
    "  auto d = instance._d();\n"
    "  ::org::eclipse::cyclonedds::core::cdr::move(streamer, d);\n";
  static const char maxfmt[] =
    "  ::org::eclipse::cyclonedds::core::cdr::max(streamer, instance._d());\n"
    "  size_t union_max = streamer.position();\n"
    "  size_t alignment_max = streamer.alignment();\n";
  static const char key_maxfmt[] =
    "  ::org::eclipse::cyclonedds::core::cdr::max(streamer, instance._d());\n";

  struct streams *streams = user_data;
  const idl_switch_type_spec_t *switch_type_spec = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (putf(&streams->write, writefmt)
   || putf(&streams->read, readfmt)
   || putf(&streams->move, movefmt)
   || putf(&streams->max, maxfmt))
    return IDL_RETCODE_NO_MEMORY;

  /* short-circuit if switch type specifier is not a key */
  if (switch_type_spec->key != IDL_TRUE)
    return IDL_RETCODE_OK;

  if (putf(&streams->key_write, writefmt)
   || putf(&streams->key_read, readfmt)
   || putf(&streams->key_move, movefmt)
   || putf(&streams->key_max, key_maxfmt))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_union(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct streams *streams = user_data;

  (void)pstate;
  (void)path;

  if (revisit) {
    static const char fmt[] = "  streamer.position(union_max);\n"
                              "  streamer.alignment(alignment_max);\n";
    if (putf(&streams->max, fmt))
      return IDL_RETCODE_NO_MEMORY;
    if (print_constructed_type_close(user_data, node))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_RETCODE_OK;
  } else {
    if (print_constructed_type_open(user_data, node))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }
}

static idl_retcode_t
process_case_label(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct streams *streams = user_data;
  const idl_literal_t *literal = ((const idl_case_label_t *)node)->const_expr;
  char *value = "";
  const char *casefmt;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (idl_mask(node) == IDL_DEFAULT_CASE_LABEL) {
    casefmt = "    default:\n";
  } else {
    casefmt = "    case %s:\n";
    if (IDL_PRINTA(&value, get_cpp11_value, literal, streams->generator) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  if (putf(&streams->write, casefmt, value)
   || putf(&streams->read, casefmt, value)
   || putf(&streams->move, casefmt, value))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_typedef(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  (void)revisit;
  (void)path;

  const char* fmt =
    "template<typename T>\n"
    "void %2$s_%1$s(T& streamer, %1$s& instance)\n{\n";
  const char* constfmt =
    "template<typename T>\n"
    "void %2$s_%1$s(T& streamer, const %1$s& instance)\n{\n";

  struct streams* streams = user_data;
  idl_typedef_t* td = (idl_typedef_t*)node;
  const idl_declarator_t* declarator;

  IDL_FOREACH(declarator, td->declarators) {
    const char* name = get_cpp11_name(declarator);

    if (putf(&streams->write, constfmt, name, "write")
     || putf(&streams->read, fmt, name, "read")
     || putf(&streams->move, constfmt, name, "move")
     || putf(&streams->max, constfmt, name, "max")
     || putf(&streams->key_write, constfmt, name, "key_write")
     || putf(&streams->key_read, fmt, name, "key_read")
     || putf(&streams->key_move, constfmt, name, "key_move")
     || putf(&streams->key_max, constfmt, name, "key_max"))
      return IDL_RETCODE_NO_MEMORY;

    idl_retcode_t ret = process_instance(pstate, streams, declarator, td->type_spec, true, TYPEDEF);
    if (ret != IDL_RETCODE_OK)
      return ret;

    if (print_constructed_type_close(streams, node))
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
}

idl_retcode_t
generate_streamers(const idl_pstate_t* pstate, struct generator *gen)
{
  struct streams streams;
  idl_visitor_t visitor;
  const char *sources[] = { NULL, NULL };

  setup_streams(&streams, gen);

  memset(&visitor, 0, sizeof(visitor));

  assert(pstate->sources);
  sources[0] = pstate->sources->path->name;
  visitor.sources = sources;

  visitor.visit = IDL_STRUCT | IDL_UNION | IDL_MEMBER | IDL_CASE | IDL_CASE_LABEL | IDL_SWITCH_TYPE_SPEC | IDL_INHERIT_SPEC | IDL_TYPEDEF | IDL_MODULE;
  visitor.accept[IDL_ACCEPT_STRUCT] = &process_struct;
  visitor.accept[IDL_ACCEPT_UNION] = &process_union;
  visitor.accept[IDL_ACCEPT_MEMBER] = &process_member;
  visitor.accept[IDL_ACCEPT_CASE] = &process_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = &process_case_label;
  visitor.accept[IDL_ACCEPT_SWITCH_TYPE_SPEC] = &process_switch_type_spec;
  visitor.accept[IDL_ACCEPT_INHERIT_SPEC] = &process_inherit_spec;
  visitor.accept[IDL_ACCEPT_TYPEDEF] = &process_typedef;
  visitor.accept[IDL_ACCEPT_MODULE] = &process_module;

  idl_retcode_t ret = IDL_RETCODE_OK;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, &streams)) == IDL_RETCODE_OK)
    ret = flush(gen, &streams);

  cleanup_streams(&streams);

  return ret;
}
