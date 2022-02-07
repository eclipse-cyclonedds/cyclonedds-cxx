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

static bool req_xtypes(const void *node)
{
  if (idl_is_struct(node)) {
    const idl_struct_t* str = (const idl_struct_t*)node;
    if (str->inherit_spec && req_xtypes(str->inherit_spec->base))
      return true;

    if (str->extensibility.value != IDL_FINAL)
      return true;

    idl_member_t *mem = NULL;
    IDL_FOREACH(mem, str->members) {
      if (req_xtypes(mem))
        return true;
    }
  } else if (idl_is_alias(node)) {
    return req_xtypes(idl_type_spec(idl_parent(node)));
  } else if (idl_is_sequence(node)) {
    return req_xtypes(((const idl_sequence_t*)node)->type_spec);
  } else if (idl_is_union(node)) {
    const idl_union_t *un = (const idl_union_t*)node;

    if (un->extensibility.value != IDL_FINAL
     || req_xtypes(un->switch_type_spec->type_spec))
      return true;

    const idl_case_t *cs = NULL;
    IDL_FOREACH(cs, un->cases) {
      if (req_xtypes(cs->type_spec) || cs->external.value)
        return true;
    }
  } else if (idl_is_enum(node)) {
    const idl_enum_t *en = (const idl_enum_t*)node;

    return en->extensibility.value != IDL_FINAL;
  } else if (idl_is_member(node)) {
    const idl_member_t *mem = (const idl_member_t*)node;
    return  mem->optional.value ||
            mem->external.value ||
            mem->must_understand.value ||
            req_xtypes(mem->type_spec);
  }

  return false;
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
  (void)revisit;
  (void)path;

  struct generator *gen = user_data;
  char *name = NULL;
  static const char *fmt =
    "template <> constexpr const char* TopicTraits<%1$s>::getTypeName()\n"
    "{\n"
    "  return \"%2$s\";\n" /* skip preceeding "::" according to convention */
    "}\n\n"
    "template <> inline ddsi_sertype* TopicTraits<%1$s>::getSerType()\n"
    "{\n"
    "  return static_cast<ddsi_sertype*>(new ddscxx_sertype<%2$s>());\n"
    "}\n\n";
  static const char *keylessfmt =
    "template <> constexpr bool TopicTraits<%1$s>::isKeyless()\n"
    "{\n"
    "  return true;\n"
    "}\n\n";
  static const char *selfcontainedfmt =
    "template <> constexpr bool TopicTraits<%1$s>::isSelfContained()\n"
    "{\n"
    "  return false;\n"
    "}\n\n";
  static const char *mincdrversionfmt =
    "template <> constexpr encoding_version TopicTraits<%1$s>::minXCDRVersion()\n"
    "{\n"
    "  return encoding_version::xcdr_v2;\n"
    "}\n\n";
  static const char *extensibilityfmt =
    "template <> constexpr extensibility TopicTraits<%1$s>::getExtensibility()\n"
    "{\n"
    "  return extensibility::ext_%2$s;\n"
    "}\n\n";
  const idl_struct_t *_struct = node;

  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, _struct, gen) < 0 ||
      idl_fprintf(gen->header.handle, fmt, name, name+2) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (req_xtypes(_struct) &&
      idl_fprintf(gen->header.handle, mincdrversionfmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (!sc_struct(_struct) &&
      idl_fprintf(gen->header.handle, selfcontainedfmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (idl_is_keyless(node, pstate->config.flags & IDL_FLAG_KEYLIST) &&
      idl_fprintf(gen->header.handle, keylessfmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (_struct->extensibility.value != IDL_FINAL &&
      idl_fprintf(gen->header.handle, extensibilityfmt, name,
        _struct->extensibility.value == IDL_APPENDABLE ? "appendable" : "mutable") < 0)
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
        "namespace topic {\n\n") < 0)
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
