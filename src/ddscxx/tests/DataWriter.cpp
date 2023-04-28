// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "Space.hpp"



/**
 * Dummy listener for the DataWriter tests
 */
class TestWriter1Listener : public virtual dds::pub::NoOpDataWriterListener<Space::Type1>{ };

static TestWriter1Listener writer1Listener;



/**
 * Fixture for the DataWriter tests
 */
class DataWriter : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::sub::Subscriber subscriber;
    dds::pub::Publisher publisher;
    dds::topic::Topic<Space::Type1> topic;
    dds::sub::DataReader<Space::Type1> reader;
    dds::pub::DataWriter<Space::Type1> writer;

    dds::pub::qos::DataWriterQos besteffort_qos;
    dds::pub::qos::DataWriterQos lifespan_qos;

    std::string partition;

    DataWriter() :
        participant(dds::core::null),
        subscriber(dds::core::null),
        publisher(dds::core::null),
        topic(dds::core::null),
        reader(dds::core::null),
        writer(dds::core::null),
        besteffort_qos(),
        lifespan_qos(),
        partition("DataWriter_test")
    {
    }

    void SetUp() { }

    void
    CreateParticipant()
    {
        if (this->participant == dds::core::null) {
            this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
            ASSERT_NE(this->participant, dds::core::null);
        }
    }

    void
    CreateTopic(bool order_source)
    {
        this->CreateParticipant();
        if (this->topic == dds::core::null) {
            dds::topic::qos::TopicQos topic_qos = this->participant.default_topic_qos();
            if (order_source) {
                topic_qos <<
                    dds::core::policy::DestinationOrder::SourceTimestamp() <<
                    dds::core::policy::Reliability::Reliable() <<
                    dds::core::policy::History::KeepAll();
            }
            this->topic = dds::topic::Topic<Space::Type1>(this->participant, "datawriter_test_topic", topic_qos);
            ASSERT_NE(this->topic, dds::core::null);
        }
    }

    void
    CreateWriter(bool order_source)
    {
        this->SetupWriter(order_source);
        if (this->writer == dds::core::null) {
            dds::pub::qos::DataWriterQos qos = this->publisher.default_datawriter_qos();
            if (order_source) {
                qos = this->topic.qos();
            }
            this->writer = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic, qos);
            ASSERT_NE(this->writer, dds::core::null);
        }
    }

    void
    CreateReader(bool order_source)
    {
        this->SetupReader(order_source);
        if (this->reader == dds::core::null) {
            dds::sub::qos::DataReaderQos qos = this->subscriber.default_datareader_qos();
            if (order_source) {
                qos = this->topic.qos();
            }
            this->reader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic, qos);
            ASSERT_NE(this->reader, dds::core::null);
        }
    }

    void
    SetupWriter(bool order_source)
    {
        this->CreateTopic(order_source);
        if (this->publisher == dds::core::null) {
            dds::pub::qos::PublisherQos pub_qos =
                    this->participant.default_publisher_qos() <<
                    dds::core::policy::Partition(this->partition);
            this->publisher = dds::pub::Publisher(this->participant, pub_qos);

            this->besteffort_qos = this->publisher.default_datawriter_qos();
            this->besteffort_qos.policy(dds::core::policy::Reliability::BestEffort());

            this->lifespan_qos = this->publisher.default_datawriter_qos();
            this->lifespan_qos.policy(dds::core::policy::Lifespan(dds::core::Duration(10, 0)));
        }
    }

    void
    SetupReader(bool order_source)
    {
        this->CreateTopic(order_source);
        if (this->subscriber == dds::core::null) {
            dds::sub::qos::SubscriberQos sub_qos =
                    this->participant.default_subscriber_qos() <<
                    dds::core::policy::Partition(this->partition);
            this->subscriber = dds::sub::Subscriber(this->participant, sub_qos);
        }
    }

    void
    SetupCommunication(bool order_source)
    {
        this->CreateWriter(order_source);
        this->CreateReader(order_source);
    }

    void
    GenerateDataInstanceHandle(
            const Space::Type1 &sample,
            dds::core::InstanceHandle &ih,
            bool order_source)
    {
        this->CreateWriter(order_source);
        ih = this->writer.register_instance(sample);
        ASSERT_NE(ih, dds::core::null);
    }

    void
    ReadAndCheckAllType1 (
            const std::vector<Space::Type1>& testDataList,
            const dds::sub::status::DataState& testState,
            bool take)
    {
        unsigned long count = 0UL;
        dds::sub::LoanedSamples<Space::Type1> samples;

        if (take) {
            samples = this->reader.take();
        } else {
            samples = this->reader.read();
        }

        ASSERT_EQ(samples.length(), testDataList.size());

        dds::sub::LoanedSamples<Space::Type1>::const_iterator it;
        for (it = samples.begin(); it != samples.end(); ++it) {
            const Space::Type1& data = it->data();
            const dds::sub::SampleInfo& info = it->info();
            const dds::sub::status::DataState& state = info.state();
            ASSERT_EQ(data, testDataList[count]);
            ASSERT_EQ(state.view_state(), testState.view_state());
            ASSERT_EQ(state.sample_state(), testState.sample_state());
            ASSERT_EQ(state.instance_state(), testState.instance_state());
            count++;
        }
    }

    void
    ReadAndCheckSampleType1 (
        const Space::Type1& testData,
        const dds::sub::status::DataState& testState,
        bool take)
    {
        uint32_t count;
        std::vector<dds::sub::Sample<Space::Type1> > singlesample;
        singlesample.resize(1);

        if (take) {
            count = this->reader.take(singlesample.begin(), 1);
        } else {
            count = this->reader.read(singlesample.begin(), 1);
        }
        ASSERT_EQ(count, 1);

        ASSERT_EQ(singlesample[0]->data(), testData);

        const dds::sub::status::DataState& state = singlesample[0]->info().state();
        ASSERT_EQ(state.view_state(), testState.view_state());
        ASSERT_EQ(state.sample_state(), testState.sample_state());
        ASSERT_EQ(state.instance_state(), testState.instance_state());
    }

    void
    TearDown()
    {
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

TEST_F(DataWriter, null)
{
    dds::pub::DataWriter<Space::Type1> writer1(dds::core::null);
    dds::pub::DataWriter<Space::Type1> writer2 = dds::core::null;
    ASSERT_EQ(writer1, dds::core::null);
    ASSERT_EQ(writer2, dds::core::null);
}

TEST_F(DataWriter, create_publisher_null)
{
    this->CreateTopic(false);
    dds::pub::DataWriter<Space::Type1> twriter = dds::core::null;

    ASSERT_THROW({
        twriter = dds::pub::DataWriter<Space::Type1>(dds::core::null, this->topic);
    }, dds::core::NullReferenceError);
}

TEST_F(DataWriter, create_topic_null)
{
    this->SetupWriter(false);
    dds::pub::DataWriter<Space::Type1> twriter = dds::core::null;

    ASSERT_THROW({
        twriter = dds::pub::DataWriter<Space::Type1>(this->publisher, dds::core::null);
    }, dds::core::NullReferenceError);
}

TEST_F(DataWriter, create)
{
    this->SetupWriter(false);
    dds::pub::DataWriter<Space::Type1> twriter = dds::core::null;
    twriter = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic);
    ASSERT_NE(twriter, dds::core::null);
}

