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

static int get_array_accessor(char* str, size_t size, const void* node, void* user_data)
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

static int get_sequence_member_accessor(char* str, size_t size, const void* node, void* user_data)
{
  (void)node;
  sequence_holder_t* sh = (sequence_holder_t*)user_data;
  const char *fmt = "%1$s[i_%2$u]";
  return idl_snprintf(str, size, fmt, sh->sequence_accessor, (uint32_t)sh->depth);
}

enum instance_mask {
  TYPEDEF           = 0x1 << 0,
  UNION_BRANCH      = 0x1 << 1,
  SEQUENCE          = 0x1 << 2,
  NORMAL_INSTANCE   = 0x1 << 3,
  KEY_INSTANCE      = 0x1 << 4
};

struct instance_location {
  char *parent;
  int type;
};
typedef struct instance_location instance_location_t;

static int get_instance_accessor(char* str, size_t size, const void* node, void* user_data)
{
  instance_location_t loc = *(instance_location_t *)user_data;
  if (loc.type & TYPEDEF) {
    return idl_snprintf(str, size, "%s", loc.parent);
  } else {
    const idl_declarator_t* decl = (const idl_declarator_t*)node;
    const char* name = get_cpp11_name(decl);
    return idl_snprintf(str, size, "%s.%s()", loc.parent, name);
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
  size_t keys;
  bool key_max_sz_unlimited;
  bool max_sz_unlimited;
  bool swapped;
};

static void setup_streams(struct streams* str, struct generator* gen)
{
  assert(str);
  memset(str, 0, sizeof(struct streams));
  str->generator = gen;
}

static void cleanup_streams(struct streams* str)
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

  streams->max_sz_unlimited = false;
  streams->key_max_sz_unlimited = false;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
write_string_streaming_functions(
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const char* accessor,
  const char* read_accessor,
  instance_location_t loc)
{
  uint32_t maximum = ((const idl_string_t*)type_spec)->maximum;

  const char* fmt = "  %2$s_string(streamer, %1$s, %3$u);\n";
  if (streams->swapped)
    fmt = "  %2$s_string_swapped(streamer, %1$s, %3$u);\n";

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->write, fmt, accessor, "write", maximum)
    || putf(&streams->read, fmt, read_accessor, "read", maximum)
    || putf(&streams->move, fmt, accessor, "move", maximum)
    || putf(&streams->max, fmt, accessor, "max", maximum)))
    return IDL_RETCODE_NO_MEMORY;

  if (maximum == 0) {
    if (loc.type & NORMAL_INSTANCE)
      streams->max_sz_unlimited = true;
    if (loc.type & KEY_INSTANCE)
      streams->key_max_sz_unlimited = true;
  }

  if (!(loc.type & KEY_INSTANCE))
    return IDL_RETCODE_OK;

  streams->keys++;

  if (putf(&streams->key_write, fmt, accessor, "write", maximum)
   || putf(&streams->key_read, fmt, read_accessor, "read", maximum)
   || putf(&streams->key_move, fmt, accessor, "move", maximum)
   || putf(&streams->key_max, fmt, accessor, "max", maximum))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
write_typedef_streaming_functions(
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const char* accessor,
  const char* read_accessor,
  instance_location_t loc)
{
  const char* fmt = "  %2$s_%3$s(streamer, %1$s);\n";
  if (streams->swapped)
    fmt = "  %2$s_%3$s_swapped(streamer, %1$s);\n";

  char* name = NULL;
  if (IDL_PRINTA(&name, get_cpp11_name_typedef, type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->write, fmt, accessor, "write", name)
    || putf(&streams->read, fmt, read_accessor, "read", name)
    || putf(&streams->move, fmt, accessor, "move", name)
    || putf(&streams->max, fmt, accessor, "max", name)))
    return IDL_RETCODE_NO_MEMORY;

  if (!(loc.type & KEY_INSTANCE))
    return IDL_RETCODE_OK;

  streams->keys++;

  if (putf(&streams->key_write, fmt, accessor, "key_write", name)
   || putf(&streams->key_read, fmt, read_accessor, "key_read", name)
   || putf(&streams->key_move, fmt, accessor, "key_move", name)
   || putf(&streams->key_max, fmt, accessor, "key_max", name))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
