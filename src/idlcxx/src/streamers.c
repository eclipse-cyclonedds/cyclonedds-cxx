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

static int get_instance_accessor(char* str, size_t size, const void* node, void* user_data)
{
  (void)user_data;
  static const char *fmt = "instance.%s()";

  const idl_declarator_t* decl = (const idl_declarator_t*)node;
  const char* name = get_cpp11_name(decl);

  return idl_snprintf(str, size, fmt, name);
}

struct streams {
  struct generator *generator;
  idl_buffer_t write;
  idl_buffer_t read;
  idl_buffer_t move;
  idl_buffer_t max;
  idl_buffer_t props;
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
  if (str->props.data)
    free(str->props.data);
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
  if (IDL_RETCODE_OK != flush_stream(&streams->props, gen->impl.handle)
   || IDL_RETCODE_OK != flush_stream(&streams->write, gen->header.handle)
   || IDL_RETCODE_OK != flush_stream(&streams->read, gen->header.handle)
   || IDL_RETCODE_OK != flush_stream(&streams->move, gen->header.handle)
   || IDL_RETCODE_OK != flush_stream(&streams->max, gen->header.handle))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

#define WRITE (1u<<0)
#define READ (1u<<1)
#define MOVE (1u<<2)
#define MAX (1u<<3)
#define PROPS (1u<<4)
#define CONST (WRITE | MOVE | MAX)
#define ALL (CONST | READ)
#define NOMAX (ALL & ~MAX)

//mapping of streaming flags and token replacements
static const char tokens[2] = {'T', 'C'};

static struct { uint32_t id; size_t O; const char *token_replacements[2]; } map[] = {
  { WRITE, offsetof(struct streams, write), {"write", "const "} },
  { READ,  offsetof(struct streams, read),  {"read",  ""} },
  { MOVE,  offsetof(struct streams, move),  {"move",  "const "} },
  { MAX,   offsetof(struct streams, max),   {"max",   "const "} },
  { PROPS, offsetof(struct streams, props), {"", ""} }
};

/* scan over string looking for {tok} */
static idl_retcode_t print_until_token(struct streams *out, uint32_t mask, const char *fmt, size_t *fmt_position)
{
  int err = 0;
  size_t start_pos = *fmt_position;
  bool end_found = false;
  while (!end_found) {
    if (fmt[*fmt_position] == '\0') {
      end_found = true;
    }
    if (fmt[*fmt_position] == '{') {
      for (size_t i = 0, n = sizeof(tokens)/sizeof(tokens[0]); i < n; i++)
        if (fmt[*fmt_position+1] == tokens[i])
          end_found = true;
    }
    if (!end_found)
      (void)(*fmt_position)++;
  }

  size_t sub_len = *fmt_position-start_pos;
  if (sub_len) {
    char *substring = NULL;
    if ((substring = malloc(sub_len+1))) {
      memcpy(substring, fmt+start_pos, sub_len);
      substring[sub_len] = '\0';

      for (uint32_t i=0, n=(sizeof(map)/sizeof(map[0])); i < n && !err; i++) {
        if (!(map[i].id & mask))
          continue;

        if (putf((idl_buffer_t*)((char*)out+map[i].O), substring))
          err = 1;
      }
      free (substring);
    } else {
      err = 1;
    }
  }

  return err ? IDL_RETCODE_NO_MEMORY : IDL_RETCODE_OK;
}

static idl_retcode_t replace_token(struct streams *out, uint32_t mask, const char *fmt, size_t *fmt_position) {
  int err = 0;

  for (size_t i = 0, ntoks = sizeof(tokens)/sizeof(tokens[0]); i < ntoks && !err; i++) {
    if (fmt[*fmt_position+1] != tokens[i])
      continue;

    for (uint32_t j=0, n=(sizeof(map)/sizeof(map[0])); j < n && !err; j++) {
      if (!(map[j].id & mask))
        continue;

      if (putf((idl_buffer_t*)((char*)out+map[j].O), map[j].token_replacements[i]))
        err = 1;
    }
  }

  *fmt_position += 3;
  return err ? IDL_RETCODE_NO_MEMORY : IDL_RETCODE_OK;
}

static idl_retcode_t multi_putf(struct streams *out, uint32_t mask, const char *fmt, ...)
{
  char *withtokens = NULL;
  size_t tlen;
  va_list ap, aq;
  int err = 0;

  va_start(ap, fmt);
  va_copy(aq, ap);
  int cnt = idl_vsnprintf(withtokens, 0, fmt, aq);
  va_end(aq);
  if (cnt >= 0) {
    tlen = (size_t)cnt;
    if (tlen != SIZE_MAX) {
      withtokens = malloc(tlen + 1u);
      if (withtokens) {
        cnt = idl_vsnprintf(withtokens, tlen + 1u, fmt, ap);
        err = ((size_t)cnt != tlen);
      } else {
        err = 1;
      }
    } else {
      err = 1;
    }
  } else {
    err = 1;
  }
  va_end(ap);

  if (!err) {
    size_t str_pos = 0;
    while (!err && withtokens[str_pos]) {
      if (print_until_token(out, mask, withtokens, &str_pos))
        err = 1;
      if (!err && withtokens[str_pos]) {
        if (replace_token(out, mask, withtokens, &str_pos))
          err = 1;
      }
    }
  }

  if (withtokens)
    free(withtokens);

  return err ? IDL_RETCODE_NO_MEMORY : IDL_RETCODE_OK;
}

static idl_retcode_t
write_streaming_functions(
  struct streams* streams,
  const char* accessor,
  bool is_optional,
  bool union_branch)
{
  static const char *union_fmt =
    "      if (!{T}(streamer, %1$s, prop, max_sizes))\n"
    "        return false;\n"
    "      props.is_present = prop.is_present;\n",
              *struct_fmt_opt =
    "      if (!streamer.start_member(prop, %1$s.has_value()) ||\n"
    "          !{T}(streamer, %1$s, prop, max_sizes) ||\n"
    "          !streamer.finish_member(prop, %1$s.has_value()))\n"
    "        return false;\n",
               *struct_fmt =
    "      if (!streamer.start_member(prop) ||\n"
    "          !{T}(streamer, %1$s, prop, max_sizes) ||\n"
    "          !streamer.finish_member(prop))\n"
    "        return false;\n";

  const char *cfmt = NULL,
             *rfmt = NULL;
  const char *read_accessor = accessor;
  if (union_branch) {
    cfmt = union_fmt;
    rfmt = cfmt;
    read_accessor = "obj";
  } else if (is_optional) {
    cfmt = struct_fmt_opt;
    rfmt = struct_fmt;
  } else {
    cfmt = struct_fmt;
    rfmt = cfmt;
  }

  if (multi_putf(streams, CONST, cfmt, accessor) ||
      multi_putf(streams, READ, rfmt, read_accessor))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_entity(
  const idl_pstate_t *pstate,
  struct streams *streams,
  const idl_type_spec_t *type_spec,
  const idl_declarator_t *declarator,
  bool union_branch)
{
  (void) pstate;

  char* accessor = NULL;
  if (IDL_PRINTA(&accessor, get_instance_accessor, declarator, accessor) < 0)
    return IDL_RETCODE_NO_MEMORY;

  static const char *max_sz_open =
    "      static const size_t max_sizes[] = {",
                    *max_sz_entry =
    "%"PRIu32", ",
                    *max_sz_close =
    "0};\n";

  if (multi_putf(streams, ALL, max_sz_open))
    return IDL_RETCODE_NO_MEMORY;

  while (idl_is_alias(type_spec) || idl_is_sequence(type_spec)) {
    if (idl_is_alias(type_spec))
      type_spec = idl_strip(type_spec, 0);
    if (idl_is_sequence(type_spec)) {
      const idl_sequence_t *seq = type_spec;
      type_spec = seq->type_spec;
      if (multi_putf(streams, ALL, max_sz_entry, seq->maximum))
        return IDL_RETCODE_NO_MEMORY;
    }
  }

  if (idl_is_string(type_spec)) {
    const idl_string_t *str = type_spec;
    if (multi_putf(streams, ALL, max_sz_entry, str->maximum))
      return IDL_RETCODE_NO_MEMORY;
  }

  if (multi_putf(streams, ALL, max_sz_close))
    return IDL_RETCODE_NO_MEMORY;

  return write_streaming_functions(streams, accessor, is_optional(declarator), union_branch);
}

static const idl_type_spec_t *
unwrap_type_spec(const idl_type_spec_t *type_spec) {
  while (idl_is_alias(type_spec) || idl_is_sequence(type_spec) || idl_is_forward(type_spec)) {
    if (idl_is_alias(type_spec))
      type_spec = idl_strip(type_spec, 0);
    else
      type_spec = idl_type_spec(type_spec);
  }
  return type_spec;
}

static idl_retcode_t
add_bitmask_props(
  const char *add_to,
  uint32_t mask,
  const idl_type_spec_t *type_spec,
  struct streams *streams)
{
  switch (get_extensibility(type_spec)) {
      case IDL_APPENDABLE:
        if (multi_putf(streams, mask, "%1$s.e_ext = extensibility::ext_appendable;\n", add_to))
          return IDL_RETCODE_NO_MEMORY;
        break;
      case IDL_MUTABLE:
        if (multi_putf(streams, mask, "%1$s.e_ext = extensibility::ext_mutable;\n", add_to))
          return IDL_RETCODE_NO_MEMORY;
        break;
      default:
        break;
  }
  if (multi_putf(streams, mask, "%1$s.is_primitive_type = false;\n", add_to))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
generate_entity_properties(
  const idl_node_t *parent,
  const idl_type_spec_t *type_spec,
  const idl_declarator_t *decl,
  struct streams *streams)
{
  type_spec = unwrap_type_spec(type_spec);

  const char *opt = is_optional(decl) ? "true" : "false";
  char *type = NULL, *p_ext = NULL;
  if (idl_is_base_type(type_spec) || idl_is_string(type_spec)) {
    if (IDL_PRINTA(&type, get_cpp11_type, type_spec, streams->generator) < 0)
      return IDL_RETCODE_NO_MEMORY;
  } else {
    if (IDL_PRINTA(&type, get_cpp11_fully_scoped_name, type_spec, streams->generator) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  switch (get_extensibility(parent)) {
    case IDL_APPENDABLE:
      p_ext = "appendable";
      break;
    case IDL_MUTABLE:
      p_ext = "mutable";
      break;
    default:
      break;
  }

  if (multi_putf(streams, PROPS, "  props.m_members_by_seq.push_back(get_type_props<%1$s>());  //::%2$s\n"
                                 "  props.m_members_by_seq.back().set_member_props(%3$"PRIu32",%4$s);\n",
                                 type, idl_identifier(decl), decl->id.value, opt) ||
      (p_ext && multi_putf(streams, PROPS, "  props.m_members_by_seq.back().p_ext = extensibility::ext_%1$s;\n", p_ext)) ||
      (idl_is_bitmask(type_spec) && add_bitmask_props("  props.m_members_by_seq.back()", PROPS, type_spec, streams)) ||
      (must_understand(type_spec) && multi_putf(streams, PROPS, "  props.m_members_by_seq.back().must_understand_local = true;\n")))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_member(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  (void)revisit;
  (void)path;

  struct streams *streams = user_data;
  const idl_member_t *mem = node;
  const idl_declarator_t *declarator = NULL;
  const idl_type_spec_t *type_spec = mem->type_spec;

  IDL_FOREACH(declarator, mem->declarators) {
    //generate case
    static const char *fmt =
      "      case %"PRIu32":\n";

    if (multi_putf(streams, ALL, fmt, declarator->id.value)
     || multi_putf(streams, ALL, "      {\n")
     || generate_entity_properties(mem->node.parent, type_spec, declarator, streams))
      return IDL_RETCODE_NO_MEMORY;

    // only use the @key annotations when you do not use the keylist
    if (!(pstate->config.flags & IDL_FLAG_KEYLIST) &&
        mem->key.value &&
        multi_putf(streams, PROPS, "  keylist.add_key_endpoint(std::list<uint32_t>{%1$"PRIu32"});\n", declarator->id.value))
      return IDL_RETCODE_NO_MEMORY;

    if (process_entity(pstate, streams, type_spec, declarator, false)
     || multi_putf(streams, ALL, "      }\n      break;\n"))
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
  (void)path;

  struct streams *streams = user_data;
  const idl_case_t* _case = (const idl_case_t*)node;
  const idl_switch_type_spec_t* _switch = ((const idl_union_t*)_case->node.parent)->switch_type_spec;
  const idl_union_t* _union = (const idl_union_t*)_case->node.parent;

  bool single = (idl_degree(_case->labels) == 1) && !(idl_mask(_case->labels) == IDL_DEFAULT_CASE_LABEL),
       simple = (idl_is_base_type(_case->type_spec) || idl_is_bitmask(_case->type_spec)) && !idl_is_array(_case->declarator);
  const idl_type_spec_t *bare_type = unwrap_type_spec(_case->type_spec);

  static const char *max_start =
    "  {\n"
    "    size_t pos = streamer.position();\n"
    "    size_t alignment = streamer.alignment();\n",
                    *max_end =
    "    if (union_max < streamer.position()) {\n"
    "      union_max = streamer.position();\n"
    "      alignment_max = streamer.alignment();\n"
    "    }\n"
    "    streamer.position(pos);\n"
    "    streamer.alignment(alignment);\n"
    "  }\n",
                    *get_props =
    "      auto prop = get_type_props<%1$s>();\n",
                    *read_end =
    "      instance.%1$s(obj%2$s);\n"
    "    }\n"
    "    break;\n",
                    *read_start =
    "    {\n"
    "      decl_ref_type(%1$s) obj%2$s;\n";

  if (revisit) {
    const char *name = get_cpp11_name(_case->declarator);

    char *accessor = NULL, *type_name = NULL;
    if (IDL_PRINTA(&type_name, get_cpp11_type, bare_type, streams->generator) < 0 ||
        IDL_PRINTA(&accessor, get_instance_accessor, _case->declarator, accessor) < 0)
      return IDL_RETCODE_NO_MEMORY;

    if (multi_putf(streams, (WRITE | MOVE), "      {\n")
     || multi_putf(streams, READ, read_start, accessor, simple ? " = 0" : "")
     || multi_putf(streams, MAX, max_start)
     || multi_putf(streams, ALL, get_props, type_name))
      return IDL_RETCODE_NO_MEMORY;

    if (idl_is_bitmask(bare_type) && add_bitmask_props("  prop", ALL, bare_type, streams))
      return IDL_RETCODE_NO_MEMORY;

    /*only read the field if the union is not read as a key stream and the switch is not the key*/
    if ((_switch->key.value && multi_putf(streams, ALL, "      if (!streamer.is_key()) {\n"))
     || process_entity(pstate, streams, _case->type_spec, _case->declarator, true)
     || (_switch->key.value && multi_putf(streams, ALL, "      } //!streamer.is_key()\n")))
      return IDL_RETCODE_NO_MEMORY;

    if (multi_putf(streams, READ, read_end, name, single ? "" : ", d")
     || multi_putf(streams, (WRITE | MOVE), "      }\n      break;\n")
     || multi_putf(streams, MAX, max_end))
      return IDL_RETCODE_NO_MEMORY;

    if (idl_next(_case)) {
      return IDL_RETCODE_OK;
    } else {
      /*if last entry, and no default case was present for this union*/
      const idl_case_label_t *def = _union->default_case;
      if (idl_is_union(def->node.parent) &&
          multi_putf(streams, READ, "    default:\n"
                                    "      instance._default(d);\n"
                                    "      props.is_present = true;\n"))
        return IDL_RETCODE_NO_MEMORY;
    }

    if (multi_putf(streams, NOMAX, "  }\n"))
      return IDL_RETCODE_NO_MEMORY;
  } else {
    if (idl_previous(_case))
      return IDL_VISIT_REVISIT;
    if (multi_putf(streams, NOMAX,  "  switch(d)\n  {\n"))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_VISIT_REVISIT;
  }

  return IDL_RETCODE_OK;
}

static const idl_declarator_t*
resolve_member(const idl_struct_t *type_spec, const char *member_name)
{
  type_spec = idl_strip(type_spec, IDL_STRIP_ALIASES | IDL_STRIP_FORWARD);

  if (idl_is_struct(type_spec)) {
    const idl_struct_t *_struct = (const idl_struct_t *)type_spec;
    const idl_member_t *member = NULL;
    const idl_declarator_t *decl = NULL;
    IDL_FOREACH(member, _struct->members) {
      IDL_FOREACH(decl, member->declarators) {
        if (0 == idl_strcasecmp(decl->name->identifier, member_name))
          return decl;
      }
    }
  }
  return NULL;
}

static idl_retcode_t
process_key(
  struct streams *streams,
  const idl_struct_t *_struct,
  const idl_key_t *key)
{
  const idl_type_spec_t *type_spec = _struct;
  const idl_declarator_t *decl = NULL;

  if (multi_putf(streams, PROPS, "    keylist.add_key_endpoint(std::list<uint32_t>{"))
    return IDL_RETCODE_NO_MEMORY;

  for (size_t i = 0; i < key->field_name->length; i++) {
    if (!(decl = resolve_member(type_spec, key->field_name->names[i]->identifier))) {
      //this happens if the key field name points to something that does not exist
      //or something that cannot be resolved, should never occur in a correctly
      //parsed idl file
      assert(0);
      return IDL_RETCODE_SEMANTIC_ERROR;
    }

    const idl_member_t *mem = (const idl_member_t *)((const idl_node_t *)decl)->parent;
    type_spec = mem->type_spec;

    if (multi_putf(streams, PROPS, "%1$"PRIu32, decl->id.value))
      return IDL_RETCODE_NO_MEMORY;
    if (i < key->field_name->length-1 &&
        multi_putf(streams, PROPS, ", ", decl->id.value))
      return IDL_RETCODE_NO_MEMORY;
  }

  if (multi_putf(streams, PROPS, "});\n"))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_keylist(
  struct streams *streams,
  const idl_struct_t *_struct)
{
  const idl_key_t *key = NULL;

  IDL_FOREACH(key, _struct->keylist->keys) {
    if (process_key(streams, _struct, key))
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_constructed_type_open(struct streams *streams, const idl_node_t *node)
{
  char* name = NULL;
  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  static const char *fmt =
    "template<typename T, std::enable_if_t<std::is_base_of<cdr_stream, T>::value, bool> = true >\n"
    "bool {T}(T& streamer, {C}%1$s& instance, entity_properties_t &props, const size_t *max_sz) {\n"
    "  (void)max_sz;\n";
  static const char *pfmt1 =
    "template<>\n"
    "entity_properties_t get_type_props<%s>()%s";
  static const char *pfmt2 =
    " {\n"
    "  static std::mutex mtx;\n"
    "  static entity_properties_t props;\n"
    "  static std::atomic_bool initialized {false};\n\n"
    "  if (initialized.load(std::memory_order_relaxed))\n"
    "    return props;\n\n"
    "  std::lock_guard<std::mutex> lock(mtx);\n"
    "  if (initialized.load(std::memory_order_relaxed))\n"
    "    return props;\n"
    "  props.clear();\n"
    "  key_endpoint keylist;\n\n";
  static const char *sfmt =
    "  if (!streamer.start_struct(props))\n"
    "    return false;\n";

  const char *estr = NULL;
  switch (get_extensibility(node)) {
    case IDL_APPENDABLE:
      estr = "ext_appendable";
      break;
    case IDL_MUTABLE:
      estr = "ext_mutable";
      break;
    default:
      break;
  }

  if (multi_putf(streams, ALL, fmt, name)
   || multi_putf(streams, PROPS, pfmt1, name, pfmt2)
   || idl_fprintf(streams->generator->header.handle, pfmt1, name, ";\n\n") < 0
   || (estr && multi_putf(streams, PROPS, "  props.e_ext = extensibility::%1$s;\n\n", estr))
   || multi_putf(streams, ALL, sfmt))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_switchbox_open(struct streams *streams)
{
  static const char *fmt =
    "  bool firstcall = true;\n"
    "  while (auto &prop = streamer.next_entity(props, firstcall)) {\n"
    "%1$s"
    "    switch (prop.m_id) {\n";
  static const char *skipfmt =
    "    if (prop.ignore) {\n"
    "      streamer.skip_entity();\n"
    "      continue;\n"
    "    }\n";

  if (multi_putf(streams, CONST, fmt, "")
   || multi_putf(streams, READ, fmt, skipfmt))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_constructed_type_close(
  struct streams *streams)
{
  static const char *fmt =
    "  return streamer.finish_struct(props);\n"
    "}\n\n";
  static const char *pfmt =
    "\n  props.m_members_by_seq.push_back(final_entry());\n\n"
    "  props.finish(keylist);\n"
    "  initialized.store(true, std::memory_order::memory_order_release);\n"
    "  return props;\n"
    "}\n\n";

  if (multi_putf(streams, ALL, fmt)
   || multi_putf(streams, PROPS, pfmt))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_switchbox_close(struct streams *streams)
{
  static const char *fmt =
    "    }\n"
    "  }\n";
  static const char *rfmt =
    "      default:\n"
    "      if (prop.must_understand_remote\n"
    "       && streamer.status(must_understand_fail))\n"
    "        return false;\n"
    "      else\n"
    "        streamer.skip_entity();\n"
    "      break;\n";

  if (multi_putf(streams, READ, rfmt)
   || multi_putf(streams, ALL, fmt))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_entry_point_functions(
  struct streams *streams,
  char *fullname)
{
  static const char *fmt =
    "template<typename S, std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S& str, {C}%1$s& instance, bool as_key) {\n"
    "  auto props = get_type_props<%1$s>();\n"
    "  str.set_mode(cdr_stream::stream_mode::{T}, as_key);\n"
    "  return {T}(str, instance, props, nullptr); \n"
    "}\n\n";

  if (multi_putf(streams, ALL, fmt, fullname))
    return IDL_RETCODE_NO_MEMORY;

return IDL_RETCODE_OK;
}

static idl_retcode_t
process_struct_contents(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const idl_struct_t *_struct,
  struct streams *streams)
{
  idl_retcode_t ret = IDL_RETCODE_OK;
  bool keylist = (pstate->config.flags & IDL_FLAG_KEYLIST) && _struct->keylist;

  size_t to_unroll = 1;
  const idl_struct_t *base = _struct;
  while (base->inherit_spec) {
    base =  (const idl_struct_t *)(base->inherit_spec->base);
    to_unroll++;
  }

  do {
    size_t depth_to_go = --to_unroll;
    base = _struct;
    while (depth_to_go--)
      base =  (const idl_struct_t *)(base->inherit_spec->base);

    if (keylist
     && (ret = process_keylist(streams, base)))
      return ret;

    const idl_member_t *member = NULL;
    IDL_FOREACH(member, base->members) {
      if ((ret = process_member(pstate, revisit, path, member, streams)))
        return ret;
    }

  } while (to_unroll);

  return ret;
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

  char *fullname = NULL;
  if (IDL_PRINTA(&fullname, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (revisit) {
    if (print_switchbox_close(user_data)
     || print_constructed_type_close(user_data)
     || (!is_nested(node) && print_entry_point_functions(streams, fullname))) /*only add entry point functions for non-nested (topic) types*/
      return IDL_RETCODE_NO_MEMORY;

    return flush(streams->generator, streams);
  } else {

    idl_retcode_t ret = IDL_RETCODE_OK;
    if ((ret = print_constructed_type_open(user_data, node))
     || (ret = print_switchbox_open(user_data))
     || (ret = process_struct_contents(pstate, revisit, path, node, streams)))
      return ret;

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
  static const char *fmt =
    "  {C}auto d = instance._d();\n"
    "  if (!{T}(streamer, d))\n"
    "    return false;\n";
  static const char *mfmt =
    "  size_t union_max = streamer.position();\n"
    "  size_t alignment_max = streamer.alignment();\n";

  struct streams *streams = user_data;

  (void)pstate;
  (void)revisit;
  (void)path;
  (void)node;

  if (multi_putf(streams, ALL, fmt)
   || multi_putf(streams, MAX, mfmt))
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

  char *fullname = NULL;
  if (IDL_PRINTA(&fullname, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (revisit) {
    if (multi_putf(streams, MAX, pfmt)
     || print_constructed_type_close(user_data)
     || (!is_nested(node) && print_entry_point_functions(streams, fullname))) /*only add entry point functions for non-nested (topic) types*/
      return IDL_RETCODE_NO_MEMORY;

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

  if (multi_putf(streams, NOMAX, casefmt, value))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
process_enum(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct streams *streams = (struct streams*)user_data;
  struct generator *gen = streams->generator;
  const idl_enum_t *_enum = (const idl_enum_t *)node;
  const idl_enumerator_t *enumerator;
  uint32_t value;
  const char *enum_name = NULL;
  idl_retcode_t ret = IDL_RETCODE_OK;
  uint32_t *already_encountered = NULL,
           n = 0;

  (void)pstate;
  (void)revisit;
  (void)path;

  char *fullname = NULL;
  if (IDL_PRINTA(&fullname, get_cpp11_fully_scoped_name, _enum, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  static const char *conv_func = "template<>\n"\
                    "%s enum_conversion<%s>(uint32_t in)%s",
                    *bb_func = "template<>\n"\
                    "constexpr bit_bound get_enum_bit_bound<%s>() { return bb_%"PRIu16"_bits; }\n\n",
                    *prop_func_open = "template<>\n"\
                    "entity_properties_t get_type_props<%s>()%s",
                    *prop_func_contents = "  entity_properties_t e;\n"
                    "  e.e_ext = extensibility::ext_%1$s;\n"
                    "  e.xtypes_necessary = %2$s;\n"
                    "  e.e_bb = get_enum_bit_bound<%3$s>();\n"
                    "  e.m_members_by_seq.push_back(final_entry());\n"
                    "  e.m_members_by_id.push_back(final_entry());\n"
                    "  e.m_keys.push_back(final_entry());\n"
                    "  return e;\n"
                    "}\n\n";

  if (multi_putf(streams, PROPS, conv_func, fullname, fullname, " {\n  switch (in) {\n")
   || idl_fprintf(gen->header.handle, conv_func, fullname, fullname, ";\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  /*count the number of enumerators*/
  IDL_FOREACH(enumerator, _enum->enumerators) {
    n++;
  }

  if (n && !(already_encountered = malloc(sizeof(uint32_t)*n)))
    return IDL_RETCODE_NO_MEMORY;

  n = 0;

  /*go over all enumerators*/
  IDL_FOREACH(enumerator, _enum->enumerators) {
    enum_name = get_cpp11_name(enumerator);
    value = enumerator->value.value;
    bool already_present = false;
    /*check for duplicates*/
    for (uint32_t i = 0; i < n && !already_present; i++) {
      if (value == already_encountered[i])
        already_present = true;
    }
    if (already_present)
      continue;

    already_encountered[n++] = value;

    if (multi_putf(streams, PROPS, "    %scase %"PRIu32":\n"
                                   "    return %s::%s;\n"
                                   "    break;\n",
                                   enumerator == _enum->default_enumerator ? "default:\n    " : "",
                                   value,
                                   fullname,
                                   enum_name) < 0) {
      ret = IDL_RETCODE_NO_MEMORY;
      break;
    }
  }

  //generate entity properties function for enums
  uint16_t bb = 32;
  if (_enum->bit_bound.annotation) {
    if (_enum->bit_bound.value > 32)
      bb = 64;
    else if (_enum->bit_bound.value > 16)
      bb = 32;
    else if (_enum->bit_bound.value > 8)
      bb = 16;
    else
      bb = 8;
  }

  char *ext = NULL;
  switch (_enum->extensibility.value) {
    case IDL_FINAL:
      ext = "final";
      break;
    case IDL_APPENDABLE:
      ext = "appendable";
      break;
    case IDL_MUTABLE:
      ext = "mutable";
      break;
  }

  if (multi_putf(streams, PROPS, "  }\n}\n\n") ||
      idl_fprintf(gen->header.handle, bb_func, fullname, bb) < 0 ||
      idl_fprintf(gen->header.handle, prop_func_open, fullname, ";\n\n") < 0 ||
      multi_putf(streams, PROPS, prop_func_open, fullname, "{\n" ) ||
      multi_putf(streams, PROPS, prop_func_contents, ext, _enum->extensibility.value == IDL_FINAL ? "false" : "true", fullname)) {
    ret = IDL_RETCODE_NO_MEMORY;
  }

  /*cleanup*/
  if (already_encountered)
    free(already_encountered);

  return ret;
}

static int make_guard(char** out, const char *in)
{
  const char *c = in;
  while (*c && *c != '<')
    c++;

  *out = malloc((size_t)(c-in+1));

  if (NULL == *out)
    return -1;

  const char *c2 = in;
  char *c3 = *out;
  while (c2 != c) {
    if (*c2 == ':')
      *c3 = '_';
    else
      *c3 = *c2;
    c2++;
    c3++;
  }
  *c3 = '\0';

  return (int)(c-in+1);
}

static int make_seq_type(char** strptr, const char *fmt, bool is_bool)
{
  int n = idl_snprintf(*strptr, 0, fmt, is_bool ? "bool" : "T", "N");
  if (n < 0)
    return n;

  *strptr = malloc((size_t)n+1);
  if (!*strptr)
    return -1;

  return idl_snprintf(*strptr, (size_t)n+1, fmt, is_bool ? "bool" : "T", "N");
}

static int make_str_type(char** strptr, const char *fmt)
{
  int n = idl_snprintf(*strptr, 0, fmt, "N");
  if (n < 0)
    return n;

  *strptr = malloc((size_t)n+1);
  if (!*strptr)
    return -1;

  return idl_snprintf(*strptr, (size_t)n+1, fmt, "N");
}

static idl_retcode_t
generate_streamer_template_linkage(struct streams *streams)
{
  assert(streams && streams->generator);
  struct generator *gen = streams->generator;
  static const char *guard_start =
    "#ifndef STREAMER_LINKAGE_%1$s\n"
    "#define STREAMER_LINKAGE_%1$s\n\n";
  static const char *guard_stop =
    "#endif //STREAMER_LINKAGE_%1$s\n\n";
  static const char *sequence_fmt =
    "template< typename S,\n"
    "          typename T,\n"
    "%1$s"  //this is the place for an optional size template parameter
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S &str, {C}%2$s& to_{T}, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  return {T}_sequence(str, to_{T}, props, max_sz);\n"
    "}\n\n";
  static const char *vector_bool_read_fmt =
    "template< typename S,\n"
    "%1$s"  //this is the place for an optional size template parameter
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool read(S &str, %2$s& to_read, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  uint32_t vec_length = 0;\n"
    "  props.is_present = false;\n"
    "  if (!str.start_consecutive(false, false) ||\n"
    "      !read(str,vec_length))\n"
    "    return false;\n"
    "  uint32_t read_length = vec_length;\n"
    "  if (max_sz && *max_sz)\n"
    "    vec_length = std::min<uint32_t>(static_cast<uint32_t>(*max_sz),vec_length);\n"
    "  to_read.resize(vec_length);\n"
    "  for (auto &&b:to_read)  //&& here due to bit_iterator not being able to be bound to a reference as it is like a reference itself\n"
    "  {\n"
    "    bool dummy = false;\n"
    "    if (!read(str, dummy))\n"
    "      return false;\n"
    "    b = dummy;\n"
    "  }\n"
    "  //dummy reads for entries beyond the sequence's maximum size\n"
    "  if (vec_length > read_length &&\n"
    "      !move(str, bool(), props, max_sz, vec_length-read_length))\n"
    "    return false;\n"
    "  props.is_present = true;\n"
    "  return str.finish_consecutive();\n"
    "}\n\n";
  static const char *vector_bool_write_fmt =
    "template< typename S,\n"
    "%1$s"  //this is the place for an optional size template parameter
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S &str, const %2$s& to_{T}, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  props.is_present = false;\n"
    "  if (!str.start_consecutive(false, false) ||\n"
    "      (max_sz && *max_sz && to_{T}.size() > *max_sz) ||\n"
    "      !{T}(str,static_cast<uint32_t>(to_{T}.size())))\n"
    "    return false;\n\n"
    "  //do {T} for entries in vector\n"
    "  for (const auto &&b:to_{T}) {\n"
    "    if (!{T}(str, bool(b)))\n"
    "      return false;\n"
    "  }\n"
    "  props.is_present = true;\n"
    "  return str.finish_consecutive();\n"
    "}\n\n";
  static const char *array_fmt =
    "template< typename S,\n"
    "          typename T,\n"
    "          size_t N,\n"
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S &str, {C}%1$s& to_{T}, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  return {T}_array(str, to_{T}, props, max_sz);\n"
    "}\n\n";
  static const char *string_stream_fmt =
    "template< typename S,\n"
    "%1$s"  //this is the place for an optional size template parameter
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S &str, {C}%2$s& to_{T}, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  return {T}_string(str, to_{T}, props, max_sz);\n"
    "}\n\n";
  static const char *string_props_fmt =
    "template<%1$s>\n"  //this is the place for an optional size template parameter
    "inline entity_properties_t get_type_props<%2$s%3$s>()\n"
    "{\n"
    "  entity_properties_t props;\n"
    "  props.is_primitive_type = false;\n"
    "  return props;\n"
    "}\n\n";
  static const char *optional_fmt =
    "template< typename S,\n"
    "          typename T,\n"
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S &str, {C}%1$s<T> &to_{T}, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  return {T}_optional(str, to_{T}, props, max_sz);\n"
    "}\n\n";
  static const char *external_fmt =
    "template< typename S,\n"
    "          typename T,\n"
    "          std::enable_if_t<std::is_base_of<cdr_stream, S>::value, bool> = true >\n"
    "bool {T}(S &str, {C}%1$s<T> &to_{T}, entity_properties_t &props, const size_t *max_sz)\n"
    "{\n"
    "  return {T}_external(str, to_{T}, props, max_sz);\n"
    "}\n\n";
  static const char *sz_templ_param =
    "          size_t N,\n";

  char *arr_guard = NULL;
  char *arr_type = NULL;
  char *seq_guard = NULL;
  char *seq_type = NULL;
  char *seq_bool_type = NULL;
  char *bnd_seq_guard = NULL;
  char *bnd_seq_type = NULL;
  char *bnd_seq_bool_type = NULL;
  char *str_guard = NULL;
  char *bnd_str_guard = NULL;
  char *bnd_str_type = NULL;
  char *opt_guard = NULL;
  char *ext_guard = NULL;

  idl_retcode_t ret = IDL_RETCODE_OK;

  if (gen->uses_array) {
    if (make_seq_type(&arr_type, gen->array_generic, false) <= 0 ||
        make_guard(&arr_guard, gen->array_format) <= 0  ||
        idl_fprintf(gen->header.handle, guard_start, arr_guard) < 0 ||
        multi_putf(streams, ALL, array_fmt, arr_type) ||
        flush(gen, streams) ||
        idl_fprintf(gen->header.handle, guard_stop, arr_guard) < 0) {
      ret = IDL_RETCODE_NO_MEMORY;
      goto err;
    }
  }

  if (gen->uses_sequence) {
    if (make_seq_type(&seq_type, gen->sequence_generic, false) <= 0 ||
        make_seq_type(&seq_bool_type, gen->sequence_generic, true) <= 0 ||
        make_guard(&seq_guard, gen->sequence_format) <= 0 ||
        idl_fprintf(gen->header.handle, guard_start, seq_guard) < 0 ||
        multi_putf(streams, ALL, sequence_fmt, "", seq_type) ||
        multi_putf(streams, WRITE|MOVE, vector_bool_write_fmt, gen->bseq_uses_size_template ? sz_templ_param : "", seq_bool_type) ||
        multi_putf(streams, READ, vector_bool_read_fmt, gen->bseq_uses_size_template ? sz_templ_param : "", seq_bool_type) ||
        flush(gen, streams) ||
        idl_fprintf(gen->header.handle, guard_stop, seq_guard) < 0) {
      ret = IDL_RETCODE_NO_MEMORY;
      goto err;
    }
  }

  if (gen->uses_bounded_sequence &&
      (!gen->uses_sequence || strcmp(gen->bounded_sequence_format, gen->sequence_format))) {
    if (make_seq_type(&bnd_seq_type, gen->bounded_sequence_generic, false) <= 0 ||
        make_seq_type(&bnd_seq_bool_type, gen->bounded_sequence_generic, true) <= 0 ||
        make_guard(&bnd_seq_guard, gen->bounded_sequence_format) <= 0 ||
        idl_fprintf(gen->header.handle, guard_start, bnd_seq_guard) < 0 ||
        multi_putf(streams, ALL, sequence_fmt, gen->bseq_uses_size_template ? sz_templ_param : "", bnd_seq_type) ||
        multi_putf(streams, WRITE|MOVE, vector_bool_write_fmt, gen->bseq_uses_size_template ? sz_templ_param : "", bnd_seq_bool_type) ||
        multi_putf(streams, READ, vector_bool_read_fmt, gen->bseq_uses_size_template ? sz_templ_param : "", bnd_seq_bool_type) ||
        flush(gen, streams) ||
        idl_fprintf(gen->header.handle, guard_stop, bnd_seq_guard) < 0) {
      ret = IDL_RETCODE_NO_MEMORY;
      goto err;
    }
  }

  if (gen->uses_string &&
      (make_str_type(&bnd_str_type, gen->bounded_string_generic) <= 0 ||
       make_guard(&str_guard, gen->string_format) <= 0 ||
       idl_fprintf(gen->header.handle, guard_start, str_guard) < 0 ||
       multi_putf(streams, WRITE, string_props_fmt, "", gen->string_format, "") ||
       multi_putf(streams, ALL, string_stream_fmt, "", gen->string_format) ||
       flush(gen, streams) ||
       idl_fprintf(gen->header.handle, guard_stop, str_guard) < 0)) {
    ret = IDL_RETCODE_NO_MEMORY;
    goto err;
  }

  if (gen->uses_bounded_string &&
      (!gen->uses_string || strcmp(gen->bounded_string_format, gen->string_format))) {
    if (make_guard(&bnd_str_guard, gen->bounded_string_format) <= 0 ||
        idl_fprintf(gen->header.handle, guard_start, bnd_str_guard) < 0 ||
        multi_putf(streams, WRITE, string_props_fmt, gen->bstr_uses_size_template ? sz_templ_param : "", bnd_str_type, gen->bstr_uses_size_template ? sz_templ_param : "") ||
        multi_putf(streams, ALL, string_stream_fmt, gen->bstr_uses_size_template ? sz_templ_param : "", bnd_str_type) ||
        flush(gen, streams) ||
        idl_fprintf(gen->header.handle, guard_stop, bnd_str_guard) < 0) {
      ret = IDL_RETCODE_NO_MEMORY;
      goto err;
    }
  }

  if (gen->uses_optional &&
      (make_guard(&opt_guard, gen->optional_format) <= 0 ||
       idl_fprintf(gen->header.handle, guard_start, opt_guard) < 0 ||
       multi_putf(streams, ALL, optional_fmt, gen->optional_format) ||
       flush(gen, streams) ||
       idl_fprintf(gen->header.handle, guard_stop, opt_guard) < 0)) {
    ret = IDL_RETCODE_NO_MEMORY;
    goto err;
  }

  if (gen->uses_external &&
      (make_guard(&ext_guard, gen->external_format) <= 0 ||
       idl_fprintf(gen->header.handle, guard_start, ext_guard) < 0 ||
       multi_putf(streams, ALL, external_fmt, gen->external_format) ||
       flush(gen, streams) ||
       idl_fprintf(gen->header.handle, guard_stop, ext_guard) < 0)) {
    ret = IDL_RETCODE_NO_MEMORY;
    goto err;
  }

  err:
  if (arr_guard)
    free(arr_guard);
  if (seq_guard)
    free(seq_guard);
  if (bnd_seq_guard)
    free(bnd_seq_guard);
  if (str_guard)
    free(str_guard);
  if (bnd_str_guard)
    free(bnd_str_guard);
  if (opt_guard)
    free(opt_guard);
  if (ext_guard)
    free(ext_guard);
  if (seq_type)
    free(seq_type);
  if (seq_bool_type)
    free(seq_bool_type);
  if (bnd_seq_type)
    free(bnd_seq_type);
  if (bnd_seq_bool_type)
    free(bnd_seq_bool_type);
  if (arr_type)
    free(arr_type);
  if (bnd_str_type)
    free(bnd_str_type);

  return ret;
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

  const char *fmt = "namespace org {\n"
                    "namespace eclipse {\n"
                    "namespace cyclonedds {\n"
                    "namespace core {\n"
                    "namespace cdr {\n\n";
  if (idl_fprintf(gen->header.handle, "%s", fmt) < 0
   || idl_fprintf(gen->impl.handle, "%s", fmt) < 0
   || generate_streamer_template_linkage(&streams))
    return IDL_RETCODE_NO_MEMORY;

  visitor.visit = IDL_STRUCT | IDL_UNION | IDL_CASE | IDL_CASE_LABEL | IDL_SWITCH_TYPE_SPEC | IDL_ENUM;
  visitor.accept[IDL_ACCEPT_STRUCT] = &process_struct;
  visitor.accept[IDL_ACCEPT_UNION] = &process_union;
  visitor.accept[IDL_ACCEPT_CASE] = &process_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = &process_case_label;
  visitor.accept[IDL_ACCEPT_SWITCH_TYPE_SPEC] = &process_switch_type_spec;
  visitor.accept[IDL_ACCEPT_ENUM] = &process_enum;

  if (idl_visit(pstate, pstate->root, &visitor, &streams)
   || flush(gen, &streams))
    return IDL_RETCODE_NO_MEMORY;

  fmt = "} //namespace cdr\n"
        "} //namespace core\n"
        "} //namespace cyclonedds\n"
        "} //namespace eclipse\n"
        "} //namespace org\n\n";
  if (idl_fprintf(gen->header.handle, "%s", fmt) < 0
   || idl_fprintf(gen->impl.handle, "%s", fmt) < 0)
    return IDL_RETCODE_NO_MEMORY;

  cleanup_streams(&streams);

  return IDL_RETCODE_OK;
}
