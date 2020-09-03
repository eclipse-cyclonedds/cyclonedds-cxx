#include "datatopic.hpp"
#include "dds/ddsi/q_radmin.h"

bool helloworld_serdata::helloworld_serdata_eqkey(const struct ddsi_serdata* a, const struct ddsi_serdata* b)
{
  (void)a;
  (void)b;
  return true;
}

uint32_t helloworld_serdata::helloworld_serdata_size(const struct ddsi_serdata* dcmn)
{
//  const helloworld_serdata* ptr = static_cast<const helloworld_serdata*>(dcmn);
//  size_t size = HelloWorldData::write_size(ptr->data(),0);
//  uint32_t size_u32 = static_cast<uint32_t>(size);
//  assert(size == size_u32);
//  return size_u32;
  return 0u;
}

struct ddsi_serdata* helloworld_serdata::helloworld_serdata_from_ser(
  const struct ddsi_sertopic* topic,
  enum ddsi_serdata_kind kind,
  const struct nn_rdata* fragchain, size_t size)
{
  auto d = std::make_unique<helloworld_serdata>(topic, kind);
  (void)fragchain;
  (void)size;
#if 0
  uint32_t off = 0;
  assert(fragchain->min == 0);
  assert(fragchain->maxp1 >= off);    //CDR header must be in first fragment

  std::vector<char> buffer(size);

  auto cursor = buffer.data();
  while (fragchain) {
    if (fragchain->maxp1 > off) {
      //only copy if this fragment adds data
      const unsigned char* payload =
        NN_RMSG_PAYLOADOFF(fragchain->rmsg, NN_RDATA_PAYLOAD_OFF(fragchain));
      auto src = payload + off - fragchain->min;
      auto n_bytes = fragchain->maxp1 - off;
      memcpy(cursor, src, n_bytes);
      cursor += n_bytes;
      off = fragchain->maxp1;
      assert(off <= size);
    }
    fragchain = fragchain->nextfrag;
  }

  HelloWorldData::read_struct(d->data(),buffer.data(),0);
#endif
  return d.release();
}

#if DDSI_SERDATA_HAS_FROM_SER_IOV
struct ddsi_serdata* helloworld_serdata::helloworld_serdata_from_ser_iov(
  const struct ddsi_sertopic* topic,
  enum ddsi_serdata_kind kind,
  ddsrt_msg_iovlen_t niov,
  const ddsrt_iovec_t* iov,
  size_t size)
{
  (void)topic;
  (void)kind;
  (void)niov;
  (void)iov;
  (void)size;
  assert(0);
  return NULL;
}
#endif

struct ddsi_serdata* helloworld_serdata::helloworld_serdata_from_keyhash(
  const struct ddsi_sertopic* topic,
  const struct ddsi_keyhash* keyhash)
{
  (void)keyhash;
  /* there is no key field, so from_keyhash is trivial */
  return new helloworld_serdata(topic, SDK_KEY);
}

void helloworld_serdata::resize(size_t requested_size)
{
  if (!requested_size) {
    m_size = 0;
    m_data.reset();
    return;
  }

  /* FIXME: CDR padding in DDSI makes me do this to avoid reading beyond the bounds
  when copying data to network.  Should fix Cyclone to handle that more elegantly.  */
  size_t n_pad_bytes = (0 - requested_size) % 4;
  m_data.reset(new byte[requested_size + n_pad_bytes]);
  m_size = requested_size + n_pad_bytes;

  // zero the very end. The caller isn't necessarily going to overwrite it.
  std::memset(byte_offset(m_data.get(), requested_size), '\0', n_pad_bytes);
}