write_basic_type_streaming_functions(
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const char* accessor,
  const char* read_accessor,
  instance_location_t loc)
{
  const char* fmt = "  %2$s(streamer, %1$s);\n";
  if (streams->swapped)
    fmt = "  %2$s_swapped(streamer, %1$s);\n";

  const char* read_fmt = fmt;
  if ((idl_type(type_spec) == IDL_BOOL) &&
      (loc.type & SEQUENCE)) {
    read_fmt =
      "  {\n"
      "    bool b(false);\n"
      "    %2$s(streamer, b);\n"
      "    %1$s = b;\n"
      "  }\n";

    fmt =
        "  {\n"
        "    const bool b = %1$s;\n"
        "    %2$s(streamer, b);\n"
        "  }\n";
  }

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->write, fmt, accessor, "write")
    || putf(&streams->read, read_fmt, read_accessor, "read")
    || putf(&streams->move, fmt, accessor, "move")
    || putf(&streams->max, fmt, accessor, "max")))
    return IDL_RETCODE_NO_MEMORY;

  if (!(loc.type & KEY_INSTANCE))
    return IDL_RETCODE_OK;

  streams->keys++;

  if (putf(&streams->key_write, fmt, accessor, "write")
   || putf(&streams->key_read, read_fmt, read_accessor, "read")
   || putf(&streams->key_move, fmt, accessor, "move")
   || putf(&streams->key_max, fmt, accessor, "max"))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
write_constructed_type_streaming_functions(
  const idl_pstate_t* pstate,
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const char* accessor,
  const char* read_accessor,
  instance_location_t loc)
{
  const char* fmt = "  %2$s(streamer, %1$s);\n";
  if (streams->swapped)
    fmt = "  %2$s_swapped(streamer, %1$s);\n";

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->write, fmt, accessor, "write")
    || putf(&streams->read, fmt, read_accessor, "read")
    || putf(&streams->move, fmt, accessor, "move")
    || putf(&streams->max, fmt, accessor, "max")))
    return IDL_RETCODE_NO_MEMORY;

  if (!(loc.type & KEY_INSTANCE))
    return IDL_RETCODE_OK;

  streams->keys++;

  if (idl_is_constr_type(type_spec) &&
      !idl_is_keyless(type_spec, pstate->flags & IDL_FLAG_KEYLIST))
  {
    fmt = "  key_%2$s(streamer, %1$s);\n";
    if (streams->swapped)
      fmt = "  key_%2$s_swapped(streamer, %1$s);\n";
  }

  if (putf(&streams->key_write, fmt, accessor, "write")
   || putf(&streams->key_read, fmt, read_accessor, "read")
   || putf(&streams->key_move, fmt, accessor, "move")
   || putf(&streams->key_max, fmt, accessor, "max"))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
write_streaming_functions(
  const idl_pstate_t* pstate,
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const char* accessor,
  const char* read_accessor,
  instance_location_t loc)
{
  if (idl_is_alias(type_spec))
    return write_typedef_streaming_functions(streams, type_spec, accessor, read_accessor, loc);
  else if (idl_is_string(type_spec))
    return write_string_streaming_functions(streams, type_spec, accessor, read_accessor, loc);
  else if (idl_is_struct(type_spec) || idl_is_union(type_spec))
    return write_constructed_type_streaming_functions(pstate, streams, type_spec, accessor, read_accessor, loc);
  else
    return write_basic_type_streaming_functions(streams, type_spec, accessor, read_accessor, loc);
}

