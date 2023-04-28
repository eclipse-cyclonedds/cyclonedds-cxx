// Copyright(c) 2020 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <string.h>

#include "idl/stream.h"
#include "idl/processor.h"
#include "idl/print.h"

#include "generator.h"

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

  if (is_nested(node))
    return IDL_RETCODE_OK;

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
write_blob(
  FILE *handle,
  const unsigned char *blob,
  size_t blob_sz)
{
  static const char *byte_fmt = " 0x%1$02x, ";

  for (size_t sz = 0; sz < blob_sz; sz++) {
    if (idl_fprintf(handle, byte_fmt, blob[sz]) < 0 ||
        (sz % 16 == 15 && idl_fprintf(handle, "\n") < 0)) {
      return IDL_RETCODE_NO_MEMORY;
    }
  }

  return IDL_RETCODE_OK;
}


static bool
emit_isKeyless(
  const idl_pstate_t* pstate,
  const void* node)
{
  if (!idl_is_struct(node)) {
    return idl_is_keyless(node, pstate->config.flags & IDL_FLAG_KEYLIST);
  } else {
    const idl_struct_t * _struct = node;
    while (_struct->inherit_spec)
      _struct = _struct->inherit_spec->base;
    return idl_is_keyless(_struct, pstate->config.flags & IDL_FLAG_KEYLIST);
  }
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

  if (is_nested(node))
    return IDL_RETCODE_OK;

  struct generator *gen = user_data;
  char *name = NULL;
  static const char *fmt =
    "template <> constexpr const char* TopicTraits<%1$s>::getTypeName()\n"
    "{\n"
    "  return \"%2$s\";\n" /* skip preceeding "::" according to convention */
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
  static const char *datarepsfmt =
    "template <> constexpr allowable_encodings_t TopicTraits<%1$s>::allowableEncodings()\n"
    "{\n"
    "  return 0x%2$Xu;\n"
    "}\n\n";
  static const char *extensibilityfmt =
    "template <> constexpr extensibility TopicTraits<%1$s>::getExtensibility()\n"
    "{\n"
    "  return extensibility::ext_%2$s;\n"
    "}\n\n";
  static const char *type_info_decl1 =
    "#ifdef DDSCXX_HAS_TYPE_DISCOVERY\n"
    "template<> constexpr unsigned int TopicTraits<%1$s>::type_map_blob_sz() { return %2$u; }\n"
    "template<> constexpr unsigned int TopicTraits<%1$s>::type_info_blob_sz() { return %3$u; }\n"
    "template<> inline const uint8_t * TopicTraits<%1$s>::type_map_blob() {\n"
    "  static const uint8_t blob[] = {\n";
  static const char *type_info_decl2 =
    "};\n"
    "  return blob;\n"
    "}\n"
    "template<> inline const uint8_t * TopicTraits<%1$s>::type_info_blob() {\n"
    "  static const uint8_t blob[] = {\n";
  static const char *type_info_decl3 =
    "};\n"
    "  return blob;\n"
    "}\n"
    "#endif //DDSCXX_HAS_TYPE_DISCOVERY\n\n";

  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, gen) < 0 ||
      idl_fprintf(gen->header.handle, fmt, name, name+2) < 0)
    return IDL_RETCODE_NO_MEMORY;

  allowable_data_representations_t reps = idl_allowable_data_representations(node);
  if (idl_requires_xcdr2(node))
    reps &= (allowable_data_representations_t)~IDL_DATAREPRESENTATION_FLAG_XCDR1;

  if (reps != 0xFFFFFFFF &&
      idl_fprintf(gen->header.handle, datarepsfmt, name, reps) < 0)
      return IDL_RETCODE_NO_MEMORY;

  if (!is_selfcontained(node) &&
      idl_fprintf(gen->header.handle, selfcontainedfmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  if (emit_isKeyless(pstate, node) &&
      idl_fprintf(gen->header.handle, keylessfmt, name) < 0)
    return IDL_RETCODE_NO_MEMORY;

  idl_extensibility_t ext = get_extensibility(node);
  if (ext != IDL_FINAL &&
      idl_fprintf(gen->header.handle, extensibilityfmt, name, ext == IDL_APPENDABLE ? "appendable" : "mutable") < 0)
    return IDL_RETCODE_NO_MEMORY;

  idl_retcode_t ret = IDL_RETCODE_OK;
  idl_typeinfo_typemap_t blobs;
  if (gen->config && gen->config->generate_typeinfo_typemap && gen->config->generate_type_info) {
    if (gen->config->generate_typeinfo_typemap(pstate, (const idl_node_t*)node, &blobs))
      ret = IDL_RETCODE_NO_MEMORY;
    else
    {
      if (idl_fprintf(gen->header.handle, type_info_decl1, name, blobs.typemap_size, blobs.typeinfo_size) < 0 ||
        write_blob(gen->header.handle, blobs.typemap, blobs.typemap_size) ||
        idl_fprintf(gen->header.handle, type_info_decl2, name) < 0 ||
        write_blob(gen->header.handle, blobs.typeinfo, blobs.typeinfo_size) ||
        idl_fprintf(gen->header.handle, "%s", type_info_decl3) < 0)
      {
        ret = IDL_RETCODE_NO_MEMORY;
      }
      free (blobs.typemap);
      free (blobs.typeinfo);
    }
  }

  return ret;
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
  static const char *fmt = "REGISTER_TOPIC_TYPE(%s)\n";

  (void)pstate;
  (void)revisit;
  (void)path;

  if (is_nested(node))
    return IDL_RETCODE_OK;

  if (IDL_PRINTA(&name, get_cpp11_fully_scoped_name, node, gen) < 0)
    return IDL_RETCODE_NO_MEMORY;
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
        "#include \"org/eclipse/cyclonedds/topic/datatopic.hpp\"\n\n"
        "namespace org {\n"
        "namespace eclipse {\n"
        "namespace cyclonedds {\n"
        "namespace topic {\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_STRUCT | IDL_UNION;
  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_traits;
  visitor.accept[IDL_ACCEPT_UNION] = &emit_traits;
  assert(pstate->sources);
  sources[0] = pstate->sources->path->name;
  visitor.sources = sources;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (idl_fprintf(generator->header.handle,
        "} //namespace topic\n"
        "} //namespace cyclonedds\n"
        "} //namespace eclipse\n"
        "} //namespace org\n\n"
        "namespace dds {\n"
        "namespace topic {\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_topic_type_name;
  visitor.accept[IDL_ACCEPT_UNION] = &emit_topic_type_name;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (idl_fprintf(generator->header.handle, "}\n}\n\n") < 0)
    return IDL_RETCODE_NO_MEMORY;

  visitor.accept[IDL_ACCEPT_STRUCT] = &emit_register_topic_type;
  visitor.accept[IDL_ACCEPT_UNION] = &emit_register_topic_type;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  if (fputs("\n", generator->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}
