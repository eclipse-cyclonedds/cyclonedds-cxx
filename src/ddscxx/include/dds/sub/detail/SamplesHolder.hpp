/*
 * Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef CYCLONEDDS_DDS_SUB_DETAIL_SAMPLES_HOLDER_HPP_
#define CYCLONEDDS_DDS_SUB_DETAIL_SAMPLES_HOLDER_HPP_

/**
 * @file
 */

#include <dds/sub/LoanedSamples.hpp>
#include "org/eclipse/cyclonedds/sub/AnyDataReaderDelegate.hpp"
#include "org/eclipse/cyclonedds/topic/BuiltinDataTopic.hpp"

namespace dds
{
namespace sub
{
namespace detail
{

template <typename T>
class LoanedSamplesHolder : public SamplesHolder
{
public:
    LoanedSamplesHolder(dds::sub::LoanedSamples<T>& samples) : samples_(samples), index_(0)
    {
    }

    void set_length(uint32_t len) {
        this->samples_.delegate()->resize(len);
    }

    uint32_t get_length() const {
        return this->index_;
    }

    SamplesHolder& operator++(int)
    {
        this->index_++;
        return *this;
    }

    void *data()
    {
        return (*this->samples_.delegate())[this->index_].delegate().data_ptr();
    }

    detail::SampleInfo& info()
    {
        return (*this->samples_.delegate())[this->index_].delegate().info();
    }

    void **cpp_sample_pointers(size_t length)
    {

        return new void * [length];
    }

    dds_sample_info_t *cpp_info_pointers(size_t length)
    {
        return new dds_sample_info_t[length];
    }

    void set_sample_contents(void** c_sample_pointers, dds_sample_info_t *info)
    {
        uint32_t cpp_sample_size = this->samples_.delegate()->length();
        auto tmp_iterator = samples_.delegate()->mbegin();
        for (uint32_t i = 0; i < cpp_sample_size; ++i, ++tmp_iterator) {
            /* Transfer ownership for ddscxx_serdata from c_sample_pointers to SampleRef objects. */
            tmp_iterator->delegate().data_ptr() = static_cast<ddscxx_serdata<T>*>(c_sample_pointers[i]);
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], tmp_iterator->delegate().info());
        }
    }

    template<typename T_ = T, IsBuiltinTopicType<T_> = true>
    void set_builtin_sample_contents(void** cxx_sample_pointers, dds_sample_info_t *info)
    {
        set_sample_contents(cxx_sample_pointers, info);
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        delete [] c_sample_pointers;
        delete [] c_sample_infos;
    }

private:
    dds::sub::LoanedSamples<T>& samples_;
    uint32_t index_;
};

class CDRSamplesHolder : public SamplesHolder
{
public:
    CDRSamplesHolder(dds::sub::LoanedSamples<org::eclipse::cyclonedds::topic::CDRBlob>& samples) : samples_(samples), index_(0)
    {
    }

    void set_length(uint32_t len) {
        this->samples_.delegate()->resize(len);
    }

    uint32_t get_length() const {
        return this->index_;
    }

    SamplesHolder& operator++(int)
    {
        this->index_++;
        return *this;
    }

    void *data()
    {
        return (*this->samples_.delegate())[this->index_].delegate().data_ptr();
    }

    detail::SampleInfo& info()
    {
        return (*this->samples_.delegate())[this->index_].delegate().info();
    }

    void **cpp_sample_pointers(size_t length)
    {
        return new void * [length];
    }

    dds_sample_info_t *cpp_info_pointers(size_t length)
    {
        return new dds_sample_info_t[length];
    }

    void set_sample_contents(void** c_sample_pointers, dds_sample_info_t *info)
    {
      struct ddsi_serdata **cdr_blobs = reinterpret_cast<struct ddsi_serdata **>(c_sample_pointers);
      uint32_t cpp_sample_size = this->samples_.delegate()->length();
      for (uint32_t i = 0; i < cpp_sample_size; ++i)
      {
          ddsrt_iovec_t blob_content;
          struct ddsi_serdata *current_blob = cdr_blobs[i];
          ddsi_serdata_to_ser_ref(current_blob, 0, ddsi_serdata_size(current_blob), &blob_content);
          org::eclipse::cyclonedds::topic::CDRBlob &sample_data = (*this->samples_.delegate())[i].delegate().data();
          memcpy(sample_data.encoding().data(), blob_content.iov_base, 4);
          sample_data.kind(static_cast<org::eclipse::cyclonedds::topic::BlobKind>(current_blob->kind));
          if (sample_data.kind() != org::eclipse::cyclonedds::topic::BlobKind::Empty)
          {
              sample_data.payload().assign(
                      reinterpret_cast<uint8_t *>(blob_content.iov_base) + 4,
                      reinterpret_cast<uint8_t *>(blob_content.iov_base) + blob_content.iov_len);
          }
          ddsi_serdata_to_ser_unref(current_blob, &blob_content);
          ddsi_serdata_unref(current_blob);
          org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], (*samples_.delegate())[i].delegate().info());
      }
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        delete [] c_sample_pointers;
        delete [] c_sample_infos;
    }

