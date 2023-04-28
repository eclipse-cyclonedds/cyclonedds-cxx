// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "Space.hpp"

/**
 * Trying to use the operator>> to get the QoS, causes a compile error because
 * it can't decide to choose between operator>>(qos) and operator>>(functor).
 */
//#define TEST_QOS_SHIFT_OUT



/**
 * Dummy listener for the DataWriter tests
 */
class TestReader1Listener : public virtual dds::sub::NoOpDataReaderListener<Space::Type1>{ };

static TestReader1Listener reader1Listener;



/**
 * Fixture for the DataReader tests
 */
class DataReader : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::sub::Subscriber subscriber;
    dds::pub::Publisher publisher;
    dds::topic::Topic<Space::Type1> topic;
    dds::sub::DataReader<Space::Type1> reader;
    dds::pub::DataWriter<Space::Type1> writer;

    dds::sub::qos::DataReaderQos reliable_qos;
    dds::sub::qos::DataReaderQos timebased_qos;

    std::string partition;

    DataReader() :
        participant(dds::core::null),
        subscriber(dds::core::null),
        publisher(dds::core::null),
        topic(dds::core::null),
        reader(dds::core::null),
        writer(dds::core::null),
        reliable_qos(),
        timebased_qos(),
        partition("DataReader_test")
    {
    }

    void SetUp() {
    }

    void CreateParticipant() {
        if (this->participant == dds::core::null) {
            this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
            ASSERT_NE(this->participant, dds::core::null);
        }
    }

    void CreateTopic() {
        if (this->topic == dds::core::null) {
            this->CreateParticipant();
            this->topic = dds::topic::Topic<Space::Type1>(this->participant, "datareader_test_topic");
            ASSERT_NE(this->topic, dds::core::null);
        }
    }

    void CreateWriter() {
        this->SetupWriter();
        if (this->writer == dds::core::null) {
            dds::pub::qos::DataWriterQos qos =
                    this->publisher.default_datawriter_qos() <<
                    dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
            this->writer = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic, qos);
            ASSERT_NE(this->writer, dds::core::null);
        }
    }

    void CreateReader() {
        this->SetupReader();
        if (this->reader == dds::core::null) {
            this->reader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic);
            ASSERT_NE(this->reader, dds::core::null);
        }
    }

    void SetupWriter() {
        this->CreateTopic();
        if (this->publisher == dds::core::null) {
            dds::pub::qos::PublisherQos pub_qos =
                    this->participant.default_publisher_qos() <<
                    dds::core::policy::Partition(this->partition);
            this->publisher = dds::pub::Publisher(this->participant, pub_qos);
        }
    }

    void SetupReader() {
        this->CreateTopic();
        if (this->subscriber == dds::core::null) {
            dds::sub::qos::SubscriberQos sub_qos =
                    this->participant.default_subscriber_qos() <<
                    dds::core::policy::Partition(this->partition);
            this->subscriber = dds::sub::Subscriber(this->participant, sub_qos);

            this->reliable_qos = this->subscriber.default_datareader_qos();
            this->reliable_qos.policy(dds::core::policy::Reliability::Reliable());

            this->timebased_qos = this->subscriber.default_datareader_qos();
            this->timebased_qos.policy(dds::core::policy::TimeBasedFilter(dds::core::Duration(10, 0)));
        }
    }

    void SetupCommunication() {
        this->CreateWriter();
        this->CreateReader();
    }

    std::vector<Space::Type1> WriteData(int32_t instances_cnt) {
        std::vector<Space::Type1> samples;
        for (int32_t i = 0; i < instances_cnt; i++) {
            samples.push_back(Space::Type1(i, i+1, i+2));
        }
        this->SetupCommunication();
        for (size_t i = 0; i < samples.size(); i++) {
            this->writer.write(samples[i]);
        }
        return samples;
    }

    void ReadFirstSamples(uint32_t cnt) {
        std::vector<dds::sub::Sample<Space::Type1> > samples(cnt);
        this->reader.read(samples.begin(), cnt);
    }

    void
    CheckData (
        const dds::sub::LoanedSamples<Space::Type1> &samples,
        const std::vector<Space::Type1>& test_data,
        const dds::sub::status::DataState& test_state =
                dds::sub::status::DataState(dds::sub::status::SampleState::not_read(),
                dds::sub::status::ViewState::new_view(),
                dds::sub::status::InstanceState::alive()))
    {
        unsigned long count = 0UL;
        ASSERT_EQ(samples.length(), test_data.size());
        dds::sub::LoanedSamples<Space::Type1>::const_iterator it;
        for (it = samples.begin(); it != samples.end(); ++it, ++count) {
            const Space::Type1& data = it->data();
            const dds::sub::SampleInfo& info = it->info();
            const dds::sub::status::DataState& state = info.state();
            ASSERT_EQ(data, test_data[count]);
            ASSERT_EQ(state.view_state(), test_state.view_state());
            ASSERT_EQ(state.sample_state(), test_state.sample_state());
            ASSERT_EQ(state.instance_state(), test_state.instance_state());
        }
    }

    void
    CheckData (
        const std::vector<dds::sub::Sample<Space::Type1> > &samples,
        const std::vector<Space::Type1>& test_data)
    {
        unsigned long count = 0UL;
        std::vector<dds::sub::Sample<Space::Type1> >::const_iterator it;
        ASSERT_EQ(samples.size(), test_data.size());
        for (it = samples.begin(); it != samples.end(); ++it, ++count) {
            const Space::Type1& data = it->data();
            ASSERT_EQ(data, test_data[count]);
        }
    }



    void TearDown() {
        this->writer = dds::core::null;
        this->reader = dds::core::null;
        this->topic = dds::core::null;
        this->publisher = dds::core::null;
        this->subscriber = dds::core::null;
        this->participant = dds::core::null;
    }

};



