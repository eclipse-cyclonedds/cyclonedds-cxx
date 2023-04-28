// Copyright(c) 2020 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "idl/file.h"
#include "idl/version.h"
#include "idl/processor.h"
#include "idl/string.h"
#include "idl/stream.h"
#include "idl/visit.h"

#include "generator.h"

static size_t istok(const char *str, size_t len, const char **toks)
{
  for (size_t num = 0; toks && toks[num]; num++) {
    if (strncmp(toks[num], str, len) == 0)
      return num+1;
  }
  return 0;
}

static int makefmtp(
  char **fmtp, const char *str, const char **toks, const char **flags)
{
  char buf[64], *fmt;
  size_t len = 0;
  size_t num, src, tok, dest;

#define FMT "%%%zu$%s"

  src = tok = 0;
  do {
    if (tok) {
      if (str[src] == '}' && (num = istok(str+tok, src-tok, toks))) {
        const char *flag;
        assert(toks && flags);
        if (!flags)
          return -1;
        flag = flags[num-1];
        len += (size_t)snprintf(buf, sizeof(buf), FMT, num, flag);
        tok = 0;
      } else if (str[src] == '}' || str[src] == '\0') {
        /* unknown token, rewind */
        len += 1;
        src = tok - 1;
        tok = 0;
      }
    } else if (str[src] == '{') {
      tok = src + 1;
    } else if (str[src]) {
      len += 1u + (str[src] == '%'); /* escape % */
    }
  } while (str[src++]);

  if (!(fmt = malloc(len + 1)))
    return -1;

  src = tok = dest = 0;
  do {
    if (tok) {
      if (str[src] == '}' && (num = istok(str+tok, src-tok, toks))) {
        const char *flag;
        assert(toks && flags);
        flag = flags[num-1];
        dest += (size_t)snprintf(&fmt[dest], (len-dest)+1, FMT, num, flag);
        tok = 0;
      } else if (str[src] == '}' || str[src] == '\0') {
        fmt[dest++] = '{';
        src = tok - 1;
        tok = 0;
      }
    } else if (str[src] == '{') {
      tok = src + 1;
    } else if (str[src] == '%') {
      fmt[dest++] = '%';
      fmt[dest++] = '%';
    } else if (str[src]) {
      fmt[dest++] = str[src];
    }
  } while (str[src++]);
  assert(dest == len);
  fmt[len] = '\0';

#undef FMT

  *fmtp = fmt;
  return (int)len;
}

/* If a keyword matches the specified identifier, prepend _cxx_ */
static const char *cpp11_keywords[] = {
#define _cxx_(str) "_cxx_" str
  _cxx_("alignas"), _cxx_("alignof"), _cxx_("and"), _cxx_("and_eq"),
  _cxx_("asm"), _cxx_("auto"), _cxx_("bitand"), _cxx_("bitor"), _cxx_("bool"),
  _cxx_("break"), _cxx_("case"), _cxx_("catch"), _cxx_("char"),
  _cxx_("char16_t"), _cxx_("char32_t"), _cxx_("class"), _cxx_("compl"),
  _cxx_("concept"), _cxx_("const"), _cxx_("constexpr"), _cxx_("const_cast"),
  _cxx_("continue"), _cxx_("decltype"), _cxx_("default"), _cxx_("delete"),
  _cxx_("do"), _cxx_("double"), _cxx_("dynamic_cast"), _cxx_("else"),
  _cxx_("enum"), _cxx_("explicit"), _cxx_("export"), _cxx_("extern"),
  _cxx_("false"), _cxx_("float"), _cxx_("for"), _cxx_("friend"), _cxx_("goto"),
  _cxx_("if"), _cxx_("inline"), _cxx_("int"), _cxx_("long"), _cxx_("mutable"),
  _cxx_("namespace"), _cxx_("new"), _cxx_("noexcept"), _cxx_("not"),
  _cxx_("not_eq"), _cxx_("nullptr"), _cxx_("operator"), _cxx_("or"),
  _cxx_("or_eq"), _cxx_("private"), _cxx_("protected"), _cxx_("public"),
  _cxx_("register"), _cxx_("reinterpret_cast"), _cxx_("requires"),
  _cxx_("return"), _cxx_("short"), _cxx_("signed"), _cxx_("sizeof"),
  _cxx_("static"), _cxx_("static_assert"), _cxx_("static_cast"),
  _cxx_("struct"), _cxx_("switch"), _cxx_("template"), _cxx_("this"),
  _cxx_("thread_local"), _cxx_("throw"), _cxx_("true"), _cxx_("try"),
  _cxx_("typedef"), _cxx_("typeid"), _cxx_("typename"), _cxx_("union"),
  _cxx_("unsigned"), _cxx_("using"), _cxx_("virtual"), _cxx_("void"),
  _cxx_("volatile"), _cxx_("wchar_t"), _cxx_("while"), _cxx_("xor"),
  _cxx_("xor_eq"), _cxx_("int16_t"), _cxx_("int32_t"), _cxx_("int64_t"),
  _cxx_("uint8_t"), _cxx_("uint16_t"), _cxx_("uint32_t"), _cxx_("uint64_t")
#undef _cxx_
};