TEST_F(DataWriter, publisher)
{
    this->CreateWriter(false);
    dds::pub::Publisher tpub = this->writer.publisher();
    ASSERT_NE(tpub, dds::core::null);
    ASSERT_EQ(tpub, this->publisher);
}

TEST_F(DataWriter, topic_description)
{
    this->CreateWriter(false);
    dds::topic::TopicDescription tdesc = this->writer.topic_description();
    ASSERT_NE(tdesc, dds::core::null);
    ASSERT_EQ(tdesc, this->topic);
}

TEST_F(DataWriter, qos_default)
{
    dds::pub::qos::DataWriterQos shift_qos;
    dds::pub::qos::DataWriterQos dflt_qos;
    dds::pub::qos::DataWriterQos get_qos;
    this->CreateWriter(false);
    get_qos = this->writer.qos();
    this->writer >> shift_qos;
    //ASSERT_EQ(get_qos, dflt_qos);
    ASSERT_EQ(get_qos, shift_qos);
    ASSERT_EQ(get_qos, this->publisher.default_datawriter_qos());
    ASSERT_NE(get_qos, this->besteffort_qos);
}

TEST_F(DataWriter, qos_nondefault_constructor)
{
    this->SetupWriter(false);
    dds::pub::qos::DataWriterQos shift_qos;
    dds::pub::qos::DataWriterQos get_qos;
    dds::pub::DataWriter<Space::Type1> wtr = dds::core::null;
    dds::core::status::StatusMask mask;
    wtr = dds::pub::DataWriter<Space::Type1>(
                                this->publisher,
                                this->topic,
                                this->lifespan_qos,
                                &writer1Listener,
                                mask);
    ASSERT_NE(wtr, dds::core::null);
    get_qos = wtr.qos();
    wtr >> shift_qos;
    ASSERT_EQ(get_qos, shift_qos);
    ASSERT_EQ(get_qos, this->lifespan_qos);
    ASSERT_NE(get_qos, this->publisher.default_datawriter_qos());
}

