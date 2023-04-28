// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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
#include "Util.hpp"
#include "Space.hpp"

/**
 * Fixture for the DataReader tests
 */
class DataReaderSelector : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::sub::Subscriber subscriber;
    dds::pub::Publisher publisher;
    dds::topic::Topic<Space::Type1> topic;
    dds::sub::DataReader<Space::Type1> reader;
    dds::pub::DataWriter<Space::Type1> writer;

    dds::sub::status::DataState already_read;

    DataReaderSelector() :
        participant(dds::core::null),
        subscriber(dds::core::null),
        publisher(dds::core::null),
        topic(dds::core::null),
        reader(dds::core::null),
        writer(dds::core::null),
        already_read(dds::sub::status::SampleState::read(),
                     dds::sub::status::ViewState::not_new_view(),
                     dds::sub::status::InstanceState::alive())
    {
    }

    void SetUp()
    {
        this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(this->participant, dds::core::null);

        this->publisher = dds::pub::Publisher(this->participant);
        ASSERT_NE(this->publisher, dds::core::null);

        this->subscriber = dds::sub::Subscriber(this->participant);
        ASSERT_NE(this->subscriber, dds::core::null);

        dds::topic::qos::TopicQos topic_qos = this->participant.default_topic_qos();
        topic_qos << dds::core::policy::Reliability::Reliable() <<
                     dds::core::policy::History::KeepAll();

        char topicname[128];
        create_unique_topic_name("selector_test_topic",topicname,sizeof(topicname));
        this->topic = dds::topic::Topic<Space::Type1>(this->participant,
                                                      topicname,
                                                      topic_qos);
        ASSERT_NE(this->topic, dds::core::null);

        dds::pub::qos::DataWriterQos writer_qos = this->publisher.default_datawriter_qos();
        writer_qos = this->topic.qos();
        this->writer = dds::pub::DataWriter<Space::Type1>(this->publisher,
                                                          this->topic,
                                                          writer_qos);
        ASSERT_NE(this->writer, dds::core::null);

        dds::sub::qos::DataReaderQos reader_qos = this->subscriber.default_datareader_qos();
        reader_qos = this->topic.qos();
        this->reader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic, reader_qos);
        ASSERT_NE(this->reader, dds::core::null);
    }

    std::vector<Space::Type1> CreateSamples(
                     int32_t instances_start,
                     int32_t instances_end,
                     int32_t samples_start,
                     int32_t samples_end)
    {
        std::vector<Space::Type1> samples;
        for (int32_t i = instances_start; i <= instances_end; i++) {
            for (int32_t s = samples_start; s <= samples_end; s++) {
                samples.push_back(Space::Type1(i, i+s+1, i+s+2));
            }
        }
        return samples;
    }

    void WriteData(const std::vector<Space::Type1>& samples)
    {
        for (size_t i = 0; i < samples.size(); i++) {
            this->writer.write(samples[i]);
        }
    }

    void CheckData (
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
        const std::vector<Space::Type1>& test_data,
        const dds::sub::status::DataState& test_state =
                dds::sub::status::DataState(dds::sub::status::SampleState::not_read(),
                dds::sub::status::ViewState::new_view(),
                dds::sub::status::InstanceState::alive()))
    {
        unsigned long count = 0UL;
        std::vector<dds::sub::Sample<Space::Type1> >::const_iterator it;
        ASSERT_EQ(samples.size(), test_data.size());
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

    void TearDown()
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

TEST_F(DataReaderSelector, reader_null)
{
    dds::sub::DataReader<Space::Type1> null_reader = dds::core::null;
    ASSERT_THROW({
        dds::sub::DataReader<Space::Type1>::Selector selector(null_reader);
    }, dds::core::NullReferenceError);
}

TEST_F(DataReaderSelector, implicit)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    read_samples = this->reader.select().read();

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    read_samples = this->reader.select().read();
    this->CheckData(read_samples, write_samples, this->already_read);
}