const char *get_cpp11_name(const void *node)
{
  const char *name = idl_identifier(node);
  static const size_t n = sizeof(cpp11_keywords)/sizeof(cpp11_keywords[0]);

  /* search through the C++ keyword list */
  for (size_t i=0; i < n; i++) {
    if (strcmp(cpp11_keywords[i] + 5, name) == 0)
      return cpp11_keywords[i];
  }

  return name;
}

static int get_cpp11_base_type(
  char *str, size_t size, const void *node, void *user_data)
{
  const char *type;

  (void)user_data;

  switch (idl_type(node)) {
    case IDL_CHAR:    type = "char";        break;
    case IDL_WCHAR:   type = "wchar";       break;
    case IDL_BOOL:    type = "bool";        break;
    case IDL_INT8:    type = "int8_t";      break;
    case IDL_UINT8:
    case IDL_OCTET:   type = "uint8_t";     break;
    case IDL_SHORT:
    case IDL_INT16:   type = "int16_t";     break;
    case IDL_USHORT:
    case IDL_UINT16:  type = "uint16_t";    break;
    case IDL_LONG:
    case IDL_INT32:   type = "int32_t";     break;
    case IDL_ULONG:
    case IDL_UINT32:  type = "uint32_t";    break;
    case IDL_LLONG:
    case IDL_INT64:   type = "int64_t";     break;
    case IDL_ULLONG:
    case IDL_UINT64:  type = "uint64_t";    break;
    case IDL_FLOAT:   type = "float";       break;
    case IDL_DOUBLE:  type = "double";      break;
    case IDL_LDOUBLE: type = "long double"; break;
    default:
      abort();
  }

  return idl_snprintf(str, size, "%s", type);
}

static int get_cpp11_templ_type(
  char *str, size_t size, const void *node, void *user_data)
{
  struct generator *gen = user_data;
  const char *fmt;

  assert(str && size);

  switch (idl_type(node)) {
    case IDL_SEQUENCE: {
      int cnt;
      char buf[128], *type = buf;
      const idl_sequence_t *sequence = node;

      if (sequence->maximum)
        fmt = gen->bounded_sequence_format;
      else
        fmt = gen->sequence_format;
      if ((cnt = get_cpp11_type(type, sizeof(buf), sequence->type_spec, gen)) < 0)
        return -1;
      if ((size_t)cnt < sizeof(buf))
        return idl_snprintf(str, size, fmt, type, sequence->maximum);
      if (!(type = malloc((size_t)cnt+1)))
        return -1;
      if ((cnt = get_cpp11_type(type, (size_t)cnt+1, sequence->type_spec, gen)) >= 0)
        cnt = idl_snprintf(str, size, fmt, type, sequence->maximum);
      free(type);
      return cnt;
    }
    case IDL_STRING: {
      const idl_string_t *string = node;

      if (string->maximum)
        fmt = gen->bounded_string_format;
      else
        fmt = gen->string_format;
      return idl_snprintf(str, size, fmt, string->maximum);
    }
    default:
      break;
  }

  abort();
}

static int get_cpp11_array_type(
  char *str, size_t size, const void *node, void *user_data)
{
  struct generator *gen = user_data;
  const idl_type_spec_t *type_spec = idl_type_spec(node);
  const idl_const_expr_t *const_expr;
  int cnt;
  char buf[1], *type = NULL;

  assert(idl_is_declarator(node));

  if ((cnt = get_cpp11_type(buf, sizeof(buf), type_spec, gen)) < 0)
    return -1;
  if (!(type = malloc((size_t)cnt + 1)))
    return -1;
  cnt = get_cpp11_type(type, (size_t)cnt + 1, type_spec, gen);
  if (cnt < 0)
    goto err_type;

  const_expr = ((const idl_declarator_t *)node)->const_expr;
  /* iterate backwards through the list so that the last entries in the list
     are the innermost arrays */
  for (const idl_const_expr_t *ce = const_expr; ce; ce = idl_next(ce))
    const_expr = ce;
  assert(const_expr);
  for (const idl_const_expr_t *ce = const_expr; ce; ce = idl_previous(ce)) {
    char *ptr = NULL;
    uint32_t dim = ((const idl_literal_t *)ce)->value.uint32;

    cnt = idl_asprintf(&ptr, gen->array_format, type, dim);
    if (cnt < 0)
      goto err_type;
    free(type);
    type = ptr;
  }

  cnt = idl_snprintf(str, size, "%s", type);
err_type:
  free(type);
  return cnt;
}