TEST_F(DataWriter, qos_immutable_constructor)
{
    this->SetupWriter(false);
    dds::pub::DataWriter<Space::Type1> wtr = dds::core::null;
    dds::core::status::StatusMask mask;
    wtr = dds::pub::DataWriter<Space::Type1>(
                                this->publisher,
                                this->topic,
                                this->besteffort_qos,
                                &writer1Listener,
                                mask);
    ASSERT_NE(wtr, dds::core::null);
    ASSERT_EQ(wtr.qos(), this->besteffort_qos);
    ASSERT_NE(wtr.qos(), this->publisher.default_datawriter_qos());
}

TEST_F(DataWriter, qos_nondefault_set)
{
    this->CreateWriter(false);
    this->writer.qos(this->lifespan_qos);
    ASSERT_EQ(this->writer.qos(), this->lifespan_qos);
    ASSERT_NE(this->writer.qos(), this->publisher.default_datawriter_qos());
}

TEST_F(DataWriter, qos_immutable_set)
{
    this->CreateWriter(false);
    ASSERT_THROW({
        this->writer.qos(this->besteffort_qos);
    }, dds::core::ImmutablePolicyError);
}

TEST_F(DataWriter, qos_nondefault_shift)
{
    this->CreateWriter(false);
    this->writer << this->lifespan_qos;
    ASSERT_EQ(this->writer.qos(), this->lifespan_qos);
    ASSERT_NE(this->writer.qos(), this->publisher.default_datawriter_qos());
}

TEST_F(DataWriter, qos_immutable_shift)
{
    this->CreateWriter(false);
    ASSERT_THROW({
        this->writer << this->besteffort_qos;
    }, dds::core::ImmutablePolicyError);
}

TEST_F(DataWriter, write_data)
{
    Space::Type1 testData(0,1,2);
    this->SetupCommunication(false);

    /* Write one sample. */
    this->writer.write(testData);

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, write_InstanceHandle)
{
    Space::Type1 testInstance(1,0,0);
    Space::Type1 testData(1,2,3);
    dds::core::InstanceHandle ih;

    /* Get a data instance handle. */
    this->GenerateDataInstanceHandle(testInstance, ih, false);
    this->CreateReader(false);

    /* Now try to write with that instance handle. */
    this->writer.write(testData, ih);

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, write_TopicInstance)
{
    dds::topic::TopicInstance<Space::Type1> ti;
    Space::Type1 testInstance(1,0,0);
    Space::Type1 testData(1,2,3);
    dds::core::InstanceHandle ih;

    /* Get a topic instance handle. */
    this->GenerateDataInstanceHandle(testInstance, ih, false);
    this->CreateReader(false);
    ti = dds::topic::TopicInstance<Space::Type1>(ih, testData);

    /* Now try to write with that instance handle. */
    this->writer.write(ti);

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, write_iter)
{
    dds::sub::status::DataState notReadState(
                        dds::sub::status::SampleState::not_read(),
                        dds::sub::status::ViewState::new_view(),
                        dds::sub::status::InstanceState::alive());

    std::vector<Space::Type1> samples;
    samples.push_back(Space::Type1(1, 2, 3));
    samples.push_back(Space::Type1(2, 3, 4));

    this->SetupCommunication(false);

    this->writer.write(samples.begin(), samples.end());

    ReadAndCheckSampleType1(samples[0], notReadState, true);
    ReadAndCheckSampleType1(samples[1], notReadState, true);
}