TEST_F(DataReaderSelector, implicit_instance)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    read_samples = this->reader.select().instance(ih).read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, implicit_next_instance)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        read_samples = this->reader.select().next_instance(ih).read();

        /* Check result. */
        this->CheckData(read_samples, expected2_samples);

        /* Another read should read the next instance. */

        /* Read through the Selector. */
        read_samples = this->reader.select().next_instance(ih).read();

        /* Check result. */
        this->CheckData(read_samples, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, implicit_state)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    read_samples = this->reader.select().state(this->already_read).read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, implicit_content)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        read_samples = this->reader.select().content(query).read();

        /* Check result. */
        this->CheckData(read_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, implicit_max_samples)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    read_samples = this->reader.select().max_samples(3).read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, implicit_multiple)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    read_samples = this->reader.select().max_samples(3).state(this->already_read).read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_LoanedSamples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    read_samples = selector.read();

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    read_samples = selector.read();
    this->CheckData(read_samples, write_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_LoanedSamples_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    selector.instance(ih);
    read_samples = selector.read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, read_LoanedSamples_next_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        selector.next_instance(ih);
        read_samples = selector.read();

        /* Check result. */
        this->CheckData(read_samples, expected2_samples);

        /* Another read should read the next instance. */

        /* Read through the Selector. */
        read_samples = selector.read();

        /* Check result. */
        this->CheckData(read_samples, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, read_LoanedSamples_state)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    selector.state(this->already_read);
    read_samples = selector.read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_LoanedSamples_content)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        selector.content(query);
        read_samples = selector.read();

        /* Check result. */
        this->CheckData(read_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, read_LoanedSamples_max_samples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    selector.max_samples(3);
    read_samples = selector.read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, read_LoanedSamples_multiple)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    selector.max_samples(3);
    selector.state(this->already_read);
    read_samples = selector.read();

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_FWIterator)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples(write_samples.size());
    cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
    ASSERT_EQ(cnt, write_samples.size());

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
    ASSERT_EQ(cnt, write_samples.size());
    this->CheckData(read_samples, write_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_FWIterator_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples(expected_samples.size());
    selector.instance(ih);
    cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
    ASSERT_EQ(cnt, read_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, read_FWIterator_next_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > read_samples2(expected2_samples.size());
        selector.next_instance(ih);
        cnt = selector.read(read_samples2.begin(), static_cast<uint32_t>(read_samples2.size()));
        ASSERT_EQ(cnt, read_samples2.size());

        /* Check result. */
        this->CheckData(read_samples2, expected2_samples);

        /* Another read should read the next instance. */

        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > read_samples3(expected3_samples.size());
        cnt = selector.read(read_samples3.begin(), static_cast<uint32_t>(read_samples3.size()));
        ASSERT_EQ(cnt, read_samples3.size());

        /* Check result. */
        this->CheckData(read_samples3, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, read_FWIterator_state)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples(expected_samples.size());
    selector.state(this->already_read);
    cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
    ASSERT_EQ(cnt, read_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_FWIterator_content)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > read_samples(expected_samples.size());
        selector.content(query);
        cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
        ASSERT_EQ(cnt, read_samples.size());

        /* Check result. */
        this->CheckData(read_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, read_FWIterator_max_samples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples(expected_samples.size());
    selector.max_samples(3);
    cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
    ASSERT_EQ(cnt, read_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, read_FWIterator_multiple)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples(expected_samples.size());
    selector.max_samples(3);
    selector.state(this->already_read);
    cnt = selector.read(read_samples.begin(), static_cast<uint32_t>(read_samples.size()));
    ASSERT_EQ(cnt, read_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_BIIterator)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(read_samples);
    cnt = selector.read(biter);
    ASSERT_EQ(cnt, write_samples.size());

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples2;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter2(read_samples2);
    cnt = selector.read(biter2);
    ASSERT_EQ(cnt, write_samples.size());
    this->CheckData(read_samples2, write_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_BIIterator_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(read_samples);
    selector.instance(ih);
    cnt = selector.read(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, read_BIIterator_next_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > read_samples2;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter2(read_samples2);
        selector.next_instance(ih);
        cnt = selector.read(biter2);
        ASSERT_EQ(cnt, expected2_samples.size());

        /* Check result. */
        this->CheckData(read_samples2, expected2_samples);

        /* Another read should read the next instance. */

        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > read_samples3;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter3(read_samples3);
        cnt = selector.read(biter3);
        ASSERT_EQ(cnt, expected3_samples.size());

        /* Check result. */
        this->CheckData(read_samples3, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, read_BIIterator_state)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(read_samples);
    selector.state(this->already_read);
    cnt = selector.read(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, read_BIIterator_content)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > read_samples;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(read_samples);
        selector.content(query);
        cnt = selector.read(biter);
        ASSERT_EQ(cnt, expected_samples.size());

        /* Check result. */
        this->CheckData(read_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, read_BIIterator_max_samples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(read_samples);
    selector.max_samples(3);
    cnt = selector.read(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}

TEST_F(DataReaderSelector, read_BIIterator_multiple)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > read_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(read_samples);
    selector.max_samples(3);
    selector.state(this->already_read);
    cnt = selector.read(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, take_LoanedSamples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    take_samples = selector.take();

    /* Check result. */
    this->CheckData(take_samples, write_samples);

    /* A second time should return nothing. */
    take_samples = selector.take();
    this->CheckData(take_samples, std::vector<Space::Type1>());
}

TEST_F(DataReaderSelector, take_LoanedSamples_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    selector.instance(ih);
    take_samples = selector.take();

    /* Check result. */
    this->CheckData(take_samples, expected_samples);
}

TEST_F(DataReaderSelector, take_LoanedSamples_next_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        selector.next_instance(ih);
        take_samples = selector.take();

        /* Check result. */
        this->CheckData(take_samples, expected2_samples);

        /* Another take should take the next instance. */

        /* Read through the Selector. */
        take_samples = selector.take();

        /* Check result. */
        this->CheckData(take_samples, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, take_LoanedSamples_state)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    selector.state(this->already_read);
    take_samples = selector.take();

    /* Check result. */
    this->CheckData(take_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, take_LoanedSamples_content)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        selector.content(query);
        take_samples = selector.take();

        /* Check result. */
        this->CheckData(take_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, take_LoanedSamples_max_samples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    selector.max_samples(3);
    take_samples = selector.take();

    /* Check result. */
    this->CheckData(take_samples, expected_samples);
}

TEST_F(DataReaderSelector, take_LoanedSamples_multiple)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> take_samples;
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    selector.max_samples(3);
    selector.state(this->already_read);
    take_samples = selector.take();

    /* Check result. */
    this->CheckData(take_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, take_FWIterator)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples(write_samples.size());
    cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
    ASSERT_EQ(cnt, take_samples.size());

    /* Check result. */
    this->CheckData(take_samples, write_samples);

    /* A second time should return nothing. */
    cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
    ASSERT_EQ(cnt, 0);
}

TEST_F(DataReaderSelector, take_FWIterator_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples(expected_samples.size());
    selector.instance(ih);
    cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
    ASSERT_EQ(cnt, take_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples);
}

TEST_F(DataReaderSelector, take_FWIterator_next_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > take_samples2(expected2_samples.size());
        selector.next_instance(ih);
        cnt = selector.take(take_samples2.begin(), static_cast<uint32_t>(take_samples2.size()));
        ASSERT_EQ(cnt, take_samples2.size());

        /* Check result. */
        this->CheckData(take_samples2, expected2_samples);

        /* Another take should take the next instance. */

        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > take_samples3(expected3_samples.size());
        cnt = selector.take(take_samples3.begin(), static_cast<uint32_t>(take_samples3.size()));
        ASSERT_EQ(cnt, take_samples3.size());

        /* Check result. */
        this->CheckData(take_samples3, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, take_FWIterator_state)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples(expected_samples.size());
    selector.state(this->already_read);
    cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
    ASSERT_EQ(cnt, take_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, take_FWIterator_content)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > take_samples(expected_samples.size());
        selector.content(query);
        cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
        ASSERT_EQ(cnt, take_samples.size());

        /* Check result. */
        this->CheckData(take_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, take_FWIterator_max_samples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples(expected_samples.size());
    selector.max_samples(3);
    cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
    ASSERT_EQ(cnt, take_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples);
}

TEST_F(DataReaderSelector, take_FWIterator_multiple)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples(expected_samples.size());
    selector.max_samples(3);
    selector.state(this->already_read);
    cnt = selector.take(take_samples.begin(), static_cast<uint32_t>(take_samples.size()));
    ASSERT_EQ(cnt, take_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, take_BIIterator)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(take_samples);
    cnt = selector.take(biter);
    ASSERT_EQ(cnt, write_samples.size());

    /* Check result. */
    this->CheckData(take_samples, write_samples);

    /* A second time should return nothing. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples2;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter2(take_samples2);
    cnt = selector.take(biter2);
    ASSERT_EQ(cnt, 0);
    this->CheckData(take_samples2, std::vector<Space::Type1>());
}

TEST_F(DataReaderSelector, take_BIIterator_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(take_samples);
    selector.instance(ih);
    cnt = selector.take(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples);
}

TEST_F(DataReaderSelector, take_BIIterator_next_instance)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected2_samples;
    std::vector<Space::Type1> expected3_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected2_samples = this->CreateSamples(2, 2,  /* instances */
                                            3, 5); /* samples   */
    expected3_samples = this->CreateSamples(3, 3,  /* instances */
                                            3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Get an instance handle. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));

    /* TODO: Getting sample of next instance is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > take_samples2;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter2(take_samples2);
        selector.next_instance(ih);
        cnt = selector.take(biter2);
        ASSERT_EQ(cnt, expected2_samples.size());

        /* Check result. */
        this->CheckData(take_samples2, expected2_samples);

        /* Another take should take the next instance. */

        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > take_samples3;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter3(take_samples3);
        cnt = selector.take(biter3);
        ASSERT_EQ(cnt, expected3_samples.size());

        /* Check result. */
        this->CheckData(take_samples3, expected3_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, take_BIIterator_state)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(take_samples);
    selector.state(this->already_read);
    cnt = selector.take(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples, this->already_read);
}

TEST_F(DataReaderSelector, take_BIIterator_content)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    uint32_t cnt;

    /* Create query. */
    const char *paramsinit[] = {"2", "3", "4"};
    std::vector<std::string> params(paramsinit, paramsinit+3);
    std::string expression = "long_1 = %0 and long_2 = %1 and long_3 = %2";
    dds::sub::Query query(this->reader, expression, params);

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        0, 2); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           1, 1); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);


    /* TODO: Getting sample with query is not yet supported. */
    ASSERT_THROW({
        /* Read through the Selector. */
        std::vector<dds::sub::Sample<Space::Type1> > take_samples;
        std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(take_samples);
        selector.content(query);
        cnt = selector.take(biter);
        ASSERT_EQ(cnt, expected_samples.size());

        /* Check result. */
        this->CheckData(take_samples, expected_samples);
    }, dds::core::UnsupportedError);
}