struct ddsi_serdata* helloworld_serdata::helloworld_serdata_from_sample(
  const struct ddsi_sertopic* topiccmn,
  enum ddsi_serdata_kind kind,
  const void* sample)
{
  try {
    const helloworld_sertopic* topic = static_cast<const helloworld_sertopic*>(topiccmn);
    auto d = std::make_unique<helloworld_serdata>(topic, kind);

    if (kind != SDK_DATA) {
      //???
    }
    else /*if (!topic->is_request_header)*/ {
      const HelloWorldData::Msg *msg = static_cast<const HelloWorldData::Msg*>(sample);
      //HelloWorldData::read_struct(d->data(), sample, 0);
      //
      // I'm thinking reading is wrong. The "const void *" is the sample
      // passed to write...
      //
      size_t sz = HelloWorldData::write_size(*msg, 0);
      fprintf(stderr, "sz is: %zu\n", sz);
      d->resize(sz);
      fprintf(stderr, "message in serdata_from_sample is: %s\n", msg->message().c_str());
      HelloWorldData::write_struct(*msg, d->data(), 0);
    }
    //else {
      ///* inject the service invocation header data into the CDR stream --
      // * I haven't checked how it is done in the official RMW implementations, so it is
      // * probably incompatible. */
      //auto wrap = *static_cast<const cdds_request_wrapper_t*>(sample);
      //HelloWorldData::read_struct(d->data(), wrap, 0);
    //}

    return d.release();
  }
  catch (std::exception& e) {
    //RMW_SET_ERROR_MSG(e.what());
    return nullptr;
  }
}

void helloworld_serdata::helloworld_serdata_to_ser(const struct ddsi_serdata* dcmn, size_t off, size_t sz, void* buf)
{
  auto d = static_cast<const helloworld_serdata*>(dcmn);
  memcpy(buf, d->data(), sz);
}

struct ddsi_serdata* helloworld_serdata::helloworld_serdata_to_ser_ref(
  const struct ddsi_serdata * dcmn, size_t off,
  size_t sz, ddsrt_iovec_t * ref)
{
  auto d = static_cast<const helloworld_serdata*>(dcmn);
  uintptr_t a, b;
  a = (uintptr_t)d->data();
  b = (uintptr_t)byte_offset(d->data(), off);
  fprintf(stderr, "%llx - %llx = %llx\n", a, b, b - a);
  ref->iov_base = d->data();//byte_offset(d->data(), off);
  ref->iov_len = (ddsrt_iov_len_t)sz;
  return ddsi_serdata_ref(d);
}

void helloworld_serdata::helloworld_serdata_to_ser_unref(struct ddsi_serdata* dcmn, const ddsrt_iovec_t* ref)
{
  static_cast<void>(ref);    // unused
  ddsi_serdata_unref(static_cast<helloworld_serdata*>(dcmn));
}

bool helloworld_serdata::helloworld_serdata_to_sample(
  const struct ddsi_serdata* dcmn, void* sample, void** bufptr,
  void* buflim)
{
  (void)dcmn;
  (void)sample;
  (void)bufptr;
  (void)buflim;
  assert(0);
  return false;
}

struct ddsi_serdata* helloworld_serdata::helloworld_serdata_to_topicless(const struct ddsi_serdata* dcmn)
{
  auto d = static_cast<const helloworld_serdata*>(dcmn);
  auto d1 = new helloworld_serdata(d->topic, SDK_KEY);
  d1->topic = nullptr;
  return d1;
}

bool helloworld_serdata::helloworld_serdata_topicless_to_sample(
  const struct ddsi_sertopic* topic,
  const struct ddsi_serdata* dcmn, void* sample,
  void** bufptr, void* buflim)
{
  (void)topic;
  (void)dcmn;
  (void)sample;
  (void)bufptr;
  (void)buflim;
  assert(0);
  /* ROS 2 doesn't do keys in a meaningful way yet */
  return true;
}

void helloworld_serdata::helloworld_serdata_free(struct ddsi_serdata* dcmn)
{
  auto* d = static_cast<const helloworld_serdata*>(dcmn);
  delete d;
}

#if DDSI_SERDATA_HAS_PRINT
size_t helloworld_serdata::helloworld_serdata_print(
  const struct ddsi_sertopic* tpcmn, const struct ddsi_serdata* dcmn, char* buf, size_t bufsize)
{
  (void)tpcmn;
  (void)dcmn;
  (void)buf;
  (void)bufsize;
  assert(0);
  return 0;
}
#endif

#if DDSI_SERDATA_HAS_GET_KEYHASH
void helloworld_serdata::helloworld_serdata_get_keyhash(
  const struct ddsi_serdata* d, struct ddsi_keyhash* buf,
  bool force_md5)
{
  (void)d;
  (void)buf;
  (void)force_md5;
  assert(0);
}
#endif