static void copy(
  char *dest, size_t off, size_t size, const char *src, size_t len)
{
  size_t cnt;

  if (off >= size)
    return;
  cnt = size - off;
  cnt = cnt > len ? len : cnt;
  memmove(dest+off, src, cnt);
}

static int get_cpp11_fully_scoped_name_seps(
  char *str, size_t size, const void *node, const char *sep)
{
  const char *name;
  size_t cnt, off, len = 0;
  static const idl_mask_t mask =
    IDL_MODULE | IDL_STRUCT | IDL_UNION | IDL_ENUM |
    IDL_ENUMERATOR | IDL_DECLARATOR | IDL_BITMASK;

  assert(str && size);

  for (const idl_node_t *n = node; n; n = n->parent) {
    if (!(idl_mask(n) & mask))
      continue;
    name = get_cpp11_name(n);
    len += strlen(name) + strlen(sep);
  }

  off = len;
  for (const idl_node_t *n = node; n; n = n->parent) {
    if (!(idl_mask(n) & mask))
      continue;
    name = get_cpp11_name(n);
    assert(name);
    cnt = strlen(name);
    assert(cnt <= off);
    off -= cnt;
    copy(str, off, size, name, cnt);
    cnt = strlen(sep);
    assert(cnt <= off);
    off -= cnt;
    copy(str, off, size, sep, cnt);
  }
  assert(off == 0);
  str[ (len < size ? len : size - 1) ] = '\0';

  return (int)len;
}

int get_cpp11_fully_scoped_name(
  char *str, size_t size, const void *node, void *user_data)
{
  (void)user_data;
  return get_cpp11_fully_scoped_name_seps(str, size, node, "::");
}

int get_cpp11_name_typedef(
  char *str, size_t size, const void *node, void *user_data)
{
  (void)user_data;
  return get_cpp11_fully_scoped_name_seps(str, size, node, "_");
}

int get_cpp11_type(
  char *str, size_t size, const void *node, void *user_data)
{
  if (idl_is_case(node)) {
    const idl_case_t *_case = node;
    if (idl_is_array(_case->declarator))
      return get_cpp11_type(str, size, _case->declarator, user_data);
    else
      return get_cpp11_type(str, size, _case->type_spec, user_data);
  } else if (idl_is_base_type(node))
    return get_cpp11_base_type(str, size, node, user_data);
  else if (idl_is_templ_type(node))
    return get_cpp11_templ_type(str, size, node, user_data);
  else if (idl_is_array(node))
    return get_cpp11_array_type(str, size, node, user_data);
  else
    return get_cpp11_fully_scoped_name(str, size, node, user_data);
}

int get_cpp11_default_value(
  char *str, size_t size, const void *node, void *user_data)
{
  struct generator *gen = user_data;
  const idl_enumerator_t *enumerator;
  const idl_enum_t *e;
  const idl_type_spec_t *unaliased = idl_strip(node, IDL_STRIP_ALIASES | IDL_STRIP_FORWARD);
  const char *value = NULL;

  if (idl_is_array(unaliased))
    return idl_snprintf(str, size, "{ }");

  switch (idl_type(unaliased)) {
    case IDL_ENUM:
      e = ((const idl_enum_t *)unaliased);
      enumerator = e->default_enumerator ? e->default_enumerator : e->enumerators;
      return get_cpp11_fully_scoped_name(str, size, enumerator, gen);
    case IDL_BOOL:
      value = "false";
      break;
    case IDL_CHAR:
    case IDL_WCHAR:
    case IDL_OCTET:
    case IDL_BITMASK:
      value = "0";
      break;
    case IDL_FLOAT:
      value = "0.0f";
      break;
    case IDL_DOUBLE:
    case IDL_LDOUBLE:
      value = "0.0";
      break;
    case IDL_INT8:
    case IDL_UINT8:
    case IDL_INT16:
    case IDL_UINT16:
    case IDL_SHORT:
    case IDL_USHORT:
    case IDL_INT32:
    case IDL_UINT32:
    case IDL_LONG:
    case IDL_ULONG:
    case IDL_INT64:
    case IDL_UINT64:
    case IDL_LLONG:
    case IDL_ULLONG:
      value = "0";
      break;
    default:
      abort();
  }

  return idl_snprintf(str, size, "%s", value);
}

