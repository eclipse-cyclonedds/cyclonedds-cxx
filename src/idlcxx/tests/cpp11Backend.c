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

#define INITIAL_RUN 0
#if INITIAL_RUN
#if _WIN32
#include <Windows.h>
#define wait_a_bit(seconds) Sleep((seconds) * 1000);
#else
#include <unistd.h>
#define wait_a_bit(seconds) sleep(seconds)
#endif
#endif

#define IDL_INPUT_STRUCT(struct_name,member_type,member_name) ""\
"struct " struct_name "\n{\n" \
"    " member_type " " member_name ";" \
"};"

#define IDL_INPUT_ENUM(enum_name,label1,label2,label3) "" \
"enum " enum_name "\n{\n" \
"  " label1 ",\n" \
"  " label2 ",\n" \
"  " label3 "\n" \
"};"

#define IDL_INPUT_UNION_1_BRANCH(union_name,discr_type,label1,case_type1,case_name1) ""\
"union " union_name " switch (" discr_type ")\n{\n"\
"case " label1 ":\n" \
"  " case_type1 " " case_name1 ";\n"\
"};"

#define IDL_INPUT_TYPEDEF(module_name,typedef_type,typedef_name) ""\
"module " module_name "{\n"\
"  typedef " typedef_type " " typedef_name ";\n"\
"};\n\n"

#define IDL_OUTPUT_STREAMER_INTERFACES ""\
"  size_t write_struct(void* data, size_t position) const;\n"\
"  size_t write_size(size_t offset) const;\n"\
"  size_t read_struct(const void* data, size_t position);\n"\
"  size_t key_size(size_t position) const;\n"\
"  size_t key_max_size(size_t position) const;\n"\
"  size_t key_stream(void* data, size_t position) const;\n"\
"  bool key(ddsi_keyhash_t& hash) const;\n"

#define IDL_OUTPUT_STRUCT_PRIM(struct_name,member_type,default_value,member_name) "" \
"class " struct_name "\n{\n" \
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
"\n" \
IDL_OUTPUT_STREAMER_INTERFACES\
"};\n\n"

#define IDL_OUTPUT_STRUCT_NO_PRIM(struct_name,member_type,member_name) "" \
"class " struct_name "\n{\n" \
"private:\n" \
"  " member_type " " member_name "_;\n" \
"\n" \
"public:\n" \
"  " struct_name "() {}\n" \
"\n"\
"  explicit " struct_name "(\n" \
"      const " member_type "& " member_name ") :\n" \
"          " member_name "_(" member_name ") {}\n" \
"\n" \
"  const " member_type "& " member_name "() const { return this->" member_name "_; }\n" \
"  " member_type "& " member_name "() { return this->" member_name "_; }\n" \
"  void " member_name "(const " member_type "& _val_) { this->" member_name "_ = _val_; }\n" \
"  void " member_name "(" member_type "&& _val_) { this->" member_name "_ = _val_; }\n" \
"\n" \
IDL_OUTPUT_STREAMER_INTERFACES\
"};\n\n"

#define IDL_OUTPUT_ENUM(enum_name,label1,label2,label3) "" \
"enum class " enum_name "\n{\n" \
"  " label1 ",\n" \
"  " label2 ",\n" \
"  " label3 ",\n" \
"};\n\n"

#define DEFAULT_DISCR_TP(label1) ""\
"    bool valid = true;\n"\
"    switch (val) {\n"\
"    case " label1 ":\n"\
"      if (m__d != " label1 ") {\n"\
"        valid = false;\n"\
"      }\n"\
"      break;\n"\
"    default:\n"\
"      if (m__d == " label1 ") {\n"\
"        valid = false;\n"\
"      }\n"\
"      break;\n"\
"    }\n"

#define BOOL_DISCR_TP(label1) ""\
"    bool valid = (val == m__d);\n"

#define SINGLE_DEFAULT(discr_type,default_discr_val) ""\
"  void _default()\n"\
"  {\n"\
"    m__d = " default_discr_val ";\n"\
"  }\n"

#define MULTI_DEFAULT(discr_type,default_discr_val) ""\
"  void _default(" discr_type " _d = " default_discr_val ")\n"\
"  {\n"\
"    m__d = _d;\n"\
"  }\n"

