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

static idl_retcode_t
process_instance(
  const idl_pstate_t *pstate,
  struct streams *streams,
  const idl_declarator_t* declarator,
  const idl_type_spec_t* type_spec,
  bool is_key)
{
  const char *name, *fmt = "  %2$s(str, instance.%1$s());\n";

  name = get_cpp11_name(declarator);

  if (putf(&streams->write, fmt, name, "write")
   || putf(&streams->read, fmt, name, "read")
   || putf(&streams->move, fmt, name, "move")
   || putf(&streams->max, fmt, name, "max"))
    return IDL_RETCODE_NO_MEMORY;

  /* short-circuit if instance is not a key */
  if (!is_key)
    return IDL_RETCODE_OK;
  streams->keys++;

  if (idl_is_constr_type(type_spec) &&
      !idl_is_keyless(type_spec, pstate->flags & IDL_FLAG_KEYLIST))
    fmt = "  key_%2$s(str, %1$s);\n";
  if (putf(&streams->key_write, fmt, name, "write")
   || putf(&streams->key_read, fmt, name, "read")
   || putf(&streams->key_move, fmt, name, "move")
   || putf(&streams->key_max, fmt, name, "max"))
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
  const idl_declarator_t *declarator;
  const idl_type_spec_t *type_spec;
  bool is_key = false;

  (void)revisit;
  (void)path;

  type_spec = ((const idl_member_t *)node)->type_spec;
  if (!(pstate->flags & IDL_FLAG_KEYLIST))
    is_key = ((const idl_member_t *)node)->key == IDL_TRUE;

  IDL_FOREACH(declarator, ((const idl_member_t *)node)->declarators) {
    if (process_instance(pstate, user_data, declarator, type_spec, is_key))
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
  char *type = NULL;
  const char *name;

  static const char writefmt[] =
    "  write(instance.%s());\n"
    "  break;\n";
  static const char readfmt[] =
    "{\n"
    "  %1$s obj;\n"
    "  read(str, obj);\n"
    "  %2$s(obj, _disc_temp);\n"
    "}\n"
    "  break;\n";
  static const char movefmt[] =
    "  move(str, instance.%s());\n"
    "  break;\n";
  static const char maxfmt[] =
    "{\n"
    "  size_t pos = str.position();\n"
    "  size_t alignment = str.alignment();\n"
    "  max(str, instance.%s());\n"
    "  union_max = std::max(str.position(),union_max);\n"
    "  str.position(pos);\n"
    "  str.alignment(alignment);\n"
    "}\n";

  (void)pstate;
  (void)path;

  /* process case labels first */
  if (!revisit)
    return IDL_VISIT_REVISIT;

  name = get_cpp11_name(_case->declarator);
  if (IDL_PRINTA(&type, get_cpp11_type, _case->type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (putf(&streams->write, writefmt, name)
   || putf(&streams->read, readfmt, type, name)
   || putf(&streams->move, movefmt, name)
   || putf(&streams->max, maxfmt, name))
    return IDL_RETCODE_NO_MEMORY;

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
  const char *fmt = "  %2$s(dynamic_cast<%s&>(instance));\n";
  const char *constfmt = "  %2$s(dynamic_cast<const %1$s&>(instance));\n";

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

static idl_retcode_t
process_keylist(struct streams *streams, const idl_keylist_t* keylist)
{
  const idl_key_t *key;

  IDL_FOREACH(key, keylist->keys) {
    streams->keys++;

    const char *fmt = "  %2$s(str, instance.%1$s()";
    for (size_t i=0; i < key->field_name->length; i++) {
      const char *identifier = key->field_name->names[i]->identifier;
      if (putf(&streams->key_write, fmt, identifier, "write") ||
          putf(&streams->key_read, fmt, identifier, "read") ||
          putf(&streams->key_move, fmt, identifier, "move") ||
          putf(&streams->key_max, fmt, identifier, "max"))
        return IDL_RETCODE_NO_MEMORY;
      fmt = ".%1$s()";
    }
    if (putf(&streams->key_write, ");\n") < 0 ||
        putf(&streams->key_read, ");\n") < 0 ||
        putf(&streams->key_move, ");\n") < 0 ||
        putf(&streams->key_max, ");\n") < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_constructed_type_open(struct streams *streams, const idl_node_t *node)
{
  char *name = NULL;
  const char *fmt =
    "template<typename T>\n"
    "void %2$s(T& str, %1$s& instance)\n{\n";
  const char *constfmt =
    "template<typename T>\n"
    "void %2$s(T& str, const %1$s& instance)\n{\n";

  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;

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
      "}\n";
  else
    close_key =
      "  (void)str;\n"
      "  (void)instance;\n"
      "}\n";

  if (putf(&streams->write, "}\n")
   || putf(&streams->read, "}\n")
   || putf(&streams->move, "}\n")
   || putf(&streams->max, "}\n"))
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
process_switch_type_spec(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  static const char writefmt[] =
    "  write(str, %1$s);\n"
    "  switch(%1$s)\n{\n";
  static const char readfmt[] =
    "  %1$s %2$s;\n"
    "  read(str, %2$s);\n"
    "  switch(%2$s)\n{\n";
  static const char movefmt[] =
    "  move(str, %1$s);\n"
    "  switch(%1$s)\n{\n";
  static const char maxfmt[] =
    "  max(str, %1$s);\n"
    "  size_t union_max = str.position();\n";
  static const char key_writefmt[] =
    "  write(str, %1$s);\n";
  static const char key_readfmt[] =
    "  %1$s %2$s;\n"
    "  read(str, %2$s);\n";
  static const char key_movefmt[] =
    "  move(str, %1$s);\n";
  static const char key_maxfmt[] =
    "  max(str, %1$s);\n";

  struct streams *streams = user_data;
  const idl_switch_type_spec_t *switch_type_spec = node;
  const idl_type_spec_t *type_spec = switch_type_spec->type_spec;
  char *cast = NULL;
  const char *accessor = "_d()";

  (void)pstate;
  (void)path;

  if (revisit) {
    if (putf(&streams->write, "  }\n")
     || putf(&streams->read, "  }\n")
     || putf(&streams->move, "  }\n")
     || putf(&streams->max, "  str.position(union_max);\n"))
      return IDL_RETCODE_NO_MEMORY;
    return IDL_RETCODE_OK;
  }

  if (IDL_PRINTA(&cast, get_cpp11_type, type_spec, streams->generator) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (putf(&streams->write, writefmt, accessor)
   || putf(&streams->read, readfmt, cast, accessor)
   || putf(&streams->move, movefmt, accessor)
   || putf(&streams->max, maxfmt, accessor))
    return IDL_RETCODE_NO_MEMORY;

  /* short-circuit if switch type specifier is not a key */
  if (switch_type_spec->key != IDL_TRUE)
    return IDL_VISIT_REVISIT;

  if (putf(&streams->write, key_writefmt, accessor)
   || putf(&streams->read, key_readfmt, cast, accessor)
   || putf(&streams->move, key_movefmt, accessor)
   || putf(&streams->max, key_maxfmt, accessor))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_VISIT_REVISIT;
}

static idl_retcode_t
process_union(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  (void)pstate;
  (void)path;

  if (revisit) {
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
  const idl_literal_t *literal;
  char *value = "";
  const char *casefmt = "default:\n";

  (void)pstate;
  (void)revisit;
  (void)path;

  literal = ((const idl_case_label_t *)node)->const_expr;
  if (literal) {
    casefmt = "case %s:\n";
    if (IDL_PRINTA(&value, get_cpp11_value, literal, streams->generator) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  if (putf(&streams->write, casefmt, value)
   || putf(&streams->read, casefmt, value)
   || putf(&streams->move, casefmt, value))
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t flush(struct generator *gen, struct streams *streams)
{
  if (streams->write.data && fputs(streams->write.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->read.data && fputs(streams->read.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->move.data && fputs(streams->move.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->max.data && fputs(streams->max.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->key_write.data && fputs(streams->key_write.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->key_read.data && fputs(streams->key_read.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->key_move.data && fputs(streams->key_move.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (fputs("\n", gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (streams->key_max.data && fputs(streams->key_max.data, gen->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

idl_retcode_t
generate_streamers(const idl_pstate_t* pstate, struct generator *gen)
{
  idl_retcode_t ret;
  struct streams streams;
  idl_visitor_t visitor;

  memset(&streams, 0, sizeof(streams));
  memset(&visitor, 0, sizeof(visitor));

  streams.generator = gen;
  visitor.visit = IDL_STRUCT | IDL_UNION | IDL_MEMBER | IDL_CASE | IDL_CASE_LABEL | IDL_SWITCH_TYPE_SPEC | IDL_INHERIT_SPEC;
  visitor.accept[IDL_ACCEPT_STRUCT] = &process_struct;
  visitor.accept[IDL_ACCEPT_UNION] = &process_union;
  visitor.accept[IDL_ACCEPT_MEMBER] = &process_member;
  visitor.accept[IDL_ACCEPT_CASE] = &process_case;
  visitor.accept[IDL_ACCEPT_CASE_LABEL] = &process_case_label;
  visitor.accept[IDL_ACCEPT_SWITCH_TYPE_SPEC] = &process_switch_type_spec;
  visitor.accept[IDL_ACCEPT_INHERIT_SPEC] = &process_inherit_spec;
  if (pstate->sources)
    visitor.sources = (const char *[]){ pstate->sources->path->name, NULL };

  if ((ret = idl_visit(pstate, pstate->root, &visitor, &streams)) == IDL_RETCODE_OK)
    ret = flush(gen, &streams);

  if (streams.write.data)
    free(streams.write.data);
  if (streams.read.data)
    free(streams.read.data);
  if (streams.move.data)
    free(streams.move.data);
  if (streams.max.data)
    free(streams.max.data);
  if (streams.key_write.data)
    free(streams.key_write.data);
  if (streams.key_read.data)
    free(streams.key_read.data);
  if (streams.key_move.data)
    free(streams.key_move.data);
  if (streams.key_max.data)
    free(streams.key_max.data);
  if (streams.key.data)
    free(streams.key.data);

  return ret;
}