int get_cpp11_base_type_const_value(
  char *str, size_t size, const void *node, void *user_data)
{
  const idl_literal_t *literal = node;

  (void)user_data;
  switch (idl_type(literal)) {
    case IDL_BOOL:
      return idl_snprintf(str, size, "%s", literal->value.bln ? "true" : "false");
    case IDL_INT8:
      return idl_snprintf(str, size, "%" PRId8, literal->value.int8);
    case IDL_UINT8:
    case IDL_OCTET:
      return idl_snprintf(str, size, "%" PRIu8, literal->value.uint8);
    case IDL_INT16:
    case IDL_SHORT:
      return idl_snprintf(str, size, "%" PRId16, literal->value.int16);
    case IDL_UINT16:
    case IDL_USHORT:
      return idl_snprintf(str, size, "%" PRIu16, literal->value.uint16);
    case IDL_INT32:
    case IDL_LONG:
      return idl_snprintf(str, size, "%" PRId32, literal->value.int32);
    case IDL_UINT32:
    case IDL_ULONG:
      return idl_snprintf(str, size, "%" PRIu32, literal->value.uint32);
    case IDL_INT64:
    case IDL_LLONG:
      return idl_snprintf(str, size, "%" PRId64, literal->value.int64);
    case IDL_UINT64:
    case IDL_ULLONG:
    case IDL_BITMASK:
      return idl_snprintf(str, size, "%" PRIu64, literal->value.uint64);
    case IDL_FLOAT:
      return idl_snprintf(str, size, "%.6f", literal->value.flt);
    case IDL_DOUBLE:
      return idl_snprintf(str, size, "%f", literal->value.dbl);
    case IDL_LDOUBLE:
      return idl_snprintf(str, size, "%Lf", literal->value.ldbl);
    case IDL_CHAR:
      if (idl_isprint(literal->value.chr))
        return idl_snprintf(str, size, "\'%c\'", literal->value.chr);
      return idl_snprintf(str, size, "0x%X", literal->value.chr);
    case IDL_STRING:
      return idl_snprintf(str, size, "\"%s\"", literal->value.str);
    default:
      break;
  }

  abort();
}

static int get_cpp11_templ_type_const_value(
  char *str, size_t size, const void *node, void *user_data)
{
  const idl_literal_t *literal = node;

  (void)user_data;
  if (!idl_is_string(literal))
    return -1;
  assert(literal->value.str);
  return idl_snprintf(str, size, "\"%s\"", literal->value.str);
}

int get_cpp11_value(
  char *str, size_t size, const void *node, void *user_data)
{
  if (idl_type(node) & (IDL_BASE_TYPE | IDL_BITMASK))
    return get_cpp11_base_type_const_value(str, size, node, user_data);
  if (idl_type(node) & IDL_TEMPL_TYPE)
    return get_cpp11_templ_type_const_value(str, size, node, user_data);
  if (idl_is_enumerator(node))
    return get_cpp11_fully_scoped_name(str, size, node, user_data);
  abort();
}

bool is_optional(
  const void *node)
{
  if (idl_is_member(node)) {
    return ((const idl_member_t *)node)->optional.value;
  } else if (idl_is_declarator(node)
          || idl_is_type_spec(node)) {
    return is_optional(idl_parent(node));
  }

  return false;
}

bool is_external(
  const void *node)
{
  if (idl_is_member(node) || idl_is_case(node)) {
    return idl_is_external(node);
  } else if (idl_is_declarator(node)
          || idl_is_type_spec(node)) {
    return is_external(idl_parent(node));
  }

  return false;
}

bool must_understand(
  const void *node)
{
  if (idl_is_member(node)) {
    return ((const idl_member_t *)node)->must_understand.value;
  } else if (idl_is_declarator(node)
          || idl_is_type_spec(node)) {
    return must_understand(idl_parent(node));
  }

  return false;
}

bool is_nested(const void *node)
{
  if (idl_is_struct(node)) {
    return ((const idl_struct_t *)node)->nested.value;
  } else {
    assert(idl_is_union(node));
    return ((const idl_union_t *)node)->nested.value;
  }
}

static bool sc_union(const idl_union_t *_union)
{
  if (!is_selfcontained(_union->switch_type_spec->type_spec))
    return false;

  const idl_case_t *_case = NULL;
  IDL_FOREACH(_case, _union->cases) {
    if (!is_selfcontained(_case->type_spec))
      return false;
  }

  return true;
}

static bool sc_struct(const idl_struct_t *str)
{
  const idl_member_t *mem = NULL;
  IDL_FOREACH(mem, str->members) {
    if (!is_selfcontained(mem->type_spec))
      return false;
  }

  if (str->inherit_spec)
    return is_selfcontained(str->inherit_spec->base);

  return true;
}

bool is_selfcontained(const void *node)
{
  if (idl_is_sequence(node)
   || idl_is_string(node)
   || idl_is_optional(node)) {
    return false;
  } else if (idl_is_typedef(node)) {
    return is_selfcontained(((const idl_typedef_t*)node)->type_spec);
  } else if (idl_is_struct(node)) {
    return sc_struct((const idl_struct_t*)node);
  } else if (idl_is_union(node)) {
    return sc_union((const idl_union_t*)node);
  } else if (idl_is_declarator(node)) {
    const idl_node_t *parent = ((const idl_node_t*)node)->parent;
    assert (idl_is_typedef(parent));
    return is_selfcontained(parent);
  } else {
    return !is_external(node);
  }
}