TEST_F(DataWriter, write_data_with_timestamp)
{
    Space::Type1 testData1(1,1,1);
    Space::Type1 testData2(1,2,2);
    Space::Type1 testData3(1,3,3);
    dds::core::Time past;

    this->SetupCommunication(true);

    /* Write sample with (implicit) current time first. */
    this->writer.write(testData1);

    /* Then write sample with the time in the past.
     * CycloneDDS does not allow to jump backwards in time, so this
     * sample should be ignored by the reader. */
    past = this->participant.current_time() - dds::core::Duration::from_secs(1);
    this->writer.write(testData2, past);

    /* Last, write sample with current time again. */
    this->writer.write(testData3, this->participant.current_time());

    /* Just a small separate check if states can be created from other states. */
    dds::sub::status::SampleState   not_read(dds::sub::status::SampleState::not_read());
    dds::sub::status::ViewState     new_view(dds::sub::status::ViewState::new_view());
    dds::sub::status::ViewState     not_new_view(dds::sub::status::ViewState::not_new_view());
    dds::sub::status::InstanceState alive(dds::sub::status::InstanceState::alive());

    /* Check result. */
    dds::sub::status::DataState notViewedState(not_read, new_view, alive);
    dds::sub::status::DataState viewedState(not_read, not_new_view, alive);
    //ReadAndCheckSampleType1(testData2, notReadState, true);  ignored by design
    ReadAndCheckSampleType1(testData1, notViewedState, true);
    ReadAndCheckSampleType1(testData3, viewedState,    true);
}

TEST_F(DataWriter, write_InstanceHandle_with_timestamp)
{
    Space::Type1 testInstance(1,0,0);
    Space::Type1 testData(1,2,3);
    dds::core::InstanceHandle ih;

    /* Get a data instance handle. */
    this->GenerateDataInstanceHandle(testInstance, ih, false);
    this->CreateReader(false);

    /* Now try to write with that instance handle. */
    this->writer.write(testData, ih, this->participant.current_time());

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, write_TopicInstance_with_timestamp)
{
    dds::topic::TopicInstance<Space::Type1> ti;
    Space::Type1 testInstance(1,0,0);
    Space::Type1 testData(1,2,3);
    dds::core::InstanceHandle ih;

    /* Get a topic instance handle. */
    this->GenerateDataInstanceHandle(testInstance, ih, false);
    this->CreateReader(false);
    ti = dds::topic::TopicInstance<Space::Type1>(ih, testData);

    /* Now try to write with that instance handle. */
    this->writer.write(ti, this->participant.current_time());

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, write_iter_with_timestamp)
{
    dds::core::Time past;

    /* Just a small separate check if states can be created from bit arrays. */
    dds::sub::status::SampleState not_read(
    dds::sub::status::SampleState::MaskType(dds::sub::status::SampleState::not_read().to_ulong()));
    dds::sub::status::ViewState new_view(
    dds::sub::status::ViewState::MaskType(dds::sub::status::ViewState::new_view().to_ulong()));
    dds::sub::status::ViewState not_new_view(
    dds::sub::status::ViewState::MaskType(dds::sub::status::ViewState::not_new_view().to_ulong()));
    dds::sub::status::InstanceState alive(
    dds::sub::status::InstanceState::MaskType(dds::sub::status::InstanceState::alive().to_ulong()));

    dds::sub::status::DataState notViewedState(not_read, new_view, alive);
    dds::sub::status::DataState viewedState(not_read, not_new_view, alive);

    std::vector<Space::Type1> samplesA;
    samplesA.push_back(Space::Type1(1, 2, 3));
    samplesA.push_back(Space::Type1(2, 3, 4));

    std::vector<Space::Type1> samplesB;
    samplesB.push_back(Space::Type1(1, 4, 5));
    samplesB.push_back(Space::Type1(2, 5, 6));

    std::vector<Space::Type1> samplesC;
    samplesC.push_back(Space::Type1(1, 6, 7));
    samplesC.push_back(Space::Type1(2, 7, 8));

    /* Non-default for ordering by source timestamp. */
    this->SetupCommunication(true);

    /* Write samples with (implicit) current time first. */
    this->writer.write(samplesA.begin(), samplesA.end());

    /* Then write sample with the time in the past.
     * CycloneDDS does not allow to jump backwards in time, so this
     * sample should be ignored by the reader. */
    past = this->participant.current_time() - dds::core::Duration::from_secs(1);
    this->writer.write(samplesB.begin(), samplesB.end(), past);

    /* Last, write sample with current time again. */
    this->writer.write(samplesC.begin(), samplesC.end(), this->participant.current_time());

    /* Check result. */
    ReadAndCheckSampleType1(samplesA[0], notViewedState, true);
    ReadAndCheckSampleType1(samplesC[0], viewedState,    true);
    ReadAndCheckSampleType1(samplesA[1], notViewedState, true);
    ReadAndCheckSampleType1(samplesC[1], viewedState,    true);
}