#define IDL_OUTPUT_UNION_1_BRANCH(union_name,discr_type,default_discr_val,discr_setter_tp,default_case_tp,label1,case_type1,case_name1) ""\
"class " union_name "\n{\n"\
"private:\n"\
"  " discr_type " m__d;\n"\
"  std::variant<\n"\
"      " case_type1 "\n"\
"  > " case_name1 ";\n"\
"\n"\
"public:\n"\
"  " union_name "() :\n"\
"      m__d(" default_discr_val ") {}\n"\
"\n"\
"  " discr_type " _d() const\n"\
"  {\n"\
"    return m__d;\n"\
"  }\n"\
"\n"\
"  void _d(" discr_type " val)\n"\
"  {\n"\
discr_setter_tp(label1)\
"\n"\
"    if (!valid) {\n"\
"      throw dds::core::InvalidArgumentError(\"New discriminator value does not match current discriminator\");\n"\
"    }\n"\
"\n"\
"    m__d = val;\n"\
"  }\n"\
"\n"\
"  const " case_type1 "& " case_name1 "() const\n"\
"  {\n"\
"    if (m__d == " label1 ") {\n"\
"      return std::get<" case_type1 ">(" case_name1 ");\n"\
"    } else {\n"\
"      throw dds::core::InvalidArgumentError(\"Requested branch does not match current discriminator\");\n"\
"    }\n"\
"  }\n"\
"\n"\
"  " case_type1 "& " case_name1 "()\n"\
"  {\n"\
"    if (m__d == " label1 ") {\n"\
"      return std::get<" case_type1 ">(" case_name1 ");\n"\
"    } else {\n"\
"      throw dds::core::InvalidArgumentError(\"Requested branch does not match current discriminator\");\n"\
"    }\n"\
"  }\n"\
"\n"\
"  void " case_name1 "(const " case_type1 "& val)\n"\
"  {\n"\
"    m__d = " label1 ";\n"\
"    " case_name1 " = val;\n"\
"  }\n"\
"\n"\
"  void " case_name1 "(" case_type1 "&& val)\n"\
"  {\n"\
"    m__d = " label1 ";\n"\
"    " case_name1 " = val;\n"\
"  }\n"\
"\n"\
default_case_tp(discr_type,default_discr_val)\
"\n" \
IDL_OUTPUT_STREAMER_INTERFACES\
"};"

#define IDL_OUTPUT_TYPEDEF(module_name,typedef_type,typedef_name) ""\
"namespace " module_name "\n"\
"{\n"\
"  typedef " typedef_type " " typedef_name ";\n\n"\
"}\n\n"

static void init()
{
#if INITIAL_RUN
  static bool initial_run = true;

  if (initial_run) {
    unsigned int secs = 8;
    wait_a_bit(8);
    printf("Sleeping for %u seconds. Please attach debugger...\n", secs);
    initial_run = false;
  }
#endif
}

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
  ctx = idl_backend_context_new(2, NULL, NULL);
  CU_ASSERT_PTR_NOT_NULL(ctx);
  ret = idl_backendGenerateType(ctx, tree);
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
                              IDL_INPUT_STRUCT("AttrHolder","wchar","w_c"),
                              IDL_INPUT_STRUCT("try","float","f"),
                              IDL_INPUT_STRUCT("AttrHolder","double","d"),
                              IDL_INPUT_STRUCT("AttrHolder","string","catch"),
                              /*IDL_INPUT_STRUCT("AttrHolder","wstring","_wname"),*/
                              IDL_INPUT_STRUCT("AttrHolder","sequence<octet>","payload"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<octet, 100>","b_payload"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<string<8>>","listBStr"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<string<8>, 5>","bListBStr"),
                              IDL_INPUT_STRUCT("AttrHolder","sequence<sequence<string>>","strSeqSeq"),
                              IDL_INPUT_STRUCT("AttrHolder","float","coordinate[3]"),
                              IDL_INPUT_STRUCT("AttrHolder","float","LineCoordinates[2][3]"),
                              IDL_INPUT_STRUCT("EmbeddedStr","long","x") IDL_INPUT_STRUCT("AttrHolder","EmbeddedStr","emb_str"),
                              IDL_INPUT_ENUM("Color","Red","Yellow","Blue") IDL_INPUT_STRUCT("AttrHolder","::Color","col")
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
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","wchar","0","w_c"),
                              IDL_OUTPUT_STRUCT_PRIM("_cxx_try","float","0.0f","f"),
                              IDL_OUTPUT_STRUCT_PRIM("AttrHolder","double","0.0","d"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::string","_cxx_catch"),
                              /*IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::wstring","wname"),*/
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<uint8_t>","payload"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<uint8_t>","b_payload"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<std::string>","listBStr"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<std::string>","bListBStr"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::vector<std::vector<std::string>>","strSeqSeq"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::array<float, 3>","coordinate"),
                              IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","std::array<std::array<float, 3>, 2>","LineCoordinates"),
                              IDL_OUTPUT_STRUCT_PRIM("EmbeddedStr","int32_t","0","x") IDL_OUTPUT_STRUCT_NO_PRIM("AttrHolder","::EmbeddedStr","emb_str"),
                              IDL_OUTPUT_ENUM("Color","Red","Yellow","Blue") IDL_OUTPUT_STRUCT_PRIM("AttrHolder","::Color","::Color::Red","col")
                              )
};