/**
 * Tests
 */



/**
 * Tests
 */

TEST_F(DataReader, null)
{
    dds::sub::DataReader<Space::Type1> reader1(dds::core::null);
    dds::sub::DataReader<Space::Type1> reader2 = dds::core::null;
    ASSERT_EQ(reader1, dds::core::null);
    ASSERT_EQ(reader2, dds::core::null);
}


TEST_F(DataReader, create_subscriber_null)
{
    this->CreateTopic();
    dds::sub::DataReader<Space::Type1> treader = dds::core::null;

    ASSERT_THROW({
        treader = dds::sub::DataReader<Space::Type1>(dds::core::null, this->topic);
    }, dds::core::NullReferenceError);
}


TEST_F(DataReader, create_topic_null)
{
    this->SetupReader();
    dds::topic::Topic<Space::Type1> null_topic = dds::core::null;
    dds::sub::DataReader<Space::Type1> treader = dds::core::null;

    ASSERT_THROW({
        treader = dds::sub::DataReader<Space::Type1>(this->subscriber, null_topic);
    }, dds::core::NullReferenceError);
}


TEST_F(DataReader, create_cftopic_null)
{
    this->SetupReader();
    dds::topic::ContentFilteredTopic<Space::Type1> null_topic = dds::core::null;
    dds::sub::DataReader<Space::Type1> treader = dds::core::null;

    ASSERT_THROW({
        treader = dds::sub::DataReader<Space::Type1>(this->subscriber, null_topic);
    }, dds::core::NullReferenceError);
}


TEST_F(DataReader, create)
{
    this->SetupReader();
    dds::sub::DataReader<Space::Type1> treader = dds::core::null;
    treader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic);
    ASSERT_NE(treader, dds::core::null);
}


TEST_F(DataReader, subscriber)
{
    this->CreateReader();
    dds::sub::Subscriber tsub = this->reader.subscriber();
    ASSERT_NE(tsub, dds::core::null);
    ASSERT_EQ(tsub, this->subscriber);
}


TEST_F(DataReader, topic_description)
{
    this->CreateReader();
    dds::topic::TopicDescription tdesc = this->reader.topic_description();
    ASSERT_NE(tdesc, dds::core::null);
    ASSERT_EQ(tdesc, this->topic);
}