static idl_retcode_t
insert_sequence_primitives_copy(
  struct streams *streams,
  const char *accessor,
  const char *read_accessor,
  instance_location_t loc,
  size_t depth,
  size_t max)
{
  const char *fmt = "  %1$s_many(streamer, %2$s.data(), se_%3$u);\n";
  const char *mfmt = "  %1$s_many(streamer, %2$s.data(), %3$u);\n";
  if (streams->swapped)
  {
    fmt = "  %1$s_many_swapped(streamer, %2$s.data(), se_%3$u);\n";
    mfmt = "  %1$s_many_swapped(streamer, %2$s.data(), %3$u);\n";
  }

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->write, fmt, "write", accessor, depth)
    || putf(&streams->read, fmt, "read", read_accessor, depth)
    || putf(&streams->move, fmt, "move", accessor, depth)
    || putf(&streams->max, mfmt, "max", accessor, max)))
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & KEY_INSTANCE) &&
      (putf(&streams->key_write, fmt, "write", accessor, depth)
    || putf(&streams->key_read, fmt, "read", read_accessor, depth)
    || putf(&streams->key_move, fmt, "move", accessor, depth)
    || putf(&streams->key_max, mfmt, "max", accessor, max)))
    return IDL_RETCODE_NO_MEMORY;

  fmt = "  }\n";

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->read, fmt)
    || putf(&streams->write, fmt)
    || putf(&streams->move, fmt)
    || putf(&streams->max, fmt)))
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & KEY_INSTANCE) &&
      (putf(&streams->key_read, fmt)
    || putf(&streams->key_write, fmt)
    || putf(&streams->key_move, fmt)
    || putf(&streams->key_max, fmt)))
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
  instance_location_t loc)
{
  uint32_t maximum = seq->maximum;

  if (maximum == 0) {
    if (loc.type & NORMAL_INSTANCE)
      streams->max_sz_unlimited = true;
    if (loc.type & KEY_INSTANCE)
      streams->key_max_sz_unlimited = true;
  }

  const char* wfmt = maximum ? "  {\n"\
                               "  uint32_t se_%1$u = uint32_t(%2$s.size());\n"\
                               "  if (se_%1$u > %4$u &&\n"
                               "      streamer.status(serialization_status::%3$s_bound_exceeded))\n"
                               "        return;\n"\
                               "  %3$s(streamer, se_%1$u);\n"
                             : "  {\n"\
                               "  uint32_t se_%1$u = uint32_t(%2$s.size());\n"\
                               "  %3$s(streamer, se_%1$u);\n";
  const char* rfmt = maximum ? "  {\n"\
                               "  uint32_t se_%1$u = 0;\n"\
                               "  read(streamer, se_%1$u);\n"\
                               "  if (se_%1$u > %3$u &&\n"
                               "      streamer.status(serialization_status::read_bound_exceeded))\n"
                               "        return;\n"\
                               "  %2$s.resize(se_%1$u);\n"
                             : "  {\n"\
                               "  uint32_t se_%1$u = 0;\n"\
                               "  read(streamer, se_%1$u);\n"\
                               "  %2$s.resize(se_%1$u);\n";
  const char* mfmt = "  {\n"\
                     "  max(streamer, uint32_t(0));\n";
  if (streams->swapped)
  {
    wfmt = maximum ?  "  {\n"\
                      "  uint32_t se_%1$u = uint32_t(%2$s.size());\n"\
                      "  if (se_%1$u > %4$u &&\n"
                      "      streamer.status(serialization_status::%3$s_bound_exceeded))\n"
                      "        return;\n"\
                      "  %3$s_swapped(streamer, se_%1$u);\n"
                    : "  {\n"\
                      "  uint32_t se_%1$u = uint32_t(%2$s.size());\n"\
                      "  %3$s_swapped(streamer, se_%1$u);\n";
    rfmt = maximum ?  "  {\n"\
                      "  uint32_t se_%1$u = 0;\n"\
                      "  read_swapped(streamer, se_%1$u);\n"\
                      "  if (se_%1$u > %3$u &&\n"
                      "      streamer.status(serialization_status::read_bound_exceeded))\n"
                      "        return;\n"\
                      "  %2$s.resize(se_%1$u);\n"
                    : "  {\n"\
                      "  uint32_t se_%1$u = 0;\n"\
                      "  read_swapped(streamer, se_%1$u);\n"\
                      "  %2$s.resize(se_%1$u);\n";
    mfmt = "  {\n"\
           "  max_swapped(streamer, uint32_t(0));\n";
  }

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->read, rfmt, depth, read_accessor, maximum)
    || putf(&streams->write, wfmt, depth, accessor, "write", maximum)
    || putf(&streams->move, wfmt, depth, accessor, "move", maximum)
    || putf(&streams->max, mfmt)))
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & KEY_INSTANCE) &&
      (putf(&streams->key_read, rfmt, depth, read_accessor, maximum)
    || putf(&streams->key_write, wfmt, depth, accessor, "write", maximum)
    || putf(&streams->key_move, wfmt, depth, accessor, "move", maximum)
    || putf(&streams->key_max, mfmt)))
    return IDL_RETCODE_NO_MEMORY;

  if (!idl_is_sequence(seq->type_spec)
   && idl_is_base_type(seq->type_spec)
   && !(idl_type(seq->type_spec) == IDL_BOOL))
    return insert_sequence_primitives_copy(streams, accessor, read_accessor, loc, depth, maximum);

  mfmt = "  for (uint32_t i_%1$u = 0; i_%1$u < %2$u; i_%1$u++) {\n";
  const char *fmt = "  for (uint32_t i_%1$u = 0; i_%1$u < se_%1$u; i_%1$u++) {\n";

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->read, fmt, depth)
    || putf(&streams->write, fmt, depth)
    || putf(&streams->move, fmt, depth)
    || putf(&streams->max, mfmt, depth, maximum)))
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & KEY_INSTANCE) &&
      (putf(&streams->key_read, fmt, depth)
    || putf(&streams->key_write, fmt, depth)
    || putf(&streams->key_move, fmt, depth)
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

  loc.type |= SEQUENCE;
  idl_retcode_t ret = IDL_RETCODE_OK;
  if (idl_is_sequence(seq->type_spec))
    ret = unroll_sequence (pstate, streams, (idl_sequence_t*)seq->type_spec, depth + 1, new_accessor, new_read_accessor, loc);
  else
    ret = write_streaming_functions (pstate, streams, seq->type_spec, new_accessor, new_read_accessor, loc);

  if (ret != IDL_RETCODE_OK)
    return ret;

  //close sequence
  wfmt = "  } //i_%1$u\n  }\n";

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->read, wfmt, depth)
    || putf(&streams->write, wfmt, depth)
    || putf(&streams->move, wfmt, depth)
    || putf(&streams->max, wfmt, depth)))
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & KEY_INSTANCE) &&
      (putf(&streams->key_read, wfmt, depth)
    || putf(&streams->key_write, wfmt, depth)
    || putf(&streams->key_move, wfmt, depth)
    || putf(&streams->key_max, wfmt, depth)))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
