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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "idl/processor.h"
#include "idl/tree.h"
#include "idlcxx/backend.h"
#include "idlcxx/backendCpp11Type.h"

#include "CUnit/Theory.h"

#define IDL_INPUT_STRUCT(struct_name,member_type,member_name) ""\
"struct " struct_name " {\n" \
"    " member_type " " member_name ";" \
"};"

#define IDL_INPUT_ENUM(enum_name,label1,label2,label3) "" \
"enum " enum_name " {\n" \
"  " label1 ",\n" \
"  " label2 ",\n" \
"  " label3 "\n" \
"};"

#define IDL_OUTPUT_STRUCT_PRIM(struct_name,member_type,default_value,member_name) "" \
"class " struct_name " {\n" \
"private:\n" \
"  " member_type " " member_name "_;\n" \
"\n" \
"public:\n" \
"  " struct_name "() :\n" \
"      " member_name "_(" default_value ") {}\n" \
"\n" \
"  explicit " struct_name "(\n" \
"      " member_type " " member_name ") :\n" \
"          " member_name "_(" member_name ") {}\n" \
"\n" \
"  " member_type " " member_name "() const { return this->" member_name "_; }\n" \
"  " member_type "& " member_name "() { return this->" member_name "_; }\n" \
"  void " member_name "(" member_type " _val_) { this->" member_name "_ = _val_; }\n" \
"};\n"

#define IDL_OUTPUT_STRUCT_NO_PRIM(struct_name,member_type,member_name) "" \
"class " struct_name " {\n" \
"private:\n" \
"  " member_type " " member_name "_;\n" \
"\n" \
"public:\n" \
"  " struct_name "() {}\n" \
"\n"\
"  explicit " struct_name "(\n" \
"      " member_type " " member_name ") :\n" \
"          " member_name "_(" member_name ") {}\n" \
"\n" \
"  const " member_type "& " member_name "() const { return this->" member_name "_; }\n" \
"  " member_type "& " member_name "() { return this->" member_name "_; }\n" \
"  void " member_name "(const " member_type "& _val_) { this->" member_name "_ = _val_; }\n" \
"  void " member_name "(" member_type "&& _val_) { this->" member_name "_ = _val_; }\n" \
"};\n"

#define IDL_OUTPUT_ENUM(enum_name,label1,label2,label3) "" \
"enum class " enum_name " {\n" \
"  " label1 ",\n" \
"  " label2 ",\n" \
"  " label3 ",\n" \
"};"

#define INITIAL_RUN 0
#if INITIAL_RUN
#if _WIN32
#include <Windows.h>
#define wait_a_bit(seconds) Sleep((seconds) * 1000);
#else
#include <unistd.h>
#define wait_a_bit(seconds) sleep(seconds)
#endif
static bool initial_run = true;
#endif

static void
test_base_type(const char *input, uint32_t flags, int32_t retcode, const char *output)
{
  int32_t ret;
  idl_tree_t *tree;
  idl_node_t *node;
  idl_backend_ctx ctx;
  bool expected_output;
  const char *mem_buf;

  ret = idl_parse_string(input, flags, &tree);
  CU_ASSERT(ret == retcode);
  if (ret != 0)
    return;
  node = tree->root;
  CU_ASSERT_PTR_NOT_NULL(node);
  if (!node)
    return;
  ctx = idl_backend_context_new(2, NULL);
  CU_ASSERT_PTR_NOT_NULL(ctx);
  ret = idl_backendGenerate(ctx, tree);
  CU_ASSERT(ret == IDL_RETCODE_OK);
  mem_buf = get_ostream_buffer(idl_get_output_stream(ctx));
  expected_output = (strncmp(mem_buf, output, strlen(output)) == 0);
  if (!expected_output) {
    printf("%s\n", mem_buf);
    printf("%s\n", output);
    for (size_t i = 0; i < strlen(output); ++i) {
      if (mem_buf[i] != output[i]) {
        printf("Diff starts here.....\n%s\n", &output[i]);
        break;
      }
    }
  }
  CU_ASSERT(expected_output);
  idl_backend_context_free(ctx);
  idl_delete_tree(tree);
}

CU_TheoryDataPoints(cpp11Backend, Struct) =
{
  /* Series of IDL input */
  CU_DataPoints(const char *, IDL_INPUT_STRUCT("AttrHolder","short","s"),
                              IDL_INPUT_STRUCT("AttrHolder","long","l"),
                              IDL_INPUT_STRUCT("AttrHolder","long long","ll"),
                              IDL_INPUT_STRUCT("AttrHolder","unsigned short","us"),
                              IDL_INPUT_STRUCT("AttrHolder","unsigned long","ul"),
                              IDL_INPUT_STRUCT("AttrHolder","unsigned long long","_ull"),
                              IDL_INPUT_STRUCT("AttrHolder","octet","o_"),
                              IDL_INPUT_STRUCT("AttrHolder","char","c"),
                              /* IDL_INPUT_STRUCT("AttrHolder","wchar","w_c"), */
                              IDL_INPUT_STRUCT("try","float","f"),
                              IDL_INPUT_STRUCT("AttrHolder","double","d"),
                              IDL_INPUT_STRUCT("AttrHolder","string","catch"),
                              /* IDL_INPUT("AttrHolder","wstring","_wname"), */
                              IDL_INPUT_STRUCT("AttrHolder","sequence<octet>","payload"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<octet, 100>","b_payload"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<string<8>>","listBStr"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<string<8>, 5>","bListBStr"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<sequence<string>>","strSeqSeq"),
                              IDL_INPUT_STRUCT("AttrHolder","float","coordinate[3]"),
                              IDL_INPUT_STRUCT("AttrHolder","float","LineCoordinates[2][3]"),
                              IDL_INPUT_ENUM("Color","Red","Yellow","Blue")
                              ),
  /* Series of corresponding C++ output */
  CU_DataPoints(const char *, IDL_OUTPUT_STRUCT_PRIM("AttrHolder","int16_t","0","s"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","int32_t","0","l"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","int64_t","0","ll"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","uint16_t","0","us"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","uint32_t","0","ul"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","uint64_t","0","ull"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","uint8_t","0","o_"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","char","0","c"),
                              /* IDL_OUTPUT_STRUCT_PRIM("AttrHolder","wchar","0","w_c"), */
                              IDL_OUTPUT_STRUCT_PRIM("_cxx_try","float","0.0f","f"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","double","0.0","d"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::string","_cxx_catch"),
                              /* IDL_OUTPUT_NO_PRIM("AttrHolder","std::wstring","wname"), */
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<uint8_t>","payload"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<uint8_t>","b_payload"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<std::string>","listBStr"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<std::string>","bListBStr"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<std::vector<std::string>>","strSeqSeq"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::array<float, 3>","coordinate"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::array<std::array<float, 3>, 2>","LineCoordinates"),
                              IDL_OUTPUT_ENUM("Color","Red","Yellow","Blue")
                              )
};

CU_Theory((const char *input, const char *output), cpp11Backend, Struct, .timeout = 30)
{
#if INITIAL_RUN
  if (initial_run) {
    unsigned int secs = 8;
    wait_a_bit(8);
    printf("Sleeping for %u seconds. Please attach debugger...\n", secs);
    initial_run = false;
  }
#endif
  test_base_type(input, 0u, IDL_RETCODE_OK, output);
}