TEST_F(DataReader, qos_default)
{
#ifdef TEST_QOS_SHIFT_OUT
    dds::sub::qos::DataReaderQos shift_qos;
#endif
    dds::sub::qos::DataReaderQos dflt_qos;
    dds::sub::qos::DataReaderQos get_qos;
    this->CreateReader();
    get_qos = this->reader.qos();
#ifdef TEST_QOS_SHIFT_OUT
    this->reader >> shift_qos;
    ASSERT_EQ(get_qos, shift_qos);
#endif
    ASSERT_EQ(get_qos, dflt_qos);
    ASSERT_EQ(get_qos, this->subscriber.default_datareader_qos());
    ASSERT_NE(get_qos, this->reliable_qos);
}


TEST_F(DataReader, qos_nondefault_constructor)
{
#ifdef TEST_QOS_SHIFT_OUT
    dds::sub::qos::DataReaderQos shift_qos;
#endif
    dds::sub::qos::DataReaderQos get_qos;
    this->CreateReader();
    dds::sub::DataReader<Space::Type1> rdr = dds::core::null;
    dds::core::status::StatusMask mask;
    rdr = dds::sub::DataReader<Space::Type1>(
                                this->subscriber,
                                this->topic,
                                this->timebased_qos,
                                &reader1Listener,
                                mask);
    ASSERT_NE(rdr, dds::core::null);
    get_qos = rdr.qos();
#ifdef TEST_QOS_SHIFT_OUT
    rdr >> shift_qos;
    ASSERT_EQ(get_qos, shift_qos);
#endif
    ASSERT_EQ(get_qos, this->timebased_qos);
    ASSERT_NE(get_qos, this->subscriber.default_datareader_qos());
}


TEST_F(DataReader, qos_immutable_constructor)
{
    this->CreateReader();
    dds::sub::DataReader<Space::Type1> rdr = dds::core::null;
    dds::core::status::StatusMask mask;
    rdr = dds::sub::DataReader<Space::Type1>(
                                this->subscriber,
                                this->topic,
                                this->reliable_qos,
                                &reader1Listener,
                                mask);
    ASSERT_NE(rdr, dds::core::null);
    ASSERT_EQ(rdr.qos(), this->reliable_qos);
    ASSERT_NE(rdr.qos(), this->subscriber.default_datareader_qos());
}


TEST_F(DataReader, qos_nondefault_set)
{
    this->CreateReader();
    this->reader.qos(this->timebased_qos);
    ASSERT_EQ(this->reader.qos(), this->timebased_qos);
    ASSERT_NE(this->reader.qos(), this->subscriber.default_datareader_qos());
}


TEST_F(DataReader, qos_immutable_set)
{
    this->CreateReader();
    ASSERT_THROW({
        this->reader.qos(this->reliable_qos);
    }, dds::core::ImmutablePolicyError);
}


TEST_F(DataReader, qos_nondefault_shift)
{
    this->CreateReader();
    this->reader << this->timebased_qos;
    ASSERT_EQ(this->reader.qos(), this->timebased_qos);
    ASSERT_NE(this->reader.qos(), this->subscriber.default_datareader_qos());
}


TEST_F(DataReader, qos_immutable_shift)
{
    this->CreateReader();
    ASSERT_THROW({
        this->reader << this->reliable_qos;
    }, dds::core::ImmutablePolicyError);
}


TEST_F(DataReader, read)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create and write data. */
    test_samples = this->WriteData(5);

    /* Check result by reading. */
    samples = this->reader.read();
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, read_no_data)
{
    dds::sub::LoanedSamples<Space::Type1> samples;

    /* Create reader. */
    this->CreateReader();

    /* Check result by reading. */
    samples = this->reader.read();
    ASSERT_EQ(samples.length(), 0);
}