TEST_F(DataWriter, writedispose)
{
    Space::Type1 testData0(0,0,0);
    Space::Type1 testData1(1,1,1);
    Space::Type1 testData2(2,2,2);
    Space::Type1 testData3(2,3,3);
    dds::sub::status::DataState notReadState(
                        dds::sub::status::SampleState::not_read(),
                        dds::sub::status::ViewState::new_view(),
                        dds::sub::status::InstanceState::alive());
    dds::sub::status::DataState notReadDisposedState(
                        dds::sub::status::SampleState::not_read(),
                        dds::sub::status::ViewState::new_view(),
                        dds::sub::status::InstanceState::not_alive_disposed());
    dds::sub::status::DataState viewedDisposedState(
                        dds::sub::status::SampleState::not_read(),
                        dds::sub::status::ViewState::not_new_view(),
                        dds::sub::status::InstanceState::not_alive_disposed());

    /* Non-default to have history-keep-all. */
    this->SetupCommunication(true);

    /* The writedispose is not part of the spec. */
    this->writer.write(testData0);
    this->writer->writedispose(testData1);
    this->writer.write(testData2);
    this->writer->writedispose(testData3);

    /* Read and check samples. */
    ReadAndCheckSampleType1(testData0, notReadState,         true);
    ReadAndCheckSampleType1(testData1, notReadDisposedState, true);
    ReadAndCheckSampleType1(testData2, notReadDisposedState, true);
    ReadAndCheckSampleType1(testData3, viewedDisposedState,  true);
}

TEST_F(DataWriter, dispose_instance)
{
    static const int32_t MAX_INSTANCES =  5;
    static const int32_t SAMPLES_CNT   = 30;
    int32_t i;

    this->SetupCommunication(false);

    /* Setup test data. */
    std::vector<Space::Type1> testDataList;
    for (i = 0; i < MAX_INSTANCES; i++) {
        testDataList.push_back(Space::Type1(i, i+1, i+2));
    }

    /* Write instances multiple times. */
    for (i = 0; i < SAMPLES_CNT; i++) {
        this->writer.write(testDataList[static_cast<uint32_t>(i % MAX_INSTANCES)]);
        /* Dispose half already. */
        if ((i % 2) == 0) {
            this->writer.dispose_instance(testDataList[static_cast<uint32_t>(i % MAX_INSTANCES)]);
        }
    }

    /* Dispose all remaining instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.dispose_instance(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Check result. */
    dds::sub::status::DataState disposedState(dds::sub::status::SampleState::not_read(),
                                              dds::sub::status::ViewState::new_view(),
                                              dds::sub::status::InstanceState::not_alive_disposed());
    this->ReadAndCheckAllType1(testDataList, disposedState, false);
}

TEST_F(DataWriter, dispose_instance_2nd)
{
    static const int32_t MAX_INSTANCES = 5;
    int32_t i;
    dds::sub::status::DataState notReadDisposedState(
                                    dds::sub::status::SampleState::not_read(),
                                    dds::sub::status::ViewState::new_view(),
                                    dds::sub::status::InstanceState::not_alive_disposed());

    this->SetupCommunication(false);

    /* Setup test data. */
    std::vector<Space::Type1> testDataList;
    for (i = 0; i < MAX_INSTANCES; i++) {
        testDataList.push_back(Space::Type1(i, i+1, i+2));
    }

    /* Write instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.write(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Dispose all instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.dispose_instance(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Check result by reading. */
    this->ReadAndCheckAllType1(testDataList, notReadDisposedState, false);

    /* Re-dispose instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.write(testDataList[static_cast<uint32_t>(i)]);
        this->writer.dispose_instance(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Check result. */
    this->ReadAndCheckAllType1(testDataList, notReadDisposedState, true);
}