TEST_F(DataReaderSelector, take_BIIterator_max_samples)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        3, 5); /* samples   */
    expected_samples = this->CreateSamples(1, 1,  /* instances */
                                           3, 5); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(take_samples);
    selector.max_samples(3);
    cnt = selector.take(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples);
}

TEST_F(DataReaderSelector, take_BIIterator_multiple)
{
    dds::sub::DataReader<Space::Type1>::Selector selector(this->reader);
    dds::sub::LoanedSamples<Space::Type1> tmp_samples;
    std::vector<Space::Type1> expected_samples;
    std::vector<Space::Type1> write_samples;
    dds::core::InstanceHandle ih;
    uint32_t cnt;

    /* Get test data. */
    write_samples = this->CreateSamples(1, 5,  /* instances */
                                        2, 8); /* samples   */
    expected_samples = this->CreateSamples(2, 2,  /* instances */
                                           2, 4); /* samples   */

    /* Write data. */
    this->WriteData(write_samples);

    /* Read specific instance to force a state update. */
    ih = this->reader.lookup_instance(Space::Type1(2,0,0));
    tmp_samples = this->reader.select().instance(ih).read();

    /* Read through the Selector. */
    std::vector<dds::sub::Sample<Space::Type1> > take_samples;
    std::back_insert_iterator< std::vector<dds::sub::Sample<Space::Type1> > > biter(take_samples);
    selector.max_samples(3);
    selector.state(this->already_read);
    cnt = selector.take(biter);
    ASSERT_EQ(cnt, expected_samples.size());

    /* Check result. */
    this->CheckData(take_samples, expected_samples, this->already_read);
}