TEST_F(DataReader, read_shift_LoanedSamples)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create and write data. */
    test_samples = this->WriteData(5);

    /* Check result by reading. */
    this->reader >> samples;
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, read_SamplesFWIterator)
{
    static const uint32_t MAX_INSTANCES = 5;
    std::vector<dds::sub::Sample<Space::Type1> > samples(MAX_INSTANCES);
    std::vector<dds::sub::Sample<Space::Type1> >::iterator iter;
    std::vector<Space::Type1> test_samples;

    /* Create and write data. */
    test_samples = this->WriteData(MAX_INSTANCES);

    /* Check result by reading. */
    iter = samples.begin();
    this->reader.read(iter, MAX_INSTANCES);
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, read_SamplesBIIterator)
{
    std::vector<dds::sub::Sample<Space::Type1> > samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(samples);
    std::vector<Space::Type1> test_samples;
    uint32_t len;

    /* Create and write data. */
    test_samples = this->WriteData(3);

    /* Check result by reading. */
    len = this->reader.read(biter);
    ASSERT_EQ(len, samples.size());
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, read_default_filter_read)
{
    dds::sub::status::DataState state =
                    dds::sub::status::DataState(dds::sub::status::SampleState::read(),
                    dds::sub::status::ViewState::not_new_view(),
                    dds::sub::status::InstanceState::alive());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::SampleState::read());
    this->reader.default_filter_state(filter);

    /* Extract not read samples. */
    std::vector<Space::Type1> read_samples;
    read_samples.push_back(test_samples[0]);
    read_samples.push_back(test_samples[1]);

    /* Check result by reading. */
    samples = this->reader.read();
    this->CheckData(samples, read_samples, state);
}


TEST_F(DataReader, read_default_filter_not_read)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::SampleState::not_read());
    this->reader.default_filter_state(filter);

    /* Extract not read samples. */
    std::vector<Space::Type1> not_read_samples;
    not_read_samples.push_back(test_samples[2]);
    not_read_samples.push_back(test_samples[3]);
    not_read_samples.push_back(test_samples[4]);

    /* Check result by reading. */
    samples = this->reader.read();
    this->CheckData(samples, not_read_samples);
}


TEST_F(DataReader, read_default_filter_new_view)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::ViewState::new_view());
    this->reader.default_filter_state(filter);

    /* Extract not read samples. */
    std::vector<Space::Type1> new_view_samples;
    new_view_samples.push_back(test_samples[2]);
    new_view_samples.push_back(test_samples[3]);
    new_view_samples.push_back(test_samples[4]);

    /* Check result by reading. */
    samples = this->reader.read();
    this->CheckData(samples, new_view_samples);
}


TEST_F(DataReader, read_default_filter_not_new_view)
{
    dds::sub::status::DataState state =
                    dds::sub::status::DataState(dds::sub::status::SampleState::read(),
                    dds::sub::status::ViewState::not_new_view(),
                    dds::sub::status::InstanceState::alive());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::ViewState::not_new_view());
    this->reader.default_filter_state(filter);

    /* Extract not read samples. */
    std::vector<Space::Type1> not_new_samples;
    not_new_samples.push_back(test_samples[0]);
    not_new_samples.push_back(test_samples[1]);

    /* Check result by reading. */
    samples = this->reader.read();
    this->CheckData(samples, not_new_samples, state);
}


TEST_F(DataReader, read_default_filter_alive)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::InstanceState::alive());
    this->reader.default_filter_state(filter);

    /* Check result by reading. */
    samples = this->reader.read();
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, read_default_filter_not_alive_disposed)
{
    dds::sub::status::DataState state(dds::sub::status::SampleState::not_read(),
                                      dds::sub::status::ViewState::new_view(),
                                      dds::sub::status::InstanceState::not_alive_disposed());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::InstanceState::not_alive_disposed());
    this->reader.default_filter_state(filter);

    /* First read should return nothing. */
    samples = this->reader.read();
    ASSERT_EQ(samples.length(), 0);

    /* Dispose the instances. */
    for (size_t i = 0; i < test_samples.size(); i++) {
        this->writer.dispose_instance(test_samples[i]);
    }

    /* Now the read should return the samples. */
    samples = this->reader.read();
    this->CheckData(samples, test_samples, state);
}


TEST_F(DataReader, read_default_filter_not_alive_no_writers)
{
    dds::sub::status::DataState state(dds::sub::status::SampleState::not_read(),
                                      dds::sub::status::ViewState::new_view(),
                                      dds::sub::status::InstanceState::not_alive_no_writers());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::InstanceState::not_alive_no_writers());
    this->reader.default_filter_state(filter);

    /* First read should return nothing. */
    samples = this->reader.read();
    ASSERT_EQ(samples.length(), 0);

    /* Unregister the instances. */
    for (size_t i = 0; i < test_samples.size(); i++) {
        this->writer.unregister_instance(test_samples[i]);
    }

    /* Now the read should return the samples. */
    samples = this->reader.read();
    this->CheckData(samples, test_samples, state);
}