idl_extensibility_t
get_extensibility(const void *node)
{
  if (idl_is_enum(node)) {
    const idl_enum_t *ptr = node;
    return ptr->extensibility.value;
  } else if (idl_is_union(node)) {
    const idl_union_t *ptr = node;
    return ptr->extensibility.value;
  } else if (idl_is_struct(node)) {
    const idl_struct_t *ptr = node;
    return ptr->extensibility.value;
  }
  return IDL_FINAL;
}

idl_type_t unalias_bitmask(const idl_node_t *node)
{
  assert(idl_is_bitmask(node));

  uint16_t width = ((const idl_bitmask_t*)node)->bit_bound.value;

  if (width > 32)
    return IDL_UINT64;
  else if (width > 16)
    return IDL_UINT32;
  else if (width > 8)
    return IDL_UINT16;
  else
    return IDL_UINT8;
}

static char *
figure_guard(const char *file)
{
  char *inc = NULL;

  if (idl_asprintf(&inc, "DDSCXX_%s", file) == -1)
    return NULL;

  /* replace any non-alphanumeric characters */
  for (char *ptr = inc; *ptr; ptr++) {
    if (idl_islower((unsigned char)*ptr))
      *ptr = (char)idl_toupper((unsigned char)*ptr);
    else if (!idl_isalnum((unsigned char)*ptr))
      *ptr = '_';
  }

  return inc;
}

