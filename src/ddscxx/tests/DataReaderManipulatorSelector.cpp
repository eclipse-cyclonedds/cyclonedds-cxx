/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"
#include "Space_DCPS.hpp"




/**
 * Fixture for the DataReader tests
 */
class ddscxx_DataReaderManipulatorSelector : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::sub::Subscriber subscriber;
    dds::pub::Publisher publisher;
    dds::topic::Topic<Space::Type1> topic;
    dds::sub::DataReader<Space::Type1> reader;
    dds::pub::DataWriter<Space::Type1> writer;

    dds::sub::status::DataState already_read;

    ddscxx_DataReaderManipulatorSelector() :
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
        this->topic = dds::topic::Topic<Space::Type1>(this->participant,
                                                      "manipulator_test_topic",
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
            this->writer.write(samples[(size_t)i]);
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

DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, reader_null)
{
    dds::sub::DataReader<Space::Type1> null_reader = dds::core::null;
    ASSERT_THROW({
        dds::sub::DataReader<Space::Type1>::ManipulatorSelector selector(null_reader);
    }, dds::core::NullReferenceError);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_read)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    this->reader >> dds::sub::read >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    this->reader >> dds::sub::read >> read_samples;
    this->CheckData(read_samples, write_samples, this->already_read);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_take)
{
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    this->reader >> dds::sub::take >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    this->reader >> dds::sub::take >> read_samples;
    this->CheckData(read_samples, std::vector<Space::Type1>());
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_instance)
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
    this->reader >> dds::sub::instance(ih) >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_next_instance)
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
        this->reader >> dds::sub::next_instance(ih) >> read_samples;

        /* Check result. */
        this->CheckData(read_samples, expected2_samples);

        /* Another read should read the next instance. */

        /* Read through the Selector. */
        this->reader >> dds::sub::next_instance(ih) >> read_samples;

        /* Check result. */
        this->CheckData(read_samples, expected3_samples);
    }, dds::core::UnsupportedError);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_state)
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
    this->reader >> dds::sub::state(this->already_read) >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_content)
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
        this->reader >> dds::sub::content(query) >> read_samples;

        /* Check result. */
        this->CheckData(read_samples, expected_samples);
    }, dds::core::UnsupportedError);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_max_samples)
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
    this->reader >> dds::sub::max_samples(3) >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, implicit_multiple)
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
    this->reader >> dds::sub::max_samples(3)
                 >> dds::sub::take
                 >> dds::sub::state(this->already_read)
                 >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_read)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    manipulator.read_mode(true);
    manipulator >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    manipulator >> read_samples;
    this->CheckData(read_samples, write_samples, this->already_read);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_take)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
    dds::sub::LoanedSamples<Space::Type1> read_samples;
    std::vector<Space::Type1> write_samples;

    /* Get test data. */
    write_samples = this->CreateSamples(0, 3,  /* instances */
                                        0, 4); /* samples   */

    /* Write test data. */
    this->WriteData(write_samples);

    /* Read through the Selector. */
    manipulator.read_mode(false);
    manipulator >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, write_samples);

    /* A second time should work as well. */
    manipulator >> read_samples;
    this->CheckData(read_samples, std::vector<Space::Type1>());
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_instance)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
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
    manipulator.instance(ih);
    manipulator >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_next_instance)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
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
        manipulator.next_instance(ih);

        /* Read through the Selector. */
        manipulator >> read_samples;

        /* Check result. */
        this->CheckData(read_samples, expected2_samples);

        /* Another read should read the next instance. */

        /* Read through the Selector. */
        manipulator >> read_samples;

        /* Check result. */
        this->CheckData(read_samples, expected3_samples);
    }, dds::core::UnsupportedError);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_state)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
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
    manipulator.state(this->already_read);
    manipulator >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_content)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
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
        manipulator.content(query);

        /* Read through the Selector. */
        manipulator >> read_samples;

        /* Check result. */
        this->CheckData(read_samples, expected_samples);
    }, dds::core::UnsupportedError);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_max_samples)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
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
    manipulator.max_samples(3);
    manipulator >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples);
}


DDSCXX_TEST_F(ddscxx_DataReaderManipulatorSelector, explicit_multiple)
{
    dds::sub::DataReader<Space::Type1>::ManipulatorSelector manipulator(this->reader);
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
    manipulator.max_samples(3);
    manipulator.read_mode(false);
    manipulator.state(this->already_read);
    manipulator >> read_samples;

    /* Check result. */
    this->CheckData(read_samples, expected_samples, this->already_read);
}