TEST_F(DataWriter, dispose_instance_after_read)
{
    static const int32_t MAX_INSTANCES = 5;
    int32_t i;

    this->SetupCommunication(false);

    /* Setup test data. */
    std::vector<Space::Type1> testDataList;
    for (i = 0; i < MAX_INSTANCES; i++) {
        testDataList.push_back(Space::Type1(i, i+1, i+2));
    }

    /* Write instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.write(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Dispose all instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.dispose_instance(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Check result by reading. */
    dds::sub::status::DataState notReadDisposedState(
                                    dds::sub::status::SampleState::not_read(),
                                    dds::sub::status::ViewState::new_view(),
                                    dds::sub::status::InstanceState::not_alive_disposed());
    this->ReadAndCheckAllType1(testDataList, notReadDisposedState, false);

    /* Re-dispose instances. */
    for (i = 0; i < MAX_INSTANCES; i++) {
        this->writer.dispose_instance(testDataList[static_cast<uint32_t>(i)]);
    }

    /* Check result. */
    dds::sub::status::DataState readDisposedState(
                                    dds::sub::status::SampleState::read(),
                                    dds::sub::status::ViewState::not_new_view(),
                                    dds::sub::status::InstanceState::not_alive_disposed());
    this->ReadAndCheckAllType1(testDataList, readDisposedState, true);
}

TEST_F(DataWriter, matched_subscriptions_sequence)
{
    this->SetupCommunication(false);

    /* TODO: Check returned instance handle against the reader instance.
     * Instance handles are not yet supported. So, this isn't either. */
    ASSERT_THROW({
        ::dds::core::InstanceHandleSeq handleSeq;
        handleSeq = dds::pub::matched_subscriptions(this->writer);
    }, dds::core::UnsupportedError);
}

TEST_F(DataWriter, matched_subscriptions_array)
{
    this->SetupCommunication(false);

    /* TODO: Check returned instance handle against the reader instance.
     * Instance handles are not yet supported. So, this isn't either. */
    ASSERT_THROW({
        ::dds::core::InstanceHandle handleArr[2]; /* Make sure we have more than enough. */
        (void)dds::pub::matched_subscriptions(this->writer, handleArr, 2);
    }, dds::core::UnsupportedError);
}

TEST_F(DataWriter, shift_sample)
{
    Space::Type1 testData(0,1,2);
    this->SetupCommunication(false);

    /* Write one sample. */
    this->writer << testData;

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, shift_pair_timestamp)
{
    Space::Type1 testData1(1,1,1);
    Space::Type1 testData2(1,2,2);
    Space::Type1 testData3(1,3,3);
    dds::core::Time past;

    this->SetupCommunication(true);

    /* Write sample with (implicit) current time first. */
    this->writer << testData1;

    /* Then write sample with the time in the past.
     * CycloneDDS does not allow to jump backwards in time, so this
     * sample should be ignored by the reader. */
    past = this->participant.current_time() - dds::core::Duration::from_secs(1);
    this->writer << std::make_pair(testData2, past);

    /* Last, write sample with current time again. */
    this->writer << std::make_pair(testData3, this->participant.current_time());

    /* Check result. */
    dds::sub::status::DataState notViewedState(
                        dds::sub::status::SampleState::not_read(),
                        dds::sub::status::ViewState::new_view(),
                        dds::sub::status::InstanceState::alive());
    dds::sub::status::DataState viewedState(
                        dds::sub::status::SampleState::not_read(),
                        dds::sub::status::ViewState::not_new_view(),
                        dds::sub::status::InstanceState::alive());
    //ReadAndCheckSampleType1(testData2, notReadState, true);  ignored by design
    ReadAndCheckSampleType1(testData1, notViewedState, true);
    ReadAndCheckSampleType1(testData3, viewedState,    true);
}

