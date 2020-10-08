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
#include "org/eclipse/cyclonedds/topic/datatopic.hpp"

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

    detail::SampleInfo* info()
    {
        return (*this->samples_.delegate())[this->index_].delegate().info_ptr();
    }

    void **cpp_sample_pointers()
    {

        uint32_t cpp_sample_size = this->samples_.delegate()->length();
        void **c_sample_pointers = new void * [cpp_sample_size];
        for (uint32_t i = 0; i < cpp_sample_size; ++i) {
            c_sample_pointers[i] = (*samples_.delegate())[i].delegate().data_ptr();
        }
        return c_sample_pointers;
    }

    dds_sample_info_t *cpp_info_pointers()
    {
        uint32_t cpp_sample_size = this->samples_.delegate()->length();
        dds_sample_info_t *c_info_pointers = new dds_sample_info_t[cpp_sample_size];
        return c_info_pointers;
    }

    void set_sample_infos(dds_sample_info_t *info)
    {
        uint32_t cpp_sample_size = this->samples_.delegate()->length();
        for (uint32_t i = 0; i < cpp_sample_size; ++i) {
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], (*samples_.delegate())[i].delegate().info());
        }
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        delete [] c_sample_infos;
        delete [] c_sample_pointers;
    }

protected:
    dds::sub::LoanedSamples<T>& samples_;
    uint32_t index_;
};

template <typename T>
class LoanedCDRSamplesHolder : public LoanedSamplesHolder<T>
{
public:
    LoanedCDRSamplesHolder(dds::sub::LoanedSamples<T>& samples) : LoanedSamplesHolder<T>(samples)
    {
    }

    void set_length(uint32_t len) {
        LoanedSamplesHolder<T>::set_length(len);
        this->index_ = len;
    }

    void **cpp_sample_pointers()
    {
        uint32_t cpp_sample_size = this->samples_.delegate()->length();
        return new void * [cpp_sample_size];
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        struct ddsi_serdata **cdr_blobs = (struct ddsi_serdata **) c_sample_pointers;
        for (uint32_t i = 0; i < this->index_; ++i)
        {
            ddsrt_iovec_t blob_content;
            struct ddsi_serdata *current_blob = cdr_blobs[i];
            ddsi_serdata_to_ser_ref(current_blob, 0, ddsi_serdata_size(current_blob), &blob_content);
            T &sample_data = (*this->samples_.delegate())[i].delegate().data();
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
        }
        delete [] c_sample_infos;
        delete [] c_sample_pointers;
    }
};

template <typename T, typename SamplesFWIterator>
class SamplesFWInteratorHolder : public SamplesHolder
{
public:
    SamplesFWInteratorHolder(SamplesFWIterator& it) : iterator(it), length(0)
    {
    }

    void set_length(uint32_t len) {
        this->length = len;

    }

    uint32_t get_length() const {
        return this->length;
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

    detail::SampleInfo* info()
    {
        return (*iterator).delegate().info_ptr();
    }

    void **cpp_sample_pointers()
    {
        void **c_sample_pointers = new void * [length];
        SamplesFWIterator tmp_iterator = iterator;
        for (uint32_t i = 0; i < length; ++i, ++tmp_iterator) {
            c_sample_pointers[i] = (*tmp_iterator).delegate().data_ptr();
        }
        return c_sample_pointers;
    }

    dds_sample_info_t *cpp_info_pointers()
    {
      dds_sample_info_t *c_info_pointers = new dds_sample_info_t[length];
      return c_info_pointers;
    }

    void set_sample_infos(dds_sample_info_t *info)
    {
        SamplesFWIterator tmp_iterator = iterator;
        for (uint32_t i = 0; i < length; ++i, ++tmp_iterator) {
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], (tmp_iterator->delegate()).info());
        }
    }

    void fini_samples_buffers(void**& c_sample_pointers, dds_sample_info_t*& c_sample_infos)
    {
        delete [] c_sample_infos;
        delete [] c_sample_pointers;
    }

private:
    SamplesFWIterator& iterator;
    uint32_t length;

};

template <typename T, typename SamplesBIIterator>
class SamplesBIIteratorHolder : public SamplesHolder
{
public:
    SamplesBIIteratorHolder(SamplesBIIterator& it) : iterator(it), length(0)
    {
    }

    void set_length(uint32_t len) {
        this->length = len;
        samples.resize(len);
    }

    uint32_t get_length() const {
        return this->length;
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

    detail::SampleInfo* info()
    {
        return this->samples[0].delegate().info_ptr();
    }

    void **cpp_sample_pointers()
    {
        void **c_sample_pointers = new void*[length];
        for (uint32_t i = 0; i < length; ++i) {
          c_sample_pointers[i] = samples[i].delegate().data_ptr();
        }
        return c_sample_pointers;
    }

    dds_sample_info_t *cpp_info_pointers()
    {
        dds_sample_info_t *c_info_pointers = new dds_sample_info_t[length];
        return c_info_pointers;
    }

    void set_sample_infos(dds_sample_info_t *info)
    {
        for (uint32_t i = 0; i < length; ++i) {
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::copy_sample_infos(info[i], *(samples[i].delegate().info_ptr()));
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
    uint32_t length;

};

}
}
}




#endif /* CYCLONEDDS_DDS_SUB_DETAIL_SAMPLES_HOLDER_HPP_ */