CU_Theory((const char *input, const char *output), cpp11Backend, Struct, .timeout = 30)
{
  init();
  test_base_type(input, 0u, IDL_RETCODE_OK, output);
}

CU_TheoryDataPoints(cpp11Backend, Enum) =
{
  /* Series of IDL input */
  CU_DataPoints(const char *, IDL_INPUT_ENUM("Color","Red","Yellow","Blue")
                              ),
  /* Series of corresponding C++ output */
  CU_DataPoints(const char *, IDL_OUTPUT_ENUM("Color","Red","Yellow","Blue"),
                              )
};

CU_Theory((const char *input, const char *output), cpp11Backend, Enum, .timeout = 30)
{
  init();
  test_base_type(input, 0u, IDL_RETCODE_OK, output);
}

CU_TheoryDataPoints(cpp11Backend, Typedef) =
{
  /* Series of IDL input */
  CU_DataPoints(const char *, IDL_INPUT_TYPEDEF("m","sequence<long>","sl")
                                  IDL_INPUT_STRUCT("s","::m::sl","l")
                             ),
  /* Series of corresponding C++ output */
  CU_DataPoints(const char *, IDL_OUTPUT_TYPEDEF("m","std::vector<int32_t>","sl")
                                  IDL_OUTPUT_STRUCT_NO_PRIM("s","::m::sl","l")
                              )
};

CU_Theory((const char *input, const char *output), cpp11Backend, Typedef, .timeout = 30)
{
  init();
  test_base_type(input, 0u, IDL_RETCODE_OK, output);
}

CU_TheoryDataPoints(cpp11Backend, Union) =
{
  /* Series of IDL input */
  CU_DataPoints(const char *, IDL_INPUT_UNION_1_BRANCH("CaseHolder1","long","1","string","name"),
                              IDL_INPUT_UNION_1_BRANCH("try","short","0","string","_module"),
                              IDL_INPUT_UNION_1_BRANCH("CaseHolder2","boolean","TRUE","string","name"),
                              IDL_INPUT_UNION_1_BRANCH("CaseHolder3","boolean","FALSE","sequence<string>","names"),
                              IDL_INPUT_ENUM("Color","Red","Yellow","Blue") IDL_INPUT_UNION_1_BRANCH("CaseHolder4","Color","Red","string","name"),
                              IDL_INPUT_ENUM("Color","Red","Yellow","Blue") IDL_INPUT_UNION_1_BRANCH("CaseHolder5","Color","Yellow","string","name")
                              ),
  /* Series of corresponding C++ output */
  CU_DataPoints(const char *, IDL_OUTPUT_UNION_1_BRANCH("CaseHolder1","int32_t","-2147483648",DEFAULT_DISCR_TP,MULTI_DEFAULT,"1","std::string","name"),
                              IDL_OUTPUT_UNION_1_BRANCH("_cxx_try","int16_t","-32768",DEFAULT_DISCR_TP,MULTI_DEFAULT,"0","std::string","module"),
                              IDL_OUTPUT_UNION_1_BRANCH("CaseHolder2","bool","false",BOOL_DISCR_TP,SINGLE_DEFAULT,"true","std::string","name"),
                              IDL_OUTPUT_UNION_1_BRANCH("CaseHolder3","bool","true",BOOL_DISCR_TP,SINGLE_DEFAULT,"false","std::vector<std::string>","names"),
                              IDL_OUTPUT_ENUM("Color","Red","Yellow","Blue") IDL_OUTPUT_UNION_1_BRANCH("CaseHolder4","::Color","::Color::Yellow",DEFAULT_DISCR_TP,MULTI_DEFAULT,"::Color::Red","std::string","name"),
                              IDL_OUTPUT_ENUM("Color","Red","Yellow","Blue") IDL_OUTPUT_UNION_1_BRANCH("CaseHolder5","::Color","::Color::Red",DEFAULT_DISCR_TP,MULTI_DEFAULT,"::Color::Yellow","std::string","name")
                              )
};

CU_Theory((const char *input, const char *output), cpp11Backend, Union)
{
  init();
  test_base_type(input, 0u, IDL_RETCODE_OK, output);
}