TEST_F(DataWriter, shift_pair_InstanceHandle)
{
    Space::Type1 testInstance(9,0,0);
    Space::Type1 testData(9,8,7);
    dds::core::InstanceHandle ih;

    /* Get a data instance handle. */
    this->GenerateDataInstanceHandle(testInstance, ih, false);
    this->CreateReader(false);

    /* Now try to write with that instance handle. */
    this->writer << std::make_pair(testData, ih);

    /* Check result. */
    dds::sub::status::DataState notReadState(dds::sub::status::SampleState::not_read(),
                                             dds::sub::status::ViewState::new_view(),
                                             dds::sub::status::InstanceState::alive());
    ReadAndCheckSampleType1(testData, notReadState, true);
}

TEST_F(DataWriter, lookup_instance)
{
    Space::Type1 testData1(101,100,200);
    Space::Type1 testData2(102,100,200);
    dds::core::InstanceHandle ih_registered1;
    dds::core::InstanceHandle ih_registered2;
    dds::core::InstanceHandle ih_found1;
    dds::core::InstanceHandle ih_found2;

    this->CreateWriter(false);

    /* See if we can find one when nothing was written yet. */
    ih_found1 = this->writer.lookup_instance(testData1);
    ASSERT_EQ(ih_found1, dds::core::null);

    /* Register two samples. */
    ih_registered1 = this->writer.register_instance(testData1);
    ASSERT_NE(ih_registered1, dds::core::null);
    ih_registered2 = this->writer.register_instance(testData2);
    ASSERT_NE(ih_registered2, dds::core::null);
    ASSERT_NE(ih_registered2, ih_registered1);

    /* See if we can find the proper ones. */
    ih_found1 = this->writer.lookup_instance(testData1);
    ASSERT_NE(ih_found1, dds::core::null);
    ASSERT_EQ(ih_found1, ih_registered1);
    ih_found2 = this->writer.lookup_instance(testData2);
    ASSERT_NE(ih_found2, dds::core::null);
    ASSERT_EQ(ih_found2, ih_registered2);
}

TEST_F(DataWriter, unregister_instance)
{
    Space::Type1 testData(201,100,200);
    dds::core::InstanceHandle ih;

    this->CreateWriter(false);

    /* Register the sample. */
    ih = this->writer.register_instance(testData);
    ASSERT_NE(ih, dds::core::null);

    /* Unregister the sample. */
    this->writer.unregister_instance(testData);
}

TEST_F(DataWriter, unregister_instance_InstanceHandle)
{
    Space::Type1 testData(301,100,200);
    dds::core::InstanceHandle ih;

    this->CreateWriter(false);

    /* Register the sample. */
    ih = this->writer.register_instance(testData);
    ASSERT_NE(ih, dds::core::null);

    /* Unregister the sample. */
    this->writer.unregister_instance(ih);
}

TEST_F(DataWriter, unregister_instance_with_timestamp)
{
    Space::Type1 testData(401,100,200);
    dds::core::InstanceHandle ih;

    this->CreateWriter(false);

    /* Register the sample. */
    ih = this->writer.register_instance(testData);
    ASSERT_NE(ih, dds::core::null);

    /* Unregister the sample. */
    this->writer.unregister_instance(testData, this->participant.current_time());
}

TEST_F(DataWriter, unregister_instance_InstanceHandle_with_timestamp)
{
    Space::Type1 testData(501,100,200);
    dds::core::InstanceHandle ih;

    this->CreateWriter(false);

    /* Register the sample. */
    ih = this->writer.register_instance(testData);
    ASSERT_NE(ih, dds::core::null);

    /* Unregister the sample. */
    this->writer.unregister_instance(ih, this->participant.current_time());
}

TEST_F(DataWriter, register_instance_with_timestamp)
{
    Space::Type1 testData1(0,1,2);
    dds::core::InstanceHandle ih;

    this->CreateWriter(false);

    /* Now try to write with that instance handle. */
    ASSERT_THROW({
        (void)this->writer.register_instance(testData1, this->participant.current_time());
    }, dds::core::UnsupportedError);
}