unroll_array(
  struct streams* streams,
  char *accessor,
  uint32_t array_depth,
  instance_location_t loc)
{
  if (array_depth) {
    const char* afmt = "  for (%1$sauto & a_%2$u:a_%3$u)\n";
    if ((loc.type & NORMAL_INSTANCE) &&
        (putf(&streams->write, afmt, "const ", array_depth+1, array_depth)
      || putf(&streams->read, afmt, "", array_depth+1, array_depth)
      || putf(&streams->move, afmt, "const ", array_depth+1, array_depth)
      || putf(&streams->max, afmt, "const ", array_depth+1, array_depth)))
      return IDL_RETCODE_NO_MEMORY;
    if ((loc.type & KEY_INSTANCE) &&
        (putf(&streams->key_write, afmt, "const ", array_depth+1, array_depth)
      || putf(&streams->key_read, afmt, "", array_depth+1, array_depth)
      || putf(&streams->key_move, afmt, "const ", array_depth+1, array_depth)
      || putf(&streams->key_max, afmt, "const ", array_depth+1, array_depth)))
      return IDL_RETCODE_NO_MEMORY;
  } else {
    const char* afmt = "  for (%1$sauto & a_%2$u:%3$s)\n";
    if ((loc.type & NORMAL_INSTANCE) &&
        (putf(&streams->write, afmt, "const ", array_depth+1, accessor)
      || putf(&streams->read, afmt, "", array_depth+1, accessor)
      || putf(&streams->move, afmt, "const ", array_depth+1, accessor)
      || putf(&streams->max, afmt, "const ", array_depth+1, accessor)))
      return IDL_RETCODE_NO_MEMORY;
    if ((loc.type & KEY_INSTANCE) &&
        (putf(&streams->key_write, afmt, "const ", array_depth+1, accessor)
      || putf(&streams->key_read, afmt, "", array_depth+1, accessor)
      || putf(&streams->key_move, afmt, "const ", array_depth+1, accessor)
      || putf(&streams->key_max, afmt, "const ", array_depth+1, accessor)))
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
insert_array_primitives_copy(
  struct streams *streams,
  const idl_declarator_t* declarator,
  const idl_literal_t *lit,
  instance_location_t loc,
  size_t n_arr,
  char *accessor)
{
  uint32_t a_size = lit->value.uint32;

  if (n_arr && IDL_PRINTA(&accessor, get_array_accessor, declarator, &n_arr) < 0)
    return IDL_RETCODE_NO_MEMORY;

  const char *fmt = "  %1$s_many(streamer, %2$s.data(), %3$u);\n";
  if (streams->swapped)
    fmt = "  %1$s_many_swapped(streamer, %2$s.data(), %3$u);\n";

  if ((loc.type & NORMAL_INSTANCE) &&
      (putf(&streams->write, fmt, "write", accessor, a_size)
    || putf(&streams->read, fmt, "read", accessor, a_size)
    || putf(&streams->move, fmt, "move", accessor, a_size)
    || putf(&streams->max, fmt, "max", accessor, a_size)))
    return IDL_RETCODE_NO_MEMORY;

  if ((loc.type & KEY_INSTANCE) &&
      (putf(&streams->key_write, fmt, "write", accessor, a_size)
    || putf(&streams->key_read, fmt, "read", accessor, a_size)
    || putf(&streams->key_move, fmt, "move", accessor, a_size)
    || putf(&streams->key_max, fmt, "max", accessor, a_size)))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_instance(
  const idl_pstate_t *pstate,
  struct streams *streams,
  const idl_declarator_t* declarator,
  const idl_type_spec_t* type_spec,
  instance_location_t loc)
{
  assert(declarator);

  char* accessor = NULL;
  if (IDL_PRINTA(&accessor, get_instance_accessor, declarator, &loc) < 0)
    return IDL_RETCODE_NO_MEMORY;

  //unroll arrays
  if (idl_is_array(declarator)) {
    uint32_t n_arr = 0;
    const idl_literal_t* lit = (const idl_literal_t*)declarator->const_expr;
    idl_retcode_t ret = IDL_RETCODE_OK;
    while (lit) {
      const idl_literal_t* next = (const idl_literal_t*)((const idl_node_t*)lit)->next;

      if (next == NULL &&
          idl_is_base_type(type_spec)) {
        return insert_array_primitives_copy(streams, declarator, lit, loc, n_arr, accessor);
      } else if ((ret = unroll_array(streams, accessor, n_arr++, loc)) != IDL_RETCODE_OK) {
        return ret;
      }

      lit = next;
    }

    //update accessor to become "a_$n_arr$"
    if (IDL_PRINTA(&accessor, get_array_accessor, declarator, &n_arr) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  char* read_accessor;
  if (loc.type & UNION_BRANCH)
    read_accessor = "obj";
  else
    read_accessor = accessor;

  //unroll sequences (if any)
  if (idl_is_sequence(type_spec))
    return unroll_sequence(pstate, streams, (idl_sequence_t*)type_spec, 1, accessor, read_accessor, loc);
  else
    return write_streaming_functions(pstate, streams, type_spec, accessor, read_accessor, loc);
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
  instance_location_t loc = { .parent = "instance", .type = NORMAL_INSTANCE };

  (void)revisit;
  (void)path;

  type_spec = ((const idl_member_t *)node)->type_spec;
  /* only use the @key annotations when you do not use the keylist */
  if (!(pstate->flags & IDL_FLAG_KEYLIST) &&
      ((const idl_member_t *)node)->key == IDL_TRUE)
    loc.type |= KEY_INSTANCE;

  IDL_FOREACH(declarator, ((const idl_member_t *)node)->declarators) {
    if (process_instance(pstate, user_data, declarator, type_spec, loc))
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
  static instance_location_t loc = { .parent = "instance", .type = UNION_BRANCH | NORMAL_INSTANCE };

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
    if (IDL_PRINTA(&accessor, get_instance_accessor, _case->declarator, &loc) < 0 ||
        IDL_PRINTA(&type, get_cpp11_type, _case->type_spec, streams->generator) < 0 ||
        (simple && IDL_PRINTA(&value, get_cpp11_default_value, _case->type_spec, streams->generator) < 0))
      return IDL_RETCODE_NO_MEMORY;

    if (putf(&streams->read, read_start, type, value)
     || putf(&streams->max, max_start))
      return IDL_RETCODE_NO_MEMORY;

    idl_retcode_t ret = IDL_RETCODE_OK;
    if ((ret = process_instance(pstate, streams, _case->declarator, _case->type_spec, loc)) != IDL_RETCODE_OK)
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
  char *type = NULL;
  const char *fmt = "  %2$s(streamer,dynamic_cast<%1$s&>(instance));\n";
  const char *constfmt = "  %2$s(streamer,dynamic_cast<const %1$s&>(instance));\n";
  if (streams->swapped)
  {
    fmt = "  %2$s(streamer,dynamic_cast<%1$s&>(instance));\n";
    constfmt = "  %2$s(streamer,dynamic_cast<const %1$s&>(instance));\n";
  }

  (void)pstate;
  (void)revisit;
  (void)path;

  if (IDL_PRINTA(&type, get_cpp11_fully_scoped_name, type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (putf(&streams->write, constfmt, type, "write")
   || putf(&streams->read, fmt, type, "read")
   || putf(&streams->move, constfmt, type, "move")
   || putf(&streams->max, constfmt, type, "max"))
    return IDL_RETCODE_NO_MEMORY;

  if (putf(&streams->key_write, constfmt, type, "key_write")
   || putf(&streams->key_read, fmt, type, "key_read")
   || putf(&streams->key_move, constfmt, type, "key_move")
   || putf(&streams->key_max, constfmt, type, "key_max"))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static const idl_declarator_t*
resolve_member(const idl_struct_t *type_spec, const char *member_name)
{
  if (idl_is_struct(type_spec)) {
    const idl_struct_t *_struct = (const idl_struct_t *)type_spec;
    const idl_member_t *member = NULL;
    const idl_declarator_t *decl = NULL;
    IDL_FOREACH(member, _struct->members) {
      IDL_FOREACH(decl, member->declarators) {
        if (idl_strcasecmp(decl->name->identifier, member_name))
          continue;
        else
          return decl;
      }
    }
  }
  return NULL;
}

static idl_retcode_t
process_key(
  const idl_pstate_t *pstate,
  struct streams *streams,
  const idl_struct_t *_struct,
  const idl_key_t *key)
{
  const idl_type_spec_t *type_spec = _struct;
  const idl_declarator_t *decl = NULL;
  instance_location_t loc = {.parent = idl_strdup("instance"), .type = KEY_INSTANCE };
  char *tmp = NULL;
  idl_retcode_t ret = IDL_RETCODE_OK;
  for (size_t i = 0; i < key->field_name->length; i++) {
    if (!(decl = resolve_member(type_spec, key->field_name->names[i]->identifier))) {
      //this happens if the key field name points to something that does not exist, or something that cannot be resolved
      assert(0);
    }
    type_spec = ((const idl_member_t *)((const idl_node_t *)decl)->parent)->type_spec;

    //using malloc because windows causes a warning for using stack allocation (IDL_PRINTA) in a loop
    if (i < key->field_name->length - 1) {
      const char* name = get_cpp11_name(decl);
      const char *fmt = "%1$s.%2$s()";
      int res = idl_snprintf(tmp, 0, fmt, loc.parent, name);
      if (res < 0) {
        ret = IDL_RETCODE_NO_MEMORY;
        goto fail;
      }
      tmp = malloc((size_t)res+1);
      res = idl_snprintf(tmp, (size_t)res+1, fmt, loc.parent, name);
      if (res < 0) {
        ret = IDL_RETCODE_NO_MEMORY;
        goto fail;
      }
      free(loc.parent);
      loc.parent = tmp;
      tmp = NULL;
    }
  }
  ret = process_instance(pstate, streams, decl, type_spec, loc);

fail:
  if (loc.parent)
    free(loc.parent);
  if (tmp)
    free(tmp);
  return ret;
}

static idl_retcode_t
process_keylist(
  const idl_pstate_t *pstate,
  struct streams *streams,
  const idl_struct_t *_struct)
{
  const idl_key_t *key = NULL;

  idl_retcode_t ret = IDL_RETCODE_OK;
  IDL_FOREACH(key, _struct->keylist->keys) {
    streams->keys++;
    if ((ret = process_key(pstate, streams, _struct, key)) != IDL_RETCODE_OK)
      break;
  }

  return ret;
}

static idl_retcode_t
print_constructed_type_open(struct streams *streams, const idl_node_t *node)
{
  char* name = NULL;
  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;
  const char *fmt =
    "template<typename T>\n"
    "void %2$s(T& streamer, %1$s& instance)\n{\n";
  const char *constfmt =
    "template<typename T>\n"
    "void %2$s(T& streamer, const %1$s& instance)\n{\n";
  if (streams->swapped)
  {
    fmt =
      "template<typename T>\n"
      "void %2$s_swapped(T& streamer, %1$s& instance)\n{\n";
    constfmt =
      "template<typename T>\n"
      "void %2$s_swapped(T& streamer, const %1$s& instance)\n{\n";
  }

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

  if (putf(&streams->write, close_key)
   || putf(&streams->read, close_key)
   || putf(&streams->move, close_key)
   || putf(&streams->max, close_key))
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
  struct streams *streams = user_data;
  static const char *fmt =
    "template<typename T>\n"
    "void %1$smax(T& streamer, const %2$s& instance) {\n"
    "  (void)instance;\n"
    "  streamer.position(SIZE_MAX);\n"
    "}\n\n";

  if (streams->swapped)
    fmt =
      "template<typename T>\n"
      "void %1$smax_swapped(T& streamer, const %2$s& instance) {\n"
      "  (void)instance;\n"
      "  streamer.position(SIZE_MAX);\n"
      "}\n\n";

  char *fullname = NULL;
  if (IDL_PRINTA(&fullname, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (revisit) {
    if (print_constructed_type_close(user_data, node))
      return IDL_RETCODE_NO_MEMORY;

    if (streams->max_sz_unlimited) {
      streams->max.used = 0;
      if (putf(&streams->max, fmt, "", fullname))
        return IDL_RETCODE_NO_MEMORY;
    }

    if (streams->key_max_sz_unlimited) {
      streams->key_max.used = 0;
      if (putf(&streams->key_max, fmt, "key_", fullname))
        return IDL_RETCODE_NO_MEMORY;
    }

    return flush(streams->generator, streams);
  } else {
    bool keylist;
    const idl_struct_t *_struct = ((const idl_struct_t *)node);

    keylist = (pstate->flags & IDL_FLAG_KEYLIST) && _struct->keylist;

    if (print_constructed_type_open(user_data, node))
      return IDL_RETCODE_NO_MEMORY;
    if (keylist && process_keylist(pstate, user_data, _struct))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }
}

static idl_retcode_t
process_switch_type_spec(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct streams *streams = user_data;

  const char *writefmt =
    "  auto d = instance._d();\n"
    "  write(streamer, d);\n";
  const char *readfmt =
    "  auto d = instance._d();\n"
    "  read(streamer, d);\n";
  const char *movefmt =
    "  auto d = instance._d();\n"
    "  move(streamer, d);\n";
  const char *maxfmt =
    "  max(streamer, instance._d());\n"
    "  size_t union_max = streamer.position();\n"
    "  size_t alignment_max = streamer.alignment();\n";
  const char *key_maxfmt =
    "  max(streamer, instance._d());\n";

  if (streams->swapped)
  {
    writefmt =
      "  auto d = instance._d();\n"
      "  write_swapped(streamer, d);\n";
    readfmt =
      "  auto d = instance._d();\n"
      "  read_swapped(streamer, d);\n";
    movefmt =
      "  auto d = instance._d();\n"
      "  move_swapped(streamer, d);\n";
    maxfmt =
      "  max_swapped(streamer, instance._d());\n"
      "  size_t union_max = streamer.position();\n"
      "  size_t alignment_max = streamer.alignment();\n";
    key_maxfmt =
      "  max_swapped(streamer, instance._d());\n";
  }

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

  static const char *pfmt =
    "  streamer.position(union_max);\n"
    "  streamer.alignment(alignment_max);\n";
  const char *ffmt =
    "template<typename T>\n"
    "void max(T& streamer, const %1$s& instance) {\n"
    "  (void)instance;\n"
    "  streamer.position(SIZE_MAX);\n"
    "}\n\n";
  if (streams->swapped)
  {
    ffmt =
      "template<typename T>\n"
      "void max_swapped(T& streamer, const %1$s& instance) {\n"
      "  (void)instance;\n"
      "  streamer.position(SIZE_MAX);\n"
      "}\n\n";
  }

  char *fullname = NULL;
  if (IDL_PRINTA(&fullname, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (revisit) {
    // Force the discriminant value to be the actual one read from the CDR, not the
    // default value associated with the setter we used.  E.g.:
    //   union U switch (long) {
    //     case 1: case 2: long a;
    //     default: string b;
    //   };
    // setting a value with a(...) will initialize the discriminant to 1, even when
    // the intended discrimant value is 2; the same problem exists with the default
    // case, as well as when where there is no "default:" but the discrimant value
    // doesn't map to any case.
    //
    // FIXME: for the vast majority of the unions, this doesn't come into play this
    // simply wastes time.
    if (putf(&streams->read, "  instance._d(d);\n"))
      return IDL_RETCODE_NO_MEMORY;

    if (putf(&streams->max, pfmt)
     || print_constructed_type_close(user_data, node))
      return IDL_RETCODE_NO_MEMORY;

    if (streams->max_sz_unlimited) {
      streams->max.used = 0;
      if (putf(&streams->max, ffmt, fullname))
        return IDL_RETCODE_NO_MEMORY;
    }

    return flush(streams->generator, streams);
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
process_typedef_decl(
  const idl_pstate_t* pstate,
  struct streams* streams,
  const idl_type_spec_t* type_spec,
  const idl_declarator_t* declarator)
{
  instance_location_t loc = { .parent = "instance", .type = TYPEDEF | NORMAL_INSTANCE | KEY_INSTANCE };

  static const char* fmt =
    "template<typename T>\n"
    "inline void %1$s_%2$s(T& streamer, %3$s& instance)\n{\n";
  static const char* constfmt =
    "template<typename T>\n"
    "inline void %1$s_%2$s(T& streamer, const %3$s& instance)\n{\n";
  static const char* closefmt =
    "  (void)instance;\n"
    "  streamer.position(SIZE_MAX);\n"
    "}\n\n";
  if (streams->swapped)
  {
      fmt =
        "template<typename T>\n"
        "inline void %1$s_%2$s_swapped(T& streamer, %3$s& instance)\n{\n";
      constfmt =
        "template<typename T>\n"
        "inline void %1$s_%2$s_swapped(T& streamer, const %3$s& instance)\n{\n";

  }
  char* name = NULL;
  if (IDL_PRINTA(&name, get_cpp11_name_typedef, declarator, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  char* fullname = NULL;
  if (IDL_PRINTA(&fullname, get_cpp11_fully_scoped_name, declarator, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (putf(&streams->write, constfmt, "write", name, fullname)
   || putf(&streams->read, fmt, "read", name, fullname)
   || putf(&streams->move, constfmt, "move", name, fullname)
   || putf(&streams->max, constfmt, "max", name, fullname)
   || putf(&streams->key_write, constfmt, "key_write", name, fullname)
   || putf(&streams->key_read, fmt, "key_read", name, fullname)
   || putf(&streams->key_move, constfmt, "key_move", name, fullname)
   || putf(&streams->key_max, constfmt, "key_max", name, fullname))
    return IDL_RETCODE_NO_MEMORY;

  idl_retcode_t ret = process_instance(pstate, streams, declarator, type_spec, loc);
  if (ret != IDL_RETCODE_OK)
    return ret;

  if (print_constructed_type_close(streams, type_spec))
    return IDL_RETCODE_NO_MEMORY;

  if (streams->max_sz_unlimited) {
    streams->max.used = 0;
    if (putf(&streams->max, constfmt, "max", name, fullname)
     || putf(&streams->max, closefmt))
      return IDL_RETCODE_NO_MEMORY;
  }

  if (streams->key_max_sz_unlimited) {
    streams->key_max.used = 0;
    if (putf(&streams->key_max, constfmt, "key_max", name, fullname)
     || putf(&streams->key_max, closefmt))
      return IDL_RETCODE_NO_MEMORY;
  }

  return flush(streams->generator, streams);
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

  struct streams* streams = user_data;
  idl_typedef_t* td = (idl_typedef_t*)node;
  const idl_declarator_t* declarator;

  IDL_FOREACH(declarator, td->declarators) {
    idl_retcode_t ret = process_typedef_decl(pstate, streams, td->type_spec, declarator);
    if (ret != IDL_RETCODE_OK)
     return ret;
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

  if (idl_fprintf(gen->header.handle,
                  "namespace org{\n"
                  "namespace eclipse{\n"
                  "namespace cyclonedds{\n"
                  "namespace core{\n"
                  "namespace cdr{\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.visit = IDL_STRUCT | IDL_UNION | IDL_MEMBER | IDL_CASE | IDL_CASE_LABEL | IDL_SWITCH_TYPE_SPEC | IDL_INHERIT_SPEC | IDL_TYPEDEF;
  visitor.accept[IDL_ACCEPT_STRUCT] = &process_struct;
  visitor.accept[IDL_ACCEPT_UNION] = &process_union;
  visitor.accept[IDL_ACCEPT_MEMBER] = &process_member;
  visitor.accept[IDL_ACCEPT_CASE] = &process_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = &process_case_label;
  visitor.accept[IDL_ACCEPT_SWITCH_TYPE_SPEC] = &process_switch_type_spec;
  visitor.accept[IDL_ACCEPT_INHERIT_SPEC] = &process_inherit_spec;
  visitor.accept[IDL_ACCEPT_TYPEDEF] = &process_typedef;

  idl_retcode_t ret = IDL_RETCODE_OK;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, &streams)) != IDL_RETCODE_OK ||
      (ret = flush(gen, &streams)) != IDL_RETCODE_OK)
    return ret;

  streams.swapped = true;

  if ((ret = idl_visit(pstate, pstate->root, &visitor, &streams)) != IDL_RETCODE_OK ||
      (ret = flush(gen, &streams)) != IDL_RETCODE_OK)
    return ret;

  if (idl_fprintf(gen->header.handle,
                  "} //namespace cdr\n"
                  "} //namespace core\n"
                  "} //namespace cyclonedds\n"
                  "} //namespace eclipse\n"
                  "} //namespace org\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  cleanup_streams(&streams);

  return ret;
}
