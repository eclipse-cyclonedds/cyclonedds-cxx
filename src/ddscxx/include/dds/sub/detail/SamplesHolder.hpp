// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

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

    uint32_t get_length() const {
        return this->index_;
    }

    SamplesHolder& operator++(int)
    {
        this->index_++;
        return *this;
    }

    void append_sample(void *sample, const dds_sample_info_t *si)
    {
        ddscxx_serdata<T> *sd = static_cast<ddscxx_serdata<T>*>(sample);
        latest_sample.delegate().data_ptr(sd);
        latest_sample.delegate().info(sample_info_from_c(si));
        samples_.delegate()->append_sample(latest_sample);
    }

private:
    dds::sub::LoanedSamples<T>& samples_;
    dds::sub::SampleRef<T> latest_sample;
    uint32_t index_;
};

class CDRSamplesHolder : public SamplesHolder
{
public:
    CDRSamplesHolder(dds::sub::LoanedSamples<org::eclipse::cyclonedds::topic::CDRBlob>& samples) : samples_(samples), index_(0)
    {
    }

    uint32_t get_length() const {
        return this->index_;
    }

    SamplesHolder& operator++(int)
    {
        this->index_++;
        return *this;
    }

    void append_sample(void *sample, const dds_sample_info_t *si)
    {
        ddsrt_iovec_t blob_content;
        ddscxx_serdata<org::eclipse::cyclonedds::topic::CDRBlob> *sd;
        org::eclipse::cyclonedds::topic::CDRBlob emptyBlob;
        dds::sub::Sample<org::eclipse::cyclonedds::topic::CDRBlob, dds::sub::detail::Sample> *buffer;

        sd = static_cast<ddscxx_serdata<org::eclipse::cyclonedds::topic::CDRBlob> *>(sample);
        dds::sub::Sample<org::eclipse::cyclonedds::topic::CDRBlob, dds::sub::detail::Sample> blob_ssmple(
            emptyBlob, sample_info_from_c(si));
        samples_.delegate()->append_sample(blob_ssmple);
        ddsi_serdata_to_ser_ref(sd, 0, ddsi_serdata_size(sd), &blob_content);
        buffer = samples_.delegate()->get_buffer();
        org::eclipse::cyclonedds::topic::CDRBlob &sample_data = buffer[samples_.length() - 1].delegate().data();
        copy_buffer_to_cdr_blob(reinterpret_cast<uint8_t *>(blob_content.iov_base),
                                blob_content.iov_len, sample_data.kind(), sample_data);
        ddsi_serdata_to_ser_unref(sd, &blob_content);
        ddsi_serdata_unref(sd);
    }

private:
    dds::sub::LoanedSamples<org::eclipse::cyclonedds::topic::CDRBlob>& samples_;
    uint32_t index_;

    void copy_buffer_to_cdr_blob(const uint8_t * buffer, const size_t size,
                                 const org::eclipse::cyclonedds::topic::BlobKind data_kind,
                                 org::eclipse::cyclonedds::topic::CDRBlob & cdr_blob) const
    {
        // update the CDR header
        memcpy(cdr_blob.encoding().data(), buffer, CDR_HEADER_SIZE);
        // if the data kind is not empty
        if (data_kind != org::eclipse::cyclonedds::topic::BlobKind::Empty) {
            // get the actual data from the buffer
            cdr_blob.payload().assign(buffer + CDR_HEADER_SIZE, buffer + size);
        }
    }

  bool update_cdrblob_from_iox_chunk (ddsi_serdata & current_blob,
                                      org::eclipse::cyclonedds::topic::CDRBlob &sample_data) {
#ifdef DDSCXX_HAS_SHM
        // if the data is available on SHM
        if (current_blob.iox_chunk && current_blob.iox_subscriber) {
            // get the user iox header
            auto iox_header = iceoryx_header_from_chunk(current_blob.iox_chunk);
            // if the iox chunk has the data in serialized form
            if (iox_header->shm_data_state == IOX_CHUNK_CONTAINS_SERIALIZED_DATA) {
              copy_buffer_to_cdr_blob(reinterpret_cast<uint8_t *>(current_blob.iox_chunk),
                                      iox_header->data_size, sample_data.kind(), sample_data);
            } else if (iox_header->shm_data_state == IOX_CHUNK_CONTAINS_RAW_DATA) {
              // serialize the data
              auto serialized_size = ddsi_sertype_get_serialized_size(current_blob.type,
                                                                      current_blob.iox_chunk);
              // create a buffer to serialize
              std::vector<uint8_t> buffer(serialized_size);
              // serialize into the buffer
              ddsi_sertype_serialize_into(current_blob.type, current_blob.iox_chunk, buffer.data(),
                                          serialized_size);
              // update the CDR blob with the serialized data
              copy_buffer_to_cdr_blob(buffer.data(), serialized_size, sample_data.kind(), sample_data);
            } else {
              // this shouldn't never happen
              ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                                     "The received sample over SHM is not initialized");
            }
            // release the chunk
            free_iox_chunk(static_cast<iox_sub_t *>(current_blob.iox_subscriber), &current_blob.iox_chunk);
            return true;
        } else {
          return false;
        }
#else
        (void) current_blob;
        (void) sample_data;
        return false;
#endif  // DDSCXX_HAS_SHM
    }
};

template <typename T, typename SamplesFWIterator>
class SamplesFWInteratorHolder : public SamplesHolder
{
public:
    SamplesFWInteratorHolder(SamplesFWIterator& it) : iterator(it), size(0)
    {
    }

    uint32_t get_length() const {
        return this->size;
    }

    SamplesHolder& operator++(int)
    {
        ++this->iterator;
        return *this;
    }

    void append_sample(void *sample, const dds_sample_info_t *si)
    {
        ddscxx_serdata<T>* sd = static_cast<ddscxx_serdata<T>*>(sample);
        (iterator++)->delegate() = dds::sub::detail::Sample<T>(*sd->getT(), sample_info_from_c(si));
        ++size;
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

    uint32_t get_length() const {
        return this->size;
    }

    SamplesHolder& operator++(int)
    {
        ++this->iterator;
        return *this;
    }

    void append_sample(void *sample, const dds_sample_info_t *si)
    {
        ddscxx_serdata<T>* sd = static_cast<ddscxx_serdata<T>*>(sample);
        last_sample.delegate().data() = *sd->getT();
        last_sample.delegate().info(sample_info_from_c(si));
        iterator = std::move(last_sample);
        ++iterator;
        ++size;
    }

private:
    SamplesBIIterator& iterator;
    dds::sub::Sample<T> last_sample;
    uint32_t size;

};

}
}
}




#endif /* CYCLONEDDS_DDS_SUB_DETAIL_SAMPLES_HOLDER_HPP_ */
