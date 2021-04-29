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
  const char *fmt, *keyless = "true";
  const idl_struct_t *_struct = node;

  (void)pstate;
  (void)revisit;
  (void)path;

  fmt = "template <>\n"
        "class TopicTraits<%1$s>\n"
        "{\n"
        "public:\n"
        "  static ::org::eclipse::cyclonedds::topic::DataRepresentationId_t getDataRepresentationId()\n"
        "  {\n"
        "    return ::org::eclipse::cyclonedds::topic::OSPL_REPRESENTATION;\n"
        "  }\n\n"
        "  static ::std::vector<uint8_t> getMetaData()\n"
        "  {\n"
        "    return ::std::vector<uint8_t>();\n"
        "  }\n\n"
        "  static ::std::vector<uint8_t> getTypeHash()\n"
        "  {\n"
        "    return ::std::vector<uint8_t>();\n"
        "  }\n\n"
        "  static ::std::vector<uint8_t> getExtentions()\n"
        "  {\n"
        "    return ::std::vector<uint8_t>();\n"
        "  }\n\n"
        "  static bool isKeyless()\n"
        "  {\n"
        "    return %3$s;\n"
        "  }\n\n"
        "  static const char *getTypeName()\n"
        "  {\n"
        "    return \"%2$s\";\n" /* skip preceeding "::" according to convention */
        "  }\n\n"
        "  static ddsi_sertopic *getSerTopic(const std::string& topic_name)\n"
        "  {\n"
        "    auto *st = new ddscxx_sertopic<%1$s>(topic_name.c_str());\n"
        "    return static_cast<ddsi_sertopic*>(st);\n"
        "  }\n\n"
        "  static size_t getSampleSize()\n"
        "  {\n"
        "    return sizeof(%1$s);\n"
        "  }\n"
        "};\n";
  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, _struct, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
  if (!idl_is_keyless(node, pstate->flags & IDL_FLAG_KEYLIST))
    keyless = "false";
  if (idl_fprintf(gen->header.handle, fmt, name, name+2, keyless) < 0)
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
  const char *fmt;
  const char *sources[] = { NULL, NULL };

  fmt = "#include \"org/eclipse/cyclonedds/topic/TopicTraits.hpp\"\n"
        "#include \"org/eclipse/cyclonedds/topic/DataRepresentation.hpp\"\n\n"
        "namespace org {\n"
        "namespace eclipse {\n"
        "namespace cyclonedds {\n"
        "namespace topic {\n";
  if (idl_fprintf(generator->header.handle, fmt) < 0)
    return IDL_RETCODE_NO_MEMORY;

  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_STRUCT;
  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_traits;
  if (pstate->sources)
    sources[0] = pstate->sources->path->name;
  visitor.sources = sources;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (fputs("}\n}\n}\n}\n\n", generator->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_register_topic_type;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (fputs("\n", generator->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}