TEST_F(DataReader, take)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create and write data. */
    test_samples = this->WriteData(5);

    /* Check result by takeing. */
    samples = this->reader.take();
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, take_no_data)
{
    dds::sub::LoanedSamples<Space::Type1> samples;

    /* Create reader. */
    this->CreateReader();

    /* Check result by reading. */
    samples = this->reader.take();
    ASSERT_EQ(samples.length(), 0);
}


TEST_F(DataReader, take_SamplesFWIterator)
{
    static const uint32_t MAX_INSTANCES = 5;
    std::vector<dds::sub::Sample<Space::Type1> > samples(MAX_INSTANCES);
    std::vector<dds::sub::Sample<Space::Type1> >::iterator iter;
    std::vector<Space::Type1> test_samples;

    /* Create and write data. */
    test_samples = this->WriteData(MAX_INSTANCES);

    /* Check result by takeing. */
    iter = samples.begin();
    this->reader.take(iter, MAX_INSTANCES);
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, take_SamplesBIIterator)
{
    std::vector<dds::sub::Sample<Space::Type1> > samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(samples);
    std::vector<Space::Type1> test_samples;
    uint32_t len;

    /* Create and write data. */
    test_samples = this->WriteData(3);

    /* Check result by takeing. */
    len = this->reader.take(biter);
    ASSERT_EQ(len, samples.size());
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, take_default_filter_read)
{
    dds::sub::status::DataState state =
                    dds::sub::status::DataState(dds::sub::status::SampleState::read(),
                    dds::sub::status::ViewState::not_new_view(),
                    dds::sub::status::InstanceState::alive());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::SampleState::read());
    this->reader.default_filter_state(filter);

    /* Extract not read samples. */
    std::vector<Space::Type1> take_samples;
    take_samples.push_back(test_samples[0]);
    take_samples.push_back(test_samples[1]);

    /* Check result by takeing. */
    samples = this->reader.take();
    this->CheckData(samples, take_samples, state);
}


TEST_F(DataReader, take_default_filter_not_read)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::SampleState::not_read());
    this->reader.default_filter_state(filter);

    /* Extract not take samples. */
    std::vector<Space::Type1> not_read_samples;
    not_read_samples.push_back(test_samples[2]);
    not_read_samples.push_back(test_samples[3]);
    not_read_samples.push_back(test_samples[4]);

    /* Check result by takeing. */
    samples = this->reader.take();
    this->CheckData(samples, not_read_samples);
}


TEST_F(DataReader, take_default_filter_new_view)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::ViewState::new_view());
    this->reader.default_filter_state(filter);

    /* Extract not read samples. */
    std::vector<Space::Type1> new_view_samples;
    new_view_samples.push_back(test_samples[2]);
    new_view_samples.push_back(test_samples[3]);
    new_view_samples.push_back(test_samples[4]);

    /* Check result by reading. */
    samples = this->reader.take();
    this->CheckData(samples, new_view_samples);
}


TEST_F(DataReader, take_default_filter_not_new_view)
{
    dds::sub::status::DataState state =
                    dds::sub::status::DataState(dds::sub::status::SampleState::read(),
                    dds::sub::status::ViewState::not_new_view(),
                    dds::sub::status::InstanceState::alive());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);
    this->ReadFirstSamples(2);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::ViewState::not_new_view());
    this->reader.default_filter_state(filter);

    /* Extract not take samples. */
    std::vector<Space::Type1> not_new_samples;
    not_new_samples.push_back(test_samples[0]);
    not_new_samples.push_back(test_samples[1]);

    /* Check result by reading. */
    samples = this->reader.take();
    this->CheckData(samples, not_new_samples, state);
}


TEST_F(DataReader, take_default_filter_alive)
{
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::InstanceState::alive());
    this->reader.default_filter_state(filter);

    /* Check result by takeing. */
    samples = this->reader.take();
    this->CheckData(samples, test_samples);
}