TEST_F(DataWriter, key_value_ti)
{
    dds::topic::TopicInstance<Space::Type1> dummy;
    dds::topic::TopicInstance<Space::Type1> ti;
    Space::Type1 testData(42,42,42);
    Space::Type1 key;
    dds::core::InstanceHandle ih;

    /* Get a data instance handle. */
    this->GenerateDataInstanceHandle(testData, ih, false);

    ti = this->writer.key_value(dummy, ih);
    ASSERT_EQ(ti.handle(), ih);

    key = ti.sample();
    ASSERT_EQ(key.long_1(), testData.long_1());
}

TEST_F(DataWriter, key_value_sample)
{
    dds::topic::TopicInstance<Space::Type1> ti;
    Space::Type1 testData(43,43,43);
    Space::Type1 dummy(0,0,0);
    Space::Type1 key;
    dds::core::InstanceHandle ih;

    /* Get a data instance handle. */
    this->GenerateDataInstanceHandle(testData, ih, false);

    key = this->writer.key_value(dummy, ih);
    ASSERT_EQ(key.long_1(), testData.long_1());
}

TEST_F(DataWriter, topic)
{
    this->CreateWriter(false);
    ASSERT_EQ(this->writer.topic(), this->topic);
}

TEST_F(DataWriter, use_after_close)
{
    dds::topic::TopicInstance<Space::Type1> ti;
    dds::core::InstanceHandle ih;
    Space::Type1 testData(1,1,1);

    /* Get a closed DataWriter. */
    this->GenerateDataInstanceHandle(Space::Type1(0,1,2), ih, false);
    this->writer.close();

    ASSERT_THROW({
        this->writer.close();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.publisher();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.topic_description();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.qos(this->lifespan_qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer << this->lifespan_qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        qos = this->writer.qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        this->writer >> qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.write(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.write(testData, ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.write(ti);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        std::vector<Space::Type1> samples(1, testData);
        this->writer.write(samples.begin(), samples.end());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.write(testData, this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.write(testData, ih, this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.write(ti, this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        std::vector<Space::Type1> samples(1, testData);
        this->writer.write(samples.begin(), samples.end(), this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer->writedispose(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.dispose_instance(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer << testData;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer << std::make_pair(testData, this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer << std::make_pair(testData, ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::InstanceHandle res;
        res = this->writer.register_instance(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::InstanceHandle res;
        res = this->writer.lookup_instance(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.unregister_instance(testData);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.unregister_instance(ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.unregister_instance(testData, this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.unregister_instance(ih, this->participant.current_time());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::topic::TopicInstance<Space::Type1> dummy;
        ti = this->writer.key_value(dummy, ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        Space::Type1 key;
        Space::Type1 dummy(0,0,0);
        key = this->writer.key_value(dummy, ih);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->writer.topic();
    }, dds::core::AlreadyClosedError);
}

TEST_F(DataWriter, use_after_deletion)
{
    dds::topic::TopicInstance<Space::Type1> ti;
    dds::core::InstanceHandle ih;
    Space::Type1 testData(1,1,1);

    /* Get a deleted DataWriter. */
    this->GenerateDataInstanceHandle(Space::Type1(0,1,2), ih, false);
    this->writer = dds::core::null;

    ASSERT_THROW({
        this->writer.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.publisher();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.topic_description();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.qos(this->lifespan_qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer << this->lifespan_qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        qos = this->writer.qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        this->writer >> qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.write(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.write(testData, ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.write(ti);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        std::vector<Space::Type1> samples(1, testData);
        this->writer.write(samples.begin(), samples.end());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.write(testData, this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.write(testData, ih, this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.write(ti, this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        std::vector<Space::Type1> samples(1, testData);
        this->writer.write(samples.begin(), samples.end(), this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer->writedispose(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.dispose_instance(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer << testData;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer << std::make_pair(testData, this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer << std::make_pair(testData, ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::InstanceHandle res;
        res = this->writer.register_instance(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::InstanceHandle res;
        res = this->writer.lookup_instance(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.unregister_instance(testData);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.unregister_instance(ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.unregister_instance(testData, this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.unregister_instance(ih, this->participant.current_time());
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::topic::TopicInstance<Space::Type1> dummy;
        ti = this->writer.key_value(dummy, ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        Space::Type1 key;
        Space::Type1 dummy(0,0,0);
        key = this->writer.key_value(dummy, ih);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->writer.topic();
    }, dds::core::NullReferenceError);
}
