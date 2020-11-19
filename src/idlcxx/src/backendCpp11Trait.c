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

#include "idlcxx/backend.h"
#include "idlcxx/backendCpp11Utils.h"
#include "idl/export.h"

static idl_retcode_t
find_key_fields(idl_backend_ctx ctx, const idl_node_t *node)
{
  bool *has_no_keys = idl_get_custom_context(ctx);

  assert(node->mask & IDL_MEMBER);
  if (node->mask & IDL_KEY) *has_no_keys = false;
  return IDL_RETCODE_OK;
}

static bool
struct_has_no_keys(idl_backend_ctx ctx, const idl_node_t *node)
{
  const idl_struct_t *struct_node = (const idl_struct_t *) node;
  bool has_no_keys = true;
  void *prev_context = idl_get_custom_context(ctx);

  assert(node->mask & IDL_STRUCT);

  idl_reset_custom_context(ctx);
  idl_set_custom_context(ctx, &has_no_keys);
  idl_walk_node_list(ctx, (const idl_node_t *) struct_node->members, find_key_fields, IDL_MEMBER);
  idl_reset_custom_context(ctx);
  idl_set_custom_context(ctx, prev_context);
  return has_no_keys;
}

static idl_retcode_t
generate_traits(idl_backend_ctx ctx, const idl_node_t *node)
{
  assert(node->mask & IDL_STRUCT);

  char *struct_name = get_cpp11_fully_scoped_name(node);
  bool is_keyless = struct_has_no_keys(ctx, node);

  idl_file_out_printf(ctx, "template <>\n");
  idl_file_out_printf(ctx, "class TopicTraits<%s>\n", struct_name);
  idl_file_out_printf(ctx, "{\n");
  idl_file_out_printf(ctx, "public:\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "static ::org::eclipse::cyclonedds::topic::DataRepresentationId_t getDataRepresentationId()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return ::org::eclipse::cyclonedds::topic::OSPL_REPRESENTATION;\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static ::std::vector<uint8_t> getMetaData()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return ::std::vector<uint8_t>();\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static ::std::vector<uint8_t> getTypeHash()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return ::std::vector<uint8_t>();\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static ::std::vector<uint8_t> getExtentions()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return ::std::vector<uint8_t>();\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static bool isKeyless()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return %s;\n", is_keyless ? "true" : "false");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static const char *getTypeName()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return \"%s\";\n", &struct_name[2] /* Skip preceeding "::" according to convention. */);
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static ddsi_sertopic *getSerTopic(const std::string& topic_name)\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "auto *st = new ddscxx_sertopic<%s>(topic_name.c_str(), getTypeName());\n", struct_name);
  idl_file_out_printf(ctx, "return static_cast<ddsi_sertopic*>(st);\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n\n");
  idl_file_out_printf(ctx, "static size_t getSampleSize()\n");
  idl_file_out_printf(ctx, "{\n");
  idl_indent_incr(ctx);
  idl_file_out_printf(ctx, "return sizeof(%s);\n", struct_name);
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "}\n");
  idl_indent_decr(ctx);
  idl_file_out_printf(ctx, "};\n");
  free(struct_name);

  return IDL_RETCODE_OK;
}

static idl_retcode_t
generate_macro_call(idl_backend_ctx ctx, const idl_node_t *node)
{
  if (node->mask & IDL_STRUCT)
  {
    char *struct_name = get_cpp11_fully_scoped_name(node);
    idl_file_out_printf(ctx, "REGISTER_TOPIC_TYPE(%s)\n", struct_name);
    free(struct_name);
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
find_topic_types(idl_backend_ctx ctx, const idl_node_t *node)
{
  idl_walkAction *action = idl_get_walk_function(ctx);

  switch (node->mask & (IDL_MODULE | IDL_STRUCT))
  {
  case IDL_MODULE:
    idl_walk_node_list(ctx, ((const idl_module_t *) node)->definitions, find_topic_types, IDL_MODULE | IDL_STRUCT);
    break;
  case IDL_STRUCT:
    action(ctx, node);
    break;
  default:
    break;
  }
  return IDL_RETCODE_OK;
}

idl_retcode_t
idl_backendGenerateTrait(idl_backend_ctx ctx, const idl_tree_t *parse_tree)
{
  idl_retcode_t result;

  idl_file_out_printf(ctx, "#include \"dds/dds.hpp\"\n");
  idl_file_out_printf(ctx, "#include \"org/eclipse/cyclonedds/topic/TopicTraits.hpp\"\n");
  idl_file_out_printf(ctx, "#include \"org/eclipse/cyclonedds/topic/DataRepresentation.hpp\"\n");
  idl_file_out_printf(ctx, "#include \"org/eclipse/cyclonedds/topic/datatopic.hpp\"\n\n");
  idl_file_out_printf(ctx, "namespace org { namespace eclipse { namespace cyclonedds { namespace topic {\n");
  idl_set_walk_function(ctx, generate_traits);
  result = idl_walk_node_list(ctx, parse_tree->root, find_topic_types, IDL_STRUCT | IDL_MODULE);
  idl_reset_walk_function(ctx);
  idl_set_walk_function(ctx, generate_macro_call);
  idl_file_out_printf(ctx, "}}}}\n\n");
  result = idl_walk_node_list(ctx, parse_tree->root, find_topic_types, IDL_MODULE | IDL_STRUCT);
  idl_reset_walk_function(ctx);
  idl_file_out_printf(ctx, "\n");

  return result;
}