TEST_F(DataReader, take_default_filter_not_alive_disposed)
{
    dds::sub::status::DataState state(dds::sub::status::SampleState::not_read(),
                                      dds::sub::status::ViewState::new_view(),
                                      dds::sub::status::InstanceState::not_alive_disposed());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::InstanceState::not_alive_disposed());
    this->reader.default_filter_state(filter);

    /* First read should return nothing. */
    samples = this->reader.take();
    ASSERT_EQ(samples.length(), 0);

    /* Dispose the instances. */
    for (size_t i = 0; i < test_samples.size(); i++) {
        this->writer.dispose_instance(test_samples[i]);
    }

    /* Now the take should return the samples. */
    samples = this->reader.take();
    this->CheckData(samples, test_samples, state);
}


TEST_F(DataReader, take_default_filter_not_alive_no_writers)
{
    dds::sub::status::DataState state(dds::sub::status::SampleState::not_read(),
                                      dds::sub::status::ViewState::new_view(),
                                      dds::sub::status::InstanceState::not_alive_no_writers());
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create, write and read some data. */
    test_samples = this->WriteData(5);

    /* Set filter. */
    dds::sub::status::DataState filter(dds::sub::status::InstanceState::not_alive_no_writers());
    this->reader.default_filter_state(filter);

    /* First take should return nothing. */
    samples = this->reader.take();
    ASSERT_EQ(samples.length(), 0);

    /* Unregister the instances. */
    for (size_t i = 0; i < test_samples.size(); i++) {
        this->writer.unregister_instance(test_samples[i]);
    }

    /* Now the take should return the samples. */
    samples = this->reader.take();
    this->CheckData(samples, test_samples, state);
}


TEST_F(DataReader, readtake1)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    dds::sub::LoanedSamples<Space::Type1> take_samples;

    /* Create and write data. */
    std::vector<Space::Type1> testDataList = this->WriteData(1);

    /* Check result by reading. */
    read_samples = this->reader.read();
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    this->CheckData(read_samples, testDataList, notReadState);

    /* Check result. */
    take_samples = this->reader.take();
    dds::sub::status::DataState readState(dds::sub::status::SampleState::read(),
                                          dds::sub::status::ViewState::not_new_view(),
                                          dds::sub::status::InstanceState::alive());
    this->CheckData(take_samples, testDataList, readState);
}


TEST_F(DataReader, readtake2)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    dds::sub::LoanedSamples<Space::Type1> take_samples;

    /* Create and write data. */
    std::vector<Space::Type1> testDataList = this->WriteData(2);

    /* Check result by reading. */
    read_samples = this->reader.read();
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    this->CheckData(read_samples, testDataList, notReadState);

    /* Check result. */
    take_samples = this->reader.take();
    dds::sub::status::DataState readState(dds::sub::status::SampleState::read(),
                                          dds::sub::status::ViewState::not_new_view(),
                                          dds::sub::status::InstanceState::alive());
    this->CheckData(take_samples, testDataList, readState);
}


TEST_F(DataReader, lookup_instance)
{
    static const uint32_t MAX_INSTANCES = 7;
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;

    /* Create and write data. */
    test_samples = this->WriteData(MAX_INSTANCES);

    /* Read the data. */
    samples = this->reader.read();
    ASSERT_EQ(samples.length(), MAX_INSTANCES);

    /* Check instance handles. */
    dds::sub::LoanedSamples<Space::Type1>::const_iterator it;
    for (it = samples.begin(); it != samples.end(); ++it) {
        const Space::Type1& d = it->data();
        dds::core::InstanceHandle ih_org = it->info().instance_handle();
        dds::core::InstanceHandle ih_key = this->reader.lookup_instance(d);
        ASSERT_EQ(ih_org, ih_key);
    }
}


TEST_F(DataReader, key_value_by_sample)
{
    static const uint32_t MAX_INSTANCES = 6;
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;
    unsigned long count = 0UL;

    /* Create and write data. */
    test_samples = this->WriteData(MAX_INSTANCES);

    /* Read the data. */
    samples = this->reader.read();
    ASSERT_EQ(samples.length(), MAX_INSTANCES);

    /* Check key values. */
    dds::sub::LoanedSamples<Space::Type1>::const_iterator it;
    for (it = samples.begin(); it != samples.end(); ++it, ++count) {
        const Space::Type1& d = it->data();
        dds::core::InstanceHandle ih = it->info().instance_handle();
        Space::Type1 key = this->reader.key_value(key, ih);
        /* long_1 is the key variable. */
        ASSERT_EQ(key.long_1(), test_samples[count].long_1());
        ASSERT_EQ(key.long_1(), d.long_1());
    }
}


