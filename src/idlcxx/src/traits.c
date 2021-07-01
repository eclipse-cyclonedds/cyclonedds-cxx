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
#include <string.h>

#include "idl/stream.h"
#include "idl/processor.h"
#include "idl/print.h"

#include "generator.h"

static bool sc_type_spec(const idl_type_spec_t *type_spec);

static bool sc_union(const idl_union_t *_union)
{
  if (!sc_type_spec(_union->switch_type_spec->type_spec))
    return false;

  const idl_case_t *_case = NULL;
  IDL_FOREACH(_case, _union->cases) {
    if (!sc_type_spec(_case->type_spec))
      return false;
  }

  return true;
}

static bool sc_struct(const idl_struct_t *str)
{
  const idl_member_t *mem = NULL;
  IDL_FOREACH(mem, str->members) {
    if (!sc_type_spec(mem->type_spec))
      return false;
  }

  if (str->inherit_spec)
    return sc_type_spec(str->inherit_spec->base);

  return true;
}

static bool sc_type_spec(const idl_type_spec_t *type_spec)
{
  if (idl_is_sequence(type_spec)
   || idl_is_string(type_spec)) {
    return false;
  } else if (idl_is_typedef(type_spec)) {
    return sc_type_spec(((const idl_typedef_t*)type_spec)->type_spec);
  } else if (idl_is_union(type_spec)) {
    return sc_union(type_spec);
  } else if (idl_is_struct(type_spec)) {
    return sc_struct(type_spec);
  }
  return true;
}

static idl_retcode_t
emit_topic_type_name(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  (void)pstate;
  (void)revisit;
  (void)path;

  struct generator *gen = user_data;
  char *name = NULL;
  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;

  static const char *fmt =
        "template <>\n"
        "struct topic_type_name<%1$s>\n"
        "{\n"
        "    static std::string value()\n"
        "    {\n"
        "      return org::eclipse::cyclonedds::topic::TopicTraits<%1$s>::getTypeName();\n"
        "    }\n"
        "};\n\n";

  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_traits(
  const idl_pstate_t* pstate,
  const bool revisit,
  const idl_path_t* path,
  const void* node,
  void* user_data)
{
  struct generator *gen = user_data;
  char *name = NULL;
  const char *fmt, *keyless = "true", *selfcontained = "true";
  const idl_struct_t *_struct = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  fmt = "template <>\n"
        "class TopicTraits<%1$s>\n"
        "{\n"
        "public:\n"
        "  static bool isKeyless()\n"
        "  {\n"
        "    return %3$s;\n"
        "  }\n\n"
        "  static const char *getTypeName()\n"
        "  {\n"
        "    return \"%2$s\";\n" /* skip preceeding "::" according to convention */
        "  }\n\n"
        "  static ddsi_sertype *getSerType()\n"
        "  {\n"
        "    auto *st = new ddscxx_sertype<%1$s>();\n"
        "    return static_cast<ddsi_sertype*>(st);\n"
        "  }\n\n"
        "  static size_t getSampleSize()\n"
        "  {\n"
        "    return sizeof(%1$s);\n"
        "  }\n\n"
        "  static bool isSelfContained()\n"
        "  {\n"
        "    return %4$s;\n"
        "  }\n"
        "};\n\n";
  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, _struct, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (!idl_is_keyless(node, pstate->flags & IDL_FLAG_KEYLIST))
    keyless = "false";
  if (!sc_struct(_struct))
    selfcontained = "false";
  if (idl_fprintf(gen->header.handle, fmt, name, name+2, keyless, selfcontained) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
emit_register_topic_type(
  const idl_pstate_t *pstate,
  const bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  char *name = NULL;
  const char *fmt;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  fmt = "REGISTER_TOPIC_TYPE(%s)\n";
  if (idl_fprintf(gen->header.handle, fmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

idl_retcode_t
generate_traits(const idl_pstate_t *pstate, struct generator *generator)
{
  idl_retcode_t ret;
  idl_visitor_t visitor;
  const char *sources[] = { NULL, NULL };

  if (idl_fprintf(generator->header.handle,
        "#include \"dds/topic/TopicTraits.hpp\"\n"
        "#include \"org/eclipse/cyclonedds/topic/TopicTraits.hpp\"\n"
        "#include \"org/eclipse/cyclonedds/topic/datatopic.hpp\"\n\n"
        "namespace org {\n"
        "namespace eclipse {\n"
        "namespace cyclonedds {\n"
        "namespace topic {\n"
        "/* all traits not explicitly set are defaulted to the values in TopicTraits.hpp */\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_STRUCT;
  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_traits;
  assert(pstate->sources);
  sources[0] = pstate->sources->path->name;
  visitor.sources = sources;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (idl_fprintf(generator->header.handle, "}\n}\n}\n}\n\n"
        "namespace dds {\n"
        "namespace topic {\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_topic_type_name;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (idl_fprintf(generator->header.handle, "}\n}\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_register_topic_type;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (fputs("\n", generator->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}