const struct ddsi_serdata_ops helloworld_serdata::helloworld_serdata_ops = {
  &helloworld_serdata::helloworld_serdata_eqkey,
  &helloworld_serdata::helloworld_serdata_size,
  &helloworld_serdata::helloworld_serdata_from_ser,
#if DDSI_SERDATA_HAS_FROM_SER_IOV
  &helloworld_serdata::helloworld_serdata_from_ser_iov,
#endif
  &helloworld_serdata::helloworld_serdata_from_keyhash,
  &helloworld_serdata::helloworld_serdata_from_sample,
  &helloworld_serdata::helloworld_serdata_to_ser,
  &helloworld_serdata::helloworld_serdata_to_ser_ref,
  &helloworld_serdata::helloworld_serdata_to_ser_unref,
  &helloworld_serdata::helloworld_serdata_to_sample,
  &helloworld_serdata::helloworld_serdata_to_topicless,
  &helloworld_serdata::helloworld_serdata_topicless_to_sample,
  &helloworld_serdata::helloworld_serdata_free
#if DDSI_SERDATA_HAS_PRINT
  , &helloworld_serdata::helloworld_serdata_print
#endif
#if DDSI_SERDATA_HAS_GET_KEYHASH
  , &helloworld_serdata::helloworld_serdata_get_keyhash
#endif
};

helloworld_serdata::helloworld_serdata(const ddsi_sertopic* topic, ddsi_serdata_kind kind)
  : ddsi_serdata{}
{
  ddsi_serdata_init(this, topic, kind);
}

const struct ddsi_sertopic_ops helloworld_sertopic::helloworld_sertopic_ops = {
  &helloworld_sertopic::helloworld_sertopic_free,
  &helloworld_sertopic::helloworld_sertopic_zero_samples,
  &helloworld_sertopic::helloworld_sertopic_realloc_samples,
  &helloworld_sertopic::helloworld_sertopic_free_samples
#if DDSI_SERTOPIC_HAS_EQUAL_AND_HASH
  , &helloworld_sertopic::helloworld_sertopic_equal,
  &helloworld_sertopic::helloworld_sertopic_hash
#endif
};

helloworld_sertopic* helloworld_sertopic::create_sertopic(
  const std::string& topic_name, const char *type_name)
{
  auto st = new helloworld_sertopic();
  ddsi_sertopic_init(
    static_cast<struct ddsi_sertopic*>(st),
    topic_name.c_str(),
    type_name,
    &helloworld_sertopic_ops,
    &helloworld_serdata::helloworld_serdata_ops,
    true);

  //st->type_support.typesupport_identifier_ = type_support_identifier;
  //st->type_support.type_support_ = type_support;
  //st->is_request_header = is_request_header;  //???
  //st->cdr_writer = rmw_cyclonedds_cpp::make_cdr_writer(std::move(message_type)); //???
  return st;
}

void helloworld_sertopic::helloworld_sertopic_free(struct ddsi_sertopic* tpcmn)
{
  helloworld_sertopic* tp = static_cast<helloworld_sertopic*>(tpcmn);
#if DDSI_SERTOPIC_HAS_TOPICKIND_NO_KEY
  ddsi_sertopic_fini(tpcmn);
#endif

  delete tp;
}

void helloworld_sertopic::helloworld_sertopic_zero_samples(const struct ddsi_sertopic* d, void* samples, size_t count)
{
  (void)d;
  (void)samples;
  (void)count;
  assert(0);
}

void helloworld_sertopic::helloworld_sertopic_realloc_samples(
  void** ptrs, const struct ddsi_sertopic* d, void* old,
  size_t oldcount, size_t count)
{
  (void)ptrs;
  (void)d;
  (void)old;
  (void)oldcount;
  (void)count;
  assert(0);
}

void helloworld_sertopic::helloworld_sertopic_free_samples(
  const struct ddsi_sertopic* d, void** ptrs, size_t count,
  dds_free_op_t op)
{
  (void)d;
  (void)ptrs;
  (void)count;
  (void)op;
  assert(0);
}

#if DDSI_SERTOPIC_HAS_EQUAL_AND_HASH
bool helloworld_sertopic::helloworld_sertopic_equal(
  const struct ddsi_sertopic* acmn, const struct ddsi_sertopic* bcmn)
{
  /* A bit of a guess: topics with the same name & type name are really the same if they have
   the same type support identifier as well */
  (void)acmn;
  (void)bcmn;
  assert(0);
  return true;
}

uint32_t helloworld_sertopic::helloworld_sertopic_hash(const struct ddsi_sertopic* tpcmn)
{
  (void)tpcmn;
  return 0x0;
}
#endif