TEST_F(DataReader, key_value_by_topic)
{
    static const uint32_t MAX_INSTANCES = 6;
    dds::sub::LoanedSamples<Space::Type1> samples;
    std::vector<Space::Type1> test_samples;
    unsigned long count = 0UL;

    /* Create and write data. */
    test_samples = this->WriteData(MAX_INSTANCES);

    /* Read the data. */
    samples = this->reader.read();
    ASSERT_EQ(samples.length(), MAX_INSTANCES);

    /* Check key values. */
    dds::sub::LoanedSamples<Space::Type1>::const_iterator it;
    for (it = samples.begin(); it != samples.end(); ++it, ++count) {
        const Space::Type1& d = it->data();
        dds::core::InstanceHandle ih = it->info().instance_handle();
        dds::topic::TopicInstance<Space::Type1> ti = this->reader.key_value(ih);
        Space::Type1 key = ti.sample();
        /* long_1 is the key variable. */
        ASSERT_EQ(key.long_1(), test_samples[count].long_1());
        ASSERT_EQ(key.long_1(), d.long_1());
    }
}


TEST_F(DataReader, use_after_close)
{
    /* Get a closed DataReader. */
    this->CreateReader();
    this->reader.close();

    ASSERT_THROW({
        this->reader.close();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->reader.subscriber();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->reader.topic_description();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->reader.qos(this->timebased_qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->reader << this->timebased_qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::qos::DataReaderQos qos;
        qos = this->reader.qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::InstanceHandle ih;
        Space::Type1 testData(1,1,1);
        ih = this->reader.lookup_instance(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::InstanceHandle ih;
        Space::Type1 testData(1,1,1);
        Space::Type1 key;
        key = this->reader.key_value(testData, ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::InstanceHandle ih;
        dds::topic::TopicInstance<Space::Type1> ti;
        ti = this->reader.key_value(ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::status::DataState filter;
        this->reader.default_filter_state(filter);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::LoanedSamples<Space::Type1> samples;
        samples = this->reader.read();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::LoanedSamples<Space::Type1> samples;
        this->reader >> samples;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples(1);
        this->reader.read(samples.begin(), 1);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(samples);
        (void)this->reader.read(biter);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::LoanedSamples<Space::Type1> samples;
        samples = this->reader.take();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples(1);
        this->reader.take(samples.begin(), 1);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(samples);
        (void)this->reader.take(biter);
    }, dds::core::AlreadyClosedError);
}


TEST_F(DataReader, use_after_deletion)
{
    /* Get a deleted DataReader. */
    this->CreateReader();
    this->reader = dds::core::null;

    ASSERT_THROW({
        this->reader.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->reader.subscriber();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->reader.topic_description();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->reader.qos(this->timebased_qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->reader << this->timebased_qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::qos::DataReaderQos qos;
        qos = this->reader.qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::InstanceHandle ih;
        Space::Type1 testData(1,1,1);
        ih = this->reader.lookup_instance(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::InstanceHandle ih;
        Space::Type1 testData(1,1,1);
        Space::Type1 key;
        key = this->reader.key_value(testData, ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::InstanceHandle ih;
        dds::topic::TopicInstance<Space::Type1> ti;
        ti = this->reader.key_value(ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::status::DataState filter;
        this->reader.default_filter_state(filter);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::LoanedSamples<Space::Type1> samples;
        samples = this->reader.read();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::LoanedSamples<Space::Type1> samples;
        this->reader >> samples;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples(1);
        this->reader.read(samples.begin(), 1);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(samples);
        (void)this->reader.read(biter);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::LoanedSamples<Space::Type1> samples;
        samples = this->reader.take();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples(1);
        this->reader.take(samples.begin(), 1);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        std::vector<dds::sub::Sample<Space::Type1> > samples;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(samples);
        (void)this->reader.take(biter);
    }, dds::core::NullReferenceError);
}