static idl_retcode_t
print_header(FILE *fh, const char *in, const char *out)
{
  static const char fmt[] =
    "/****************************************************************\n"
    "\n"
    "  Generated by Eclipse Cyclone DDS IDL to CXX Translator\n"
    "  File name: %s\n"
    "  Source: %s\n"
    "  Cyclone DDS: v%s\n"
    "\n"
    "*****************************************************************/\n";

  if (idl_fprintf(fh, fmt, in, out, IDL_VERSION) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
print_impl_header(FILE *fh, const char *in, const char *out, const char *hdr)
{
  const char *fmt = "#include \"%s\"\n\n";
  if (print_header(fh, in, out)
   || idl_fprintf(fh, fmt, hdr) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t print_guard_if(FILE* fh, const char *guard)
{
  static const char *fmt =
    "#ifndef %1$s\n"
    "#define %1$s\n\n";
  if (idl_fprintf(fh, fmt, guard) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t print_guard_endif(FILE *fh, const char *guard)
{
  static const char fmt[] =
    "#endif // %s\n";
  if (idl_fprintf(fh, fmt, guard) < 0)
    return IDL_RETCODE_NO_MEMORY;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
register_union(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const idl_location_t *loc;
  const char *src = NULL;

  (void)revisit;
  (void)path;
  loc = idl_location(node);
  assert(loc);
  if (pstate->sources)
    src = pstate->sources->path->name;
  /* do not include headers if required by types in includes */
  if (src && strcmp(loc->first.source->path->name, src) == 0)
    gen->uses_union = true;
  return IDL_RETCODE_OK;
}

static idl_retcode_t
register_optional_or_external(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  (void)pstate;
  (void)revisit;
  (void)path;

  struct generator *gen = user_data;

  if (is_optional(node))
    gen->uses_optional = true;

  if (idl_is_external(node))
    gen->uses_external = true;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
register_types(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  struct generator *gen = user_data;
  const idl_type_spec_t *type_spec;
  const idl_location_t *loc;
  const char *src = NULL;

  (void)pstate;
  (void)revisit;
  (void)path;

  if (idl_is_typedef(node))
  {
    const idl_typedef_t *td = node;
    if (idl_is_array(td->declarators))
      gen->uses_array = true;
  }

  if (idl_is_array(node))
    gen->uses_array = true;

  type_spec = idl_strip(idl_type_spec(node), IDL_STRIP_ALIASES | IDL_STRIP_FORWARD);
  loc = idl_location(type_spec);
  assert(type_spec && loc);
  if (pstate->sources)
    src = pstate->sources->path->name;
  /* do not include headers if required by types in includes */
  if (src && strcmp(loc->first.source->path->name, src) != 0)
    return IDL_VISIT_DONT_RECURSE;

  if (idl_is_array(type_spec))
    gen->uses_array = true;

  type_spec = idl_strip(idl_type_spec(node), IDL_STRIP_ALIASES | IDL_STRIP_ALIASES_ARRAY | IDL_STRIP_FORWARD);
  loc = idl_location(type_spec);
  assert(type_spec && loc);
  /* do not include headers if required by types in includes */
  if (src && strcmp(loc->first.source->path->name, src) != 0)
    return IDL_VISIT_DONT_RECURSE;

  if (idl_is_sequence(type_spec)) {
    if (idl_is_bounded(type_spec))
      gen->uses_bounded_sequence = true;
    else
      gen->uses_sequence = true;
    return IDL_VISIT_TYPE_SPEC;
  } else if (idl_is_string(type_spec)) {
    if (idl_is_bounded(type_spec))
      gen->uses_bounded_string = true;
    else
      gen->uses_string = true;
  } else if (idl_is_union(type_spec)) {
    gen->uses_union = true;
  } else if (idl_is_integer_type(type_spec)) {
    gen->uses_integers = true;
  } else if (idl_type(type_spec) == IDL_OCTET) {
    gen->uses_integers = true;
  }

  return IDL_RETCODE_OK;
}

static idl_retcode_t
register_bitmask(
  const idl_pstate_t *pstate,
  bool revisit,
  const idl_path_t *path,
  const void *node,
  void *user_data)
{
  (void)pstate;
  (void)revisit;
  (void)path;
  (void)node;

  struct generator *gen = user_data;

  assert(idl_is_bitmask(node));
  assert(gen);

  gen->uses_integers = true;

  return IDL_RETCODE_OK;
}

static idl_retcode_t
generate_includes(const idl_pstate_t *pstate, struct generator *generator)
{
  idl_retcode_t ret;
  idl_visitor_t visitor;
  const char *sources[] = { NULL, NULL };
  const idl_source_t *include, *source = pstate->sources;

  /* determine which "system" headers to include */
  memset(&visitor, 0, sizeof(visitor));
  visitor.visit = IDL_DECLARATOR | IDL_SEQUENCE | IDL_UNION | IDL_MEMBER | IDL_CONST | IDL_TYPEDEF | IDL_CASE | IDL_BITMASK;
  visitor.accept[IDL_ACCEPT_DECLARATOR] = &register_types;
  visitor.accept[IDL_ACCEPT_TYPEDEF] = &register_types;
  visitor.accept[IDL_ACCEPT_MEMBER] = &register_optional_or_external;
  visitor.accept[IDL_ACCEPT_CASE] = &register_optional_or_external;
  visitor.accept[IDL_ACCEPT_SEQUENCE] = &register_types;
  visitor.accept[IDL_ACCEPT_CONST] = &register_types;
  visitor.accept[IDL_ACCEPT_UNION] = &register_union;
  visitor.accept[IDL_ACCEPT_BITMASK] = &register_bitmask;
  assert(pstate->sources);
  sources[0] = pstate->sources->path->name;
  visitor.sources = sources;
  if ((ret = idl_visit(pstate, pstate->root, &visitor, generator)))
    return ret;

  for (include = source->includes; include; include = include->next) {
    int cnt;
    const char *file = include->file->name;
    const char *ext = strrchr(file, '.');
    if (ext && idl_strcasecmp(ext, ".idl") == 0) {
      const char *fmt = "#include \"%.*s.hpp\"\n";
      int len = (int)(ext - file);
      cnt = idl_fprintf(generator->header.handle, fmt, len, file);
    } else {
      const char *fmt = "#include \"%s\"\n";
      cnt = idl_fprintf(generator->header.handle, fmt, file);
    }
    if (cnt < 0 || fputs("\n", generator->header.handle) < 0)
      return IDL_RETCODE_NO_MEMORY;
  }

  { int len = 0;
    const char *incs[10];

    if (generator->uses_integers)
      incs[len++] = "<cstdint>";
    if (generator->uses_array)
      incs[len++] = generator->array_include;
    if (generator->uses_sequence)
      incs[len++] = generator->sequence_include;
    if (generator->uses_bounded_sequence)
      incs[len++] = generator->bounded_sequence_include;
    if (generator->uses_string)
      incs[len++] = generator->string_include;
    if (generator->uses_bounded_string)
      incs[len++] = generator->bounded_string_include;
    if (generator->uses_union) {
      incs[len++] = generator->union_include;
      incs[len++] = "<dds/core/Exception.hpp>\n";
    }
    if (generator->uses_optional)
      incs[len++] = generator->optional_include;
    if (generator->uses_external)
      incs[len++] = generator->external_include;

    for (int i=0, j; i < len; i++) {
      for (j=0; j < i && strcmp(incs[i], incs[j]) != 0; j++) ;
      if (j < i)
        continue;
      const char *fmt = "#include %s\n";
      if (idl_fprintf(generator->header.handle, fmt, incs[i]) < 0)
        return IDL_RETCODE_NO_MEMORY;
    }
  }

  if (fputs("\n", generator->header.handle) < 0)
    return IDL_RETCODE_NO_MEMORY;

  return IDL_RETCODE_OK;
}

#if _WIN32
__declspec(dllexport)
#endif
idl_retcode_t generate_nosetup(const idl_pstate_t *pstate, struct generator *gen)
{
  idl_retcode_t ret;
  char *guard;

  if (!(guard = figure_guard(gen->header.path)))
    return IDL_RETCODE_NO_MEMORY;
  if ((ret = print_header(gen->header.handle, gen->path, gen->header.path)))
    goto err_print;
  if ((ret = print_impl_header(gen->impl.handle, gen->path, gen->impl.path, gen->header.path)))
    goto err_print;
  if ((ret = print_guard_if(gen->header.handle, guard)))
    goto err_print;
  if ((ret = generate_includes(pstate, gen)))
    goto err_print;
  if ((ret = generate_types(pstate, gen)))
    goto err_print;
  if ((ret = generate_traits(pstate, gen)))
    goto err_print;
  if ((ret = generate_streamers(pstate, gen)))
    goto err_print;
  if ((ret = print_guard_endif(gen->header.handle, guard)))
    goto err_print;
  free(guard);
  return IDL_RETCODE_OK;
err_print:
  free(guard);
  return ret;
}

const char *seq_tmpl = "std::vector<{TYPE}>";
const char *seq_inc = "<vector>";
const char *arr_tmpl = "std::array<{TYPE}, {DIMENSION}>";
const char *arr_inc = "<array>";
const char *bnd_seq_tmpl = "std::vector<{TYPE}>";
const char *bnd_seq_inc = "<vector>";
const char *str_tmpl = "std::string";
const char *str_inc = "<string>";
const char *bnd_str_tmpl = "std::string";
const char *bnd_str_inc = "<string>";
#if IDLCXX_USE_BOOST
const char *opt_tmpl = "boost::optional";
const char *opt_inc = "\"boost/optional.hpp\"";
const char *uni_tmpl = "boost::variant";
const char *uni_get_tmpl = "boost::get";
const char *uni_inc = "\"boost/variant.hpp\"";
#else
const char *opt_tmpl = "std::optional";
const char *opt_inc = "<optional>";
const char *uni_tmpl = "std::variant";
const char *uni_get_tmpl = "std::get";
const char *uni_inc = "<variant>";
#endif
const char *ext_tmpl = "dds::core::external";
const char *ext_inc = "<dds/core/External.hpp>";

static const char *arr_toks[] = { "TYPE", "DIMENSION", NULL };
static const char *arr_flags[] = { "s", PRIu32, NULL };
static const char *seq_toks[] = { "TYPE", NULL };
static const char *seq_flags[] = { "s", NULL };
static const char *bnd_seq_toks[] = { "TYPE", "BOUND", NULL };
static const char *bnd_seq_flags[] = { "s", PRIu32, NULL };
static const char *bnd_str_toks[] = { "BOUND", NULL };
static const char *bnd_str_flags[] = { PRIu32, NULL };

#if _WIN32
__declspec(dllexport)
#endif
idl_retcode_t generate(const idl_pstate_t *pstate, const idlc_generator_config_t *config)
{
  idl_retcode_t ret = IDL_RETCODE_NO_MEMORY;
  char *dir = NULL, *basename = NULL, *empty = "";
  const char *sep, *ext, *file, *path;
  struct generator gen;

  assert(pstate->paths);
  assert(pstate->paths->name);
  path = pstate->sources->path->name;
  assert(path);

  /* use relative directory if user provided a relative path, use current
     word directory otherwise */
  sep = ext = NULL;
  for (const char *ptr = path; ptr[0]; ptr++) {
    if (idl_isseparator((unsigned char)ptr[0]) && ptr[1] != '\0')
      sep = ptr;
    else if (ptr[0] == '.')
      ext = ptr;
  }

  file = sep ? sep + 1 : path;
  if (idl_isabsolute(path) || !sep)
    dir = empty;
  else if (!(dir = idl_strndup(path, (size_t)(sep-path))))
    goto err_dir;
  if (!(basename = idl_strndup(file, ext ? (size_t)(ext-file) : strlen(file))))
    goto err_basename;

  /* replace backslashes by forward slashes */
  for (char *ptr = dir; *ptr; ptr++) {
    if (*ptr == '\\')
      *ptr = '/';
  }

  memset(&gen, 0, sizeof(gen));
  gen.path = file;
  gen.config = config;

  sep = dir[0] == '\0' ? "" : "/";
  if (idl_asprintf(&gen.header.path, "%s%s%s.hpp", dir, sep, basename) < 0)
    goto err_hdr;
  if (!(gen.header.handle = idl_fopen(gen.header.path, "wb")))
    goto err_hdr_fh;
  if (idl_asprintf(&gen.impl.path, "%s%s%s.cpp", dir, sep, basename) < 0)
    goto err_impl;
  if (!(gen.impl.handle = idl_fopen(gen.impl.path, "wb")))
    goto err_impl_fh;

  /* generate format strings from templates */
  if (makefmtp(&gen.array_format, arr_tmpl, arr_toks, arr_flags) < 0)
    goto err_arr;
  if (makefmtp(&gen.sequence_format, seq_tmpl, seq_toks, seq_flags) < 0)
    goto err_seq;
  if (makefmtp(&gen.bounded_sequence_format, bnd_seq_tmpl, bnd_seq_toks, bnd_seq_flags) < 0)
    goto err_bnd_seq;
  if (makefmtp(&gen.string_format, str_tmpl, NULL, NULL) < 0)
    goto err_str;
  if (makefmtp(&gen.bounded_string_format, bnd_str_tmpl, bnd_str_toks, bnd_str_flags) < 0)
    goto err_bnd_str;
  if (makefmtp(&gen.optional_format, opt_tmpl, NULL, NULL) < 0)
    goto err_opt;
  if (makefmtp(&gen.external_format, ext_tmpl, NULL, NULL) < 0)
    goto err_ext;
  if (makefmtp(&gen.union_format, uni_tmpl, NULL, NULL) < 0)
    goto err_uni;
  if (makefmtp(&gen.union_getter_format, uni_get_tmpl, NULL, NULL) < 0)
    goto err_uni_get;
  /* copy include directives verbatim */
  gen.array_include = arr_inc;
  gen.sequence_include = seq_inc;
  gen.bounded_sequence_include = bnd_seq_inc;
  gen.string_include = str_inc;
  gen.bounded_string_include = bnd_str_inc;
  gen.optional_include = opt_inc;
  gen.union_include = uni_inc;
  gen.external_include = ext_inc;

  ret = generate_nosetup(pstate, &gen);

  free(gen.union_getter_format);
err_uni_get:
  free(gen.union_format);
err_uni:
  free(gen.external_format);
err_ext:
  free(gen.optional_format);
err_opt:
  free(gen.bounded_string_format);
err_bnd_str:
  free(gen.string_format);
err_str:
  free(gen.bounded_sequence_format);
err_bnd_seq:
  free(gen.sequence_format);
err_seq:
  free(gen.array_format);
err_arr:
  fclose(gen.impl.handle);
err_impl_fh:
  free(gen.impl.path);
err_impl:
  fclose(gen.header.handle);
err_hdr_fh:
  free(gen.header.path);
err_hdr:
  free(basename);
err_basename:
  if (dir && dir != empty)
    free(dir);
err_dir:
  return ret;
}

static const idlc_option_t *opts[] = {
  &(idlc_option_t) {
    IDLC_STRING, { .string = &seq_tmpl },
    'f', "sequence-template", "ns_name::vector<{TYPE} ...>",
    "Template used for sequences instead of std::vector. \"{TYPE}\" tags are "
    "replaced by the respective type specifier, other text is copied "
    "verbatim. (default: std::vector<TYPE>)."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &seq_inc },
    'f', "sequence-include", "<header>",
    "Header to include if template for sequence-template is used."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &bnd_seq_tmpl },
    'f', "bounded-sequence-template", "ns_name::vector<{TYPE}, {BOUND} ...>",
    "Template used for bounded sequences instead of std::vector. \"{TYPE}\" "
    "and \"{BOUND}\" tags are replaced by the type specifier and the "
    "maximum size respectively, other text is copied verbatim."
    "(default: std::vector<TYPE>)."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &bnd_seq_inc },
    'f', "bounded-sequence-include", "<header>",
    "Header to include if template for bounded-sequence-template is used."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &str_tmpl },
    'f', "string-template", "ns_name::string<...>",
    "Template to use for strings instead of std::string. "
    "(default: std:string)."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &str_inc },
    'f', "string-include", "<header>",
    "Header to include if template for string-template is used.",
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &bnd_str_tmpl },
    'f', "bounded-string-template", "ns_name::string<{BOUND} ...>",
    "Template to use for strings instead of std::string. \"{BOUND}\" "
    "tags are replaced by the repective maximum value, other text is copied "
    "verbatim. (default: std::string)"
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &bnd_str_inc },
    'f', "bounded-string-include", "<header>",
    "Header to include if template for bounded-string-template is used."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &opt_tmpl },
    'f', "optional-template", "ns_name::optional<...>",
    "Template to use for optionals instead of std::optional."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &opt_inc },
    'f', "optional-include", "<header>",
    "Header to include if template for optional-template is used."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &arr_tmpl },
    'f', "array-template", "ns_name::array<{TYPE}, {DIMENSION} ...>",
    "Template to use for arrays instead of std::array<{TYPE}, {DIMENSION}>. "
    "{TYPE} and {DIMENSION} tags are replaced by the respective type and "
    "dimension, other text is copied verbatim."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &arr_inc },
    'f', "array-include", "<header>",
    "Header to include if template for array-template is used."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &uni_tmpl },
    'f', "union-template", "ns_name::variant",
    "Template to use for unions instead of std::variant. Copied verbatim."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &uni_get_tmpl },
    'f', "union-getter-template", "ns_name::get",
    "Template to use for reading the value of a variant. Copied verbatim."
  },
  &(idlc_option_t) {
    IDLC_STRING, { .string = &uni_inc },
    'f', "union-include", "<header>",
    "Header to include if template for union-template is used."
  },
  NULL
};

#if _WIN32
__declspec(dllexport)
#endif
const idlc_option_t** generator_options(void)
{
  return opts;
}