private:
    dds::sub::LoanedSamples<org::eclipse::cyclonedds::topic::CDRBlob>& samples_;
    uint32_t index_;
};

template <typename T, typename SamplesFWIterator>
class SamplesFWIteratorHolder : public SamplesHolder
{
public:
    SamplesFWIteratorHolder(SamplesFWIterator& it) : iterator(it), size(0)
    {
    }

    void set_length(uint32_t len) {
        this->size = len;

    }

    uint32_t get_length() const {
        return this->size;
    }

    SamplesHolder& operator++(int)
    {
        ++this->iterator;
        return *this;
    }

    void *data()
    {
        return (*iterator).delegate().data_ptr();
    }

    detail::SampleInfo& info()
    {
        return (*iterator).delegate().info();
    }

    void **cpp_sample_pointers(size_t length)
    {
        void **c_sample_pointers = new void * [length];
        SamplesFWIterator tmp_iterator = iterator;
        for (uint32_t i = 0; i < length; ++i, ++tmp_iterator) {
            c_sample_pointers[i] = (*tmp_iterator).delegate().data_ptr();
        }
        return c_sample_pointers;
    }

    dds_sample_info_t *cpp_info_pointers(size_t length)
    {
      return new dds_sample_info_t[length];
    }

    template<typename T_ = T, IsBuiltinTopicType<T_> = true>
    void set_builtin_sample_contents(void** cxx_sample_pointers, dds_sample_info_t *info)
    {
        /* Samples have already been deserialized in their containers during the read/take call. */
        SamplesFWIterator tmp_iterator = iterator;
        for (uint32_t i = 0; i < size; ++i, ++tmp_iterator) {
            auto ptr = static_cast<ddscxx_serdata<T>*>(cxx_sample_pointers[i]);
            tmp_iterator->delegate().data() = *ptr->getT();
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], tmp_iterator->delegate().info());
            delete(ptr);
            cxx_sample_pointers[i] = nullptr;
        }
    }

    void set_sample_contents(void**, dds_sample_info_t *info)
    {
        /* Samples have already been deserialized in their containers during the read/take call. */
        SamplesFWIterator tmp_iterator = iterator;
        for (uint32_t i = 0; i < size; ++i, ++tmp_iterator) {
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], tmp_iterator->delegate().info());
        }
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        delete [] c_sample_infos;
        delete [] c_sample_pointers;
    }

private:
    SamplesFWIterator& iterator;
    uint32_t size;

};

template <typename T, typename SamplesBIIterator>
class SamplesBIIteratorHolder : public SamplesHolder
{
public:
    SamplesBIIteratorHolder(SamplesBIIterator& it) : iterator(it), size(0)
    {
    }

    void set_length(uint32_t len) {
        this->size = len;
        samples.resize(len);
    }

    uint32_t get_length() const {
        return this->size;
    }

    SamplesHolder& operator++(int)
    {
        ++this->iterator;
        return *this;
    }

    void *data()
    {
        return this->samples[0].delegate().data_ptr();
    }

    detail::SampleInfo& info()
    {
        return this->samples[0].delegate().info();
    }

    void **cpp_sample_pointers(size_t length)
    {
        set_length(static_cast<uint32_t>(length));
        void **c_sample_pointers = new void*[length];
        for (uint32_t i = 0; i < length; ++i) {
          c_sample_pointers[i] = samples[i].delegate().data_ptr();
        }
        return c_sample_pointers;
    }

    dds_sample_info_t *cpp_info_pointers(size_t length)
    {
        dds_sample_info_t *c_info_pointers = new dds_sample_info_t[length];
        return c_info_pointers;
    }

    template<typename T_ = T, IsBuiltinTopicType<T_> = true>
    void set_builtin_sample_contents(void** cxx_sample_pointers, dds_sample_info_t *info)
    {
        /* Samples have already been deserialized in their containers during the read/take call. */
        for (uint32_t i = 0; i < size; ++i, ++iterator) {
            auto ptr = static_cast<ddscxx_serdata<T>*>(cxx_sample_pointers[i]);
            samples[i].data(*ptr->getT());
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], samples[i].delegate().info());
            iterator = samples[i];
            delete(ptr);
            cxx_sample_pointers[i] = nullptr;
        }
    }

    void set_sample_contents(void**, dds_sample_info_t *info)
    {
        /* Samples have already been deserialized in their containers during the read/take call. */
        for (uint32_t i = 0; i < size; ++i) {
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], samples[i].delegate().info());
            this->iterator = std::move(samples[i]);
            this->iterator++;
        }
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        delete [] c_sample_infos;
        delete [] c_sample_pointers;
    }

private:
    SamplesBIIterator& iterator;
    std::vector< dds::sub::Sample<T, dds::sub::detail::Sample> > samples;
    uint32_t size;

};

}
}
}




#endif /* CYCLONEDDS_DDS_SUB_DETAIL_SAMPLES_HOLDER_HPP_ */
